# Distributed Imaging Services - Design Document

**Author:** Haobo (Brian) Liu  
**Email:** h349liu@gmail.com  
**Project:** Distributed Imaging Services for Voyis Interview

---

## Executive Summary

This document outlines the software architecture for a distributed image processing system consisting of three independent applications that communicate via Inter-Process Communication (IPC). The system demonstrates modern C++ design principles, robust error handling, and scalable architecture suitable for real-time image processing pipelines.

## System Overview

### Purpose
Create a loosely-coupled, resilient distributed system for processing underwater imaging data that can:
- Handle continuous data streams
- Process images to extract features
- Store results for later analysis
- Operate reliably in the presence of failures

### Design Goals

1. **Loose Coupling**: Applications should be independent and not tightly bound
2. **Resilience**: System should handle process failures gracefully
3. **Flexibility**: Applications can start in any order
4. **Performance**: Handle large images (30+ MB) efficiently
5. **Maintainability**: Clean, modern C++ code that's easy to understand and extend

## Architecture

### High-Level Architecture

```
┌──────────────────────────────────────────────────────────────┐
│                     Operating System                         │
│                                                              │
│  ┌────────────────┐  ┌────────────────┐  ┌────────────────┐│
│  │   Process 1    │  │   Process 2    │  │   Process 3    ││
│  │  Image Gen.    │  │  Feature Ext.  │  │  Data Logger   ││
│  │                │  │                │  │                ││
│  │  ┌──────────┐  │  │  ┌──────────┐  │  │  ┌──────────┐ ││
│  │  │Publisher │──┼──┼─▶│Subscriber│  │  │  │Subscriber│ ││
│  │  └──────────┘  │  │  └──────────┘  │  │  └──────────┘ ││
│  │                │  │       │        │  │       │        ││
│  │                │  │  ┌──────────┐  │  │  ┌──────────┐ ││
│  │                │  │  │Publisher │──┼──┼─▶│ Database │ ││
│  │                │  │  └──────────┘  │  │  └──────────┘ ││
│  └────────────────┘  └────────────────┘  └────────────────┘│
│                                                              │
│         ZeroMQ IPC Layer (tcp://localhost:5555-5556)        │
└──────────────────────────────────────────────────────────────┘
```

### Component Architecture

#### 1. Image Generator (App 1)

**Responsibilities**:
- Read images from filesystem
- Package images with metadata
- Publish to downstream consumers
- Loop indefinitely

**Design Pattern**: Producer Pattern

**Key Classes**:
```cpp
class ImagePublisher {
    - ZeroMQ Context and Socket
    - Image file management
    - Continuous publishing loop
    - Error recovery
}
```

**Data Flow**:
```
File System → Read Image → Create Metadata → Serialize → ZMQ Publish
```

#### 2. Feature Extractor (App 2)

**Responsibilities**:
- Receive images from Image Generator
- Extract SIFT features
- Republish image + features
- Handle processing errors

**Design Pattern**: Processor Pattern (Subscriber-Publisher)

**Key Classes**:
```cpp
class SIFTProcessor {
    - OpenCV SIFT detector
    - Feature extraction logic
    - Data format conversion
}
```

**Data Flow**:
```
ZMQ Subscribe → Deserialize → SIFT Processing → Add Features → Serialize → ZMQ Publish
```

#### 3. Data Logger (App 3)

**Responsibilities**:
- Receive processed data
- Store in SQLite database
- Maintain data integrity
- Provide statistics

**Design Pattern**: Consumer Pattern

**Key Classes**:
```cpp
class DatabaseManager {
    - SQLite connection
    - Transaction management
    - Schema maintenance
    - Query interface
}
```

**Data Flow**:
```
ZMQ Subscribe → Deserialize → Validate → Database Transaction → Commit
```

## Inter-Process Communication (IPC)

### Technology Choice: ZeroMQ

**Rationale**:
ZeroMQ was chosen over alternatives because it provides:

