#!/bin/bash
# Quick Metal build and run script for FBNeo CPS2 games

# Set this to the path of your ROM
ROM_PATH="$1"

if [ -z "$ROM_PATH" ]; then
    echo "Usage: $0 /path/to/rom.zip"
    echo "Please provide a path to your ROM file (e.g. mvsc.zip)"
    exit 1
fi

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
RESET='\033[0m'

echo -e "${BLUE}Building FBNeo Metal frontend...${RESET}"

# Clean any previous build artifacts
make -f makefile.metal clean

# Build with debug info
make -f makefile.metal

# Check if build succeeded
if [ $? -ne 0 ]; then
    echo -e "${RED}Build failed! See errors above.${RESET}"
    exit 1
fi

# Make sure the binary is executable
chmod +x fbneo_metal

echo -e "${GREEN}Build successful!${RESET}"
echo -e "${BLUE}Running game: ${YELLOW}$(basename "$ROM_PATH")${RESET}"

# Run the game with ROM path
./fbneo_metal "$ROM_PATH"

# Capture exit code
EXIT_CODE=$?

if [ $EXIT_CODE -ne 0 ]; then
    echo -e "${RED}Game exited with error code: $EXIT_CODE${RESET}"
    exit $EXIT_CODE
fi

echo -e "${GREEN}Game session ended normally.${RESET}" 