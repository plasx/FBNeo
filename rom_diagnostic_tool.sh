#!/bin/bash

# FBNeo Metal - ROM Diagnostic Tool
# This script helps diagnose ROM loading issues by running detailed verification
# without launching the full emulator

# Colors for terminal output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print usage information
function show_usage {
    echo "Usage: $0 <rom_file.zip> [options]"
    echo
    echo "Options:"
    echo "  --verify      Verify ROM against FBNeo driver (default)"
    echo "  --dump        Dump ZIP contents"
    echo "  --diagnose    Run full diagnostics"
    echo "  --test-render Test ROM rendering with our fixes"
    echo "  --help        Show this help message"
    echo
    echo "Example: $0 mvsc.zip --diagnose"
    exit 1
}

# Check if we have enough arguments
if [ $# -lt 1 ]; then
    show_usage
fi

# Parse arguments
ROM_FILE="$1"
shift

# Default action
ACTION="verify"

# Parse options
while [ $# -gt 0 ]; do
    case "$1" in
        --verify)
            ACTION="verify"
            ;;
        --dump)
            ACTION="dump"
            ;;
        --diagnose)
            ACTION="diagnose"
            ;;
        --test-render)
            ACTION="test-render"
            ;;
        --help)
            show_usage
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            show_usage
            ;;
    esac
    shift
done

echo -e "${BLUE}FBNeo Metal - ROM Diagnostic Tool${NC}"
echo -e "${BLUE}====================================${NC}"

# Check if ROM file exists
if [ ! -f "$ROM_FILE" ]; then
    echo -e "${RED}ROM file not found: $ROM_FILE${NC}"
    exit 1
fi

echo -e "${GREEN}ROM file: $ROM_FILE${NC}"

# Make sure we have the necessary object files
OBJECTS_NEEDED="obj/rom_loading_debug.o obj/rom_verify.o"
NEED_COMPILE=0

for OBJ in $OBJECTS_NEEDED; do
    if [ ! -f "$OBJ" ]; then
        NEED_COMPILE=1
        break
    fi
done

if [ $NEED_COMPILE -eq 1 ]; then
    echo -e "${YELLOW}Compiling ROM diagnostic utilities...${NC}"
    
    # Compile ROM loading debug utilities
    clang++ -std=c++11 -c src/burner/metal/rom_loading_debug.cpp -o obj/rom_loading_debug.o
    if [ $? -ne 0 ]; then
        echo -e "${RED}Failed to compile ROM loading debug utilities${NC}"
        exit 1
    fi
    
    # Compile ROM verification utilities
    clang++ -std=c++11 -c src/burner/metal/rom_verify.cpp -o obj/rom_verify.o
    if [ $? -ne 0 ]; then
        echo -e "${RED}Failed to compile ROM verification utilities${NC}"
        exit 1
    fi
fi

# Create debug output directory
mkdir -p debug_output

# Set environment variables
export FBNEO_ROM_PATH="."

# Special case for testing rendering
if [ "$ACTION" == "test-render" ]; then
    echo -e "${BLUE}Testing ROM rendering with our fixes...${NC}"
    
    # Compile our fixes if needed
    if [ ! -f "obj/metal_stubs_fixed.o" ]; then
        echo -e "${YELLOW}Compiling ROM rendering fixes...${NC}"
        clang -c src/burner/metal/metal_stubs.c -o obj/metal_stubs_fixed.o
        if [ $? -ne 0 ]; then
            echo -e "${RED}Failed to compile ROM rendering fixes${NC}"
            exit 1
        fi
    fi
    
    # Build the fixed emulator
    echo -e "${YELLOW}Building emulator with ROM rendering fixes...${NC}"
    make -f makefile.metal DISABLE_OPTIMIZE=1 ENABLE_ROM_DEBUG=1
    if [ $? -ne 0 ]; then
        echo -e "${RED}Failed to build emulator with fixes${NC}"
        exit 1
    fi
    
    # Make the binary executable
    chmod +x ./bin/metal/fbneo_metal
    
    # Run the emulator with the ROM
    echo -e "${GREEN}Running emulator with ROM: $ROM_FILE${NC}"
    ./bin/metal/fbneo_metal "$ROM_FILE" 2>debug_output/render_test.log
    
    echo -e "${GREEN}Test complete.${NC}"
    echo -e "${YELLOW}Check debug_output/render_test.log for details.${NC}"
    exit 0
fi

# Create ROM diagnostic tool
echo -e "${YELLOW}Building ROM diagnostic tool...${NC}"

DIAGNOSTIC_SOURCES="obj/rom_loading_debug.o obj/rom_verify.o src/burner/metal/metal_zip_extract.cpp"
DIAGNOSTIC_FLAGS="-framework Foundation -framework AppKit"
DIAGNOSTIC_INCLUDES="-I. -Isrc/burn -Isrc/burn/drv/cps -Isrc/burner -Isrc/burner/metal"

# Build a simplified diagnostic helper
clang++ -std=c++11 $DIAGNOSTIC_INCLUDES $DIAGNOSTIC_FLAGS -o rom_diagnostic $DIAGNOSTIC_SOURCES src/burner/metal/rom_diagnostic.cpp

if [ ! -f "rom_diagnostic" ]; then
    echo -e "${RED}Failed to build diagnostic tool${NC}"
    exit 1
fi

# Run the action
case "$ACTION" in
    verify)
        echo -e "${BLUE}Verifying ROM against FBNeo driver...${NC}"
        ./rom_diagnostic --verify "$ROM_FILE"
        ;;
    dump)
        echo -e "${BLUE}Dumping ZIP contents...${NC}"
        ./rom_diagnostic --dump "$ROM_FILE"
        ;;
    diagnose)
        echo -e "${BLUE}Running full diagnostics...${NC}"
        ./rom_diagnostic --diagnose "$ROM_FILE"
        ;;
esac

# Check for log file and display results
if [ -f "rom_loading_debug.log" ]; then
    echo -e "${GREEN}Moving ROM loading debug log to debug_output directory...${NC}"
    mv rom_loading_debug.log debug_output/rom_diagnostic.log
    
    # Display last few lines
    echo
    echo -e "${YELLOW}Summary from diagnostic log:${NC}"
    grep -A 10 "summary" debug_output/rom_diagnostic.log | tail -n 15
    echo
    echo -e "${GREEN}Full diagnostic log saved to debug_output/rom_diagnostic.log${NC}"
else
    echo -e "${RED}No diagnostic log was generated!${NC}"
fi

echo -e "${BLUE}Diagnostic process complete.${NC}" 