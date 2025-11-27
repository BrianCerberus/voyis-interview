/*
 * Image Generator Application
 * Author: Haobo (Brian) Liu
 * Email: h349liu@gmail.com
 * Project: Distributed Imaging Services for Voyis Interview
 */

#include "image_publisher.h"
#include "logger.h"
#include <csignal>
#include <iostream>
#include <memory>

static std::unique_ptr<imaging::ImagePublisher> g_publisher;

void signalHandler(int signum) {
    imaging::Logger::info("Interrupt signal (" + std::to_string(signum) + ") received");
    if (g_publisher) {
        g_publisher->stop();
    }
}

int main(int argc, char* argv[]) {
    // Set up signal handler
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    imaging::Logger::info("=== Image Generator Starting ===");
    
    // Parse command line arguments
    std::string image_directory = "./deep_sea_imaging/raw";
    std::string endpoint = "tcp://*:5555";
    
    if (argc > 1) {
        image_directory = argv[1];
    }
    if (argc > 2) {
        endpoint = argv[2];
    }
    
    imaging::Logger::info("Image directory: " + image_directory);
    imaging::Logger::info("Publish endpoint: " + endpoint);
    
    // Create and initialize publisher
    g_publisher = std::make_unique<imaging::ImagePublisher>(endpoint);
    
    if (!g_publisher->initialize()) {
        imaging::Logger::error("Failed to initialize publisher");
        return 1;
    }
    
    // Load images
    if (!g_publisher->loadImagesFromDirectory(image_directory)) {
        imaging::Logger::error("Failed to load images from directory");
        return 1;
    }
    
    // Start publishing
    g_publisher->publishImages();
    
    imaging::Logger::info("=== Image Generator Stopped ===");
    return 0;
}
