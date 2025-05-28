#!/bin/bash
# debug_run.sh - Build and run FBNeo with debug output

# Colors for better visibility
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

ROM_PATH="/Users/plasx/dev/ROMs/mvsc.zip"

# Print banner
echo -e "${BLUE}===============================================${NC}"
echo -e "${GREEN}FBNeo Metal Debug Runner${NC}"
echo -e "${BLUE}===============================================${NC}"

# Check if ROM path is provided
if [ "$1" != "" ]; then
    ROM_PATH="$1"
fi

# Check if ROM file exists
if [ ! -f "$ROM_PATH" ]; then
    echo -e "${RED}ROM file not found: ${ROM_PATH}${NC}"
    echo -e "${YELLOW}Please provide a valid ROM path as an argument or update the script.${NC}"
    exit 1
fi

echo -e "${YELLOW}Building FBNeo Metal...${NC}"
make -f makefile.metal clean && make -f makefile.metal -j10

if [ $? -ne 0 ]; then
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi

echo -e "${GREEN}Build successful.${NC}"
echo -e "${YELLOW}Running with ROM: ${ROM_PATH}${NC}"
echo -e "${BLUE}===============================================${NC}"

# Run the emulator in debug-only mode with the ROM path
# Use grep to filter out duplicate messages for cleaner output
./fbneo_metal --debug-only "$ROM_PATH" | uniq

echo -e "${BLUE}===============================================${NC}"
echo -e "${GREEN}Debug session complete.${NC}" 