#!/bin/bash
# Complete build script for FBNeo Metal

echo "Building FBNeo Metal Implementation"

# Clean up previous build
rm -rf build/metal
mkdir -p build/metal/obj/src/burner/metal/fixes
mkdir -p build/metal/obj/src/burner/metal

# Set compiler flags
CFLAGS="-std=c11 -O3 -Wall -DHAVE_STDINT_H -DHAVE_STDBOOL_H -DMETAL_IMPLEMENTATION_FILE -DUSE_METAL_RENDERER -arch arm64 -DENHANCED_DEBUG_MODE=1"
CXXFLAGS="-std=c++17 -O3 -Wall -DMACOSX -DUSE_METAL_FIXES -DMETAL_IMPLEMENTATION_FILE -DUSE_METAL_RENDERER -DDARWIN -arch arm64 -DENHANCED_DEBUG_MODE=1"
OBJCXXFLAGS="-std=c++17 -O3 -fobjc-arc -DMACOSX -DUSE_METAL_FIXES -DMETAL_IMPLEMENTATION_FILE -DUSE_REAL_METAL_RENDERER -arch arm64 -DENHANCED_DEBUG_MODE=1"
INCLUDES="-Isrc -Isrc/burn -Isrc/burner -Isrc/cpu -Isrc/dep -Isrc/burner/metal"
FRAMEWORKS="-framework Cocoa -framework Metal -framework MetalKit -framework AVFoundation -framework AudioToolbox -framework CoreAudio -framework AudioUnit -framework CoreML -framework Vision -framework CoreGraphics -framework CoreImage -framework GameController -framework QuartzCore -lz"

