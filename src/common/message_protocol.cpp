/*
 * Message Protocol Implementation
 * Author: Haobo (Brian) Liu
 * Email: h349liu@gmail.com
 * Project: Distributed Imaging Services for Voyis Interview
 */

#include "message_protocol.h"
#include <cstring>
#include <chrono>

namespace imaging {

// Helper function implementations
void MessageProtocol::writeUint32(std::vector<uint8_t>& buffer, uint32_t value) {
    buffer.push_back((value >> 24) & 0xFF);
    buffer.push_back((value >> 16) & 0xFF);
    buffer.push_back((value >> 8) & 0xFF);
    buffer.push_back(value & 0xFF);
}

void MessageProtocol::writeUint64(std::vector<uint8_t>& buffer, uint64_t value) {
    for (int i = 7; i >= 0; --i) {
        buffer.push_back((value >> (i * 8)) & 0xFF);
    }
}

void MessageProtocol::writeFloat(std::vector<uint8_t>& buffer, float value) {
    uint32_t temp;
    std::memcpy(&temp, &value, sizeof(float));
    writeUint32(buffer, temp);
}

void MessageProtocol::writeString(std::vector<uint8_t>& buffer, const std::string& str) {
    writeUint32(buffer, static_cast<uint32_t>(str.size()));
    buffer.insert(buffer.end(), str.begin(), str.end());
}

uint32_t MessageProtocol::readUint32(const uint8_t* data, size_t& offset) {
    uint32_t value = (static_cast<uint32_t>(data[offset]) << 24) |
                     (static_cast<uint32_t>(data[offset + 1]) << 16) |
                     (static_cast<uint32_t>(data[offset + 2]) << 8) |
                     static_cast<uint32_t>(data[offset + 3]);
    offset += 4;
    return value;
}

uint64_t MessageProtocol::readUint64(const uint8_t* data, size_t& offset) {
    uint64_t value = 0;
    for (int i = 0; i < 8; ++i) {
        value = (value << 8) | static_cast<uint64_t>(data[offset + i]);
    }
    offset += 8;
    return value;
}

float MessageProtocol::readFloat(const uint8_t* data, size_t& offset) {
    uint32_t temp = readUint32(data, offset);
    float value;
    std::memcpy(&value, &temp, sizeof(float));
    return value;
}

std::string MessageProtocol::readString(const uint8_t* data, size_t& offset, size_t max_length) {
    uint32_t length = readUint32(data, offset);
    if (length > max_length) {
        return "";
    }
    std::string str(reinterpret_cast<const char*>(data + offset), length);
    offset += length;
    return str;
}

// Serialize image data message
std::vector<uint8_t> MessageProtocol::serializeImageData(
    const ImageMetadata& metadata,
    const std::vector<uint8_t>& image_data) {
    
    std::vector<uint8_t> buffer;
    
    // Message type
    buffer.push_back(static_cast<uint8_t>(MessageType::IMAGE_DATA));
    
    // Metadata
    writeUint64(buffer, metadata.timestamp);
    writeUint32(buffer, metadata.width);
    writeUint32(buffer, metadata.height);
    writeUint32(buffer, metadata.channels);
    writeUint32(buffer, metadata.data_size);
    writeString(buffer, metadata.filename);
    
    // Image data
    buffer.insert(buffer.end(), image_data.begin(), image_data.end());
    
    return buffer;
}

// Deserialize image data message
bool MessageProtocol::deserializeImageData(
    const std::vector<uint8_t>& message,
    ImageMetadata& metadata,
    std::vector<uint8_t>& image_data) {
    
    if (message.size() < 30) {  // Minimum size check
        return false;
    }
    
    size_t offset = 0;
    
    // Check message type
    MessageType type = static_cast<MessageType>(message[offset++]);
    if (type != MessageType::IMAGE_DATA) {
        return false;
    }
    
    // Deserialize metadata
    metadata.timestamp = readUint64(message.data(), offset);
    metadata.width = readUint32(message.data(), offset);
    metadata.height = readUint32(message.data(), offset);
    metadata.channels = readUint32(message.data(), offset);
    metadata.data_size = readUint32(message.data(), offset);
    metadata.filename = readString(message.data(), offset, 256);
    
    // Deserialize image data
    if (offset + metadata.data_size > message.size()) {
        return false;
    }
    
    image_data.assign(message.begin() + offset, message.begin() + offset + metadata.data_size);
    
    return true;
}

// Serialize processed data message
std::vector<uint8_t> MessageProtocol::serializeProcessedData(
    const ImageMetadata& metadata,
    const std::vector<uint8_t>& image_data,
    const std::vector<KeyPoint>& keypoints,
    const std::vector<float>& descriptors) {
    
    std::vector<uint8_t> buffer;
    
    // Message type
    buffer.push_back(static_cast<uint8_t>(MessageType::PROCESSED_DATA));
    
    // Metadata
    writeUint64(buffer, metadata.timestamp);
    writeUint32(buffer, metadata.width);
    writeUint32(buffer, metadata.height);
    writeUint32(buffer, metadata.channels);
    writeUint32(buffer, metadata.data_size);
    writeString(buffer, metadata.filename);
    
    // Image data
    buffer.insert(buffer.end(), image_data.begin(), image_data.end());
    
    // Keypoints
    writeUint32(buffer, static_cast<uint32_t>(keypoints.size()));
    for (const auto& kp : keypoints) {
        writeFloat(buffer, kp.x);
        writeFloat(buffer, kp.y);
        writeFloat(buffer, kp.size);
        writeFloat(buffer, kp.angle);
        writeFloat(buffer, kp.response);
        writeUint32(buffer, static_cast<uint32_t>(kp.octave));
    }
    
    // Descriptors
    writeUint32(buffer, static_cast<uint32_t>(descriptors.size()));
    for (float desc : descriptors) {
        writeFloat(buffer, desc);
    }
    
    return buffer;
}

// Deserialize processed data message
bool MessageProtocol::deserializeProcessedData(
    const std::vector<uint8_t>& message,
    ImageMetadata& metadata,
    std::vector<uint8_t>& image_data,
    std::vector<KeyPoint>& keypoints,
    std::vector<float>& descriptors) {
    
    if (message.size() < 30) {
        return false;
    }
    
    size_t offset = 0;
    
    // Check message type
    MessageType type = static_cast<MessageType>(message[offset++]);
    if (type != MessageType::PROCESSED_DATA) {
        return false;
    }
    
    // Deserialize metadata
    metadata.timestamp = readUint64(message.data(), offset);
    metadata.width = readUint32(message.data(), offset);
    metadata.height = readUint32(message.data(), offset);
    metadata.channels = readUint32(message.data(), offset);
    metadata.data_size = readUint32(message.data(), offset);
    metadata.filename = readString(message.data(), offset, 256);
    
    // Deserialize image data
    if (offset + metadata.data_size > message.size()) {
        return false;
    }
    image_data.assign(message.begin() + offset, message.begin() + offset + metadata.data_size);
    offset += metadata.data_size;
    
    // Deserialize keypoints
    if (offset + 4 > message.size()) {
        return false;
    }
    uint32_t num_keypoints = readUint32(message.data(), offset);
    keypoints.clear();
    keypoints.reserve(num_keypoints);
    
    for (uint32_t i = 0; i < num_keypoints; ++i) {
        if (offset + 24 > message.size()) {
            return false;
        }
        KeyPoint kp;
        kp.x = readFloat(message.data(), offset);
        kp.y = readFloat(message.data(), offset);
        kp.size = readFloat(message.data(), offset);
        kp.angle = readFloat(message.data(), offset);
        kp.response = readFloat(message.data(), offset);
        kp.octave = static_cast<int>(readUint32(message.data(), offset));
        keypoints.push_back(kp);
    }
    
    // Deserialize descriptors
    if (offset + 4 > message.size()) {
        return false;
    }
    uint32_t num_descriptors = readUint32(message.data(), offset);
    descriptors.clear();
    descriptors.reserve(num_descriptors);
    
    for (uint32_t i = 0; i < num_descriptors; ++i) {
        if (offset + 4 > message.size()) {
            return false;
        }
        descriptors.push_back(readFloat(message.data(), offset));
    }
    
    return true;
}

// Serialize heartbeat message
std::vector<uint8_t> MessageProtocol::serializeHeartbeat(const std::string& app_name) {
    std::vector<uint8_t> buffer;
    buffer.push_back(static_cast<uint8_t>(MessageType::HEARTBEAT));
    writeString(buffer, app_name);
    writeUint64(buffer, std::chrono::system_clock::now().time_since_epoch().count());
    return buffer;
}

// Get message type
MessageType MessageProtocol::getMessageType(const std::vector<uint8_t>& message) {
    if (message.empty()) {
        return MessageType::SHUTDOWN;
    }
    return static_cast<MessageType>(message[0]);
}

} // namespace imaging