1. **Automatic Reconnection**: Handles network failures gracefully
2. **Buffering**: Built-in message queuing prevents data loss
3. **Patterns**: Pub-Sub pattern fits our use case perfectly
4. **Performance**: Near-native socket performance
5. **Zero Broker**: Eliminates single point of failure

**Comparison with Alternatives**:

| Technology | Pros | Cons | Decision |
|------------|------|------|----------|
| ZeroMQ | Fast, resilient, patterns | Learning curve | ✅ **Chosen** |
| Shared Memory | Fastest | Complex sync, single host | ❌ Too complex |
| Unix Sockets | Simple | Single host only | ❌ Limited |
| gRPC | Modern, typed | Overhead, complexity | ❌ Overkill |
| RabbitMQ | Feature-rich | Requires broker | ❌ Extra dependency |

### Message Protocol Design

#### Protocol Structure

We use a custom binary protocol optimized for image data:

```
Message Header (Common to all messages):
┌──────────────────────────────────────────┐
│ Type (1 byte)                            │
├──────────────────────────────────────────┤

IMAGE_DATA Message:
├──────────────────────────────────────────┤
│ Timestamp (8 bytes, uint64)              │
│ Width (4 bytes, uint32)                  │
│ Height (4 bytes, uint32)                 │
│ Channels (4 bytes, uint32)               │
│ Data Size (4 bytes, uint32)              │
│ Filename Length (4 bytes, uint32)        │
│ Filename (variable, UTF-8)               │
│ Image Data (variable, binary)            │
└──────────────────────────────────────────┘

PROCESSED_DATA Message:
├──────────────────────────────────────────┤
│ [IMAGE_DATA fields...]                   │
│ Keypoint Count (4 bytes, uint32)         │
│ For each keypoint:                       │
│   - X (4 bytes, float)                   │
│   - Y (4 bytes, float)                   │
│   - Size (4 bytes, float)                │
│   - Angle (4 bytes, float)               │
│   - Response (4 bytes, float)            │
│   - Octave (4 bytes, int32)              │
│ Descriptor Count (4 bytes, uint32)       │
│ Descriptor Data (variable, float array)  │
└──────────────────────────────────────────┘
```

**Design Rationale**:
- Binary format for efficiency
- Big-endian byte order for consistency
- Length-prefixed strings for safety
- Type byte for extensibility

### Communication Patterns

#### Pattern 1: Image Publisher (App 1)
```cpp
ZMQ_PUB socket on tcp://*:5555
- Non-blocking sends (ZMQ_DONTWAIT)
- High water mark set to 100
- Linger period: 1000ms
```

#### Pattern 2: Feature Processor (App 2)
```cpp
Subscriber:
  ZMQ_SUB socket on tcp://localhost:5555
  - Subscribe to all messages
  - Receive timeout: 1000ms

Publisher:
  ZMQ_PUB socket on tcp://*:5556
  - Non-blocking sends
  - High water mark set to 100
```

#### Pattern 3: Data Logger (App 3)
```cpp
Subscriber:
  ZMQ_SUB socket on tcp://localhost:5556
  - Subscribe to all messages
  - Receive timeout: 1000ms
  - Large buffer (100MB) for big messages
```

## Data Storage Design

### Database Schema

The SQLite schema is normalized for efficient storage and querying:

```sql
-- Primary table for image metadata and data
CREATE TABLE images (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp INTEGER NOT NULL,           -- Unix timestamp in nanoseconds
    filename TEXT NOT NULL,               -- Original filename
    width INTEGER NOT NULL,               -- Image width in pixels
    height INTEGER NOT NULL,              -- Image height in pixels
    channels INTEGER NOT NULL,            -- Number of color channels
    data_size INTEGER NOT NULL,           -- Size of image data in bytes
    image_data BLOB NOT NULL,             -- Raw image data
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- Keypoints extracted by SIFT
CREATE TABLE keypoints (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    image_id INTEGER NOT NULL,            -- Foreign key to images
    x REAL NOT NULL,                      -- X coordinate
    y REAL NOT NULL,                      -- Y coordinate
    size REAL NOT NULL,                   -- Keypoint size
    angle REAL NOT NULL,                  -- Orientation angle
    response REAL NOT NULL,               -- Corner response
    octave INTEGER NOT NULL,              -- Scale octave
    FOREIGN KEY (image_id) REFERENCES images(id) ON DELETE CASCADE
);

-- SIFT descriptors (128-dimensional vectors)
CREATE TABLE descriptors (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    image_id INTEGER NOT NULL,            -- Foreign key to images
    descriptor_data BLOB NOT NULL,        -- Binary descriptor data
    FOREIGN KEY (image_id) REFERENCES images(id) ON DELETE CASCADE
);

-- Performance indices
CREATE INDEX idx_keypoints_image_id ON keypoints(image_id);
CREATE INDEX idx_descriptors_image_id ON descriptors(image_id);
CREATE INDEX idx_images_filename ON images(filename);
```

**Design Considerations**:

1. **Normalization**: One-to-many relationships reduce redundancy
2. **BLOBs**: Store binary data directly (images, descriptors)
3. **Indices**: Speed up joins and lookups
4. **Foreign Keys**: Maintain referential integrity
5. **Cascade Deletes**: Automatic cleanup of related data

### Transaction Management

All database operations use transactions:

```cpp
BEGIN TRANSACTION;
  INSERT INTO images...
  INSERT INTO keypoints... (multiple)
  INSERT INTO descriptors...
COMMIT;
```

**Benefits**:
- ACID compliance (Atomicity, Consistency, Isolation, Durability)
- All-or-nothing semantics
- Better performance (single fsync)

## Error Handling Strategy

### Design Philosophy

**"Fail gracefully, log extensively, continue when possible"**

### Error Categories and Responses

| Error Type | Example | Response | Recovery |
|------------|---------|----------|----------|
| **Transient** | Network timeout | Log warning, retry | Automatic |
| **Data** | Corrupt message | Log error, skip | Continue with next |
| **Resource** | Out of memory | Log error, cleanup | Reduce buffer size |
| **Fatal** | Cannot bind socket | Log error, exit | Manual restart |

### Implementation

#### Image Generator
```cpp
- File read errors: Log and skip file
- Send failures: Log and continue (non-blocking)
- Invalid directory: Log and exit
```

#### Feature Extractor
```cpp
- Receive timeout: Continue waiting
- SIFT failure: Log and skip frame
- Invalid message: Log and discard
- Send failure: Log and continue
```

#### Data Logger
```cpp
- Receive timeout: Continue waiting
- Database error: Log, rollback transaction
- Disk full: Log error, exit (needs intervention)
```

## Performance Considerations

### Bottleneck Analysis

```
Image Generator:    ~100 fps (limited by disk I/O)
Feature Extractor:  ~2-5 fps (limited by SIFT computation)  ← BOTTLENECK
Data Logger:        ~10 fps (limited by database writes)
```

**System Throughput**: Limited by Feature Extractor (~2-5 fps)

### Optimization Strategies

#### 1. Memory Management
```cpp
// Pre-allocate buffers
std::vector<uint8_t> buffer(50MB);

// Reuse OpenCV matrices
cv::Mat img;  // Reused across frames

// Reserve vector capacity
keypoints.reserve(estimated_count);
```

#### 2. I/O Optimization
```cpp
// Non-blocking ZMQ sends
zmq_send(socket, data, size, ZMQ_DONTWAIT);

// Batch database inserts
BEGIN TRANSACTION;
  // Multiple inserts
COMMIT;

// Memory-mapped file I/O (via OpenCV)
```

#### 3. Parallel Processing
- Each application runs in its own process
- Operating system handles scheduling
- Could add multi-threading within apps for further speedup

### Scalability

**Current Design**:
- Single-threaded applications
- Single machine deployment
- Sequential image processing

**Possible Extensions**:
1. **Horizontal Scaling**: Multiple Feature Extractors
   ```
   Image Gen → [Feature Ext 1]
            → [Feature Ext 2] → Data Logger
            → [Feature Ext 3]
   ```

2. **Pipeline Parallelism**: Different stages on different machines
3. **Data Parallelism**: Process multiple images simultaneously

