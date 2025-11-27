/*
 * Feature Extractor Application
 * Author: Haobo (Brian) Liu
 * Email: h349liu@gmail.com
 * Project: Distributed Imaging Services for Voyis Interview
 */

#include "sift_processor.h"
#include "message_protocol.h"
#include "logger.h"
#include <zmq.h>
#include <csignal>
#include <thread>
#include <atomic>

static std::atomic<bool> g_running(true);

void signalHandler(int signum) {
    imaging::Logger::info("Interrupt signal (" + std::to_string(signum) + ") received");
    g_running = false;
}

int main(int argc, char* argv[]) {
    // Set up signal handler
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    imaging::Logger::info("=== Feature Extractor Starting ===");
    
    // Parse command line arguments
    std::string subscribe_endpoint = "tcp://localhost:5555";
    std::string publish_endpoint = "tcp://*:5556";
    
    if (argc > 1) {
        subscribe_endpoint = argv[1];
    }
    if (argc > 2) {
        publish_endpoint = argv[2];
    }
    
    imaging::Logger::info("Subscribe endpoint: " + subscribe_endpoint);
    imaging::Logger::info("Publish endpoint: " + publish_endpoint);
    
    // Create ZeroMQ context
    void* context = zmq_ctx_new();
    if (!context) {
        imaging::Logger::error("Failed to create ZeroMQ context");
        return 1;
    }
    
    // Create subscriber socket
    void* subscriber = zmq_socket(context, ZMQ_SUB);
    if (!subscriber) {
        imaging::Logger::error("Failed to create subscriber socket");
        zmq_ctx_destroy(context);
        return 1;
    }
    
    // Subscribe to all messages
    zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, "", 0);
    
    // Set timeout for receiving
    int timeout = 1000;  // 1 second
    zmq_setsockopt(subscriber, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
    
    // Connect to publisher
    if (zmq_connect(subscriber, subscribe_endpoint.c_str()) != 0) {
        imaging::Logger::error("Failed to connect to: " + subscribe_endpoint);
        zmq_close(subscriber);
        zmq_ctx_destroy(context);
        return 1;
    }
    
    imaging::Logger::info("Connected to image generator");
    
    // Create publisher socket
    void* publisher = zmq_socket(context, ZMQ_PUB);
    if (!publisher) {
        imaging::Logger::error("Failed to create publisher socket");
        zmq_close(subscriber);
        zmq_ctx_destroy(context);
        return 1;
    }
    
    int linger = 1000;
    zmq_setsockopt(publisher, ZMQ_LINGER, &linger, sizeof(linger));
    
    if (zmq_bind(publisher, publish_endpoint.c_str()) != 0) {
        imaging::Logger::error("Failed to bind to: " + publish_endpoint);
        zmq_close(publisher);
        zmq_close(subscriber);
        zmq_ctx_destroy(context);
        return 1;
    }
    
    imaging::Logger::info("Publisher bound to: " + publish_endpoint);
    
    // Give time for subscribers to connect
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Create SIFT processor
    imaging::SIFTProcessor processor;
    
    imaging::Logger::info("Starting feature extraction...");
    
    uint64_t frame_count = 0;
    std::vector<uint8_t> receive_buffer(50 * 1024 * 1024);  // 50MB buffer
    
    while (g_running) {
        // Receive image data
        int received = zmq_recv(subscriber, receive_buffer.data(), receive_buffer.size(), 0);
        
        if (received == -1) {
            if (errno == EAGAIN || errno == EINTR) {
                // Timeout or interrupted, continue
                continue;
            }
            imaging::Logger::error("Error receiving message: " + std::string(zmq_strerror(errno)));
            continue;
        }
        
        if (received == 0) {
            continue;
        }
        
        // Copy received data to message vector
        std::vector<uint8_t> message(receive_buffer.begin(), receive_buffer.begin() + received);
        
        // Deserialize image data
        imaging::ImageMetadata metadata;
        std::vector<uint8_t> image_data;
        
        if (!imaging::MessageProtocol::deserializeImageData(message, metadata, image_data)) {
            imaging::Logger::error("Failed to deserialize image data");
            continue;
        }
        
        frame_count++;
        imaging::Logger::info("Processing frame " + std::to_string(frame_count) + 
                            ": " + metadata.filename);
        
        // Extract SIFT features
        std::vector<imaging::KeyPoint> keypoints;
        std::vector<float> descriptors;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        if (!processor.processImage(image_data, keypoints, descriptors)) {
            imaging::Logger::error("Failed to process image: " + metadata.filename);
            continue;
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        imaging::Logger::info("Extracted " + std::to_string(keypoints.size()) + 
                            " keypoints in " + std::to_string(duration.count()) + " ms");
        
        // Serialize processed data
        std::vector<uint8_t> processed_message = 
            imaging::MessageProtocol::serializeProcessedData(metadata, image_data, 
                                                            keypoints, descriptors);
        
        // Publish processed data
        int sent = zmq_send(publisher, processed_message.data(), processed_message.size(), ZMQ_DONTWAIT);
        if (sent == -1) {
            imaging::Logger::warning("Failed to send processed data");
        } else {
            imaging::Logger::info("Published processed frame: " + metadata.filename);
        }
    }
    
    imaging::Logger::info("Cleaning up...");
    
    // Cleanup
    zmq_close(publisher);
    zmq_close(subscriber);
    zmq_ctx_destroy(context);
    
    imaging::Logger::info("=== Feature Extractor Stopped ===");
    return 0;
}
