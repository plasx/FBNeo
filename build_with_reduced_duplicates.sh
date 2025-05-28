#!/bin/bash

# Colors for better output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;36m'
NC='\033[0m' # No Color

echo -e "${BLUE}Building FBNeo Metal with minimal set of files${NC}"
echo "========================================="

# Clean the build first
echo -e "${YELLOW}Cleaning build...${NC}"
make -f makefile.metal clean

# Create build directory structure
echo -e "${YELLOW}Creating directories...${NC}"
mkdir -p build/metal/obj

# Compile Metal shaders first
echo -e "${YELLOW}Compiling Metal shaders...${NC}"
make -f makefile.metal metal_shaders

# Create a single C file with all the stubs we need
echo -e "${YELLOW}Creating consolidated stubs file...${NC}"

cat > build/metal/stubs.c << EOF
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>  // For bool type

// Basic type definitions
typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef unsigned long long UINT64;
typedef signed char INT8;
typedef signed short INT16;
typedef signed int INT32;
typedef signed long long INT64;

// Debug system functions
void Debug_PrintSectionHeader(const char* section) {
    printf("======= %s =======\n", section);
}

void Debug_Log(int level, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

void Debug_Init() {
    printf("Debug system initialized\n");
}

// ROM functions
bool ROM_CheckIntegrity(const char* path) {
    printf("ROM integrity check called for: %s\n", path);
    return true;
}

bool ROM_Verify(const char* romPath) {
    printf("Verifying ROM: %s\n", romPath);
    return true;
}

// Input functions
void InputMake(void) {
    printf("InputMake called\n");
}

// Graphics functions
void Graphics_InitComponents() {
    printf("Graphics components initialized\n");
}

// Metal bridge functions
int FBNeoInit() {
    printf("FBNeoInit called\n");
    return 0;
}

int FBNeoExit() {
    printf("FBNeoExit called\n");
    return 0;
}

int RunFrame(int draw) {
    printf("RunFrame called with draw=%d\n", draw);
    return 0;
}

int LoadROM_FullPath(const char* path) {
    printf("Loading ROM from path: %s\n", path);
    return 0;
}

// Various Metal function stubs
int Metal_GenerateTestPattern(int type) {
    printf("Metal_GenerateTestPattern(%d) called\n", type);
    return 0;
}

int Metal_ProcessAudio() {
    printf("Metal_ProcessAudio called\n");
    return 0;
}

void Metal_ProcessKeyDown(int keyCode) {
    printf("Metal_ProcessKeyDown(%d) called\n", keyCode);
}

void Metal_ProcessKeyUp(int keyCode) {
    printf("Metal_ProcessKeyUp(%d) called\n", keyCode);
}

void Metal_UpdateInputState() {
    printf("Metal_UpdateInputState called\n");
}

void Metal_UnloadROM() {
    printf("Metal_UnloadROM called\n");
}

// Sound variables
int nBurnSoundLen = 0;
int nBurnSoundRate = 44100;
short* pBurnSoundOut = NULL;

// Frame buffer
typedef struct {
    unsigned int* data;
    int width;
    int height;
    int pitch;
    int updated;
} EmulatorFrameBuffer;

EmulatorFrameBuffer g_frameBuffer = {0};
EOF

# Compile the stubs file
echo -e "${YELLOW}Compiling stubs file...${NC}"
clang -std=c11 -O3 -Wall -DHAVE_STDINT_H -DHAVE_STDBOOL_H -DMETAL_IMPLEMENTATION_FILE -DUSE_METAL_RENDERER -DINCLUDE_CPS_DRIVER=1 -DUSE_CYCLONE=0 -DTCHAR_DEFINED=1 -DMETAL_CPS_STUB=1 -arch arm64 -DTCHAR_DEFINED -DENHANCED_DEBUG_MODE=1 -c build/metal/stubs.c -o build/metal/obj/stubs.o

# Compile the main files directly
echo -e "${YELLOW}Compiling main files...${NC}"
clang++ -std=c++17 -O3 -fobjc-arc -DMACOSX -DUSE_METAL_FIXES -DMETAL_IMPLEMENTATION_FILE -DUSE_REAL_METAL_RENDERER -DINCLUDE_CPS_DRIVER=1 -DUSE_CYCLONE=0 -DTCHAR_DEFINED=1 -DMETAL_CPS_STUB=1 -arch arm64 -DTCHAR_DEFINED -DENHANCED_DEBUG_MODE=1 -Isrc -Isrc/burn -Isrc/burner -Isrc/cpu -Isrc/dep -Isrc/burner/metal -Isrc/cpu/z80 -Isrc/burn/snd -c src/burner/metal/metal_standalone_main.mm -o build/metal/obj/standalone_main.o

# Now link everything together
echo -e "${YELLOW}Linking final executable...${NC}"
clang++ -std=c++17 -O3 -fobjc-arc -DMACOSX -DUSE_METAL_FIXES -DMETAL_IMPLEMENTATION_FILE -DUSE_REAL_METAL_RENDERER -DINCLUDE_CPS_DRIVER=1 -DUSE_CYCLONE=0 -DTCHAR_DEFINED=1 -DMETAL_CPS_STUB=1 -arch arm64 -DTCHAR_DEFINED -DENHANCED_DEBUG_MODE=1 -o build/metal/fbneo_metal build/metal/obj/standalone_main.o build/metal/obj/stubs.o -framework Cocoa -framework Metal -framework MetalKit -framework AVFoundation -framework AudioToolbox -framework CoreAudio -framework AudioUnit -framework CoreML -framework Vision -framework CoreGraphics -framework CoreImage -framework GameController -framework QuartzCore -lz

if [ $? -eq 0 ]; then
    echo -e "${GREEN}Build successful!${NC}"
    cp build/metal/fbneo_metal ./fbneo_metal
    chmod +x ./fbneo_metal
    echo -e "${GREEN}Executable copied to ./fbneo_metal${NC}"
else
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi 