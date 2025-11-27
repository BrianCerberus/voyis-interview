/*
 * Database Manager Implementation
 * Author: Haobo (Brian) Liu
 * Email: h349liu@gmail.com
 * Project: Distributed Imaging Services for Voyis Interview
 */

#include "database_manager.h"
#include "logger.h"
#include <sstream>

namespace imaging {

DatabaseManager::DatabaseManager(const std::string& db_path)
    : db_path_(db_path), db_(nullptr) {
}

DatabaseManager::~DatabaseManager() {
    if (db_) {
        sqlite3_close(db_);
    }
}

bool DatabaseManager::initialize() {
    Logger::info("Initializing database: " + db_path_);
    
    // Open database
    int rc = sqlite3_open(db_path_.c_str(), &db_);
    if (rc != SQLITE_OK) {
        Logger::error("Failed to open database: " + std::string(sqlite3_errmsg(db_)));
        return false;
    }
    
    // Enable foreign keys
    if (!executeSql("PRAGMA foreign_keys = ON;")) {
        return false;
    }
    
    // Create tables
    return createTables();
}

bool DatabaseManager::createTables() {
    // Images table
    std::string create_images_table = R"(
        CREATE TABLE IF NOT EXISTS images (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp INTEGER NOT NULL,
            filename TEXT NOT NULL,
            width INTEGER NOT NULL,
            height INTEGER NOT NULL,
            channels INTEGER NOT NULL,
            data_size INTEGER NOT NULL,
            image_data BLOB NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );
    )";
    
    if (!executeSql(create_images_table)) {
        return false;
    }
    
    // Keypoints table
    std::string create_keypoints_table = R"(
        CREATE TABLE IF NOT EXISTS keypoints (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            image_id INTEGER NOT NULL,
            x REAL NOT NULL,
            y REAL NOT NULL,
            size REAL NOT NULL,
            angle REAL NOT NULL,
            response REAL NOT NULL,
            octave INTEGER NOT NULL,
            FOREIGN KEY (image_id) REFERENCES images(id) ON DELETE CASCADE
        );
    )";
    
    if (!executeSql(create_keypoints_table)) {
        return false;
    }
    
    // Descriptors table
    std::string create_descriptors_table = R"(
        CREATE TABLE IF NOT EXISTS descriptors (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            image_id INTEGER NOT NULL,
            descriptor_data BLOB NOT NULL,
            FOREIGN KEY (image_id) REFERENCES images(id) ON DELETE CASCADE
        );
    )";
    
    if (!executeSql(create_descriptors_table)) {
        return false;
    }
    
    // Create indices for faster queries
    executeSql("CREATE INDEX IF NOT EXISTS idx_keypoints_image_id ON keypoints(image_id);");
    executeSql("CREATE INDEX IF NOT EXISTS idx_descriptors_image_id ON descriptors(image_id);");
    executeSql("CREATE INDEX IF NOT EXISTS idx_images_filename ON images(filename);");
    
    Logger::info("Database tables created successfully");
    return true;
}

bool DatabaseManager::executeSql(const std::string& sql) {
    char* err_msg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err_msg);
    
    if (rc != SQLITE_OK) {
        Logger::error("SQL error: " + std::string(err_msg));
        sqlite3_free(err_msg);
        return false;
    }
    
    return true;
}

bool DatabaseManager::storeProcessedData(const ImageMetadata& metadata,
                                         const std::vector<uint8_t>& image_data,
                                         const std::vector<KeyPoint>& keypoints,
                                         const std::vector<float>& descriptors) {
    // Begin transaction
    if (!executeSql("BEGIN TRANSACTION;")) {
        return false;
    }
    
    // Insert image
    sqlite3_stmt* stmt = nullptr;
    const char* insert_image_sql = R"(
        INSERT INTO images (timestamp, filename, width, height, channels, data_size, image_data)
        VALUES (?, ?, ?, ?, ?, ?, ?);
    )";
    
    int rc = sqlite3_prepare_v2(db_, insert_image_sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        Logger::error("Failed to prepare statement: " + std::string(sqlite3_errmsg(db_)));
        executeSql("ROLLBACK;");
        return false;
    }
    
    // Bind parameters
    sqlite3_bind_int64(stmt, 1, metadata.timestamp);
    sqlite3_bind_text(stmt, 2, metadata.filename.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, metadata.width);
    sqlite3_bind_int(stmt, 4, metadata.height);
    sqlite3_bind_int(stmt, 5, metadata.channels);
    sqlite3_bind_int(stmt, 6, metadata.data_size);
    sqlite3_bind_blob(stmt, 7, image_data.data(), image_data.size(), SQLITE_TRANSIENT);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        Logger::error("Failed to insert image: " + std::string(sqlite3_errmsg(db_)));
        executeSql("ROLLBACK;");
        return false;
    }
    
    int64_t image_id = sqlite3_last_insert_rowid(db_);
    
    // Insert keypoints
    const char* insert_keypoint_sql = R"(
        INSERT INTO keypoints (image_id, x, y, size, angle, response, octave)
        VALUES (?, ?, ?, ?, ?, ?, ?);
    )";
    
    for (const auto& kp : keypoints) {
        rc = sqlite3_prepare_v2(db_, insert_keypoint_sql, -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            Logger::error("Failed to prepare keypoint statement");
            executeSql("ROLLBACK;");
            return false;
        }
        
        sqlite3_bind_int64(stmt, 1, image_id);
        sqlite3_bind_double(stmt, 2, kp.x);
        sqlite3_bind_double(stmt, 3, kp.y);
        sqlite3_bind_double(stmt, 4, kp.size);
        sqlite3_bind_double(stmt, 5, kp.angle);
        sqlite3_bind_double(stmt, 6, kp.response);
        sqlite3_bind_int(stmt, 7, kp.octave);
        
        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        
        if (rc != SQLITE_DONE) {
            Logger::error("Failed to insert keypoint");
            executeSql("ROLLBACK;");
            return false;
        }
    }
    
    // Insert descriptors
    if (!descriptors.empty()) {
        const char* insert_descriptor_sql = R"(
            INSERT INTO descriptors (image_id, descriptor_data)
            VALUES (?, ?);
        )";
        
        rc = sqlite3_prepare_v2(db_, insert_descriptor_sql, -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            Logger::error("Failed to prepare descriptor statement");
            executeSql("ROLLBACK;");
            return false;
        }
        
        sqlite3_bind_int64(stmt, 1, image_id);
        sqlite3_bind_blob(stmt, 2, descriptors.data(), 
                         descriptors.size() * sizeof(float), SQLITE_TRANSIENT);
        
        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        
        if (rc != SQLITE_DONE) {
            Logger::error("Failed to insert descriptors");
            executeSql("ROLLBACK;");
            return false;
        }
    }
    
    // Commit transaction
    if (!executeSql("COMMIT;")) {
        executeSql("ROLLBACK;");
        return false;
    }
    
    return true;
}

int64_t DatabaseManager::getTotalImagesStored() {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT COUNT(*) FROM images;";
    
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return -1;
    }
    
    int64_t count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int64(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    return count;
}

int64_t DatabaseManager::getTotalKeypointsStored() {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT COUNT(*) FROM keypoints;";
    
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return -1;
    }
    
    int64_t count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int64(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    return count;
}

} // namespace imaging
