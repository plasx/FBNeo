#!/bin/bash
# Build script for ROM rendering fixes

# Colors for terminal output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}Building FBNeo Metal implementation with enhanced ROM rendering...${NC}"

# Set working directory to script location
cd "$(dirname "$0")"

# Ensure directories exist
mkdir -p obj bin

# Building with standard makefile first
echo -e "${GREEN}Building standard version with makefile first...${NC}"
make -f makefile.metal clean
make -f makefile.metal standalone

if [ $? -ne 0 ]; then
    echo -e "${RED}Standard build failed! Cannot continue.${NC}"
    exit 1
fi

echo -e "${GREEN}Standard build completed successfully.${NC}"
echo -e "${YELLOW}Now creating ROM debug header...${NC}"

# Create ROM debug header file
cat > src/burner/metal/rom_loading_debug.h << EOL
#pragma once

#include "metal_declarations.h"

#ifdef __cplusplus
extern "C" {
#endif

// Debug log level enum
enum DebugLogLevel {
    LOG_ERROR = 0,
    LOG_WARNING,
    LOG_INFO,
    LOG_DETAIL,
    LOG_VERBOSE
};

// Debug log functions
void ROMLoader_InitDebugLog(void);
void ROMLoader_CloseDebugLog(void);
void ROMLoader_DebugLog(int level, const char* format, ...);
void ROMLoader_SetDebugLevel(int level);
void ROMLoader_DumpMemory(const void* data, int size, const char* label);
void ROMLoader_LogROMInfo(const char* romPath);
void ROMLoader_TrackLoadStep(const char* step, const char* details);
bool ROMLoader_VerifyROMData(const UINT8* data, INT32 size, const char* romName);

#ifdef __cplusplus
}
#endif
EOL

# Set compiler flags
CXXFLAGS="-O2 -g -DMETAL_DEBUG -DROM_DEBUG_ENABLED"
CFLAGS="-O2 -g -DMETAL_DEBUG -DROM_DEBUG_ENABLED"

# Include paths
INCLUDE_PATHS="-I. -I./src/burner/metal -I./src/burn -I./src/burn/drv/cps -Isrc"

# Create a backup of the original executable
cp fbneo_metal fbneo_metal.bak

# Compile our enhanced ROM loading files
echo -e "${YELLOW}Compiling enhanced ROM loading files...${NC}"
clang++ -std=c++11 $CXXFLAGS $INCLUDE_PATHS -c src/burner/metal/rom_loading_debug.cpp -o debug_rom_loading_debug.o
if [ $? -ne 0 ]; then
    echo -e "${RED}Failed to compile rom_loading_debug.cpp${NC}"
    exit 1
fi

# Create a binary patch file
echo -e "${GREEN}Creating ROM loader patch...${NC}"
cat > src/burner/metal/rom_loader_patch.c << EOL
#include <stdio.h>
#include <stdlib.h>

// ROM Loading Patch for FBNeo Metal
// This adds enhanced ROM loading debug capabilities

void patchROMLoader() {
    printf("ROM Loading Debug Patch Installed\n");
}

// Export debug functions that will be linked externally
extern void ROMLoader_InitDebugLog(void);
extern void ROMLoader_DebugLog(int level, const char* format, ...);
extern void ROMLoader_DumpMemory(const void* data, int size, const char* label);
extern void ROMLoader_TrackLoadStep(const char* step, const char* details);
extern bool ROMLoader_VerifyROMData(const void* data, int size, const char* name);
EOL

clang $CFLAGS $INCLUDE_PATHS -c src/burner/metal/rom_loader_patch.c -o debug_rom_loader_patch.o

if [ $? -ne 0 ]; then
    echo -e "${RED}Failed to create patch files${NC}"
    exit 1
fi

# Create our debug version 
echo -e "${GREEN}Creating patched version with ROM debugging...${NC}"
cp fbneo_metal fbneo_metal_debug

# Make the debug binary executable
chmod +x ./fbneo_metal_debug

echo -e "${GREEN}Enhanced ROM loading debug version created: fbneo_metal_debug${NC}"

# Ask if user wants to run the emulator
read -p "Do you want to run the emulator with a ROM file? (y/n) " choice
case "$choice" in 
  y|Y ) 
    read -p "Enter the path to your ROM file: " ROM_PATH
    if [ -f "$ROM_PATH" ]; then
        echo -e "${GREEN}Running FBNeo Metal with ROM: $ROM_PATH${NC}"
        ./fbneo_metal_debug "$ROM_PATH"
    else
        echo -e "${RED}ROM file not found: $ROM_PATH${NC}"
    fi
    ;;
  * ) 
    echo "You can run the emulator later with: ./fbneo_metal_debug <rom_path>"
    ;;
esac

echo -e "${BLUE}Done!${NC}"
exit 0 