## Resilience and Fault Tolerance

### Design Principles

1. **No Single Point of Failure**: Each app can restart independently
2. **Graceful Degradation**: System continues with reduced capacity
3. **Self-Healing**: Automatic reconnection via ZeroMQ
4. **Data Durability**: Database provides persistence

### Failure Scenarios

#### Scenario 1: Image Generator Crashes
```
Before: Gen → Ext → Logger
During: ∅ → Ext → Logger (waiting for images)
After:  Gen → Ext → Logger (resumes automatically)
```
**Impact**: Processing pauses until restart. No data loss.

#### Scenario 2: Feature Extractor Crashes
```
Before: Gen → Ext → Logger
During: Gen → ∅ → Logger (no processed data)
After:  Gen → Ext → Logger (resumes automatically)
```
**Impact**: Raw images are lost during downtime. Processed data resumes.

#### Scenario 3: Data Logger Crashes
```
Before: Gen → Ext → Logger
During: Gen → Ext → ∅ (processed data buffered in ZMQ)
After:  Gen → Ext → Logger (processes buffered data)
```
**Impact**: Small buffer of data preserved by ZMQ.

### Testing Resilience

We can verify resilience with these tests:

1. **Test 1**: Start apps in random order
2. **Test 2**: Kill and restart apps during operation
3. **Test 3**: Simulate network delays
4. **Test 4**: Fill disk (database full)
5. **Test 5**: Memory pressure

## Code Organization

### Module Structure

```
Distributed Imaging Services
│
├── Common (Shared by all apps)
│   ├── MessageProtocol (serialization)
│   └── Logger (logging utility)
│
├── Image Generator
│   ├── ImagePublisher (main logic)
│   └── main (entry point)
│
├── Feature Extractor
│   ├── SIFTProcessor (OpenCV interface)
│   └── main (entry point)
│
└── Data Logger
    ├── DatabaseManager (SQLite interface)
    └── main (entry point)
```

### Dependency Graph

```
image_generator:
  - common (MessageProtocol, Logger)
  - OpenCV (image reading)
  - ZeroMQ (publishing)

feature_extractor:
  - common (MessageProtocol, Logger)
  - OpenCV (SIFT processing)
  - ZeroMQ (subscribe + publish)

data_logger:
  - common (MessageProtocol, Logger)
  - SQLite (storage)
  - ZeroMQ (subscribing)
```

### Build System (CMake)

The project uses modern CMake (3.15+):

```cmake
# Top-level structure
project(DistributedImagingServices)
  ├── common library (static)
  ├── image_generator executable
  ├── feature_extractor executable
  └── data_logger executable
```

## Future Enhancements

### Short-Term (Could be added in days)

1. **Configuration Files**: YAML/JSON config instead of CLI args
2. **Metrics Dashboard**: Web UI showing throughput, latency
3. **Compression**: Compress images before sending
4. **Rate Limiting**: Configurable frame rate

### Medium-Term (Could be added in weeks)

1. **Multiple Feature Extractors**: Load balancing
2. **Cloud Storage**: Store to S3/MinIO instead of SQLite
3. **REST API**: Query interface for database
4. **Docker Deployment**: Containerize each application

### Long-Term (Research/Complex)

1. **Distributed Processing**: Kubernetes orchestration
2. **Real-Time Visualization**: Stream processed images to UI
3. **Machine Learning**: Add classification/detection
4. **GPU Acceleration**: CUDA for faster SIFT

## Conclusion

This distributed imaging system demonstrates:

✅ **Modern C++17**: Clean, idiomatic code  
✅ **Robust IPC**: ZeroMQ pub-sub pattern  
✅ **Fault Tolerance**: Graceful failure handling  
✅ **Scalability**: Clear path to horizontal scaling  
✅ **Maintainability**: Well-structured, documented code  
✅ **Performance**: Efficient processing pipeline  

The architecture is production-ready for the specified use case and extensible for future requirements.

---

**Document Version**: 1.0  
**Last Updated**: November 2025  
**Author**: Created for Voyis Technical Interview
