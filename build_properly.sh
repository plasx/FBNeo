#!/bin/bash
# Simple script to build FBNeo Metal with ROM loading
set -e # Exit on any error

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}====================================${NC}"
echo -e "${YELLOW}  Building FBNeo Metal Standalone  ${NC}"
echo -e "${YELLOW}====================================${NC}"

# Clean first
echo -e "${YELLOW}Cleaning previous build...${NC}"
make -f makefile.metal clean

# Build with multiple jobs
echo -e "${YELLOW}Building FBNeo Metal (standalone version)...${NC}"
make -f makefile.metal -j10 standalone

# Check if build succeeded
if [ ! -f fbneo_metal_standalone ]; then
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi

# Create a symbolic link
echo -e "${GREEN}Creating fbneo_metal symlink...${NC}"
ln -sf fbneo_metal_standalone fbneo_metal
chmod +x fbneo_metal

# ROM path
ROM_PATH="${1:-/Users/plasx/dev/ROMs/mvsc.zip}"

# Run with the ROM
echo -e "${GREEN}Running FBNeo Metal with ROM: ${ROM_PATH}${NC}"
./fbneo_metal "${ROM_PATH}" 