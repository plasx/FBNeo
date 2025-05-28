#!/bin/bash
# Streamlined build script for FBNeo Metal focusing on fixing linking issues

echo "Building FBNeo Metal with fixed linking..."

# Setup directories
mkdir -p build/metal
mkdir -p build/obj

# Set compiler flags
CFLAGS="-O2 -DUSE_METAL_RENDERER -DTCHAR_DEFINED=1"
CXXFLAGS="$CFLAGS -std=c++11"
OBJCXXFLAGS="$CXXFLAGS -fobjc-arc"
INCLUDES="-Isrc -Isrc/burn -Isrc/burner -Isrc/burner/metal -Isrc/cpu -Isrc/dep"
FRAMEWORKS="-framework Cocoa -framework Metal -framework MetalKit -framework CoreGraphics -framework QuartzCore"

# Compile metal_standalone_main.mm explicitly to ensure main is included
echo "Compiling main entry point..."
clang++ $OBJCXXFLAGS $INCLUDES -c src/burner/metal/metal_standalone_main.mm -o build/obj/metal_standalone_main.o

# Compile metal_renderer_bridge.cpp
echo "Compiling bridge implementation..."
clang++ $CXXFLAGS $INCLUDES -c src/burner/metal/metal_renderer_bridge.cpp -o build/obj/metal_renderer_bridge.o

# Compile other stub files
echo "Compiling stub files..."
clang $CFLAGS $INCLUDES -c metal_sound_globals.c -o build/obj/metal_sound_globals.o
clang $CFLAGS $INCLUDES -c metal_bridge_stub.c -o build/obj/metal_bridge_stub.o
clang $CFLAGS $INCLUDES -c metal_additional_stubs.c -o build/obj/metal_additional_stubs.o
clang $CFLAGS $INCLUDES -c metal_error_stubs.c -o build/obj/metal_error_stubs.o
clang $CFLAGS $INCLUDES -c metal_game_stub.c -o build/obj/metal_game_stub.o
clang $CFLAGS $INCLUDES -c metal_rom_loader_stub.c -o build/obj/metal_rom_loader_stub.o
clang $CFLAGS $INCLUDES -c metal_memory_stub.c -o build/obj/metal_memory_stub.o

# Create the missing_stubs.c if it doesn't exist
if [ ! -f "build/metal/missing_stubs.c" ]; then
    mkdir -p build/metal
    echo '#include <stdio.h>
void MissingStub(void) {}' > build/metal/missing_stubs.c
fi

# Compile missing_stubs.c
clang $CFLAGS $INCLUDES -c build/metal/missing_stubs.c -o build/obj/missing_stubs.o

# Link all objects
echo "Linking final executable..."
clang++ -o fbneo_metal build/obj/*.o $FRAMEWORKS

# Ensure executable permissions
chmod +x fbneo_metal

# Check if build succeeded
if [ -f "fbneo_metal" ]; then
    echo "Build successful! Run with: ./fbneo_metal <rom_path>"
else
    echo "Build failed. Check error messages above."
    exit 1
fi 