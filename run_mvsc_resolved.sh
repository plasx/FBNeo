#!/bin/bash
# Run script for Marvel vs Capcom with resolved Metal implementation

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
RESET='\033[0m'

# Check if the executable exists
if [ ! -f ./fbneo_metal ]; then
  echo -e "${RED}Error: fbneo_metal executable not found.${RESET}"
  echo -e "${YELLOW}Please run build_final_resolved.sh first.${RESET}"
  exit 1
fi

# Default ROM path - adjust as needed
ROM_PATH="$HOME/Downloads/mvsc.zip"

# Check if a different ROM path was provided
if [ "$1" != "" ]; then
  ROM_PATH="$1"
fi

# Verify ROM file exists
if [ ! -f "$ROM_PATH" ]; then
  echo -e "${RED}Error: ROM file not found at $ROM_PATH${RESET}"
  echo -e "${YELLOW}Usage: ./run_mvsc_resolved.sh [path/to/mvsc.zip]${RESET}"
  exit 1
fi

echo -e "${BLUE}Running Marvel vs Capcom with Metal implementation...${RESET}"
echo -e "${BLUE}ROM: ${YELLOW}$ROM_PATH${RESET}"

# Set up debug environment variables
export FBNEO_METAL_DEBUG=1
export FBNEO_METAL_LOG_LEVEL=4  # Verbose logging

# Run the emulator with the ROM
./fbneo_metal "$ROM_PATH"

# Check exit status
if [ $? -ne 0 ]; then
  echo -e "${RED}Emulator exited with an error.${RESET}"
  exit 1
else
  echo -e "${GREEN}Emulator session completed successfully.${RESET}"
fi 