# Distributed Imaging Services - Quick Start

**Author:** Haobo (Brian) Liu  
**Email:** h349liu@gmail.com  
**GitHub:** https://github.com/BrianCerberus/voyis-interview

---

## 🎯 Project Overview

A distributed image processing system with **3 independent C++ applications** communicating via **ZeroMQ IPC**, performing **SIFT feature extraction** on underwater images, and storing results in **SQLite database**.

```
┌─────────────────┐ ZMQ    ┌─────────────────┐ ZMQ    ┌──────────────┐
│ Image Generator │──5555─→│Feature Extractor│──5556─→│ Data Logger  │
│  Reads images   │        │  SIFT detection │        │ SQLite store │
└─────────────────┘        └─────────────────┘        └──────────────┘
```

## 📊 Key Stats

- **Languages:** C++17 (100%)
- **Lines of Code:** ~3,000
- **Applications:** 3 independent processes
- **Tests:** 7 unit tests + 8 resilience scenarios
- **Documentation:** Comprehensive (README + inline docs)
- **Image Dataset:** 2,481 underwater images (~3.5GB)

## 🚀 Quick Start (3 Commands)

```bash
# 1. Download images from OneDrive
./download_images.sh

# 2. Build everything (includes unit tests)
./build.sh

# 3. Run all applications
./run_all.sh
```

That's it! The system will start processing images.

## 📥 Image Dataset