# Compile Metal shaders
echo "Compiling Metal shaders..."
mkdir -p src/burner/metal/.build
xcrun -sdk macosx metal -c src/burner/metal/DefaultShader.metal -o src/burner/metal/.build/DefaultShader.air
xcrun -sdk macosx metal -c src/burner/metal/enhanced_metal_shaders.metal -o src/burner/metal/.build/enhanced_metal_shaders.air
xcrun -sdk macosx metallib src/burner/metal/.build/*.air -o src/burner/metal/fbneo_shaders.metallib
cp -f src/burner/metal/fbneo_shaders.metallib src/burner/metal/default.metallib
cp -f src/burner/metal/fbneo_shaders.metallib .
cp -f src/burner/metal/default.metallib .

# Compile C files
echo "Compiling C files..."
clang $CFLAGS $INCLUDES -c src/burner/metal/fixes/metal_c_globals.c -o build/metal/obj/src/burner/metal/fixes/metal_c_globals.o
clang $CFLAGS $INCLUDES -c src/burner/metal/fixes/metal_c_linkage_functions.c -o build/metal/obj/src/burner/metal/fixes/metal_c_linkage_functions.o
clang $CFLAGS $INCLUDES -c src/burner/metal/fixes/metal_clean_stubs.c -o build/metal/obj/src/burner/metal/fixes/metal_clean_stubs.o
clang $CFLAGS $INCLUDES -c src/burner/metal/fixes/metal_stubs.c -o build/metal/obj/src/burner/metal/fixes/metal_stubs.o
clang $CFLAGS $INCLUDES -c src/burner/metal/fixes/missing_stubs.c -o build/metal/obj/src/burner/metal/fixes/missing_stubs.o
clang $CFLAGS $INCLUDES -c src/burner/metal/fixes/ai_stubs.c -o build/metal/obj/src/burner/metal/fixes/ai_stubs.o
clang $CFLAGS $INCLUDES -c src/burner/metal/fixes/zlib_stubs.c -o build/metal/obj/src/burner/metal/fixes/zlib_stubs.o
clang $CFLAGS $INCLUDES -c src/burner/metal/fixes/rom_path_stubs.c -o build/metal/obj/src/burner/metal/fixes/rom_path_stubs.o
clang $CFLAGS $INCLUDES -c src/burner/metal/fixes/metal_zip_extract.c -o build/metal/obj/src/burner/metal/fixes/metal_zip_extract.o
clang $CFLAGS $INCLUDES -c src/burner/metal/fixes/metal_rom_loader.c -o build/metal/obj/src/burner/metal/fixes/metal_rom_loader.o
clang $CFLAGS $INCLUDES -c src/burner/metal/fixes/memory_tracking.c -o build/metal/obj/src/burner/metal/fixes/memory_tracking.o
clang $CFLAGS $INCLUDES -c src/burner/metal/fixes/hardware_tracking.c -o build/metal/obj/src/burner/metal/fixes/hardware_tracking.o
clang $CFLAGS $INCLUDES -c src/burner/metal/fixes/graphics_tracking.c -o build/metal/obj/src/burner/metal/fixes/graphics_tracking.o
clang $CFLAGS $INCLUDES -c src/burner/metal/fixes/audio_stubs.c -o build/metal/obj/src/burner/metal/fixes/audio_stubs.o
clang $CFLAGS $INCLUDES -c src/burner/metal/debug_system.c -o build/metal/obj/src/burner/metal/debug_system.o
clang $CFLAGS $INCLUDES -c src/burner/metal/audio_loop_monitor.c -o build/metal/obj/src/burner/metal/audio_loop_monitor.o

# Compile C++ files
echo "Compiling C++ files..."
clang++ $CXXFLAGS $INCLUDES -c src/burner/metal/rom_verifier_stub.cpp -o build/metal/obj/src/burner/metal/rom_verifier_stub.o
clang++ $CXXFLAGS $INCLUDES -c src/burner/metal/rom_loading_debug.cpp -o build/metal/obj/src/burner/metal/rom_loading_debug.o
clang++ $CXXFLAGS $INCLUDES -c src/burner/metal/metal_globals.cpp -o build/metal/obj/src/burner/metal/metal_globals.o

# Compile Objective-C++ files
echo "Compiling Objective-C++ files..."
clang++ $OBJCXXFLAGS $INCLUDES -c src/burner/metal/metal_standalone_main.mm -o build/metal/obj/src/burner/metal/metal_standalone_main.o
clang++ $OBJCXXFLAGS $INCLUDES -c src/burner/metal/metal_bridge.mm -o build/metal/obj/src/burner/metal/metal_bridge.o
clang++ $OBJCXXFLAGS $INCLUDES -c src/burner/metal/metal_main.mm -o build/metal/obj/src/burner/metal/metal_main.o

# Link the final binary - IMPORTANT: Include metal_main.o which has the main() function
echo "Linking final binary..."
clang++ $OBJCXXFLAGS $INCLUDES -o fbneo_metal \
  build/metal/obj/src/burner/metal/fixes/metal_c_globals.o \
  build/metal/obj/src/burner/metal/fixes/metal_c_linkage_functions.o \
  build/metal/obj/src/burner/metal/fixes/metal_clean_stubs.o \
  build/metal/obj/src/burner/metal/fixes/metal_stubs.o \
  build/metal/obj/src/burner/metal/fixes/missing_stubs.o \
  build/metal/obj/src/burner/metal/fixes/ai_stubs.o \
  build/metal/obj/src/burner/metal/fixes/zlib_stubs.o \
  build/metal/obj/src/burner/metal/fixes/rom_path_stubs.o \
  build/metal/obj/src/burner/metal/fixes/metal_zip_extract.o \
  build/metal/obj/src/burner/metal/fixes/metal_rom_loader.o \
  build/metal/obj/src/burner/metal/fixes/memory_tracking.o \
  build/metal/obj/src/burner/metal/fixes/hardware_tracking.o \
  build/metal/obj/src/burner/metal/fixes/graphics_tracking.o \
  build/metal/obj/src/burner/metal/fixes/audio_stubs.o \
  build/metal/obj/src/burner/metal/metal_main.o \
  build/metal/obj/src/burner/metal/metal_bridge.o \
  build/metal/obj/src/burner/metal/metal_globals.o \
  build/metal/obj/src/burner/metal/rom_verifier_stub.o \
  build/metal/obj/src/burner/metal/rom_loading_debug.o \
  build/metal/obj/src/burner/metal/debug_system.o \
  build/metal/obj/src/burner/metal/audio_loop_monitor.o \
  $FRAMEWORKS

# Make the binary executable
chmod +x fbneo_metal

echo "Build completed successfully!"
echo "Run the emulator with: ./fbneo_metal [rompath]" 