/*
 * Image Publisher Implementation
 * Author: Haobo (Brian) Liu
 * Email: h349liu@gmail.com
 * Project: Distributed Imaging Services for Voyis Interview
 */

#include "image_publisher.h"
#include "logger.h"
#include <opencv2/opencv.hpp>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <thread>
#include <algorithm>

namespace fs = std::filesystem;

namespace imaging {

ImagePublisher::ImagePublisher(const std::string& endpoint)
    : endpoint_(endpoint), context_(nullptr), publisher_(nullptr), 
      running_(false), current_index_(0) {
}

ImagePublisher::~ImagePublisher() {
    stop();
    if (publisher_) {
        zmq_close(publisher_);
    }
    if (context_) {
        zmq_ctx_destroy(context_);
    }
}

bool ImagePublisher::initialize() {
    Logger::info("Initializing Image Publisher...");
    
    // Create ZeroMQ context
    context_ = zmq_ctx_new();
    if (!context_) {
        Logger::error("Failed to create ZeroMQ context");
        return false;
    }
    
    // Create publisher socket
    publisher_ = zmq_socket(context_, ZMQ_PUB);
    if (!publisher_) {
        Logger::error("Failed to create publisher socket");
        return false;
    }
    
    // Set socket options for better reliability
    int linger = 1000;  // 1 second linger on close
    zmq_setsockopt(publisher_, ZMQ_LINGER, &linger, sizeof(linger));
    
    int sndhwm = 100;  // Send high water mark
    zmq_setsockopt(publisher_, ZMQ_SNDHWM, &sndhwm, sizeof(sndhwm));
    
    // Bind to endpoint
    if (zmq_bind(publisher_, endpoint_.c_str()) != 0) {
        Logger::error("Failed to bind to endpoint: " + endpoint_);
        return false;
    }
    
    Logger::info("Publisher bound to: " + endpoint_);
    
    // Give time for subscribers to connect
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    return true;
}

bool ImagePublisher::loadImagesFromDirectory(const std::string& directory) {
    Logger::info("Loading images from directory: " + directory);
    
    if (!fs::exists(directory) || !fs::is_directory(directory)) {
        Logger::error("Directory does not exist: " + directory);
        return false;
    }
    
    image_paths_.clear();
    
    // Supported image extensions
    std::vector<std::string> extensions = {".png", ".jpg", ".jpeg", ".bmp", ".tiff"};
    
    for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            std::string path = entry.path().string();
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            
            if (std::find(extensions.begin(), extensions.end(), ext) != extensions.end()) {
                image_paths_.push_back(path);
            }
        }
    }
    
    // Sort paths for consistent ordering
    std::sort(image_paths_.begin(), image_paths_.end());
    
    Logger::info("Found " + std::to_string(image_paths_.size()) + " images");
    
    return !image_paths_.empty();
}

bool ImagePublisher::readImageFile(const std::string& path, std::vector<uint8_t>& buffer) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        return false;
    }
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    buffer.resize(size);
    return file.read(reinterpret_cast<char*>(buffer.data()), size).good();
}

bool ImagePublisher::getImageInfo(const std::string& path, uint32_t& width, 
                                   uint32_t& height, uint32_t& channels) {
    cv::Mat img = cv::imread(path, cv::IMREAD_UNCHANGED);
    if (img.empty()) {
        return false;
    }
    
    width = img.cols;
    height = img.rows;
    channels = img.channels();
    
    return true;
}

void ImagePublisher::publishImages() {
    if (image_paths_.empty()) {
        Logger::error("No images to publish");
        return;
    }
    
    running_ = true;
    current_index_ = 0;
    
    Logger::info("Starting continuous image publishing...");
    Logger::info("Press Ctrl+C to stop");
    
    uint64_t frame_count = 0;
    
    while (running_) {
        const std::string& path = image_paths_[current_index_];
        
        // Read image data
        std::vector<uint8_t> image_data;
        if (!readImageFile(path, image_data)) {
            Logger::error("Failed to read image: " + path);
            current_index_ = (current_index_ + 1) % image_paths_.size();
            continue;
        }
        
        // Get image metadata
        ImageMetadata metadata;
        metadata.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        metadata.data_size = static_cast<uint32_t>(image_data.size());
        metadata.filename = fs::path(path).filename().string();
        
        if (!getImageInfo(path, metadata.width, metadata.height, metadata.channels)) {
            Logger::error("Failed to get image info: " + path);
            current_index_ = (current_index_ + 1) % image_paths_.size();
            continue;
        }
        
        // Serialize message
        std::vector<uint8_t> message = MessageProtocol::serializeImageData(metadata, image_data);
        
        // Send message
        int sent = zmq_send(publisher_, message.data(), message.size(), ZMQ_DONTWAIT);
        if (sent == -1) {
            if (errno == EAGAIN) {
                Logger::warning("Send buffer full, skipping frame");
            } else {
                Logger::error("Failed to send message: " + std::string(zmq_strerror(errno)));
            }
        } else {
            frame_count++;
            if (frame_count % 10 == 0) {
                Logger::info("Published frame " + std::to_string(frame_count) + 
                           ": " + metadata.filename + 
                           " (" + std::to_string(metadata.width) + "x" + 
                           std::to_string(metadata.height) + ", " + 
                           std::to_string(metadata.data_size / 1024) + " KB)");
            }
        }
        
        // Move to next image (loop back if at end)
        current_index_ = (current_index_ + 1) % image_paths_.size();
        
        // Small delay to avoid overwhelming the system
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    Logger::info("Stopped publishing images");
}

void ImagePublisher::stop() {
    running_ = false;
}

} // namespace imaging
