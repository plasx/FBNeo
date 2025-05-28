#!/bin/bash
# Build script for FBNeo Metal frontend
# This script avoids duplicate symbol errors by carefully selecting which files to compile

set -e  # Exit on error

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
RESET='\033[0m'

echo -e "${BLUE}Building FBNeo Metal frontend (fixed build)...${RESET}"

# Set compiler flags
ARCH="-arch arm64"
CFLAGS="-std=c11 -O3 -Wall -DHAVE_STDINT_H -DHAVE_STDBOOL_H -DMETAL_IMPLEMENTATION_FILE -DUSE_METAL_RENDERER -DINCLUDE_CPS_DRIVER=1 -DUSE_CYCLONE=0 -DTCHAR_DEFINED=1 -DMETAL_CPS_STUB=1 -DTCHAR_DEFINED -DENHANCED_DEBUG_MODE=1"
CXXFLAGS="-std=c++17 -O3 -Wall -DMACOSX -DUSE_METAL_FIXES -DMETAL_IMPLEMENTATION_FILE -DUSE_METAL_RENDERER -DDARWIN -DINCLUDE_CPS_DRIVER=1 -DUSE_CYCLONE=0 -DTCHAR_DEFINED=1 -DMETAL_CPS_STUB=1 -DTCHAR_DEFINED -DENHANCED_DEBUG_MODE=1"
OBJCXXFLAGS="-std=c++17 -O3 -fobjc-arc -DMACOSX -DUSE_METAL_FIXES -DMETAL_IMPLEMENTATION_FILE -DUSE_REAL_METAL_RENDERER -DINCLUDE_CPS_DRIVER=1 -DUSE_CYCLONE=0 -DTCHAR_DEFINED=1 -DMETAL_CPS_STUB=1 -DTCHAR_DEFINED -DENHANCED_DEBUG_MODE=1"
INCLUDES="-Isrc -Isrc/burn -Isrc/burner -Isrc/cpu -Isrc/dep -Isrc/burner/metal -Isrc/cpu/z80 -Isrc/burn/snd"
FRAMEWORKS="-framework Cocoa -framework Metal -framework MetalKit -framework AVFoundation -framework AudioToolbox -framework CoreAudio -framework AudioUnit -framework CoreML -framework Vision -framework CoreGraphics -framework CoreImage -framework GameController -framework QuartzCore"
LIBS="-lz"

# Setup build directories
mkdir -p build/metal_fixed/obj
mkdir -p build/metal_fixed/obj/src/burner/metal
BUILD_DIR="build/metal_fixed"
OBJ_DIR="$BUILD_DIR/obj"

# Clean up any previous build
rm -f fbneo_metal

# Selected source files - eliminate duplicates
METAL_C_FILES=(
    src/burner/metal/metal_rom_loader.c
    src/burner/metal/memory_tracking.c
    src/burner/metal/hardware_tracking.c
    src/burner/metal/graphics_tracking.c
    src/burner/metal/debug_system.c
    src/burner/metal/metal_error_handling.c
    src/burner/metal/metal_game_status.c
    src/burner/metal/metal_game_control.c
    src/burner/metal/graphics_tracking_extensions.c
    src/burner/metal/audio_loop_monitor.c
    src/burner/metal/fixes/metal_c_globals.c
    sound_stubs.c
)

METAL_CPP_FILES=(
    src/burner/metal/metal_zip_extract.cpp
    src/burner/metal/rom_verifier_stub.cpp
    src/burner/metal/rom_loading_debug.cpp
)

METAL_OBJC_FILES=(
    src/burner/metal/metal_standalone_main.mm
    src/burner/metal/metal_bridge.mm
    src/burner/metal/metal_renderer_complete.mm
    src/burner/metal/metal_input_handler.mm
    src/burner/metal/metal_audio_integration.mm
)

# Also include stub files (but avoid duplicates)
# Only include either metal_stubs.c OR metal_clean_stubs.c (not both)
STUB_FILES=(
    src/burner/metal/fixes/metal_cps_stubs.c
    src/burner/metal/fixes/metal_sound_stubs.c
    src/burner/metal/fixes/metal_cpu_stubs.c
    src/burner/metal/fixes/metal_c_linkage_functions.c
    # Use clean stubs but not both sets of stubs
    src/burner/metal/fixes/metal_clean_stubs.c
    src/burner/metal/fixes/burn_sound_impl.c
)

# Compile C files
echo -e "${BLUE}Compiling C files...${RESET}"
C_OBJECTS=()
for file in "${METAL_C_FILES[@]}" "${STUB_FILES[@]}"; do
    obj_file="$OBJ_DIR/${file%.c}.o"
    mkdir -p "$(dirname "$obj_file")"
    echo "Compiling $file"
    clang $CFLAGS $ARCH $INCLUDES -c "$file" -o "$obj_file"
    C_OBJECTS+=("$obj_file")
done

# Compile C++ files
echo -e "${BLUE}Compiling C++ files...${RESET}"
CPP_OBJECTS=()
for file in "${METAL_CPP_FILES[@]}"; do
    obj_file="$OBJ_DIR/${file%.cpp}.o"
    mkdir -p "$(dirname "$obj_file")"
    echo "Compiling $file"
    clang++ $CXXFLAGS $ARCH $INCLUDES -c "$file" -o "$obj_file"
    CPP_OBJECTS+=("$obj_file")
done

# Compile Objective-C++ files
echo -e "${BLUE}Compiling Objective-C++ files...${RESET}"
OBJC_OBJECTS=()
for file in "${METAL_OBJC_FILES[@]}"; do
    obj_file="$OBJ_DIR/${file%.mm}.o"
    mkdir -p "$(dirname "$obj_file")"
    echo "Compiling $file"
    clang++ $OBJCXXFLAGS $ARCH $INCLUDES -c "$file" -o "$obj_file"
    OBJC_OBJECTS+=("$obj_file")
done

# Link all objects
echo -e "${BLUE}Linking final executable...${RESET}"
clang++ $OBJCXXFLAGS $ARCH -o fbneo_metal "${C_OBJECTS[@]}" "${CPP_OBJECTS[@]}" "${OBJC_OBJECTS[@]}" $FRAMEWORKS $LIBS

# Check if build succeeded
if [ $? -eq 0 ]; then
    echo -e "${GREEN}Build successful!${RESET}"
    chmod +x fbneo_metal
    echo -e "${BLUE}Output: ${YELLOW}fbneo_metal${RESET}"
    echo -e "${BLUE}Run it with: ${YELLOW}./fbneo_metal /path/to/rom.zip${RESET}"
else
    echo -e "${RED}Build failed!${RESET}"
    exit 1
fi
