/*
 * Image Publisher Header
 * Author: Haobo (Brian) Liu
 * Email: h349liu@gmail.com
 * Project: Distributed Imaging Services for Voyis Interview
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <zmq.h>
#include "message_protocol.h"

namespace imaging {

class ImagePublisher {
public:
    ImagePublisher(const std::string& endpoint);
    ~ImagePublisher();
    
    // Initialize the publisher
    bool initialize();
    
    // Load images from a directory
    bool loadImagesFromDirectory(const std::string& directory);
    
    // Publish images continuously
    void publishImages();
    
    // Stop publishing
    void stop();
    
private:
    std::string endpoint_;
    void* context_;
    void* publisher_;
    std::vector<std::string> image_paths_;
    bool running_;
    size_t current_index_;
    
    // Read image file into buffer
    bool readImageFile(const std::string& path, std::vector<uint8_t>& buffer);
    
    // Get image dimensions from OpenCV
    bool getImageInfo(const std::string& path, uint32_t& width, uint32_t& height, uint32_t& channels);
};

} // namespace imaging
