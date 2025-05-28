#!/bin/bash
# Build script for the standalone FBNeo Metal app

set -e

echo "Building FBNeo Metal Standalone App..."

# Detect architecture
if [ "$(uname -m)" == "arm64" ]; then
    ARCH="-arch arm64"
else
    ARCH="-arch x86_64"
fi

# Required frameworks
FRAMEWORKS="-framework Metal -framework MetalKit -framework Cocoa -framework CoreGraphics -framework QuartzCore -framework Foundation -framework AppKit -framework CoreAudio -framework AudioToolbox"

# Create output directory if it doesn't exist
mkdir -p bin/metal

# Compile using the metalapp.mm in the root directory
clang++ $ARCH -std=c++17 -O2 metalapp.mm $FRAMEWORKS -o bin/metal/fbneo_metal_app

echo "Build complete. Application is at bin/metal/fbneo_metal_app"
echo "Run with: ./bin/metal/fbneo_metal_app" 