#!/bin/bash
# Simple script to build FBNeo Metal in basic demo mode
set -e # Exit on any error

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}====================================${NC}"
echo -e "${YELLOW}   Building FBNeo Metal Demo       ${NC}"
echo -e "${YELLOW}====================================${NC}"

# Clean first
echo -e "${YELLOW}Cleaning previous build...${NC}"
make -f makefile.metal clean

# Build the demo version
echo -e "${YELLOW}Building FBNeo Metal (demo version)...${NC}"
make -f makefile.metal demo

# Check if build succeeded
if [ ! -f fbneo_metal_demo ]; then
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi

# Make executable
chmod +x fbneo_metal_demo

# Create a symbolic link for convenience
ln -sf fbneo_metal_demo fbneo_metal

# ROM path (just for display - demo will simply render a test pattern)
ROM_PATH="${1:-/Users/plasx/dev/ROMs/mvsc.zip}"

# Run the demo
echo -e "${GREEN}Running FBNeo Metal Demo (ignoring ROM: ${ROM_PATH})${NC}"
./fbneo_metal_demo 