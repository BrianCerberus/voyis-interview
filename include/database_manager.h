/*
 * Database Manager Header
 * Author: Haobo (Brian) Liu
 * Email: h349liu@gmail.com
 * Project: Distributed Imaging Services for Voyis Interview
 */

#pragma once

#include <string>
#include <sqlite3.h>
#include "message_protocol.h"

namespace imaging {

class DatabaseManager {
public:
    DatabaseManager(const std::string& db_path);
    ~DatabaseManager();
    
    // Initialize database and create tables
    bool initialize();
    
    // Store processed image data
    bool storeProcessedData(const ImageMetadata& metadata,
                           const std::vector<uint8_t>& image_data,
                           const std::vector<KeyPoint>& keypoints,
                           const std::vector<float>& descriptors);
    
    // Get statistics
    int64_t getTotalImagesStored();
    int64_t getTotalKeypointsStored();

private:
    std::string db_path_;
    sqlite3* db_;
    
    // Create database schema
    bool createTables();
    
    // Helper to execute SQL
    bool executeSql(const std::string& sql);
};

} // namespace imaging
