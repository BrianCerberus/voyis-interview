#!/bin/bash

# Download Image Dataset from OneDrive
#
# Author: Haobo (Brian) Liu
# Email: h349liu@gmail.com
# Project: Distributed Imaging Services for Voyis Interview

set -e

echo "=========================================================================="
echo "  Image Dataset Download Script"
echo "  Distributed Imaging Services"
echo "=========================================================================="
echo ""

# OneDrive sharing link
ONEDRIVE_SHARE_LINK="https://1drv.ms/f/c/036a5c315af71232/IgDRG0rJhCPxTaiOjrjCU9D_AUQsKgDDI2tqaZl3Ao6ivQY"

# Check if images already exist
if [ -d "deep_sea_imaging/raw" ] && [ "$(ls -A deep_sea_imaging/raw/*.png 2>/dev/null | wc -l)" -gt 0 ]; then
    IMAGE_COUNT=$(ls deep_sea_imaging/raw/*.png 2>/dev/null | wc -l)
    echo "✓ Image dataset already exists!"
    echo "  Location: deep_sea_imaging/raw/"
    echo "  Images found: $IMAGE_COUNT"
    echo ""
    read -p "Do you want to re-download? (y/N): " response
    if [[ ! "$response" =~ ^[Yy]$ ]]; then
        echo "Skipping download."
        exit 0
    fi
    echo ""
    echo "Removing existing images..."
    rm -rf deep_sea_imaging/raw/*
fi

# Create directory
echo "Creating directory structure..."
mkdir -p deep_sea_imaging/raw
echo "✓ Created: deep_sea_imaging/raw/"
echo ""

# Check for required tools
echo "Checking required tools..."
MISSING_TOOLS=0

if ! command -v wget &> /dev/null && ! command -v curl &> /dev/null; then
    echo "✗ Missing: wget or curl (needed for downloading)"
    MISSING_TOOLS=1
else
    if command -v wget &> /dev/null; then
        echo "✓ Found: wget"
        DOWNLOADER="wget"
    else
        echo "✓ Found: curl"
        DOWNLOADER="curl"
    fi
fi

if ! command -v unzip &> /dev/null; then
    echo "✗ Missing: unzip (needed for extraction)"
    MISSING_TOOLS=1
else
    echo "✓ Found: unzip"
fi

if [ $MISSING_TOOLS -eq 1 ]; then
    echo ""
    echo "Please install missing tools:"
    echo "  Ubuntu/Debian: sudo apt-get install wget unzip"
    echo "  Fedora/RHEL:   sudo dnf install wget unzip"
    echo "  Arch:          sudo pacman -S wget unzip"
    echo ""
    exit 1
fi

echo ""
echo "=========================================================================="
echo "  Download Methods"
echo "=========================================================================="
echo ""
echo "OneDrive direct download via API can be unreliable. Choose a method:"
echo ""
echo "  1) Try automatic download (may fail due to OneDrive restrictions)"
echo "  2) Manual download with browser (recommended)"
echo "  3) Use OneDrive sync client"
echo ""
read -p "Choose method (1/2/3): " method

case $method in
    1)
        echo ""
        echo "Attempting automatic download..."
        echo ""
        
        # Try direct download link
        # Convert share link to download link
        TEMP_ZIP="deep_sea_imaging/dataset_temp.zip"
        
        # OneDrive direct download attempts
        # Method 1: Add download parameter
        echo "Trying download method 1..."
        if [ "$DOWNLOADER" = "wget" ]; then
            wget --max-redirect=5 --timeout=30 -O "$TEMP_ZIP" "${ONEDRIVE_SHARE_LINK}?download=1" 2>&1 || true
        else
            curl -L --max-redirs 5 --max-time 30 -o "$TEMP_ZIP" "${ONEDRIVE_SHARE_LINK}?download=1" 2>&1 || true
        fi
        
        # Check if we got a valid zip file
        if [ -f "$TEMP_ZIP" ] && [ -s "$TEMP_ZIP" ]; then
            FILE_TYPE=$(file -b "$TEMP_ZIP" 2>/dev/null || echo "unknown")
            if [[ "$FILE_TYPE" == *"Zip"* ]] || [[ "$FILE_TYPE" == *"archive"* ]]; then
                echo "✓ Download successful!"
                echo "  File size: $(du -h "$TEMP_ZIP" | cut -f1)"
                echo ""
                echo "Extracting images..."
                
                if unzip -q "$TEMP_ZIP" -d deep_sea_imaging/ 2>&1; then
                    # Find where the images are
                    if [ -d "deep_sea_imaging/raw" ]; then
                        IMAGE_COUNT=$(ls deep_sea_imaging/raw/*.png 2>/dev/null | wc -l)
                        echo "✓ Extraction complete!"
                        echo "  Images extracted: $IMAGE_COUNT"
                        rm -f "$TEMP_ZIP"
                        echo ""
                        echo "=========================================================================="
                        echo "  ✓ Download Complete!"
                        echo "=========================================================================="
                        exit 0
                    else
                        echo "⚠ Extracted but 'raw' folder not found. Checking structure..."
                        find deep_sea_imaging -name "*.png" | head -5
                    fi
                else
                    echo "✗ Extraction failed"
                fi
            else
                echo "✗ Downloaded file is not a valid ZIP archive"
                echo "  File type: $FILE_TYPE"
            fi
            rm -f "$TEMP_ZIP"
        fi
        
        echo ""
        echo "Automatic download failed. Please use manual method."
        ;;
    
    3)
        echo ""
        echo "Using OneDrive sync client:"
        echo ""
        echo "1. Install OneDrive client for Linux:"
        echo "   https://github.com/abraunegg/onedrive"
        echo ""
        echo "2. Sync the shared folder"
        echo ""
        echo "3. Copy to project directory:"
        echo "   cp -r ~/OneDrive/raw deep_sea_imaging/"
        echo ""
        exit 0
        ;;
esac

# Manual download instructions
echo ""
echo "=========================================================================="
echo "  Manual Download Instructions (Recommended)"
echo "=========================================================================="
echo ""
echo "Follow these steps to download the image dataset:"
echo ""
echo "Step 1: Open the link in your browser"
echo "----------------------------------------"
echo "$ONEDRIVE_SHARE_LINK"
echo ""
echo "Step 2: Download the files"
echo "----------------------------------------"
echo "  • Click the 'Download' button at the top of the page"
echo "  • OneDrive will prepare a ZIP file"
echo "  • Save it to your Downloads folder"
echo ""
echo "Step 3: Extract to project directory"
echo "----------------------------------------"
echo "Run one of these commands:"
echo ""
echo "  # If downloaded as raw.zip:"
echo "  unzip ~/Downloads/raw.zip -d deep_sea_imaging/"
echo ""
echo "  # If downloaded as a different name, find it first:"
echo "  ls -lh ~/Downloads/*.zip"
echo "  unzip ~/Downloads/[filename].zip -d deep_sea_imaging/"
echo ""
echo "  # Or if you extracted it already:"
echo "  mv ~/Downloads/raw deep_sea_imaging/"
echo ""
echo "Step 4: Verify the installation"
echo "----------------------------------------"
echo "  ls deep_sea_imaging/raw/*.png | wc -l"
echo "  # Should show 2481 images"
echo ""
echo "Step 5: Once images are in place, build the project"
echo "----------------------------------------"
echo "  ./build.sh"
echo ""
echo "=========================================================================="
echo ""
echo "Expected directory structure:"
echo "  deep_sea_imaging/"
echo "    └── raw/"
echo "        ├── image_0001.png"
echo "        ├── image_0002.png"
echo "        └── ... (2481 total images)"
echo ""
echo "=========================================================================="

