#!/bin/bash
# Simple direct build script for ROM debugging files

# Colors for terminal output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}Building ROM debugging files directly...${NC}"

# Set compiler flags
CXXFLAGS="-O2 -g -DMETAL_DEBUG -DROM_DEBUG_ENABLED"
CFLAGS="-O2 -g -DMETAL_DEBUG -DROM_DEBUG_ENABLED"

# Include paths
INCLUDE_PATHS="-I. -I./src/burner/metal -I./src/burn -I./src/burn/drv/cps -Isrc"

# Create debug directory
mkdir -p debug

# Create debug header file
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

# Compile our debug objects
echo -e "${GREEN}Compiling ROM loading debug utilities...${NC}"
clang++ -std=c++11 $CXXFLAGS $INCLUDE_PATHS -c src/burner/metal/rom_loading_debug.cpp -o debug/rom_loading_debug.o

if [ $? -ne 0 ]; then
    echo -e "${RED}Failed to compile ROM loading debug. Check the code for errors.${NC}"
    exit 1
fi

echo -e "${GREEN}Compiling zip extraction utilities...${NC}"
clang++ -std=c++11 $CXXFLAGS $INCLUDE_PATHS -c src/burner/metal/metal_zip_extract.cpp -o debug/metal_zip_extract.o

if [ $? -ne 0 ]; then
    echo -e "${RED}Failed to compile zip extraction utilities. Check the code for errors.${NC}"
    exit 1
fi

echo -e "${GREEN}ROM debugging utilities compiled successfully.${NC}"
echo -e "${YELLOW}You need to include these in your project manually:${NC}"
echo -e "  - src/burner/metal/rom_loading_debug.h"
echo -e "  - debug/rom_loading_debug.o"

# Create a simple diagnostic tool
echo -e "${GREEN}Creating ROM diagnostic tool...${NC}"
cat > src/burner/metal/rom_diagnostic.c << EOL
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rom_loading_debug.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <romfile.zip>\n", argv[0]);
        return 1;
    }
    
    ROMLoader_InitDebugLog();
    ROMLoader_SetDebugLevel(LOG_VERBOSE);
    ROMLoader_DebugLog(LOG_INFO, "Analyzing ROM file: %s", argv[1]);
    
    ROMLoader_LogROMInfo(argv[1]);
    
    return 0;
}
EOL

# Compile and link the diagnostic tool
echo -e "${GREEN}Building ROM diagnostic tool...${NC}"
clang $CFLAGS $INCLUDE_PATHS -c src/burner/metal/rom_diagnostic.c -o debug/rom_diagnostic.o

if [ $? -ne 0 ]; then
    echo -e "${RED}Failed to compile ROM diagnostic tool.${NC}"
    exit 1
fi

# Link the diagnostic tool
clang -o rom_diagnostic debug/rom_diagnostic.o debug/rom_loading_debug.o debug/metal_zip_extract.o -lz

if [ $? -ne 0 ]; then
    echo -e "${RED}Failed to link ROM diagnostic tool.${NC}"
    exit 1
fi

# Make the diagnostic tool executable
chmod +x ./rom_diagnostic

echo -e "${GREEN}ROM diagnostic tool created successfully: rom_diagnostic${NC}"
echo -e "${YELLOW}You can use this tool to analyze ROM files without launching the emulator:${NC}"
echo -e "  ./rom_diagnostic <romfile.zip>"

echo -e "${BLUE}Done!${NC}"
exit 0 