#!/bin/bash
# ROM analysis script

# Colors for terminal output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Check if we have a ROM file argument
if [ $# -lt 1 ]; then
    echo -e "${RED}Usage: $0 <rom_file.zip>${NC}"
    echo -e "Please provide the path to a ROM file."
    exit 1
fi

ROM_FILE="$1"

# Check if the ROM file exists
if [ ! -f "$ROM_FILE" ]; then
    echo -e "${RED}ROM file not found: $ROM_FILE${NC}"
    exit 1
fi

# Check if the diagnostic tool exists
if [ ! -f "./rom_diagnostic" ]; then
    echo -e "${YELLOW}ROM diagnostic tool not found, building it...${NC}"
    ./build_rom_render_fix_direct.sh
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}Failed to build ROM diagnostic tool.${NC}"
        exit 1
    fi
fi

# Create debug output directory
mkdir -p debug_output

# Run the diagnostic tool
echo -e "${BLUE}Analyzing ROM file: $ROM_FILE${NC}"
./rom_diagnostic "$ROM_FILE" > debug_output/rom_analysis.log 2>&1

# Check if the analysis completed successfully
if [ $? -ne 0 ]; then
    echo -e "${RED}ROM analysis failed.${NC}"
    exit 1
fi

# Display the analysis results
echo -e "${GREEN}ROM analysis completed successfully.${NC}"
echo -e "${YELLOW}Summary of ROM file:${NC}"
grep -A 10 "Analyzing ROM file" debug_output/rom_analysis.log | head -15
echo -e "..."
echo -e "${BLUE}Full analysis saved to:${NC} debug_output/rom_analysis.log"

exit 0 