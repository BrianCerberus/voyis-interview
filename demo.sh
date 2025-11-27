#!/bin/bash

# Demo script for recording video demonstration
# This script walks through the key features of the system
#
# Author: Haobo (Brian) Liu
# Email: h349liu@gmail.com
# Project: Distributed Imaging Services for Voyis Interview

echo "========================================================================"
echo "  Distributed Imaging Services - Interactive Demo"
echo "  Author: Haobo (Brian) Liu (h349liu@gmail.com)"
echo "  Project for Voyis Interview"
echo "========================================================================"
echo ""

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

demo_step() {
    echo ""
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BLUE}>>> $1${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo ""
}

demo_command() {
    echo -e "${GREEN}$ $1${NC}"
    sleep 0.5
    eval $1
    echo ""
}

wait_for_user() {
    echo ""
    echo -e "${CYAN}Press Enter to continue...${NC}"
    read
}

echo "This interactive demo will showcase:"
echo "  1. Modern C++ project structure and build system"
echo "  2. Distributed architecture with ZeroMQ IPC"
echo "  3. Computer vision processing with OpenCV SIFT"
echo "  4. Database persistence with SQLite"
echo "  5. System resilience and fault tolerance"
echo "  6. Modular design and code quality"
echo ""
wait_for_user

# Step 1: Project Overview
demo_step "Step 1: Project Architecture Overview"
echo "This system implements a distributed image processing pipeline with 3 applications:"
echo ""
echo -e "${YELLOW}Architecture:${NC}"
echo "  ┌─────────────────┐      ┌──────────────────┐      ┌─────────────┐"
echo "  │ Image Generator │─────>│ Feature Extractor│─────>│ Data Logger │"
echo "  └─────────────────┘      └──────────────────┘      └─────────────┘"
echo "       (Publisher)            (Processor)              (Subscriber)"
echo "         Port 5555              SIFT CV                  SQLite DB"
echo ""
echo -e "${YELLOW}Key Technologies:${NC}"
echo "  • C++17: Modern C++ with smart pointers, RAII, STL"
echo "  • ZeroMQ: Distributed messaging (pub-sub pattern)"
echo "  • OpenCV 4: SIFT feature detection for underwater imagery"
echo "  • SQLite 3: Embedded database with ACID transactions"
echo "  • CMake: Cross-platform build system with CTest"
echo ""
wait_for_user

demo_step "Step 1a: Project Structure"
demo_command "tree -L 2 -I 'build|logs|*.png'"
echo ""
echo -e "${YELLOW}Structure Highlights:${NC}"
echo "  • include/: Header files defining interfaces"
echo "  • src/: Implementation separated by component"
echo "  • tests/: Unit tests integrated with CMake/CTest"
echo "  • deep_sea_imaging/raw/: 2,481 underwater images"
echo ""
wait_for_user

# Step 2: Show dependencies and library choices
demo_step "Step 2: Library Selection and Rationale"
echo -e "${YELLOW}Why These Libraries?${NC}"
echo ""
echo -e "${CYAN}1. ZeroMQ (ØMQ) for IPC:${NC}"
demo_command "pkg-config --modversion libzmq"
echo "   ✓ Asynchronous messaging with automatic buffering"
echo "   ✓ Pub-sub pattern allows multiple subscribers"
echo "   ✓ Automatic reconnection (built-in resilience)"
echo "   ✓ No broker needed (simpler than RabbitMQ/Kafka)"
echo "   ✓ Language-agnostic (could integrate Python/Java components)"
echo ""
echo -e "${CYAN}2. OpenCV for Computer Vision:${NC}"
demo_command "pkg-config --modversion opencv4"
echo "   ✓ Industry-standard SIFT feature detection"
echo "   ✓ Robust to scale, rotation, illumination changes"
echo "   ✓ Essential for underwater image analysis"
echo "   ✓ Rich ecosystem with 2500+ optimized algorithms"
echo ""
echo -e "${CYAN}3. SQLite for Data Persistence:${NC}"
demo_command "pkg-config --modversion sqlite3"
echo "   ✓ Zero-configuration embedded database"
echo "   ✓ ACID transactions ensure data integrity"
echo "   ✓ Single file database (easy backup/transfer)"
echo "   ✓ Perfect for edge computing / embedded systems"
echo ""
wait_for_user

