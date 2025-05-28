#!/bin/bash
# Script to run fbneo_metal with forced unbuffered output and tee to a log file

# Colors for better visualization
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Show usage information if no arguments provided
if [ "$#" -eq 0 ]; then
    echo -e "${YELLOW}Usage: $0 /path/to/rom.zip${NC}"
    echo -e "${YELLOW}Example: $0 /Users/plasx/dev/ROMs/mvsc.zip${NC}"
    exit 1
fi

# Get ROM path from arguments
ROM_PATH="$1"

# Create a temporary log directory if it doesn't exist
LOG_DIR="/tmp/fbneo_logs"
mkdir -p $LOG_DIR

# Generate a timestamp for the log file
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
LOG_FILE="$LOG_DIR/fbneo_metal_$TIMESTAMP.log"

echo -e "${GREEN}Running FBNeo Metal with debug output forced to terminal${NC}"
echo -e "${GREEN}ROM: $ROM_PATH${NC}"
echo -e "${GREEN}Debug log will also be saved to: $LOG_FILE${NC}"
echo -e "${GREEN}=======================================================${NC}"

# Run the emulator with stdbuf to force unbuffered output and tee to both terminal and log file
# The 'script' command creates a terminal session which prevents output buffering
script -q /dev/null stdbuf -oL -eL ./fbneo_metal "$ROM_PATH" | tee "$LOG_FILE"

# Display a message when done
echo -e "${BLUE}Emulation ended. Debug log saved to: $LOG_FILE${NC}"

# Offer to view the log file
echo -e "${YELLOW}Would you like to view the log file? (y/n)${NC}"
read -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    less "$LOG_FILE"
fi 