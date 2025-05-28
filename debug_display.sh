#!/bin/bash
# Script to display the debug output in the format requested

# Colors for better visualization
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Compile the debug preview program
echo -e "${BLUE}Compiling debug preview...${NC}"
clang debug_preview.c -o debug_preview

if [ $? -ne 0 ]; then
    echo -e "${YELLOW}Failed to compile debug_preview.c${NC}"
    exit 1
fi

# Get ROM path argument or use default
ROM_PATH="$1"
if [ -z "$ROM_PATH" ]; then
    ROM_PATH="/Users/plasx/dev/ROMs/mvsc.zip"
    echo -e "${YELLOW}No ROM path provided, using default: $ROM_PATH${NC}"
fi

# Clear screen and run the preview
clear
echo -e "${GREEN}Running debug preview with ROM: $ROM_PATH${NC}"
echo -e "${GREEN}====================================================${NC}"
./debug_preview "$ROM_PATH"

# Clean up
rm -f debug_preview 