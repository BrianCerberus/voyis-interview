/*
 * Data Logger Application
 * Author: Haobo (Brian) Liu
 * Email: h349liu@gmail.com
 * Project: Distributed Imaging Services for Voyis Interview
 */

#include "database_manager.h"
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
    
    imaging::Logger::info("=== Data Logger Starting ===");
    
    // Parse command line arguments
    std::string subscribe_endpoint = "tcp://localhost:5556";
    std::string db_path = "imaging_data.db";
    
    if (argc > 1) {
        subscribe_endpoint = argv[1];
    }
    if (argc > 2) {
        db_path = argv[2];
    }
    
    imaging::Logger::info("Subscribe endpoint: " + subscribe_endpoint);
    imaging::Logger::info("Database path: " + db_path);
    
    // Initialize database
    imaging::DatabaseManager db_manager(db_path);
    if (!db_manager.initialize()) {
        imaging::Logger::error("Failed to initialize database");
        return 1;
    }
    
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
    
    // Connect to feature extractor
    if (zmq_connect(subscriber, subscribe_endpoint.c_str()) != 0) {
        imaging::Logger::error("Failed to connect to: " + subscribe_endpoint);
        zmq_close(subscriber);
        zmq_ctx_destroy(context);
        return 1;
    }
    
    imaging::Logger::info("Connected to feature extractor");
    imaging::Logger::info("Starting data logging...");
    
    uint64_t frame_count = 0;
    uint64_t last_stats_time = 0;
    std::vector<uint8_t> receive_buffer(100 * 1024 * 1024);  // 100MB buffer
    
    while (g_running) {
        // Receive processed data
        int received = zmq_recv(subscriber, receive_buffer.data(), receive_buffer.size(), 0);
        
        if (received == -1) {
            if (errno == EAGAIN || errno == EINTR) {
                // Timeout or interrupted, continue
                // Print stats periodically
                auto now = std::chrono::system_clock::now().time_since_epoch().count();
                if (now - last_stats_time > 10000000000LL) {  // Every 10 seconds
                    int64_t total_images = db_manager.getTotalImagesStored();
                    int64_t total_keypoints = db_manager.getTotalKeypointsStored();
                    imaging::Logger::info("Stats - Total images: " + std::to_string(total_images) + 
                                        ", Total keypoints: " + std::to_string(total_keypoints));
                    last_stats_time = now;
                }
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
        
        // Deserialize processed data
        imaging::ImageMetadata metadata;
        std::vector<uint8_t> image_data;
        std::vector<imaging::KeyPoint> keypoints;
        std::vector<float> descriptors;
        
        if (!imaging::MessageProtocol::deserializeProcessedData(message, metadata, 
                                                               image_data, keypoints, descriptors)) {
            imaging::Logger::error("Failed to deserialize processed data");
            continue;
        }
        
        frame_count++;
        imaging::Logger::info("Received frame " + std::to_string(frame_count) + 
                            ": " + metadata.filename + " with " + 
                            std::to_string(keypoints.size()) + " keypoints");
        
        // Store in database
        auto start_time = std::chrono::high_resolution_clock::now();
        
        if (!db_manager.storeProcessedData(metadata, image_data, keypoints, descriptors)) {
            imaging::Logger::error("Failed to store data: " + metadata.filename);
            continue;
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        imaging::Logger::info("Stored frame " + metadata.filename + " in " + 
                            std::to_string(duration.count()) + " ms");
    }
    
    imaging::Logger::info("Cleaning up...");
    
    // Print final statistics
    int64_t total_images = db_manager.getTotalImagesStored();
    int64_t total_keypoints = db_manager.getTotalKeypointsStored();
    imaging::Logger::info("Final Stats - Total images: " + std::to_string(total_images) + 
                        ", Total keypoints: " + std::to_string(total_keypoints));
    
    // Cleanup
    zmq_close(subscriber);
    zmq_ctx_destroy(context);
    
    imaging::Logger::info("=== Data Logger Stopped ===");
    return 0;
}