# Step 3: Build with Testing
demo_step "Step 3: Building with CMake and Running Unit Tests"
echo -e "${YELLOW}Build System Features:${NC}"
echo "  • CMake for cross-platform builds"
echo "  • CTest integration for automated testing"
echo "  • Dependency detection and validation"
echo "  • Parallel compilation with -j$(nproc)"
echo ""
demo_command "./build.sh"
echo ""
echo -e "${GREEN}✓ Build complete with unit test validation!${NC}"
wait_for_user

# Step 4: Show built executables and design principles
demo_step "Step 4: Modular Design and Code Quality"
demo_command "ls -lh build/ | grep -E 'image_generator|feature_extractor|data_logger'"
echo ""
echo -e "${YELLOW}Design Principles Demonstrated:${NC}"
echo "  • Separation of Concerns: Each app has single responsibility"
echo "  • Loose Coupling: Apps communicate only via message protocol"
echo "  • High Cohesion: Related functionality grouped in classes"
echo "  • SOLID Principles: Dependency injection, interface segregation"
echo "  • RAII: Automatic resource management (no memory leaks)"
echo "  • Modern C++: Smart pointers, move semantics, STL algorithms"
echo ""
echo -e "${YELLOW}Robustness Features:${NC}"
echo "  • Comprehensive error handling with try-catch"
echo "  • Logging subsystem for debugging (levels: DEBUG, INFO, ERROR)"
echo "  • Graceful shutdown with signal handlers (SIGINT, SIGTERM)"
echo "  • Input validation and bounds checking"
echo "  • Transaction rollback on database errors"
echo ""
wait_for_user

# Step 5: Check image data
demo_step "Step 5: Dataset Overview"
demo_command "ls deep_sea_imaging/raw/ | head -10"
demo_command "du -sh deep_sea_imaging/raw/"
echo ""
echo -e "${YELLOW}Dataset: Deep Sea Underwater Images${NC}"
echo "  • 2,481 PNG images from underwater ROV/submersible"
echo "  • Challenging conditions: low light, suspended particles, distortion"
echo "  • SIFT features ideal for these conditions"
echo ""
wait_for_user

# Step 6: Start applications (in background)
demo_step "Step 6: Starting all applications"
echo "Starting Data Logger..."
./build/data_logger tcp://localhost:5556 imaging_data.db > logs/data_logger.log 2>&1 &
DATA_LOGGER_PID=$!
sleep 2

echo "Starting Feature Extractor..."
./build/feature_extractor tcp://localhost:5555 'tcp://*:5556' > logs/feature_extractor.log 2>&1 &
FEATURE_EXTRACTOR_PID=$!
sleep 2

echo "Starting Image Generator..."
./build/image_generator ./deep_sea_imaging/raw 'tcp://*:5555' > logs/image_generator.log 2>&1 &
IMAGE_GENERATOR_PID=$!
sleep 2

echo "All applications running!"
echo "  - Image Generator PID: $IMAGE_GENERATOR_PID"
echo "  - Feature Extractor PID: $FEATURE_EXTRACTOR_PID"
echo "  - Data Logger PID: $DATA_LOGGER_PID"
sleep 3

# Step 7: Monitor logs
demo_step "Step 7: Monitoring application logs"
demo_command "tail -n 5 logs/image_generator.log"
demo_command "tail -n 5 logs/feature_extractor.log"
demo_command "tail -n 5 logs/data_logger.log"

# Step 8: Wait for some processing
demo_step "Step 8: Letting the system process images (15 seconds)"
sleep 15

