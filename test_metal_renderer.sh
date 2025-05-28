#!/bin/bash
# ============================================================================
# FBNeo Metal Renderer Test Script
# ============================================================================
# This script helps test the Metal renderer with different ROMs and measures
# performance metrics to ensure compatibility and optimization.
# ============================================================================

# Terminal colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Configuration
ROMS_DIR=""
LOG_FILE="metal_test_results.txt"
TEST_DURATION=30 # seconds per ROM

# Function to show help
show_help() {
    echo -e "${CYAN}FBNeo Metal Renderer Test Script${NC}"
    echo ""
    echo "Usage: $0 [options]"
    echo ""
    echo "Options:"
    echo "  -r, --roms DIR      Specify ROMs directory"
    echo "  -t, --time SEC      Set test duration per ROM (default: 30 seconds)"
    echo "  -l, --log FILE      Specify log file (default: metal_test_results.txt)"
    echo "  -h, --help          Show this help message"
    echo ""
    echo "Example:"
    echo "  $0 --roms ~/roms/arcade --time 20"
    echo ""
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -r|--roms)
            ROMS_DIR="$2"
            shift 2
            ;;
        -t|--time)
            TEST_DURATION="$2"
            shift 2
            ;;
        -l|--log)
            LOG_FILE="$2"
            shift 2
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            show_help
            exit 1
            ;;
    esac
done

# Check if ROMs directory is specified
if [ -z "$ROMS_DIR" ]; then
    echo -e "${YELLOW}ROMs directory not specified. Using current directory.${NC}"
    ROMS_DIR="."
fi

# Check if ROMs directory exists
if [ ! -d "$ROMS_DIR" ]; then
    echo -e "${RED}Error: ROMs directory '$ROMS_DIR' does not exist.${NC}"
    exit 1
fi

# Initialize log file
echo "FBNeo Metal Renderer Test Results" > "$LOG_FILE"
echo "Date: $(date)" >> "$LOG_FILE"
echo "System: $(uname -a)" >> "$LOG_FILE"
echo "ROMs Directory: $ROMS_DIR" >> "$LOG_FILE"
echo "Test Duration: $TEST_DURATION seconds per ROM" >> "$LOG_FILE"
echo "----------------------------------------" >> "$LOG_FILE"

# Check if FBNeo.app exists
if [ ! -d "FBNeo.app" ]; then
    echo -e "${YELLOW}FBNeo.app not found. Building it now...${NC}"
    make -f makefile.metal enhanced
    
    if [ ! -d "FBNeo.app" ]; then
        echo -e "${RED}Failed to build FBNeo.app.${NC}"
        exit 1
    fi
fi

# Find all ROMs (zip files)
echo -e "${BLUE}Searching for ROMs in $ROMS_DIR...${NC}"
ROMS=$(find "$ROMS_DIR" -name "*.zip" -type f)
ROM_COUNT=$(echo "$ROMS" | wc -l)

if [ "$ROM_COUNT" -eq 0 ]; then
    echo -e "${RED}No ROMs found in $ROMS_DIR.${NC}"
    exit 1
fi

echo -e "${GREEN}Found $ROM_COUNT ROMs.${NC}"
echo ""

# Function to run a ROM test
run_rom_test() {
    local rom="$1"
    local rom_name=$(basename "$rom")
    
    echo -e "${CYAN}Testing ROM: $rom_name${NC}"
    echo "Testing ROM: $rom_name" >> "$LOG_FILE"
    
    # Enable detailed performance logging
    defaults write com.fbneo.metal DetailedPerformanceLogging -bool true
    
    # Run FBNeo with the ROM
    echo -e "${YELLOW}Running for $TEST_DURATION seconds...${NC}"
    timeout "$TEST_DURATION" open FBNeo.app --args "$rom"
    
    # Give it a moment to close properly
    sleep 2
    
    # Check if a performance log was created
    LOG_PATH="$HOME/Documents/FBNeo_PerformanceLog.csv"
    if [ -f "$LOG_PATH" ]; then
        # Process performance log
        AVG_FPS=$(awk -F, 'NR>1 {total += 1000/$4; count++} END {if (count > 0) print total/count; else print "N/A"}' "$LOG_PATH")
        AVG_CPU=$(awk -F, 'NR>1 {total += $2; count++} END {if (count > 0) print total/count; else print "N/A"}' "$LOG_PATH")
        AVG_GPU=$(awk -F, 'NR>1 {total += $3; count++} END {if (count > 0) print total/count; else print "N/A"}' "$LOG_PATH")
        
        echo "  Average FPS: $AVG_FPS" >> "$LOG_FILE"
        echo "  CPU Time: $AVG_CPU ms" >> "$LOG_FILE"
        echo "  GPU Time: $AVG_GPU ms" >> "$LOG_FILE"
        
        # Save the log with ROM-specific name
        cp "$LOG_PATH" "${rom_name%.zip}_perflog.csv"
        
        echo -e "${GREEN}Performance data collected: Average FPS=$AVG_FPS${NC}"
    else
        echo "  No performance data collected" >> "$LOG_FILE"
        echo -e "${YELLOW}No performance data collected${NC}"
    fi
    
    echo "----------------------------------------" >> "$LOG_FILE"
    echo ""
}

# Run tests for each ROM
echo -e "${CYAN}Starting ROM tests...${NC}"
echo ""

ROM_INDEX=1
for rom in $ROMS; do
    echo -e "${BLUE}[$ROM_INDEX/$ROM_COUNT] ${NC}"
    run_rom_test "$rom"
    ROM_INDEX=$((ROM_INDEX + 1))
    
    # Ask to continue after each ROM
    if [ $ROM_INDEX -le $ROM_COUNT ]; then
        read -p "Press Enter to continue with next ROM, or 'q' to quit: " input
        if [ "$input" = "q" ]; then
            echo -e "${YELLOW}Testing stopped by user.${NC}"
            break
        fi
    fi
done

echo -e "${GREEN}Testing completed. Results saved to $LOG_FILE${NC}"
echo ""
echo -e "${CYAN}Summary:${NC}"
echo -e "  ROMs tested: $((ROM_INDEX - 1)) of $ROM_COUNT"
echo -e "  Log file: $LOG_FILE"
echo -e "  Performance logs: ${rom_name%.zip}_perflog.csv (for each ROM)"
echo "" 