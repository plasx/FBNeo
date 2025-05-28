#!/bin/bash

# CPS2 Emulation Test Script for FBNeo Metal
# Usage: ./test_cps2.sh [rom_path]

# Configuration
EMU_PATH="./fbneo_metal"
DEFAULT_ROM_PATH="/path/to/roms/mvsc.zip"
LOG_FILE="cps2_test_log.txt"

# Color codes for terminal output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print header
echo -e "${BLUE}=======================================${NC}"
echo -e "${BLUE}  FBNeo Metal CPS2 Emulation Tester   ${NC}"
echo -e "${BLUE}=======================================${NC}"

# Check for emulator executable
if [ ! -f "$EMU_PATH" ]; then
    echo -e "${RED}Error: Emulator not found at $EMU_PATH${NC}"
    echo "Please build the emulator first with 'make -f makefile.metal'"
    exit 1
fi

# Get ROM path from command line or use default
ROM_PATH=${1:-$DEFAULT_ROM_PATH}
echo -e "${YELLOW}Using ROM: $ROM_PATH${NC}"

# Check if ROM exists
if [ ! -f "$ROM_PATH" ]; then
    echo -e "${RED}Error: ROM file not found at $ROM_PATH${NC}"
    echo "Please provide a valid path to a CPS2 ROM file"
    exit 1
fi

# Start logging
echo "===== CPS2 Test Log - $(date) =====" > $LOG_FILE
echo "ROM: $ROM_PATH" >> $LOG_FILE
echo "=================================" >> $LOG_FILE

# Run basic test (minimal mode)
echo -e "${YELLOW}Running minimal mode test...${NC}"
echo "===== Minimal Mode Test =====" >> $LOG_FILE
$EMU_PATH -v -mode 0 -frames 300 "$ROM_PATH" 2>&1 | tee -a $LOG_FILE
echo -e "${GREEN}Minimal mode test completed${NC}"

# Run CPS2 mode test
echo -e "${YELLOW}Running CPS2 mode test...${NC}"
echo "===== CPS2 Mode Test =====" >> $LOG_FILE
$EMU_PATH -v -mode 1 -frames 300 "$ROM_PATH" 2>&1 | tee -a $LOG_FILE
echo -e "${GREEN}CPS2 mode test completed${NC}"

# Summary
echo -e "${BLUE}=======================================${NC}"
echo -e "${GREEN}Testing completed. Results saved to $LOG_FILE${NC}"
echo -e "${BLUE}=======================================${NC}"

# Check for common error messages in the log
if grep -q "ERROR" $LOG_FILE; then
    echo -e "${RED}Errors detected in the log:${NC}"
    grep "ERROR" $LOG_FILE
    exit 1
fi

echo -e "${GREEN}No errors detected in the log.${NC}"
exit 0 