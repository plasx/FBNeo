#!/bin/bash
# Build a simplified but integrated Metal implementation for FBNeo

set -e

echo "====================================================="
echo "FBNeo Metal Integrated Build Script"
echo "====================================================="
echo "Building Metal renderer integrated with FBNeo"
echo ""

# Create output directories
mkdir -p obj/intf/video/metal
mkdir -p obj/burner/metal

# Compile Metal implementation
echo "Compiling Metal implementation..."
clang++ -fobjc-arc -g -O2 -Wall -std=c++11 \
    -framework Metal -framework MetalKit -framework Cocoa -framework CoreGraphics \
    -I. -Isrc -Isrc/burn -Isrc/burn/devices -Isrc/burner -Isrc/burner/metal \
    -c src/intf/video/metal/vid_metal.mm -o obj/intf/video/metal/vid_metal.o

# Compile C++ interface
echo "Compiling C++ interface..."
clang++ -g -O2 -Wall -std=c++11 \
    -I. -Isrc -Isrc/burn -Isrc/burn/devices -Isrc/burner -Isrc/burner/metal \
    -c src/intf/video/metal/vid_metal.cpp -o obj/intf/video/metal/vid_metal_cpp.o

# Compile test application
echo "Compiling test application..."
clang++ -fobjc-arc -g -O2 -Wall -std=c++11 \
    -framework Metal -framework MetalKit -framework Cocoa -framework CoreGraphics \
    -I. -Isrc -Isrc/burn -Isrc/burn/devices -Isrc/burner -Isrc/burner/metal \
    -c src/burner/metal/fbneo_metal_integrated.mm -o obj/burner/metal/fbneo_metal_integrated.o

# Link everything together
echo "Linking Metal integrated executable..."
clang++ -fobjc-arc -g -O2 -Wall -std=c++11 \
    -framework Metal -framework MetalKit -framework Cocoa -framework CoreGraphics \
    obj/intf/video/metal/vid_metal.o \
    obj/intf/video/metal/vid_metal_cpp.o \
    obj/burner/metal/fbneo_metal_integrated.o \
    -o fbneo_metal_integrated

# Set executable permissions
chmod +x fbneo_metal_integrated

echo "Build complete. You can now run ./fbneo_metal_integrated" 