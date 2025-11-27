#!/bin/bash

# Build script for Distributed Imaging Services
# This script builds all three applications

set -e  # Exit on error

echo "=================================="
echo "Building Distributed Imaging Services"
echo "=================================="

# Check for required dependencies
echo "Checking dependencies..."

if ! command -v cmake &> /dev/null; then
    echo "ERROR: cmake not found. Please install cmake."
    exit 1
fi

if ! command -v g++ &> /dev/null; then
    echo "ERROR: g++ not found. Please install g++."
    exit 1
fi

if ! pkg-config --exists opencv4; then
    echo "ERROR: OpenCV not found. Please install opencv (libopencv-dev)."
    exit 1
fi

if ! pkg-config --exists libzmq; then
    echo "ERROR: ZeroMQ not found. Please install zeromq (libzmq3-dev)."
    exit 1
fi

if ! pkg-config --exists sqlite3; then
    echo "ERROR: SQLite3 not found. Please install sqlite3 (libsqlite3-dev)."
    exit 1
fi

echo "All dependencies found!"
echo ""

# Create build directory
echo "Creating build directory..."
mkdir -p build
cd build

# Run CMake
echo "Running CMake..."
cmake ..

# Build
echo "Building..."
make -j$(nproc)

echo ""
echo "=================================="
echo "Running Unit Tests..."
echo "=================================="
make test_report

echo ""
echo "=================================="
echo "Build completed successfully!"
echo "=================================="
echo ""
echo "Executables are located in: build/"
echo "  - image_generator"
echo "  - feature_extractor"
echo "  - data_logger"
echo ""
echo "Test executables:"
echo "  - test_message_protocol"
echo "  - test_database"
echo ""
echo "To run the applications, see run_all.sh or run them individually."
echo "To run tests manually: cd build && ctest --output-on-failure"
