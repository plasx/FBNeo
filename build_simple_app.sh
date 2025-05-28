#!/bin/bash
# Build script for the simple FBNeo Metal app

set -e

echo "Building Simple FBNeo Metal Demo App..."

# Detect architecture
if [ "$(uname -m)" == "arm64" ]; then
    ARCH="-arch arm64"
else
    ARCH="-arch x86_64"
fi

# Required frameworks
FRAMEWORKS="-framework Metal -framework MetalKit -framework Cocoa -framework CoreGraphics -framework QuartzCore -framework Foundation -framework AppKit"

# Create output directory if it doesn't exist
mkdir -p bin/metal

# Compile
clang++ $ARCH -std=c++17 -O2 simple_metalapp.mm $FRAMEWORKS -o bin/metal/simple_fbneo_app

echo "Build complete. Application is at bin/metal/simple_fbneo_app"
echo "Run with: ./bin/metal/simple_fbneo_app" 