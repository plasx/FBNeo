#!/bin/bash
# Build and run Marvel vs Capcom with Metal frontend

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
RESET='\033[0m'

echo -e "${BLUE}Building FBNeo Metal...${RESET}"
make -f makefile.metal clean && make -f makefile.metal -j10

if [ $? -ne 0 ]; then
    echo -e "${RED}Build failed!${RESET}"
    exit 1
fi

echo -e "${GREEN}Build succeeded!${RESET}"

# Default ROM path - adjust as needed
ROM_PATH="$HOME/dev/ROMs/mvsc.zip"

# Check if a different ROM path was provided
if [ "$1" != "" ]; then
    ROM_PATH="$1"
fi

# Verify ROM file exists
if [ ! -f "$ROM_PATH" ]; then
    echo -e "${RED}Error: ROM file not found at $ROM_PATH${RESET}"
    echo -e "${YELLOW}Usage: ./build_and_run_mvsc.sh [path/to/mvsc.zip]${RESET}"
    exit 1
fi

echo -e "${BLUE}Running Marvel vs Capcom with Metal implementation...${RESET}"
echo -e "${BLUE}ROM: ${YELLOW}$ROM_PATH${RESET}"

# Run the emulator
./fbneo_metal "$ROM_PATH"
