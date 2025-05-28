#!/bin/bash
# debug_gui_run.sh - Build and run FBNeo GUI with debug output

# Colors for better visibility
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

ROM_PATH="/Users/plasx/dev/ROMs/mvsc.zip"

# Print banner
echo -e "${BLUE}===============================================${NC}"
echo -e "${GREEN}FBNeo Metal GUI Debug Runner${NC}"
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

# Capture terminal output in a log file
LOG_FILE="fbneo_debug_$(date +%Y%m%d_%H%M%S).log"
echo -e "${YELLOW}Saving debug output to: ${LOG_FILE}${NC}"

# Run the emulator with enhanced debugging enabled
./fbneo_metal --enhanced-debug "${ROM_PATH}" 2>&1 | tee "${LOG_FILE}"

echo -e "${BLUE}===============================================${NC}"
echo -e "${GREEN}Debug session complete. Log saved to ${LOG_FILE}${NC}" 