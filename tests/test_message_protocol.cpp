/**
 * Unit Tests for Message Protocol
 * 
 * Author: Haobo (Brian) Liu
 * Email: h349liu@gmail.com
 * Project: Distributed Imaging Services for Voyis Interview
 */

#include "message_protocol.h"
#include <cassert>
#include <iostream>
#include <vector>

using namespace imaging;

// Test helper
#define TEST_ASSERT(condition, message) \
    if (!(condition)) { \
        std::cerr << "FAILED: " << message << std::endl; \
        return false; \
    }

bool test_serialize_deserialize_image_data() {
    std::cout << "Testing: Image data serialization/deserialization..." << std::endl;
    
    // Create test data
    ImageMetadata metadata;
    metadata.timestamp = 123456789;
    metadata.width = 1920;
    metadata.height = 1080;
    metadata.channels = 3;
    metadata.filename = "test_image.png";
    
    std::vector<uint8_t> image_data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    metadata.data_size = image_data.size();
    
    // Serialize
    std::vector<uint8_t> serialized = MessageProtocol::serializeImageData(metadata, image_data);
    
    TEST_ASSERT(!serialized.empty(), "Serialized data should not be empty");
    TEST_ASSERT(serialized.size() > 50, "Serialized data should have reasonable size");
    
    // Deserialize
    ImageMetadata decoded_metadata;
    std::vector<uint8_t> decoded_image_data;
    
    bool result = MessageProtocol::deserializeImageData(serialized, decoded_metadata, decoded_image_data);
    
    TEST_ASSERT(result, "Deserialization should succeed");
    TEST_ASSERT(decoded_metadata.timestamp == metadata.timestamp, "Timestamp mismatch");
    TEST_ASSERT(decoded_metadata.width == metadata.width, "Width mismatch");
    TEST_ASSERT(decoded_metadata.height == metadata.height, "Height mismatch");
    TEST_ASSERT(decoded_metadata.channels == metadata.channels, "Channels mismatch");
    TEST_ASSERT(decoded_metadata.filename == metadata.filename, "Filename mismatch");
    TEST_ASSERT(decoded_image_data.size() == image_data.size(), "Image data size mismatch");
    TEST_ASSERT(decoded_image_data == image_data, "Image data content mismatch");
    
    std::cout << "  ✓ PASSED" << std::endl;
    return true;
}

bool test_serialize_deserialize_processed_data() {
    std::cout << "Testing: Processed data serialization/deserialization..." << std::endl;
    
    // Create test data
    ImageMetadata metadata;
    metadata.timestamp = 987654321;
    metadata.width = 640;
    metadata.height = 480;
    metadata.channels = 1;
    metadata.filename = "processed.png";
    
    std::vector<uint8_t> image_data = {255, 128, 64, 32, 16, 8, 4, 2, 1};
    metadata.data_size = image_data.size();
    
    // Create keypoints
    std::vector<KeyPoint> keypoints;
    KeyPoint kp1, kp2;
    kp1.x = 100.5f;
    kp1.y = 200.5f;
    kp1.size = 10.0f;
    kp1.angle = 45.0f;
    kp1.response = 0.8f;
    kp1.octave = 2;
    
    kp2.x = 300.5f;
    kp2.y = 400.5f;
    kp2.size = 20.0f;
    kp2.angle = 90.0f;
    kp2.response = 0.9f;
    kp2.octave = 3;
    
    keypoints.push_back(kp1);
    keypoints.push_back(kp2);
    
    // Create descriptors (simplified)
    std::vector<float> descriptors = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f};
    
    // Serialize
    std::vector<uint8_t> serialized = MessageProtocol::serializeProcessedData(
        metadata, image_data, keypoints, descriptors);
    
    TEST_ASSERT(!serialized.empty(), "Serialized processed data should not be empty");
    
    // Deserialize
    ImageMetadata decoded_metadata;
    std::vector<uint8_t> decoded_image_data;
    std::vector<KeyPoint> decoded_keypoints;
    std::vector<float> decoded_descriptors;
    
    bool result = MessageProtocol::deserializeProcessedData(
        serialized, decoded_metadata, decoded_image_data, 
        decoded_keypoints, decoded_descriptors);
    
    TEST_ASSERT(result, "Deserialization should succeed");
    TEST_ASSERT(decoded_keypoints.size() == 2, "Keypoint count mismatch");
    TEST_ASSERT(decoded_keypoints[0].x == kp1.x, "Keypoint 1 x mismatch");
    TEST_ASSERT(decoded_keypoints[1].y == kp2.y, "Keypoint 2 y mismatch");
    TEST_ASSERT(decoded_descriptors.size() == 5, "Descriptor count mismatch");
    TEST_ASSERT(decoded_descriptors[0] == 0.1f, "Descriptor value mismatch");
    
    std::cout << "  ✓ PASSED" << std::endl;
    return true;
}

bool test_message_type() {
    std::cout << "Testing: Message type detection..." << std::endl;
    
    ImageMetadata metadata;
    metadata.timestamp = 111111;
    metadata.width = 100;
    metadata.height = 100;
    metadata.channels = 3;
    metadata.data_size = 10;
    metadata.filename = "test.png";
    
    std::vector<uint8_t> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    std::vector<uint8_t> msg = MessageProtocol::serializeImageData(metadata, data);
    MessageType type = MessageProtocol::getMessageType(msg);
    
    TEST_ASSERT(type == MessageType::IMAGE_DATA, "Message type should be IMAGE_DATA");
    
    std::cout << "  ✓ PASSED" << std::endl;
    return true;
}

bool test_heartbeat() {
    std::cout << "Testing: Heartbeat message..." << std::endl;
    
    std::vector<uint8_t> hb = MessageProtocol::serializeHeartbeat("TestApp");
    TEST_ASSERT(!hb.empty(), "Heartbeat should not be empty");
    
    MessageType type = MessageProtocol::getMessageType(hb);
    TEST_ASSERT(type == MessageType::HEARTBEAT, "Message type should be HEARTBEAT");
    
    std::cout << "  ✓ PASSED" << std::endl;
    return true;
}

int main() {
    std::cout << "\n======================================" << std::endl;
    std::cout << "Message Protocol Unit Tests" << std::endl;
    std::cout << "Author: Haobo (Brian) Liu" << std::endl;
    std::cout << "======================================\n" << std::endl;
    
    int passed = 0;
    int total = 0;
    
    total++; if (test_serialize_deserialize_image_data()) passed++;
    total++; if (test_serialize_deserialize_processed_data()) passed++;
    total++; if (test_message_type()) passed++;
    total++; if (test_heartbeat()) passed++;
    
    std::cout << "\n======================================" << std::endl;
    std::cout << "Results: " << passed << "/" << total << " tests passed" << std::endl;
    std::cout << "======================================\n" << std::endl;
    
    return (passed == total) ? 0 : 1;
}
