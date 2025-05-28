#!/bin/bash
# Direct build script for FBNeo Metal that focuses on creating a working executable

set -e  # Exit on error

echo "Building FBNeo Metal (Simplified Build)"
echo "======================================="

# Ensure build directories exist
mkdir -p build/obj

# Set compiler flags
CFLAGS="-O2 -DUSE_METAL_RENDERER -DTCHAR_DEFINED=1"
CXXFLAGS="$CFLAGS -std=c++11"
OBJCXXFLAGS="$CXXFLAGS -fobjc-arc"
INCLUDES="-Isrc -Isrc/burn -Isrc/burner -Isrc/burner/metal -Isrc/cpu -Isrc/dep"
FRAMEWORKS="-framework Cocoa -framework Metal -framework MetalKit -framework CoreGraphics -framework QuartzCore"

# Clean previous build
rm -f fbneo_metal
rm -f build/obj/*.o

# Compile the main entry point first
echo "Compiling main entry point..."
clang++ $OBJCXXFLAGS $INCLUDES -c src/burner/metal/metal_standalone_main.mm -o build/obj/metal_standalone_main.o

# Compile the renderer bridge
echo "Compiling renderer bridge..."
clang++ $CXXFLAGS $INCLUDES -c src/burner/metal/metal_renderer_bridge.cpp -o build/obj/metal_renderer_bridge.o

# Compile our comprehensive C stubs
echo "Compiling C stubs..."
clang $CFLAGS $INCLUDES -c build/metal_complete_stubs.c -o build/obj/metal_complete_stubs.o

# Compile our C++ stubs
echo "Compiling C++ stubs..."
clang++ $CXXFLAGS $INCLUDES -c build/metal_cpp_stubs.cpp -o build/obj/metal_cpp_stubs.o

# Link everything together
echo "Linking final executable..."
clang++ -o fbneo_metal build/obj/metal_standalone_main.o build/obj/metal_renderer_bridge.o build/obj/metal_complete_stubs.o build/obj/metal_cpp_stubs.o $FRAMEWORKS

# Ensure the executable exists and has proper permissions
if [ -f "fbneo_metal" ]; then
    chmod +x fbneo_metal
    # Create a symbolic link as an alternate name if needed
    ln -sf fbneo_metal fbneo
    echo "Build successful! Executable created: fbneo_metal"
    echo "Run with: ./fbneo_metal /path/to/mvsc.zip"
else
    echo "ERROR: Build failed - executable not created"
    exit 1
fi 