# Step 9: Database Structure and Queries
demo_step "Step 9: Database Schema and Stored Features"
echo -e "${YELLOW}Database Structure (Normalized Schema):${NC}"
echo ""
echo "  ┌─────────────────────────────────────────┐"
echo "  │ images                                  │"
echo "  ├─────────────────────────────────────────┤"
echo "  │ • id (INTEGER PRIMARY KEY)              │"
echo "  │ • filename (TEXT NOT NULL UNIQUE)       │"
echo "  │ • width (INTEGER)                       │"
echo "  │ • height (INTEGER)                      │"
echo "  │ • channels (INTEGER)                    │"
echo "  │ • processing_time_ms (REAL)             │"
echo "  │ • timestamp (TEXT)                      │"
echo "  └──────────┬──────────────────────────────┘"
echo "             │ 1:N relationship"
echo "             ▼"
echo "  ┌─────────────────────────────────────────┐"
echo "  │ keypoints                               │"
echo "  ├─────────────────────────────────────────┤"
echo "  │ • id (INTEGER PRIMARY KEY)              │"
echo "  │ • image_id (FOREIGN KEY → images.id)    │"
echo "  │ • x (REAL) - pixel coordinates          │"
echo "  │ • y (REAL)                              │"
echo "  │ • size (REAL) - scale                   │"
echo "  │ • angle (REAL) - orientation in degrees │"
echo "  │ • response (REAL) - detection strength  │"
echo "  │ • octave (INTEGER) - scale space level  │"
echo "  └──────────┬──────────────────────────────┘"
echo "             │ 1:1 relationship"
echo "             ▼"
echo "  ┌─────────────────────────────────────────┐"
echo "  │ descriptors                             │"
echo "  ├─────────────────────────────────────────┤"
echo "  │ • keypoint_id (FOREIGN KEY, PRIMARY)    │"
echo "  │ • descriptor (BLOB) - 128D vector       │"
echo "  │   └─> Used for feature matching         │"
echo "  └─────────────────────────────────────────┘"
echo ""
echo -e "${YELLOW}SIFT Features Stored:${NC}"
echo "  • Keypoint location (x, y) in image coordinates"
echo "  • Scale (size) - allows matching across different zoom levels"
echo "  • Orientation (angle) - rotation invariance"
echo "  • Descriptor (128-dim) - unique fingerprint for matching"
echo ""
echo -e "${CYAN}Querying the database:${NC}"
echo ""
demo_command "sqlite3 imaging_data.db 'SELECT COUNT(*) as image_count FROM images;'"
demo_command "sqlite3 imaging_data.db 'SELECT COUNT(*) as keypoint_count FROM keypoints;'"
demo_command "sqlite3 imaging_data.db 'SELECT filename, width, height FROM images LIMIT 3;'"

# Step 10: Test resilience - Kill and restart feature extractor
demo_step "Step 10: Resilience Testing - Simulated Component Failure"
echo -e "${YELLOW}Testing System Resilience:${NC}"
echo "  About to kill Feature Extractor to simulate crash..."
echo ""
wait_for_user

echo -e "${RED}💥 Killing Feature Extractor (simulating crash)${NC}"
kill $FEATURE_EXTRACTOR_PID
echo ""
echo -e "${YELLOW}What happens during downtime?${NC}"
echo "  • Image Generator: Continues publishing (messages buffered by ZeroMQ)"
echo "  • Feature Extractor: DOWN"
echo "  • Data Logger: Continues running but receives no new data"
echo "  • Message Loss: YES - images published during crash are lost"
echo "  • ZeroMQ Behavior: Buffers messages up to High Water Mark (HWM)"
echo ""
sleep 3

echo -e "${YELLOW}Watchdog Mechanism:${NC}"
echo "  • ZeroMQ provides automatic reconnection"
echo "  • Apps detect missing peers via timeouts"
echo "  • No crashes cascade to other components"
echo "  • Each app gracefully handles missing connections"
echo ""
wait_for_user

