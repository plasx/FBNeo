#!/bin/bash
# Build script for the FBNeo Metal standalone renderer

set -e

# Determine architecture
if [[ "$(uname -m)" == "arm64" ]]; then
    ARCH="arm64"
    echo "Building for Apple Silicon (ARM64)"
else
    ARCH="x86_64"
    echo "Building for Intel (x86_64)"
fi

# Create build directories
echo "Creating build directories..."
mkdir -p build/metal/shaders

# Compile Metal shader
echo "Compiling Metal shaders..."
xcrun -sdk macosx metal -c src/intf/video/metal/metal_standalone.mm.metal -o build/metal/shaders/metal_standalone.air
xcrun -sdk macosx metallib build/metal/shaders/metal_standalone.air -o build/metal/shaders/metal_standalone.metallib

# Create symbolic link in current directory for easier loading
echo "Creating symbolic link to metallib file..."
ln -sf build/metal/shaders/metal_standalone.metallib ./metal_standalone.metallib

# Compile and link
echo "Compiling and linking Metal application..."
clang++ -fobjc-arc -O3 -Wall -arch $ARCH \
    -framework Metal -framework MetalKit -framework Cocoa -framework QuartzCore \
    src/intf/video/metal/metal_standalone.mm \
    -o fbneo_metal_standalone

# Set permissions
chmod +x fbneo_metal_standalone

echo "Build complete. You can now run ./fbneo_metal_standalone" 