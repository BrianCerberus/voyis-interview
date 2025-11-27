# Distributed Imaging Services

**Author:** Haobo (Brian) Liu  
**Email:** h349liu@gmail.com  
**Project:** Distributed Imaging Services for Voyis Interview

---

A distributed image processing system written in modern C++ that generates, processes, and stores image data. The system consists of three loosely-coupled applications communicating via ZeroMQ for inter-process communication.

## Table of Contents

- [Overview](#overview)
- [System Architecture](#system-architecture)
- [Requirements](#requirements)
- [Building the Project](#building-the-project)
- [Running the Applications](#running-the-applications)
- [Application Details](#application-details)
- [Design Decisions](#design-decisions)
- [Troubleshooting](#troubleshooting)

## Overview

This project demonstrates a robust, modular distributed system with the following capabilities:

- **Image Generation**: Continuously reads and publishes images from disk
- **Feature Extraction**: Performs SIFT (Scale-Invariant Feature Transform) detection on images
- **Data Storage**: Stores processed images and extracted features in a SQLite database

### Key Features

✅ **Loosely Coupled**: Applications run as independent processes  
✅ **Resilient**: Applications can start in any order and handle failures gracefully  
✅ **Scalable**: Handles images from a few KB to 30+ MB  
✅ **Modern C++17**: Clean, maintainable code using modern C++ features  
✅ **Robust IPC**: Uses ZeroMQ pub-sub pattern for reliable communication  

## System Architecture

```
┌─────────────────────┐      ┌─────────────────────┐      ┌─────────────────────┐
│  Image Generator    │      │ Feature Extractor   │      │   Data Logger       │
│   (App 1)           │      │   (App 2)           │      │   (App 3)           │
│                     │      │                     │      │                     │
│  • Reads images     │─────▶│  • Receives images  │─────▶│  • Receives data    │
│  • Publishes via    │ ZMQ  │  • SIFT detection   │ ZMQ  │  • Stores in SQLite │
│    ZeroMQ           │:5555 │  • Republishes      │:5556 │  • Persists results │
│  • Loops forever    │      │                     │      │                     │
└─────────────────────┘      └─────────────────────┘      └─────────────────────┘
```

### Communication Flow

1. **Image Generator** reads images from disk and publishes them on `tcp://*:5555`
2. **Feature Extractor** subscribes to port 5555, processes images with SIFT, and publishes on `tcp://*:5556`
3. **Data Logger** subscribes to port 5556 and stores everything in a SQLite database

### IPC Mechanism: ZeroMQ

**Why ZeroMQ?**
- **Resilient**: Handles disconnections and reconnections automatically
- **Flexible**: Supports multiple messaging patterns (we use pub-sub)
- **Performant**: Zero-copy messaging, minimal overhead
- **Language Agnostic**: Easy to extend with Python, Java, etc.
- **No Broker Required**: Direct process-to-process communication

## Requirements

### System Requirements

- **OS**: Linux (tested on Ubuntu 20.04+)
- **Compiler**: GCC 7.0+ or Clang 6.0+ (C++17 support required)
- **CMake**: Version 3.15 or higher
- **Memory**: At least 2GB RAM recommended
- **Disk**: 1GB for build artifacts and database

### Dependencies

Install the following packages:

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libopencv-dev \
    libzmq3-dev \
    libsqlite3-dev \
    pkg-config
```

**Fedora/RHEL:**
```bash
sudo dnf install -y \
    gcc-c++ \
    cmake \
    opencv-devel \
    zeromq-devel \
    sqlite-devel \
    pkg-config
```

**Arch Linux:**
```bash
sudo pacman -S base-devel cmake opencv zeromq sqlite pkg-config
```

### Verifying Dependencies

```bash
# Check compiler
g++ --version

# Check CMake
cmake --version

# Check OpenCV
pkg-config --modversion opencv4

# Check ZeroMQ
pkg-config --modversion libzmq

# Check SQLite
pkg-config --modversion sqlite3
```

## Building the Project

### Quick Build

The simplest way to build the project:

```bash
./build.sh
```

This script will:
1. Check all dependencies
2. Create a `build/` directory
3. Run CMake configuration
4. Compile all three applications
5. Place executables in `build/`

### Manual Build

If you prefer to build manually:

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

### Build Output

After a successful build, you'll have these executables in the `build/` directory:
- `image_generator` - App 1
- `feature_extractor` - App 2
- `data_logger` - App 3

## Running the Applications

### Option 1: Run All Together (Recommended)

The easiest way to run all three applications:

```bash
./run_all.sh
```

This will:
- Start all applications in the correct order
- Display logs from all applications
- Use tmux if available (for better terminal management)
- Create log files in `logs/` directory

To stop all applications:
```bash
./stop_all.sh
```

Or press `Ctrl+C` in the terminal.

### Option 2: Run Individually

You can run each application in separate terminals:

**Terminal 1 - Data Logger:**
```bash
./build/data_logger tcp://localhost:5556 imaging_data.db
```

**Terminal 2 - Feature Extractor:**
```bash
./build/feature_extractor tcp://localhost:5555 'tcp://*:5556'
```

**Terminal 3 - Image Generator:**
```bash
./build/image_generator ./deep_sea_imaging/raw 'tcp://*:5555'
```

### Command Line Arguments

#### Image Generator
```bash
./build/image_generator [IMAGE_DIRECTORY] [PUBLISH_ENDPOINT]
```
- `IMAGE_DIRECTORY`: Path to folder containing images (default: `./deep_sea_imaging/raw`)
- `PUBLISH_ENDPOINT`: ZeroMQ endpoint to publish on (default: `tcp://*:5555`)

#### Feature Extractor
```bash
./build/feature_extractor [SUBSCRIBE_ENDPOINT] [PUBLISH_ENDPOINT]
```
- `SUBSCRIBE_ENDPOINT`: Where to receive images from (default: `tcp://localhost:5555`)
- `PUBLISH_ENDPOINT`: Where to publish processed data (default: `tcp://*:5556`)

#### Data Logger
```bash
./build/data_logger [SUBSCRIBE_ENDPOINT] [DATABASE_PATH]
```
- `SUBSCRIBE_ENDPOINT`: Where to receive data from (default: `tcp://localhost:5556`)
- `DATABASE_PATH`: SQLite database file path (default: `imaging_data.db`)

### Testing Resilience

The system is designed to handle process failures gracefully:

**Test 1: Start in any order**
```bash
# Start logger first
./build/data_logger &
sleep 2

# Start generator second
./build/image_generator &
sleep 2

# Start extractor last
./build/feature_extractor &
```

**Test 2: Restart a crashed process**
```bash
# Kill the feature extractor
pkill feature_extractor

# Restart it - it will reconnect automatically
./build/feature_extractor &
```

**Test 3: Stop and restart Image Generator**
```bash
# The system continues when generator restarts
# No data loss, just a gap in processing
```

## Application Details

### App 1: Image Generator

**Purpose**: Simulates a camera by continuously reading images from disk.

**Key Features**:
- Scans directory for image files (PNG, JPG, BMP, TIFF)
- Reads images and packages them with metadata
- Publishes continuously in a loop
- Handles large images (tested up to 50MB+)
- Non-blocking sends to prevent blocking the pipeline

**Logging**:
- Image dimensions and file sizes
- Publishing rate and frame count
- Errors during image reading

### App 2: Feature Extractor

**Purpose**: Performs computer vision processing using SIFT feature detection.

**Key Features**:
- Receives images from Image Generator
- Converts to grayscale for SIFT processing
- Extracts keypoints (distinctive points in the image)
- Computes 128-dimensional descriptors for each keypoint
- Publishes original image + features to Data Logger
- Performance metrics for each frame

**SIFT Details**:
- Scale-Invariant Feature Transform
- Robust to rotation, scale, illumination changes
- Typical output: 500-3000 keypoints per image
- Each keypoint has: position (x, y), size, angle, response, octave

### App 3: Data Logger

**Purpose**: Persists all processed data for future analysis.

**Key Features**:
- Receives processed data from Feature Extractor
- Stores in a normalized SQLite database
- Transactional writes (all or nothing)
- Indexed for fast queries
- Statistics tracking

**Database Schema**:

```sql
-- Images table
CREATE TABLE images (
    id INTEGER PRIMARY KEY,
    timestamp INTEGER,
    filename TEXT,
    width INTEGER,
    height INTEGER,
    channels INTEGER,
    data_size INTEGER,
    image_data BLOB,
    created_at DATETIME
);

-- Keypoints table
CREATE TABLE keypoints (
    id INTEGER PRIMARY KEY,
    image_id INTEGER,
    x REAL,
    y REAL,
    size REAL,
    angle REAL,
    response REAL,
    octave INTEGER,
    FOREIGN KEY (image_id) REFERENCES images(id)
);

-- Descriptors table
CREATE TABLE descriptors (
    id INTEGER PRIMARY KEY,
    image_id INTEGER,
    descriptor_data BLOB,
    FOREIGN KEY (image_id) REFERENCES images(id)
);
```

**Querying the Database**:

```bash
# Open the database
sqlite3 imaging_data.db

# Count stored images
SELECT COUNT(*) FROM images;

# Count total keypoints
SELECT COUNT(*) FROM keypoints;

# Get average keypoints per image
SELECT AVG(kp_count) FROM (
    SELECT COUNT(*) as kp_count 
    FROM keypoints 
    GROUP BY image_id
);

# Find images with most keypoints
SELECT i.filename, COUNT(k.id) as keypoint_count
FROM images i
JOIN keypoints k ON i.id = k.image_id
GROUP BY i.id
ORDER BY keypoint_count DESC
LIMIT 10;
```

## Design Decisions

### 1. ZeroMQ for IPC

**Alternatives Considered**:
- Shared Memory: Fast but complex, requires careful synchronization
- gRPC: Adds complexity, requires protocol buffers
- Unix Sockets: Limited to single machine
- Message Queues (POSIX): Less flexible, platform-specific

**Why ZeroMQ?**:
- Automatic reconnection handling
- Simple pub-sub pattern
- High performance with minimal overhead
- Built-in buffering and backpressure handling

### 2. Custom Binary Protocol

We implement a lightweight binary serialization protocol instead of using Protobuf or JSON:

**Advantages**:
- Zero external dependencies beyond ZeroMQ
- Efficient for large binary data (images)
- Full control over memory layout
- Easy to debug and extend

**Message Format**:
```
[1 byte: MessageType]
[8 bytes: timestamp]
[4 bytes: width]
[4 bytes: height]
[4 bytes: channels]
[4 bytes: data_size]
[4 bytes: filename_length]
[N bytes: filename]
[M bytes: image_data]
[... keypoints and descriptors for processed messages ...]
```

### 3. SQLite for Storage

**Why SQLite?**:
- Zero configuration required
- ACID compliant (data integrity)
- Single file database (easy to backup)
- Excellent for read-heavy workloads
- Sufficient for millions of records

**Alternative**: Could use PostgreSQL/MySQL for a production distributed system.

### 4. Error Handling Strategy

Each application is designed to:
- Log errors but continue running
- Handle missing publishers/subscribers gracefully
- Use timeouts to avoid indefinite blocking
- Validate all received data before processing

### 5. Performance Considerations

- **Non-blocking sends**: Prevent pipeline stalls
- **Buffer management**: Pre-allocated buffers reduce allocations
- **Database transactions**: Batch operations for better I/O
- **Parallel processing**: Each app runs independently

## Troubleshooting

### Build Issues

**Issue**: `opencv not found`
```bash
# Install OpenCV
sudo apt-get install libopencv-dev

# Or build from source if needed
```

**Issue**: `libzmq not found`
```bash
# Install ZeroMQ development package
sudo apt-get install libzmq3-dev
```

**Issue**: CMake version too old
```bash
# Upgrade CMake
sudo apt-get install cmake
# Or download from cmake.org
```

### Runtime Issues

**Issue**: Applications can't connect
```bash
# Check if ports are available
netstat -tuln | grep 5555
netstat -tuln | grep 5556

# Check firewall
sudo ufw status
```

**Issue**: High memory usage
- Reduce publishing rate in Image Generator
- Process fewer images or reduce image size
- Check database size: `du -h imaging_data.db`

**Issue**: No images found
```bash
# Verify image directory
ls -lh ./deep_sea_imaging/raw/

# Check supported formats
# PNG, JPG, JPEG, BMP, TIFF
```

**Issue**: SIFT not available
```bash
# Ensure OpenCV is compiled with contrib modules
# SIFT is now in opencv-contrib
```

### Performance Tuning

**Slow processing?**
- Adjust sleep time in Image Generator (currently 100ms)
- Enable OpenCV optimizations (automatic in release build)
- Use SSD for database storage
- Increase ZeroMQ buffer sizes

**Database growing too large?**
```sql
-- Remove old data
DELETE FROM images WHERE created_at < datetime('now', '-7 days');

-- Vacuum to reclaim space
VACUUM;
```

## Testing and Validation

### Verify System is Working

1. **Check logs**:
```bash
tail -f logs/image_generator.log
tail -f logs/feature_extractor.log
tail -f logs/data_logger.log
```

2. **Query database**:
```bash
sqlite3 imaging_data.db "SELECT COUNT(*) FROM images;"
```

3. **Monitor system resources**:
```bash
top -p $(pgrep -d',' -f 'image_generator|feature_extractor|data_logger')
```

### Expected Performance

On a typical modern system:
- **Image Generator**: 10 frames/second
- **Feature Extractor**: 2-5 frames/second (depending on image complexity)
- **Data Logger**: 5-10 frames/second
- **SIFT extraction**: 100-500ms per frame

## Project Structure

```
.
├── CMakeLists.txt              # Build configuration
├── README.md                   # This file
├── DESIGN.md                   # Detailed design document
├── build.sh                    # Build script
├── run_all.sh                  # Run all apps script
├── stop_all.sh                 # Stop all apps script
├── include/                    # Header files
│   ├── message_protocol.h      # IPC message definitions
│   ├── logger.h                # Logging utility
│   ├── image_publisher.h       # App 1 header
│   ├── sift_processor.h        # App 2 header
│   └── database_manager.h      # App 3 header
├── src/                        # Source files
│   ├── common/                 # Shared code
│   │   ├── message_protocol.cpp
│   │   └── logger.cpp
│   ├── image_generator/        # App 1
│   │   ├── main.cpp
│   │   └── image_publisher.cpp
│   ├── feature_extractor/      # App 2
│   │   ├── main.cpp
│   │   └── sift_processor.cpp
│   └── data_logger/            # App 3
│       ├── main.cpp
│       └── database_manager.cpp
├── deep_sea_imaging/           # Test data
│   └── raw/                    # Image files
├── build/                      # Build output (created by build.sh)
└── logs/                       # Log files (created at runtime)
```

## License

This project is provided as-is for demonstration purposes.

## Authors

Created for Voyis Technical Interview - November 2025

## Demo Video

See the included video demonstration showing:
1. Building the project
2. Starting all applications
3. Monitoring the pipeline
4. Querying stored data
5. Testing resilience (stopping/restarting apps)
