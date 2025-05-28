#!/bin/bash

# Colors for terminal output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

echo -e "${YELLOW}Building ROM functionality modules and test programs...${NC}"

# Check if we have a ROM path to test
if [ "$1" == "" ]; then
    ROM_PATH=$(find ~/ROMs ~/roms ~/Documents/ROMs ~/Documents/roms -type f -name "*.zip" 2>/dev/null | head -1)
    if [ "$ROM_PATH" == "" ]; then
        echo -e "${RED}No ROM path provided and couldn't find a ROM automatically.${NC}"
        echo "Please provide a ROM path as an argument:"
        echo "  $0 /path/to/rom.zip"
        exit 1
    else
        echo -e "${CYAN}Found ROM automatically: $ROM_PATH${NC}"
    fi
else
    ROM_PATH="$1"
    echo -e "${CYAN}Using provided ROM: $ROM_PATH${NC}"
fi

# Build the test programs using the makefile
echo -e "${YELLOW}Building test programs...${NC}"
make -f makefile.metal tests

if [ $? -ne 0 ]; then
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi

echo -e "${GREEN}Build successful!${NC}"

# Create directories for test output
mkdir -p test_results

# Run the ROM verification test
echo -e "\n${MAGENTA}Running ROM verification test...${NC}"
./build/metal/test/rom_verify_test "$ROM_PATH" | tee test_results/rom_verify_results.txt

# Run the ROM path manager test
echo -e "\n${MAGENTA}Running ROM path manager test...${NC}"
./build/metal/test/rom_path_test | tee test_results/rom_path_results.txt

echo -e "\n${GREEN}Tests completed!${NC}"
echo "Test results saved to:"
echo "  - test_results/rom_verify_results.txt"
echo "  - test_results/rom_path_results.txt"

# Print a summary of the CPS2 ROM detection
echo -e "\n${BLUE}CPS2 ROM Detection Summary:${NC}"
grep -A 2 "CPS2 ROM Detection" test_results/rom_verify_results.txt

# Print a summary of the verification result
echo -e "\n${BLUE}ROM Verification Summary:${NC}"
grep -A 2 "ROM Set Verification" test_results/rom_verify_results.txt | head -3

# Print a summary of ROM paths detected
echo -e "\n${BLUE}ROM Path Detection Summary:${NC}"
grep -A 10 "Configured ROM paths" test_results/rom_path_results.txt | head -10

# Make the test programs executable
chmod +x build/metal/test/rom_verify_test
chmod +x build/metal/test/rom_path_test 