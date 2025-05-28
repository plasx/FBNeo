#!/bin/bash
# =============================================================================
# FBNeo Metal Renderer Performance Test Script
# =============================================================================
# This script builds the FBNeo Metal renderer, runs it with specified ROMs,
# and collects performance metrics for analysis
# =============================================================================

# Terminal colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Default configuration
ROM_PATH=""
TEST_DURATION=30
REPORT_FILE="metal_performance_report.txt"
BUILD_TYPE="enhanced"
VERBOSE=0

# Function to show help
show_help() {
    echo -e "${CYAN}FBNeo Metal Renderer Performance Test${NC}"
    echo ""
    echo "Usage: $0 [options] [rom_path]"
    echo ""
    echo "Options:"
    echo "  -h, --help         Show this help message"
    echo "  -d, --duration N   Run each test for N seconds (default: 30)"
    echo "  -r, --report FILE  Save report to FILE (default: metal_performance_report.txt)"
    echo "  -b, --build TYPE   Build type: enhanced, standalone, demo (default: enhanced)"
    echo "  -v, --verbose      Show verbose output"
    echo ""
    echo "Examples:"
    echo "  $0 ~/roms/sf2.zip"
    echo "  $0 -d 60 -r sf2_report.txt ~/roms/sf2.zip"
    echo ""
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -d|--duration)
            TEST_DURATION="$2"
            shift 2
            ;;
        -r|--report)
            REPORT_FILE="$2"
            shift 2
            ;;
        -b|--build)
            BUILD_TYPE="$2"
            shift 2
            ;;
        -v|--verbose)
            VERBOSE=1
            shift
            ;;
        *)
            ROM_PATH="$1"
            shift
            ;;
    esac
done

# Validate build type
if [[ "$BUILD_TYPE" != "enhanced" && "$BUILD_TYPE" != "standalone" && "$BUILD_TYPE" != "demo" ]]; then
    echo -e "${RED}Error: Invalid build type. Use enhanced, standalone, or demo.${NC}"
    exit 1
fi

# Build the Metal renderer
echo -e "${BLUE}Building FBNeo Metal renderer (${BUILD_TYPE})...${NC}"
if [ $VERBOSE -eq 1 ]; then
    make -f makefile.metal clean
    make -f makefile.metal $BUILD_TYPE
else
    make -f makefile.metal clean > /dev/null
    make -f makefile.metal $BUILD_TYPE > /dev/null
fi

# Check if build was successful
if [ "$BUILD_TYPE" == "enhanced" ] && [ ! -d "FBNeo.app" ]; then
    echo -e "${RED}Build failed! FBNeo.app not created.${NC}"
    exit 1
elif [ "$BUILD_TYPE" == "standalone" ] && [ ! -f "fbneo_metal_standalone" ]; then
    echo -e "${RED}Build failed! fbneo_metal_standalone not created.${NC}"
    exit 1
elif [ "$BUILD_TYPE" == "demo" ] && [ ! -f "fbneo_metal_demo" ]; then
    echo -e "${RED}Build failed! fbneo_metal_demo not created.${NC}"
    exit 1
fi

echo -e "${GREEN}Build successful!${NC}"

# Initialize report file
echo "FBNeo Metal Renderer Performance Report" > "$REPORT_FILE"
echo "Date: $(date)" >> "$REPORT_FILE"
echo "System: $(uname -sm)" >> "$REPORT_FILE"
echo "macOS: $(sw_vers -productVersion)" >> "$REPORT_FILE"
echo "Build: $BUILD_TYPE" >> "$REPORT_FILE"
echo "=============================================" >> "$REPORT_FILE"

# Enable performance counters
defaults write com.fbneo.metal PerformanceCountersEnabled -bool true
defaults write com.fbneo.metal DetailedLoggingEnabled -bool true