Images hosted on OneDrive (too large for GitHub):
- **Link:** [Download Dataset](https://1drv.ms/f/c/036a5c315af71232/IgDRG0rJhCPxTaiOjrjCU9D_AUQsKgDDI2tqaZl3Ao6ivQY)
- **Size:** 3.5GB (2,481 PNG files)
- **Location:** `deep_sea_imaging/raw/`

## 🔧 System Requirements

- **OS:** Linux (Ubuntu 20.04+, Fedora, Arch)
- **Compiler:** GCC 7+ or Clang 6+ (C++17)
- **CMake:** 3.15+
- **Dependencies:** OpenCV 4.x, ZeroMQ 4.x, SQLite 3.x

Install dependencies:
```bash
# Ubuntu/Debian
sudo apt-get install build-essential cmake libopencv-dev libzmq3-dev libsqlite3-dev

# Or use the script
./install_dependencies.sh
```

## 🏗️ Architecture

### Technology Stack

| Component | Technology | Purpose |
|-----------|-----------|---------|
| **IPC** | ZeroMQ (pub-sub) | Inter-process communication |
| **CV** | OpenCV 4 SIFT | Feature extraction |
| **Database** | SQLite 3 | Data persistence |
| **Build** | CMake + CTest | Build system + testing |
| **Language** | Modern C++17 | Smart pointers, RAII, STL |

### Key Design Principles

✅ **Loosely Coupled** - Apps communicate only via messages  
✅ **Fault Tolerant** - Apps can start in any order, handle failures  
✅ **Scalable** - Handles images from 1KB to 50MB+  
✅ **Testable** - Unit tests + resilience testing  
✅ **Documented** - Inline docs + comprehensive README  

## 🧪 Testing

### Unit Tests (7 tests)
```bash
cd build && ctest --verbose
```

Tests:
- Message serialization/deserialization (4 tests)
- Database operations (3 tests)

### Resilience Tests (8 scenarios)
```bash
./test_resilience.sh
```

Tests all failure combinations:
1. Single app down (3 tests)
2. Two apps down (3 tests)  
3. All apps down (1 test)
4. Runtime crash recovery (1 test)

## 📁 Project Structure

```
voyis-interview/
├── src/                      # Source code
│   ├── image_generator/      # App 1: Image publisher
│   ├── feature_extractor/    # App 2: SIFT processor
│   ├── data_logger/          # App 3: Database storage
│   └── common/               # Shared libraries
├── include/                  # Header files
├── tests/                    # Unit tests
├── build.sh                  # One-command build
├── run_all.sh               # Start all apps
├── test_resilience.sh       # Resilience testing
├── download_images.sh       # Download dataset
└── README.md                # Full documentation
```

## 🎮 Usage Examples

### Start Everything
```bash
./run_all.sh
```

### Individual Applications
```bash
# Terminal 1
./build/data_logger tcp://localhost:5556 imaging_data.db

# Terminal 2
./build/feature_extractor tcp://localhost:5555 'tcp://*:5556'

# Terminal 3
./build/image_generator ./deep_sea_imaging/raw 'tcp://*:5555'
```

### Query Results
```bash
sqlite3 imaging_data.db "SELECT COUNT(*) FROM images;"
sqlite3 imaging_data.db "SELECT COUNT(*) FROM keypoints;"
sqlite3 imaging_data.db "SELECT filename, width, height FROM images LIMIT 5;"
```

### Stop Everything
```bash
./stop_all.sh
```

## 📊 Database Schema

```sql
images (id, filename, width, height, channels, timestamp, image_data)
  ├── keypoints (id, image_id, x, y, size, angle, response, octave)
  └── descriptors (keypoint_id, descriptor_data BLOB[128D])
```

**Sample Query:**
```sql
-- Average keypoints per image
SELECT AVG(kp_count) FROM (
    SELECT COUNT(*) as kp_count 
    FROM keypoints 
    GROUP BY image_id
);
```

## 🔍 What Makes This Project Stand Out

### Technical Excellence
- **Modern C++17**: Smart pointers, RAII, move semantics, STL
- **Distributed Systems**: ZeroMQ pub-sub, automatic reconnection
- **Computer Vision**: OpenCV SIFT for underwater imagery
- **Database Design**: Normalized schema, ACID transactions

### Engineering Practices
- **Automated Testing**: Unit tests integrated in build process
- **Resilience Engineering**: Comprehensive failure scenario testing
- **Build Automation**: One-command build with dependency checking
- **Code Quality**: SOLID principles, error handling, logging

### Documentation Quality
- **Comprehensive README**: Architecture, usage, troubleshooting
- **Inline Documentation**: Well-commented code
- **Design Rationale**: Why each technology was chosen
- **Quick Start Guide**: Get running in 3 commands

### Production Ready
- **Error Handling**: Graceful degradation, detailed logging
- **Performance**: Non-blocking I/O, efficient serialization
- **Maintainability**: Modular design, clear interfaces
- **Scalability**: Handles large datasets, extensible architecture

## 🐛 Troubleshooting

**Images not found?**
```bash
./download_images.sh
# Or manually download from OneDrive link above
```

**Build fails?**
```bash
./install_dependencies.sh
```

**Apps won't connect?**
```bash
# Check ports are free
netstat -tuln | grep -E "5555|5556"
```

**Need help?**
- See full README.md for detailed documentation
- Check logs/ directory for error messages
- Contact: h349liu@gmail.com

## 📝 Performance Metrics

Typical performance on modern hardware:
- **Image Generator:** 10 FPS
- **Feature Extractor:** 2-5 FPS (depends on image complexity)
- **Data Logger:** 5-10 FPS
- **SIFT Extraction:** 100-500ms per frame
- **Typical Keypoints:** 500-3000 per image

## 📚 Additional Resources

- **Full Documentation:** [README.md](README.md)
- **Source Code:** Browse `src/` and `include/` directories  
- **Unit Tests:** See `tests/` directory
- **Build System:** Check `CMakeLists.txt`

## 🤝 Contact

**Haobo (Brian) Liu**  
📧 h349liu@gmail.com  
🔗 GitHub: https://github.com/BrianCerberus/voyis-interview

---

**Project:** Distributed Imaging Services for Voyis Interview  
**Date:** November 2024  
**Status:** ✅ Complete with comprehensive testing and documentation
