#!/bin/bash

# Colors for terminal output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}Building ROM verification test...${NC}"

# Create build directory if it doesn't exist
mkdir -p build/test

# Compile the test program
clang++ -std=c++17 -o build/test/rom_verify_test \
    src/burner/metal/tests/rom_verify_test.cpp \
    src/burner/metal/rom_verify.cpp \
    -Isrc -Isrc/burn -Isrc/burn/devices -Isrc/burn/snd \
    -Isrc/burn/drv -Isrc/burn/drv/capcom -Isrc/burner -Isrc/burner/metal \
    -Isrc/burner/metal/app -Isrc/burner/metal/fixes -Isrc/cpu -Isrc/cpu/m68k \
    -Isrc/cpu/z80 -Isrc/intf/video -Isrc/intf/audio -Isrc/intf/input \
    -DMETAL_BUILD -lcrypto -lssl

if [ $? -eq 0 ]; then
    echo -e "${GREEN}Build successful!${NC}"
    echo "ROM verification test executable is at: build/test/rom_verify_test"
    echo ""
    echo "Usage: ./build/test/rom_verify_test <rom_path>"
    echo "Example: ./build/test/rom_verify_test /path/to/roms/mvsc.zip"
    
    # Make the script executable
    chmod +x build/test/rom_verify_test
else
    echo -e "${RED}Build failed!${NC}"
fi 