# Run the test
if [ -n "$ROM_PATH" ]; then
    echo -e "${BLUE}Running test with ROM: ${ROM_PATH}${NC}"
    echo "ROM: $(basename "$ROM_PATH")" >> "$REPORT_FILE"
    echo "Test Duration: ${TEST_DURATION} seconds" >> "$REPORT_FILE"
    
    # Run the app with the ROM
    if [ "$BUILD_TYPE" == "enhanced" ]; then
        echo -e "${YELLOW}Running enhanced renderer (timeout: ${TEST_DURATION}s)...${NC}"
        timeout $TEST_DURATION open FBNeo.app --args "$ROM_PATH"
    elif [ "$BUILD_TYPE" == "standalone" ]; then
        echo -e "${YELLOW}Running standalone renderer (timeout: ${TEST_DURATION}s)...${NC}"
        timeout $TEST_DURATION ./fbneo_metal_standalone "$ROM_PATH"
    else
        echo -e "${YELLOW}Running demo renderer (timeout: ${TEST_DURATION}s)...${NC}"
        timeout $TEST_DURATION ./fbneo_metal_demo
    fi
    
    # Give some time for the app to close
    sleep 2
    
    # Check for performance log
    PERFLOG="$HOME/Documents/FBNeo_PerformanceLog.csv"
    if [ -f "$PERFLOG" ]; then
        echo -e "${GREEN}Performance log found!${NC}"
        
        # Process the performance log to extract metrics
        if [ -x "$(command -v awk)" ]; then
            # Calculate average metrics
            AVG_FPS=$(awk -F, 'NR>1 {total += 1000/$4; count++} END {printf "%.2f", total/count}' "$PERFLOG")
            AVG_CPU=$(awk -F, 'NR>1 {total += $2; count++} END {printf "%.2f", total/count}' "$PERFLOG")
            AVG_GPU=$(awk -F, 'NR>1 {total += $3; count++} END {printf "%.2f", total/count}' "$PERFLOG")
            
            # Calculate min/max metrics
            MIN_FPS=$(awk -F, 'NR>1 {fps = 1000/$4; if(NR==2 || fps<min) min=fps} END {printf "%.2f", min}' "$PERFLOG")
            MAX_FPS=$(awk -F, 'NR>1 {fps = 1000/$4; if(fps>max) max=fps} END {printf "%.2f", max}' "$PERFLOG")
            
            # Add to report
            echo "" >> "$REPORT_FILE"
            echo "Performance Metrics:" >> "$REPORT_FILE"
            echo "- Average FPS: $AVG_FPS" >> "$REPORT_FILE"
            echo "- Min/Max FPS: $MIN_FPS / $MAX_FPS" >> "$REPORT_FILE"
            echo "- Average CPU Time: $AVG_CPU ms" >> "$REPORT_FILE"
            echo "- Average GPU Time: $AVG_GPU ms" >> "$REPORT_FILE"
            
            # Display metrics
            echo -e "${CYAN}Performance Metrics:${NC}"
            echo -e "  Average FPS: ${GREEN}$AVG_FPS${NC}"
            echo -e "  Min/Max FPS: $MIN_FPS / $MAX_FPS"
            echo -e "  CPU Time: $AVG_CPU ms"
            echo -e "  GPU Time: $AVG_GPU ms"
            
            # Save a copy of the performance log
            cp "$PERFLOG" "$(basename "$ROM_PATH" .zip)_performance.csv"
            echo -e "${BLUE}Performance data saved to: $(basename "$ROM_PATH" .zip)_performance.csv${NC}"
        else
            echo -e "${YELLOW}Warning: awk not found, cannot process performance metrics${NC}"
        fi
    else
        echo -e "${YELLOW}No performance log found at $PERFLOG${NC}"
        echo "No performance data collected" >> "$REPORT_FILE"
    fi
else
    echo -e "${YELLOW}No ROM specified, skipping test run${NC}"
    echo "No ROM specified for testing" >> "$REPORT_FILE"
fi

echo -e "${GREEN}Test completed! Report saved to: $REPORT_FILE${NC}" 