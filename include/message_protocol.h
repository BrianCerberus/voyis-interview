/*
 * Message Protocol Header
 * Author: Haobo (Brian) Liu
 * Email: h349liu@gmail.com
 * Project: Distributed Imaging Services for Voyis Interview
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <memory>

namespace imaging {

// Message types for IPC communication
enum class MessageType : uint8_t {
    IMAGE_DATA = 1,
    PROCESSED_DATA = 2,
    HEARTBEAT = 3,
    SHUTDOWN = 4
};

// Image metadata structure
struct ImageMetadata {
    uint64_t timestamp;
    uint32_t width;
    uint32_t height;
    uint32_t channels;
    uint32_t data_size;
    std::string filename;
    
    ImageMetadata() 
        : timestamp(0), width(0), height(0), channels(0), data_size(0) {}
};

// Keypoint structure for SIFT features
struct KeyPoint {
    float x;
    float y;
    float size;
    float angle;
    float response;
    int octave;
    
    KeyPoint() 
        : x(0), y(0), size(0), angle(0), response(0), octave(0) {}
};

// Message protocol class for serialization/deserialization
class MessageProtocol {
public:
    // Serialize image data message
    static std::vector<uint8_t> serializeImageData(
        const ImageMetadata& metadata,
        const std::vector<uint8_t>& image_data
    );
    
    // Deserialize image data message
    static bool deserializeImageData(
        const std::vector<uint8_t>& message,
        ImageMetadata& metadata,
        std::vector<uint8_t>& image_data
    );
    
    // Serialize processed data message (image + keypoints)
    static std::vector<uint8_t> serializeProcessedData(
        const ImageMetadata& metadata,
        const std::vector<uint8_t>& image_data,
        const std::vector<KeyPoint>& keypoints,
        const std::vector<float>& descriptors
    );
    
    // Deserialize processed data message
    static bool deserializeProcessedData(
        const std::vector<uint8_t>& message,
        ImageMetadata& metadata,
        std::vector<uint8_t>& image_data,
        std::vector<KeyPoint>& keypoints,
        std::vector<float>& descriptors
    );
    
    // Serialize heartbeat message
    static std::vector<uint8_t> serializeHeartbeat(const std::string& app_name);
    
    // Get message type from serialized message
    static MessageType getMessageType(const std::vector<uint8_t>& message);

private:
    // Helper functions for serialization
    static void writeUint32(std::vector<uint8_t>& buffer, uint32_t value);
    static void writeUint64(std::vector<uint8_t>& buffer, uint64_t value);
    static void writeFloat(std::vector<uint8_t>& buffer, float value);
    static void writeString(std::vector<uint8_t>& buffer, const std::string& str);
    
    // Helper functions for deserialization
    static uint32_t readUint32(const uint8_t* data, size_t& offset);
    static uint64_t readUint64(const uint8_t* data, size_t& offset);
    static float readFloat(const uint8_t* data, size_t& offset);
    static std::string readString(const uint8_t* data, size_t& offset, size_t max_length);
};

} // namespace imaging