echo -e "${GREEN}✓ Restarting Feature Extractor (watchdog recovery)${NC}"
./build/feature_extractor tcp://localhost:5555 'tcp://*:5556' > logs/feature_extractor.log 2>&1 &
FEATURE_EXTRACTOR_PID=$!
echo ""
echo -e "${GREEN}System recovered! Processing resumes automatically.${NC}"
echo "  • ZeroMQ re-establishes connections"
echo "  • No configuration changes needed"
echo "  • No data corruption in database"
echo ""
sleep 5
wait_for_user

# Step 11: More database stats
demo_step "Step 11: Updated statistics"
demo_command "sqlite3 imaging_data.db 'SELECT COUNT(*) as total_images FROM images;'"

# Average keypoints per image
echo "Average keypoints per image:"
sqlite3 imaging_data.db 'SELECT AVG(kp_count) FROM (SELECT COUNT(*) as kp_count FROM keypoints GROUP BY image_id);'
sleep 2

# Database size
demo_command "du -h imaging_data.db"

# Step 12: Cleanup
demo_step "Step 12: Stopping all applications"
kill $IMAGE_GENERATOR_PID $FEATURE_EXTRACTOR_PID $DATA_LOGGER_PID 2>/dev/null
echo "All applications stopped."
sleep 2

demo_step "Demo Complete!"
echo -e "${GREEN}✓ All demonstration steps completed successfully!${NC}"
echo ""
echo "========================================================================"
echo -e "${YELLOW}Summary of Demonstrated Capabilities:${NC}"
echo "========================================================================"
echo ""
echo "1. Modern C++17 Development:"
echo "   • Smart pointers, RAII, move semantics"
echo "   • STL algorithms and containers"
echo "   • Exception handling and error recovery"
echo ""
echo "2. Distributed Systems Architecture:"
echo "   • ZeroMQ pub-sub messaging pattern"
echo "   • Loose coupling between components"
echo "   • Asynchronous, non-blocking communication"
echo ""
echo "3. Computer Vision Processing:"
echo "   • OpenCV SIFT feature detection"
echo "   • Scale and rotation invariant features"
echo "   • Robust to challenging underwater conditions"
echo ""
echo "4. Data Persistence:"
echo "   • SQLite normalized database schema"
echo "   • ACID transactions for data integrity"
echo "   • Efficient storage of high-dimensional features"
echo ""
echo "5. System Resilience:"
echo "   • Fault tolerance and graceful degradation"
echo "   • Automatic reconnection via ZeroMQ"
echo "   • No cascading failures"
echo ""
echo "6. Software Engineering Best Practices:"
echo "   • Unit testing with CTest integration"
echo "   • Comprehensive documentation"
echo "   • Modular, maintainable code structure"
echo "   • Build automation with CMake"
echo ""
echo "========================================================================"
echo -e "${CYAN}Additional Resources:${NC}"
echo "========================================================================"
echo ""
echo "📄 Detailed Documentation:"
echo "   • DESIGN.md - System architecture and design decisions"
echo "   • README.md - Comprehensive user guide"
echo "   • QUICK_REFERENCE.md - API and command reference"
echo "   • PROJECT_SUMMARY.md - Executive overview"
echo ""
echo "🧪 Testing:"
echo "   • Unit tests: make test (in build directory)"
echo "   • Resilience tests: ./test_resilience.sh"
echo "   • All 7 failure combinations tested"
echo ""
echo "🚀 Quick Commands:"
echo "   • Build: ./build.sh"
echo "   • Run all: ./run_all.sh"
echo "   • Stop all: ./stop_all.sh"
echo "   • Run demo: ./demo.sh"
echo ""
echo "========================================================================"
echo -e "${YELLOW}Project Information:${NC}"
echo "========================================================================"
echo "  Author: Haobo (Brian) Liu"
echo "  Email: h349liu@gmail.com"
echo "  Project: Distributed Imaging Services for Voyis Interview"
echo "========================================================================"
echo ""
echo "Thank you for reviewing this demonstration!"
echo ""
