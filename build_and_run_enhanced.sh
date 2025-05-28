#!/bin/bash
# =============================================================================
# Build and run the enhanced Metal implementation with improved renderer
# and input handling system
# =============================================================================

# Set colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print banner
echo -e "${BLUE}===================================${NC}"
echo -e "${BLUE}  FBNeo Enhanced Metal Renderer    ${NC}"
echo -e "${BLUE}===================================${NC}"
echo -e "${YELLOW}Features:${NC}"
echo -e "${YELLOW}- Enhanced rendering with aspect ratio preservation${NC}"
echo -e "${YELLOW}- Complete keyboard input handling system${NC}"
echo -e "${YELLOW}- Fullscreen toggle support with Alt+Enter${NC}"
echo -e "${BLUE}===================================${NC}"

# Check if Metal is supported
if [ "$(uname)" != "Darwin" ]; then
    echo -e "${RED}Error: This script is for macOS only${NC}"
    exit 1
fi

# Ensure we're in the right directory
cd "$(dirname "$0")"

# Check for necessary directories
if [ ! -d "src/burner/metal" ]; then
    echo -e "${RED}Error: Metal source directory not found${NC}"
    exit 1
fi

# Create Metal shader directory
mkdir -p src/burner/metal

# Clean build artifacts
echo -e "${YELLOW}Cleaning previous build...${NC}"
make -f makefile.metal clean

# Build the enhanced Metal version
echo -e "${YELLOW}Building enhanced Metal implementation...${NC}"
make -f makefile.metal enhanced

# Check if build was successful
if [ $? -ne 0 ] || [ ! -f "./fbneo_metal_enhanced" ]; then
    echo -e "${RED}Error: Build failed${NC}"
    exit 1
fi

# Make executable
chmod +x ./fbneo_metal_enhanced

# Run the application
echo -e "${GREEN}Build successful! Running FBNeo with enhanced Metal renderer...${NC}"
echo -e "${YELLOW}Default controls:${NC}"
echo -e "${YELLOW}- Arrow keys: Movement${NC}"
echo -e "${YELLOW}- Z/X/C/A/S/D: Buttons 1-6${NC}"
echo -e "${YELLOW}- 1/2: Coin/Start${NC}"
echo -e "${YELLOW}- Space: Pause${NC}"
echo -e "${YELLOW}- ESC: Quit${NC}"
echo -e "${YELLOW}- Enter: Toggle fullscreen${NC}"
echo -e "${GREEN}See METAL_INPUT.md for complete key map${NC}"

# If a ROM path was provided, use it
if [ $# -eq 1 ]; then
    echo -e "${YELLOW}Using ROM path: $1${NC}"
    ./fbneo_metal_enhanced "$1"
else
    echo -e "${YELLOW}No ROM path provided, starting without a ROM...${NC}"
    ./fbneo_metal_enhanced
fi 