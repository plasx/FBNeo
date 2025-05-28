#!/bin/bash

# Colors for terminal output
GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

echo -e "${BLUE}===================================================${NC}"
echo -e "${BLUE}     FBNeo Metal - Enhanced Implementation         ${NC}"
echo -e "${BLUE}===================================================${NC}"

# Check if a ROM was provided as an argument
ROM_PATH=""
if [ $# -gt 0 ]; then
    ROM_PATH="$1"
else
    # Set default ROM path
    ROM_PATH="/Users/plasx/dev/ROMs/mvsc.zip"
fi

# Verify the ROM file exists
if [ -f "$ROM_PATH" ]; then
    echo -e "${GREEN}ROM file found at: $ROM_PATH${NC}"
    echo -e "${GREEN}File size: $(stat -f %z "$ROM_PATH") bytes${NC}"
else
    echo -e "${RED}ROM file not found at: $ROM_PATH${NC}"
    echo -e "${YELLOW}Please specify a valid ROM path as an argument:${NC}"
    echo -e "${YELLOW}  ./run_fbneo_metal_updated.sh /path/to/rom.zip${NC}"
    exit 1
fi

# Display system info
echo -e "${GREEN}=== System Information ===${NC}"
echo -e "${GREEN}OS: $(uname -s) $(uname -r) $(uname -m)${NC}"
echo -e "${GREEN}Game: $(basename "$ROM_PATH")${NC}"

# Check if build is needed
if [ ! -f "./fbneo_metal" ]; then
    echo -e "${YELLOW}Executable not found, building...${NC}"
    make -f makefile.metal clean
    make -f makefile.metal
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}Build failed. Please check for errors.${NC}"
        exit 1
    fi
    
    echo -e "${GREEN}Build successful!${NC}"
fi

# Check if we need to rebuild based on file modification times
NEED_REBUILD=0
for source_file in src/burner/metal/*.mm src/burner/metal/*.cpp src/burner/metal/*.h src/burner/metal/app/*.mm; do
    if [ -f "$source_file" ] && [ "$source_file" -nt "./fbneo_metal" ]; then
        echo -e "${YELLOW}Source file has been modified: $source_file${NC}"
        NEED_REBUILD=1
        break
    fi
done

if [ $NEED_REBUILD -eq 1 ]; then
    echo -e "${YELLOW}Rebuilding due to modified source files...${NC}"
    make -f makefile.metal
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}Build failed. Please check for errors.${NC}"
        exit 1
    fi
    
    echo -e "${GREEN}Rebuild successful!${NC}"
fi

# Launch the emulator with the ROM file
echo -e "${GREEN}Launching FBNeo Metal with ROM: $(basename "$ROM_PATH")...${NC}"
echo -e "${BLUE}===================================================${NC}"

# Use appropriate environment variables and pass the ROM path
METAL_DEVICE_WRAPPER_TYPE=1 ./fbneo_metal "$ROM_PATH" 

# Check exit status
EXIT_STATUS=$?
if [ $EXIT_STATUS -eq 0 ]; then
    echo -e "${GREEN}=== Success! ===${NC}"
    echo -e "${GREEN}The emulator exited successfully.${NC}"
else
    echo -e "${RED}=== Execution Failed ===${NC}"
    echo -e "${RED}The emulator exited with code: $EXIT_STATUS${NC}"
    echo -e "${RED}Check the log output above for details.${NC}"
fi

echo -e "${BLUE}===================================================${NC}" 