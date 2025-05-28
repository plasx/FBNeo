#!/bin/bash
# FBNeo Metal Standalone Builder
set -e  # Exit on error

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}==========================================${NC}"
echo -e "${BLUE}  FBNeo Metal Standalone Builder         ${NC}"
echo -e "${BLUE}==========================================${NC}"

# Clean the build
echo -e "${YELLOW}Cleaning previous build...${NC}"
make -f makefile.metal clean

# Run the build with multiple jobs
echo -e "${YELLOW}Building FBNeo Metal standalone...${NC}"
make -f makefile.metal -j10 standalone

# Check if build succeeded
if [ ! -f "fbneo_metal_standalone" ]; then
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi

# Make it executable and create a symlink
echo -e "${GREEN}Build successful!${NC}"
chmod +x fbneo_metal_standalone
ln -sf fbneo_metal_standalone fbneo_metal

# Specify the ROM path (use provided arg or default)
ROM_PATH="${1:-/Users/plasx/dev/ROMs/mvsc.zip}"

# Run the emulator with verbose logging
echo -e "${YELLOW}Running FBNeo Metal with ROM: ${ROM_PATH}${NC}"
echo -e "${YELLOW}Press Ctrl+C to exit${NC}"
./fbneo_metal "$ROM_PATH" 