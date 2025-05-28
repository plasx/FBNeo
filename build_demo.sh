#!/bin/bash
# Build the enhanced FBNeo Metal demo

# Set working directory to script location
cd "$(dirname "$0")"

echo "========================================="
echo "Building FBNeo Metal Enhanced Demo"
echo "========================================="

# Process command line arguments
RUN_AFTER_BUILD=0
ROM_PATH=""

if [ "$1" == "--run" ]; then
    RUN_AFTER_BUILD=1
    if [ -n "$2" ]; then
        ROM_PATH="$2"
    else
        # Default ROM path
        ROM_PATH="/Users/plasx/dev/ROMs/mvsc.zip"
    fi
fi

# Make sure the build directory exists (including subdirectories)
mkdir -p bin/metal
mkdir -p build/metal/obj/src/burner/metal
mkdir -p build/metal/obj/src/burner/metal/fixes

# Clean any previous build
echo "Cleaning previous build..."
rm -f bin/metal/fbneo_metal
rm -f ./fbneo_metal

# Compile our custom demo main
echo "Compiling enhanced demo main..."
clang -g -O2 -Wall -Isrc/dep/generated -Isrc -Isrc/burn -Isrc/burn/devices -Isrc/burn/snd -Isrc/burn/drv \
-Isrc/burn/drv/capcom -Isrc/burner -Isrc/burner/metal -Isrc/burner/metal/app -Isrc/burner/metal/fixes -Isrc/cpu \
-Isrc/cpu/m68k -Isrc/cpu/z80 -Isrc/intf/video -Isrc/intf/audio -Isrc/intf/input -I/opt/homebrew/include \
-Isrc/burn/cpu_generated -DMETAL_BUILD -fcommon -Wno-everything \
-c src/burner/metal/metal_demo_main.c -o build/metal/obj/src/burner/metal/metal_demo_main.o

if [ $? -ne 0 ]; then
    echo "Error: Failed to compile demo main"
    exit 1
fi

# Compile our enhanced metal stubs
echo "Compiling enhanced metal stubs..."
clang++ -g -O2 -Wall -Isrc/dep/generated -Isrc -Isrc/burn -Isrc/burn/devices -Isrc/burn/snd -Isrc/burn/drv \
-Isrc/burn/drv/capcom -Isrc/burner -Isrc/burner/metal -Isrc/burner/metal/app -Isrc/burner/metal/fixes -Isrc/cpu \
-Isrc/cpu/m68k -Isrc/cpu/z80 -Isrc/intf/video -Isrc/intf/audio -Isrc/intf/input -I/opt/homebrew/include \
-Isrc/burn/cpu_generated -DMETAL_BUILD -fcommon -Wno-everything -std=c++17 -Wno-deprecated-declarations \
-Wno-writable-strings -Wno-c++11-narrowing -Wno-missing-braces -Wno-incompatible-pointer-types -fpermissive \
-c src/burner/metal/metal_stubs.c -o build/metal/obj/src/burner/metal/metal_stubs.o

if [ $? -ne 0 ]; then
    echo "Error: Failed to compile metal stubs"
    exit 1
fi

# Build minimal set of required files
echo "Building minimal required Metal files..."
clang++ -g -O2 -std=c++17 -DMETAL_BUILD -Isrc -Isrc/burner -Isrc/burner/metal -Wno-everything \
-c src/burner/metal/fixes/genre_variables.c -o build/metal/obj/src/burner/metal/fixes/genre_variables.o

if [ $? -ne 0 ]; then
    echo "Error: Failed to compile genre_variables.c"
    exit 1
fi

clang++ -g -O2 -std=c++17 -DMETAL_BUILD -Isrc -Isrc/burner -Isrc/burner/metal -Wno-everything \
-c src/burner/metal/fixes/wrapper_functions.c -o build/metal/obj/src/burner/metal/fixes/wrapper_functions.o

if [ $? -ne 0 ]; then
    echo "Error: Failed to compile wrapper_functions.c"
    exit 1
