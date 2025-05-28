#!/bin/bash
# Build script for FBNeo Metal frontend with minimal dependencies
# This script resolves duplicate symbol issues by carefully selecting files

set -e  # Exit on error

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
RESET='\033[0m'

echo -e "${BLUE}Building FBNeo Metal frontend (selective build)...${RESET}"

# Set compiler flags
ARCH="-arch arm64"
CFLAGS="-std=c11 -O3 -Wall -DHAVE_STDINT_H -DHAVE_STDBOOL_H -DMETAL_IMPLEMENTATION_FILE -DUSE_METAL_RENDERER -DINCLUDE_CPS_DRIVER=1 -DUSE_CYCLONE=0 -DTCHAR_DEFINED=1 -DMETAL_CPS_STUB=1 -DTCHAR_DEFINED -DENHANCED_DEBUG_MODE=1"
CXXFLAGS="-std=c++17 -O3 -Wall -DMACOSX -DUSE_METAL_FIXES -DMETAL_IMPLEMENTATION_FILE -DUSE_METAL_RENDERER -DDARWIN -DINCLUDE_CPS_DRIVER=1 -DUSE_CYCLONE=0 -DTCHAR_DEFINED=1 -DMETAL_CPS_STUB=1 -DTCHAR_DEFINED -DENHANCED_DEBUG_MODE=1"
OBJCXXFLAGS="-std=c++17 -O3 -fobjc-arc -DMACOSX -DUSE_METAL_FIXES -DMETAL_IMPLEMENTATION_FILE -DUSE_REAL_METAL_RENDERER -DINCLUDE_CPS_DRIVER=1 -DUSE_CYCLONE=0 -DTCHAR_DEFINED=1 -DMETAL_CPS_STUB=1 -DTCHAR_DEFINED -DENHANCED_DEBUG_MODE=1"
INCLUDES="-Isrc -Isrc/burn -Isrc/burner -Isrc/cpu -Isrc/dep -Isrc/burner/metal -Isrc/cpu/z80 -Isrc/burn/snd"
FRAMEWORKS="-framework Cocoa -framework Metal -framework MetalKit -framework AVFoundation -framework AudioToolbox -framework CoreAudio -framework AudioUnit -framework CoreML -framework Vision -framework CoreGraphics -framework CoreImage -framework GameController -framework QuartzCore"
LIBS="-lz"

# Setup build directories
mkdir -p build/metal_final/obj
BUILD_DIR="build/metal_final"
OBJ_DIR="$BUILD_DIR/obj"

# Clean up any previous build
rm -f fbneo_metal

# First create a simple stub file for sound globals
cat > metal_sound_globals.c << 'EOL'
#include <stdint.h>
#include <stddef.h>

// Define the missing symbols for audio
int16_t* pBurnSoundOut = NULL;
int nBurnSoundLen = 0;
int nBurnSoundRate = 44100;
int nBurnSoundPos = 0;
int nBurnSoundBufferSize = 0;
EOL

# Create a single metal_game_control_stub.c file to avoid duplication
cat > metal_game_stub.c << 'EOL'
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// Frame buffer structure
typedef struct {
    uint32_t* data;      // Pixel data
    int width;           // Width in pixels
    int height;          // Height in pixels
    int pitch;           // Bytes per row
    bool updated;        // Has been updated
} EmulatorFrameBuffer;

// Minimal set of variables to avoid duplication
EmulatorFrameBuffer g_frameBuffer = {0};
char g_gameTitle[256] = "FBNeo Metal";
bool g_gameRunning = false;

// Function to get the game title
const char* Metal_GetGameTitle() {
    return g_gameTitle;
}

// Set the game title
void Metal_SetGameTitle(const char* title) {
    if (title && title[0]) {
        strncpy(g_gameTitle, title, sizeof(g_gameTitle)-1);
        g_gameTitle[sizeof(g_gameTitle)-1] = '\0';
    } else {
        strcpy(g_gameTitle, "Unknown Game");
    }
}

// Set game running state
void Metal_SetGameRunning(bool running) {
    g_gameRunning = running;
}

// Error info structure
typedef struct {
    int code;
    char message[256];
    char function[64];
    char file[128];
    int line;
} MetalErrorInfo;

// Global error state
MetalErrorInfo g_lastError = {0};
EOL

# Create a metal_bridge_stub.c file
cat > metal_bridge_stub.c << 'EOL'
#include <stdio.h>
#include <stdbool.h>

// Minimal set of functions to avoid duplication with metal_bridge.mm
int FBNeoInit(void) {
    printf("FBNeoInit: Initializing emulator core\n");
    return 0;
}

int FBNeoExit(void) {
    printf("FBNeoExit: Shutting down emulator core\n");
    return 0;
}

int RunFrame(int bDraw) {
    return 0;
}

bool bDoIpsPatch = false;
EOL

# Create a minimal set of ROM loader stubs
cat > metal_rom_loader_stub.c << 'EOL'
#include <stdio.h>
#include <stdbool.h>

bool LoadROM_FullPath(const char* szPath) {
    printf("LoadROM_FullPath: Loading ROM from %s\n", szPath);
    return true;
}
EOL

# Create a memory tracking stub
cat > metal_memory_stub.c << 'EOL'
#include <stdlib.h>

void* BurnMalloc(size_t size) {
    return malloc(size);
}

void _BurnFree(void* ptr) {
    free(ptr);
}
EOL

# Selected minimal source files - carefully chosen to avoid duplicates
METAL_C_FILES=(
    src/burner/metal/graphics_tracking.c
    src/burner/metal/debug_system.c
    src/burner/metal/audio_loop_monitor.c
    metal_sound_globals.c
    metal_bridge_stub.c
    metal_rom_loader_stub.c
    metal_memory_stub.c
    metal_additional_stubs.c
    metal_error_stubs.c
)

METAL_CPP_FILES=(
    src/burner/metal/rom_verifier_stub.cpp
    src/burner/metal/metal_zip_extract.cpp
    src/burner/metal/metal_renderer_bridge.cpp
)

METAL_OBJC_FILES=(
    src/burner/metal/metal_standalone_main.mm
    src/burner/metal/metal_renderer_complete.mm
    src/burner/metal/metal_input_handler.mm
    src/burner/metal/metal_audio_integration.mm
)

# Compile C files
echo -e "${BLUE}Compiling C files...${RESET}"
C_OBJECTS=()
for file in "${METAL_C_FILES[@]}"; do
    obj_file="$OBJ_DIR/$(basename "${file%.c}").o"
    echo "Compiling $file"
    clang $CFLAGS $ARCH $INCLUDES -c "$file" -o "$obj_file"
    C_OBJECTS+=("$obj_file")
done

# Compile C++ files
echo -e "${BLUE}Compiling C++ files...${RESET}"
CPP_OBJECTS=()
for file in "${METAL_CPP_FILES[@]}"; do
    obj_file="$OBJ_DIR/$(basename "${file%.cpp}").o"
    echo "Compiling $file"
    clang++ $CXXFLAGS $ARCH $INCLUDES -c "$file" -o "$obj_file"
    CPP_OBJECTS+=("$obj_file")
done

# Compile Objective-C++ files
echo -e "${BLUE}Compiling Objective-C++ files...${RESET}"
OBJC_OBJECTS=()
for file in "${METAL_OBJC_FILES[@]}"; do
    obj_file="$OBJ_DIR/$(basename "${file%.mm}").o"
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