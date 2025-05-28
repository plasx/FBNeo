#!/bin/bash
# Very minimal build script for testing purposes

set -e  # Exit on error

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
RESET='\033[0m'

echo -e "${BLUE}Building minimal Cocoa app...${RESET}"

# Clean up any previous build
rm -f fbneo_minimal

# Compile and link
echo -e "${BLUE}Compiling and linking...${RESET}"
clang++ -std=c++17 -O1 -fobjc-arc -arch arm64 \
    -framework Cocoa -framework Metal -framework MetalKit \
    -o fbneo_minimal build/simplified/main_test.mm

# Check if build succeeded
if [ $? -eq 0 ]; then
    echo -e "${GREEN}Build successful!${RESET}"
    chmod +x fbneo_minimal
    echo -e "${BLUE}Output: ${YELLOW}fbneo_minimal${RESET}"
    echo -e "${BLUE}Run it with: ${YELLOW}./fbneo_minimal${RESET}"
else
    echo -e "${RED}Build failed!${RESET}"
    exit 1
fi 