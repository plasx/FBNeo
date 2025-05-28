#!/bin/bash
# Run FBNeo Metal with dtach to force unbuffered output

# Colors for better visualization
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Show usage information if no arguments provided
if [ "$#" -eq 0 ]; then
    echo -e "${YELLOW}Usage: $0 /path/to/rom.zip${NC}"
    echo -e "${YELLOW}Example: $0 /Users/plasx/dev/ROMs/mvsc.zip${NC}"
    exit 1
fi

# Get ROM path from arguments
ROM_PATH="$1"

# Create a temporary socket
SOCKET="/tmp/fbneo_dtach_socket"

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

# Run in dtach with no controlling terminal, redirecting output to tee
# The -n flag creates a new session without detaching
# The -E flag makes dtach not interpret Ctrl+\ as detach sequence
dtach -n "$SOCKET" -E ./fbneo_metal "$ROM_PATH" 2>&1 | tee "$LOG_FILE"

echo -e "${BLUE}Emulation ended. Debug log saved to: $LOG_FILE${NC}"

# Offer to view the log file
echo -e "${YELLOW}Would you like to view the log file? (y/n)${NC}"
read -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    less "$LOG_FILE"
fi
