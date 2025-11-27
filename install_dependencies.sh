#!/bin/bash

# Installation script for dependencies on various Linux distributions
# Run this if dependencies are missing
#
# Author: Haobo (Brian) Liu
# Email: h349liu@gmail.com
# Project: Distributed Imaging Services for Voyis Interview

echo "============================================"
echo "  Dependency Installation Script"
echo "  Distributed Imaging Services"
echo "============================================"
echo ""

# Detect OS
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$NAME
else
    echo "Cannot detect OS. Please install dependencies manually."
    exit 1
fi

echo "Detected OS: $OS"
echo ""

install_ubuntu() {
    echo "Installing dependencies for Ubuntu/Debian..."
    sudo apt-get update
    sudo apt-get install -y \
        build-essential \
        cmake \
        pkg-config \
        libopencv-dev \
        libzmq3-dev \
        libsqlite3-dev \
        git \
        tmux
    echo "Installation complete!"
}

install_fedora() {
    echo "Installing dependencies for Fedora/RHEL..."
    sudo dnf install -y \
        gcc-c++ \
        cmake \
        pkg-config \
        opencv-devel \
        zeromq-devel \
        sqlite-devel \
        git \
        tmux
    echo "Installation complete!"
}

install_arch() {
    echo "Installing dependencies for Arch Linux..."
    sudo pacman -S --needed \
        base-devel \
        cmake \
        pkg-config \
        opencv \
        zeromq \
        sqlite \
        git \
        tmux
    echo "Installation complete!"
}

case "$OS" in
    *Ubuntu*|*Debian*)
        install_ubuntu
        ;;
    *Fedora*|*"Red Hat"*|*CentOS*)
        install_fedora
        ;;
    *Arch*)
        install_arch
        ;;
    *)
        echo "Unsupported OS: $OS"
        echo ""
        echo "Please install these packages manually:"
        echo "  - C++ compiler (GCC 7+ or Clang 6+)"
        echo "  - CMake 3.15+"
        echo "  - OpenCV 4.x (with contrib modules)"
        echo "  - ZeroMQ 4.x"
        echo "  - SQLite 3.x"
        echo "  - pkg-config"
        exit 1
        ;;
esac

echo ""
echo "Verifying installation..."
echo ""

# Verify installations
echo -n "C++ Compiler: "
g++ --version | head -n1

echo -n "CMake: "
cmake --version | head -n1

echo -n "OpenCV: "
pkg-config --modversion opencv4 || echo "NOT FOUND - may need to install opencv-contrib"

echo -n "ZeroMQ: "
pkg-config --modversion libzmq || echo "NOT FOUND"

echo -n "SQLite: "
pkg-config --modversion sqlite3 || echo "NOT FOUND"

echo ""
echo "============================================"
echo "  Dependency check complete!"
echo "============================================"
echo ""