fi

# Generate stubs for renderer
echo "Generating stubs for renderer..."
cat > src/burner/metal/metal_renderer_stubs.c << 'EOF'
#include <stdio.h>
#include "metal_renderer_c.h"

// Stub implementation of MetalRenderer_Init
int MetalRenderer_Init(void* view) {
    printf("Stub: MetalRenderer_Init called\n");
    return 0;
}

// Stub implementation of MetalRenderer_UpdateFrame
void MetalRenderer_UpdateFrame(const void* frameData, unsigned int width, unsigned int height) {
    printf("Stub: MetalRenderer_UpdateFrame called with %dx%d frame\n", width, height);
}

// Stub implementation of MetalRenderer_GetWidth
int MetalRenderer_GetWidth() {
    return 384;
}

// Stub implementation of MetalRenderer_GetHeight
int MetalRenderer_GetHeight() {
    return 224;
}
EOF

clang++ -g -O2 -std=c++17 -DMETAL_BUILD -Isrc -Isrc/burner -Isrc/burner/metal -Wno-everything \
-c src/burner/metal/metal_renderer_stubs.c -o build/metal/obj/src/burner/metal/metal_renderer_stubs.o

if [ $? -ne 0 ]; then
    echo "Error: Failed to compile metal_renderer_stubs.c"
    exit 1
fi

# Compile the demo stubs
echo "Compiling demo stubs..."
clang -g -O2 -Wall -Isrc/dep/generated -Isrc -Isrc/burn -Isrc/burn/devices -Isrc/burn/snd -Isrc/burn/drv \
-Isrc/burn/drv/capcom -Isrc/burner -Isrc/burner/metal -Isrc/burner/metal/app -Isrc/burner/metal/fixes -Isrc/cpu \
-Isrc/cpu/m68k -Isrc/cpu/z80 -Isrc/intf/video -Isrc/intf/audio -Isrc/intf/input -I/opt/homebrew/include \
-Isrc/burn/cpu_generated -DMETAL_BUILD -fcommon -Wno-everything \
-c src/burner/metal/metal_demo_stubs.c -o build/metal/obj/src/burner/metal/metal_demo_stubs.o

if [ $? -ne 0 ]; then
    echo "Error: Failed to compile demo stubs"
    exit 1
fi

# Link the demo binary
echo "Linking demo binary..."
clang++ -o bin/metal/fbneo_metal \
build/metal/obj/src/burner/metal/metal_demo_main.o \
build/metal/obj/src/burner/metal/fixes/genre_variables.o \
build/metal/obj/src/burner/metal/fixes/wrapper_functions.o \
build/metal/obj/src/burner/metal/metal_renderer_stubs.o \
build/metal/obj/src/burner/metal/metal_demo_stubs.o \
-framework Metal -framework MetalKit -framework Cocoa -framework CoreVideo -framework AudioToolbox \
-framework CoreGraphics -framework CoreML -framework Vision -framework MetalPerformanceShaders

if [ $? -ne 0 ]; then
    echo "Error: Failed to link demo binary"
    exit 1
fi

# Make the binary executable
chmod +x bin/metal/fbneo_metal

# Create a symlink
echo "Creating symlink..."
ln -sf bin/metal/fbneo_metal ./fbneo_metal
chmod +x ./fbneo_metal

echo "========================================="
echo "Build successful!"
echo "Run the demo with: ./fbneo_metal /path/to/rom.zip"
echo "========================================="

# Run the demo if requested
if [ $RUN_AFTER_BUILD -eq 1 ]; then
    if [ -f "$ROM_PATH" ]; then
        echo "Running demo with ROM: $ROM_PATH"
        ./fbneo_metal "$ROM_PATH"
    else
        echo "Warning: ROM file not found at $ROM_PATH"
        echo "Running demo with dummy path..."
        ./fbneo_metal "/dummy/path/mvsc.zip"
    fi
fi

exit 0 