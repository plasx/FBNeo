#!/bin/bash
# Test script for FBNeo Metal implementation

# Colors for better output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;36m'
NC='\033[0m' # No Color

echo -e "${BLUE}FBNeo Metal Test Script${NC}"
echo "==========================="

# Check if the executable exists
if [ ! -f "./fbneo_metal" ]; then
    echo -e "${RED}Error: fbneo_metal executable not found. Run ./build/build_metal_core.sh first.${NC}"
    exit 1
fi

# Common ROM paths to check
ROM_PATHS=(
    "./roms"
    "$HOME/ROMs"
    "$HOME/roms"
    "$HOME/Documents/ROMs"
    "$HOME/Documents/roms"
)

# Find available ROMs
echo -e "${YELLOW}Searching for ROMs...${NC}"
FOUND_ROMS=()

for dir in "${ROM_PATHS[@]}"; do
    if [ -d "$dir" ]; then
        echo -e "Checking ${BLUE}$dir${NC}..."
        for rom in "$dir"/*.zip; do
            if [ -f "$rom" ]; then
                FOUND_ROMS+=("$rom")
                echo "  Found ROM: $(basename "$rom")"
            fi
        done
    fi
done

# Test the first 3 ROMs found
echo ""
if [ ${#FOUND_ROMS[@]} -eq 0 ]; then
    echo -e "${RED}No ROMs found. Place ROMs in one of these directories:${NC}"
    printf "  %s\n" "${ROM_PATHS[@]}"
    exit 1
fi

echo -e "${YELLOW}Running tests with the first 3 ROMs found:${NC}"
echo ""

for i in {0..2}; do
    if [ $i -lt ${#FOUND_ROMS[@]} ]; then
        rom="${FOUND_ROMS[$i]}"
        echo -e "${BLUE}Testing ROM: $(basename "$rom")${NC}"
        echo "======================================"
        
        echo -e "${YELLOW}Running: ./fbneo_metal \"$rom\"${NC}"
        ./fbneo_metal "$rom"
        
        echo "======================================"
        echo ""
        
        # Ask if user wants to continue
        read -p "Continue to next ROM? (y/n) " -n 1 -r
        echo ""
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            break
        fi
    fi
done

echo -e "${GREEN}Tests completed.${NC}" 