# Download image dataset from OneDrive
download_images() {
    echo ""
    echo "============================================"
    echo "  Image Dataset Download"
    echo "============================================"
    echo ""
    
    if [ -d "deep_sea_imaging/raw" ] && [ "$(ls -A deep_sea_imaging/raw 2>/dev/null | wc -l)" -gt 0 ]; then
        echo "Image dataset already exists in deep_sea_imaging/raw/"
        echo "Found $(ls deep_sea_imaging/raw/*.png 2>/dev/null | wc -l) images."
        read -p "Do you want to re-download? (y/N): " response
        if [[ ! "$response" =~ ^[Yy]$ ]]; then
            echo "Skipping download."
            return 0
        fi
    fi
    
    echo "Downloading deep sea imaging dataset from OneDrive..."
    echo "Source: https://1drv.ms/f/c/036a5c315af71232/IgDRG0rJhCPxTaiOjrjCU9D_AUQsKgDDI2tqaZl3Ao6ivQY"
    echo ""
    
    # Create directory structure
    mkdir -p deep_sea_imaging/raw
    
    # Check if wget or curl is available
    if command -v wget &> /dev/null; then
        DOWNLOADER="wget"
    elif command -v curl &> /dev/null; then
        DOWNLOADER="curl"
    else
        echo "ERROR: Neither wget nor curl is installed."
        echo "Please install wget or curl to download the dataset."
        echo ""
        echo "Manual download instructions:"
        echo "1. Visit: https://1drv.ms/f/c/036a5c315af71232/IgDRG0rJhCPxTaiOjrjCU9D_AUQsKgDDI2tqaZl3Ao6ivQY"
        echo "2. Download the 'raw' folder"
        echo "3. Extract to: deep_sea_imaging/raw/"
        return 1
    fi
    
    # Convert OneDrive share link to direct download link
    # OneDrive share link format: https://1drv.ms/f/c/XXXXXXXX/YYYYYYYY
    # We'll download as a zip file
    ONEDRIVE_LINK="https://1drv.ms/f/c/036a5c315af71232/IgDRG0rJhCPxTaiOjrjCU9D_AUQsKgDDI2tqaZl3Ao6ivQY"
    
    # Create direct download link by replacing /f/ with /download and adding ?download=1
    DOWNLOAD_LINK="https://api.onedrive.com/v1.0/shares/u!aHR0cHM6Ly8xZHJ2Lm1zL2YvYy8wMzZhNWMzMTVhZjcxMjMyL0lnRFJHMHJKaENQeFRhaU9qcmpDVTlEX0FVUXNLZ0RESXRxYVpsM0FvNml2UVk/root/content"
    
    echo "Note: OneDrive direct downloads can be tricky. If automatic download fails,"
    echo "please follow the manual instructions below."
    echo ""
    
    # Try to download using wget or curl
    if [ "$DOWNLOADER" = "wget" ]; then
        wget --max-redirect=10 -O deep_sea_imaging/dataset.zip "$ONEDRIVE_LINK?download=1" 2>&1 | grep -E "Saving|Length|Downloaded"
        RESULT=$?
    else
        curl -L -o deep_sea_imaging/dataset.zip "$ONEDRIVE_LINK?download=1"
        RESULT=$?
    fi
    
    # Check if download was successful
    if [ $RESULT -eq 0 ] && [ -f "deep_sea_imaging/dataset.zip" ] && [ -s "deep_sea_imaging/dataset.zip" ]; then
        echo ""
        echo "Download complete! Extracting..."
        
        # Check if unzip is available
        if command -v unzip &> /dev/null; then
            unzip -q deep_sea_imaging/dataset.zip -d deep_sea_imaging/
            if [ -d "deep_sea_imaging/raw" ]; then
                echo "Extraction complete!"
                IMAGE_COUNT=$(ls deep_sea_imaging/raw/*.png 2>/dev/null | wc -l)
                echo "Found $IMAGE_COUNT images in deep_sea_imaging/raw/"
                rm -f deep_sea_imaging/dataset.zip
                echo ""
                echo "✓ Image dataset ready!"
                return 0
            else
                echo "Warning: Expected 'raw' folder not found after extraction."
                echo "Please check deep_sea_imaging/ directory manually."
            fi
        else
            echo "ERROR: unzip is not installed."
            echo "Please install unzip and run: unzip deep_sea_imaging/dataset.zip -d deep_sea_imaging/"
            return 1
        fi
    else
        echo ""
        echo "============================================"
        echo "  Automatic Download Failed"
        echo "============================================"
        echo ""
        echo "Please download the dataset manually:"
        echo ""
        echo "1. Open this link in your browser:"
        echo "   https://1drv.ms/f/c/036a5c315af71232/IgDRG0rJhCPxTaiOjrjCU9D_AUQsKgDDI2tqaZl3Ao6ivQY"
        echo ""
        echo "2. Download the 'raw' folder (or use OneDrive's download button)"
        echo ""
        echo "3. If downloaded as ZIP, extract it:"
        echo "   unzip ~/Downloads/raw.zip -d deep_sea_imaging/"
        echo ""
        echo "4. If downloaded as folder, move it:"
        echo "   mv ~/Downloads/raw deep_sea_imaging/"
        echo ""
        echo "5. Verify the structure:"
        echo "   ls deep_sea_imaging/raw/*.png | head"
        echo ""
        echo "Expected location: deep_sea_imaging/raw/*.png"
        echo ""
        return 1
    fi
}

# Ask if user wants to download images
echo ""
read -p "Do you want to download the image dataset from OneDrive? (Y/n): " download_choice
if [[ ! "$download_choice" =~ ^[Nn]$ ]]; then
    download_images
fi

echo ""
echo "============================================"
echo "  Setup Complete!"
echo "============================================"
echo ""
echo "Next steps:"
echo "  1. If images downloaded successfully, run: ./build.sh"
echo "  2. If manual download needed, see instructions above"
echo ""
echo "If OpenCV shows 'NOT FOUND', you may need to:"
echo "  - Install opencv-contrib packages"
echo "  - Or build OpenCV from source with contrib modules"
