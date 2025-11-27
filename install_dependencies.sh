#!/bin/bash

# Installation script for dependencies on various Linux distributions
# Run this if dependencies are missing

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
echo "If all dependencies are installed, run:"
echo "  ./build.sh"
echo ""
echo "If OpenCV shows 'NOT FOUND', you may need to:"
echo "  - Install opencv-contrib-python"
echo "  - Or build OpenCV from source with contrib modules"
