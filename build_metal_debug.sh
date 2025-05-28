#!/bin/bash
# Metal debugging build script for FBNeo
set -e

echo "==== FBNeo Metal Debug Build Script ===="

# Clean up previous builds
echo "Cleaning up previous builds..."
make -f makefile.metal clean

# Compile shaders and ensure they're in the correct location
echo "Compiling Metal shaders..."
cd src/burner/metal
./compile_shaders.sh
# Ensure compatibility with existing code
cp -f fbneo_shaders.metallib default.metallib
cd ../../..

# Try to locate the previous successful build files
echo "Checking previous build artifacts..."
find . -name "fbneo*" -type f -print

# Check build directories
echo "Creating build directories..."
mkdir -p obj/macosx/metal
mkdir -p obj/macosx/burn
mkdir -p obj/macosx/burner
mkdir -p obj/macosx/cpu
mkdir -p obj/macosx/dep
mkdir -p src/burner/metal/build

# Build the Metal port
echo "Building the Metal port..."
ARCH="arm64"
if [[ "$(uname -m)" == "arm64" ]]; then
    echo "Building for Apple Silicon (ARM64)"
    ARCH="arm64"
else
    echo "Building for Intel (x86_64)"
    ARCH="x86_64"
fi

# Force a rebuild with explicit settings
make -f makefile FBA_LIBTYPE=macosx MACOS_METAL_BUILD=1 FORCE=1 VERBOSE=1 \
     CFLAGS="-O3 -arch $ARCH -std=c11" \
     CXXFLAGS="-O3 -arch $ARCH -std=c++17 -DMACOSX -DUSE_METAL_FIXES"

# Check for the binary
if [ -f "fbneo" ]; then
    echo "Build successful! Found binary at ./fbneo"
    cp -f fbneo fbneo_metal
    chmod +x fbneo_metal
    echo "Renamed to fbneo_metal"
    
    # Print binary info
    file fbneo_metal
    
    echo "To run:"
    echo "./fbneo_metal [romname]"
else
    echo "Build failed. Binary not found."
    # Search for any files that might be the binary
    find . -name "fbneo*" -type f -print
fi

echo "==== Build process completed ====" 