/**
 * Unit Tests for Database Manager
 * 
 * Author: Haobo (Brian) Liu
 * Email: h349liu@gmail.com
 * Project: Distributed Imaging Services for Voyis Interview
 */

#include "database_manager.h"
#include <cassert>
#include <iostream>
#include <filesystem>

using namespace imaging;
namespace fs = std::filesystem;

#define TEST_ASSERT(condition, message) \
    if (!(condition)) { \
        std::cerr << "FAILED: " << message << std::endl; \
        return false; \
    }

bool test_database_initialization() {
    std::cout << "Testing: Database initialization..." << std::endl;
    
    const std::string test_db = "test_init.db";
    
    // Remove if exists
    if (fs::exists(test_db)) {
        fs::remove(test_db);
    }
    
    DatabaseManager db(test_db);
    TEST_ASSERT(db.initialize(), "Database should initialize successfully");
    TEST_ASSERT(fs::exists(test_db), "Database file should be created");
    
    // Cleanup
    fs::remove(test_db);
    
    std::cout << "  ✓ PASSED" << std::endl;
    return true;
}

bool test_store_and_retrieve() {
    std::cout << "Testing: Store and retrieve data..." << std::endl;
    
    const std::string test_db = "test_store.db";
    
    if (fs::exists(test_db)) {
        fs::remove(test_db);
    }
    
    DatabaseManager db(test_db);
    TEST_ASSERT(db.initialize(), "Database initialization failed");
    
    // Create test data
    ImageMetadata metadata;
    metadata.timestamp = 123456789;
    metadata.width = 1920;
    metadata.height = 1080;
    metadata.channels = 3;
    metadata.data_size = 100;
    metadata.filename = "test_image.png";
    
    std::vector<uint8_t> image_data(100, 128);
    
    std::vector<KeyPoint> keypoints;
    KeyPoint kp;
    kp.x = 100.0f;
    kp.y = 200.0f;
    kp.size = 10.0f;
    kp.angle = 45.0f;
    kp.response = 0.8f;
    kp.octave = 2;
    keypoints.push_back(kp);
    
    std::vector<float> descriptors(128, 0.5f);
    
    // Store data
    bool stored = db.storeProcessedData(metadata, image_data, keypoints, descriptors);
    TEST_ASSERT(stored, "Data storage should succeed");
    
    // Verify counts
    int64_t image_count = db.getTotalImagesStored();
    int64_t keypoint_count = db.getTotalKeypointsStored();
    
    TEST_ASSERT(image_count == 1, "Should have 1 image stored");
    TEST_ASSERT(keypoint_count == 1, "Should have 1 keypoint stored");
    
    // Cleanup
    fs::remove(test_db);
    
    std::cout << "  ✓ PASSED" << std::endl;
    return true;
}

bool test_multiple_inserts() {
    std::cout << "Testing: Multiple inserts..." << std::endl;
    
    const std::string test_db = "test_multiple.db";
    
    if (fs::exists(test_db)) {
        fs::remove(test_db);
    }
    
    DatabaseManager db(test_db);
    TEST_ASSERT(db.initialize(), "Database initialization failed");
    
    // Insert multiple records
    for (int i = 0; i < 5; i++) {
        ImageMetadata metadata;
        metadata.timestamp = 1000000 + i;
        metadata.width = 640;
        metadata.height = 480;
        metadata.channels = 3;
        metadata.data_size = 50;
        metadata.filename = "test_" + std::to_string(i) + ".png";
        
        std::vector<uint8_t> image_data(50, i);
        std::vector<KeyPoint> keypoints;
        std::vector<float> descriptors;
        
        bool stored = db.storeProcessedData(metadata, image_data, keypoints, descriptors);
        TEST_ASSERT(stored, "Insert " + std::to_string(i) + " should succeed");
    }
    
    int64_t image_count = db.getTotalImagesStored();
    TEST_ASSERT(image_count == 5, "Should have 5 images stored");
    
    // Cleanup
    fs::remove(test_db);
    
    std::cout << "  ✓ PASSED" << std::endl;
    return true;
}

int main() {
    std::cout << "\n======================================" << std::endl;
    std::cout << "Database Manager Unit Tests" << std::endl;
    std::cout << "Author: Haobo (Brian) Liu" << std::endl;
    std::cout << "======================================\n" << std::endl;
    
    int passed = 0;
    int total = 0;
    
    total++; if (test_database_initialization()) passed++;
    total++; if (test_store_and_retrieve()) passed++;
    total++; if (test_multiple_inserts()) passed++;
    
    std::cout << "\n======================================" << std::endl;
    std::cout << "Results: " << passed << "/" << total << " tests passed" << std::endl;
    std::cout << "======================================\n" << std::endl;
    
    return (passed == total) ? 0 : 1;
}
