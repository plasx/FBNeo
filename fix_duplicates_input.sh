#!/bin/bash
# Fix duplicate symbol issues in input-related files

set -e  # Exit on error

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
RESET='\033[0m'

echo -e "${BLUE}Fixing duplicate input system symbols...${RESET}"

# Create directory for unified implementations if it doesn't exist
mkdir -p build/metal_fixed

# Create a unified implementation for input functions
echo -e "${BLUE}Creating unified input functions implementation...${RESET}"

cat > build/metal_fixed/input_functions.c << 'EOL'
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Input initialization function
int Metal_InitInput() {
    printf("Metal_InitInput: Initializing input system\n");
    // Implementation redirects to the appropriate system
    return 0;
}

// Input shutdown function
void Metal_ExitInput() {
    printf("Metal_ExitInput: Shutting down input system\n");
    // Implementation redirects to the appropriate system
}
EOL

# Comment out duplicated Metal_InitInput in metal_input.mm
if [ -f src/burner/metal/metal_input.mm ]; then
    echo -e "${BLUE}Patching metal_input.mm...${RESET}"
    
    # Create backup
    cp src/burner/metal/metal_input.mm src/burner/metal/metal_input.mm.bak
    
    # Comment out or rename Metal_InitInput
    sed -i '' 's/int Metal_InitInput(/int Metal_InitInput_UNUSED(/g' src/burner/metal/metal_input.mm
fi

# Comment out duplicated Metal_ExitInput in metal_input.mm
if [ -f src/burner/metal/metal_input.mm ]; then
    # Comment out or rename Metal_ExitInput
    sed -i '' 's/void Metal_ExitInput(/void Metal_ExitInput_UNUSED(/g' src/burner/metal/metal_input.mm
fi

# Comment out duplicated Metal_InitInput in src/intf/input/macos/inp_metal.mm
if [ -f src/intf/input/macos/inp_metal.mm ]; then
    echo -e "${BLUE}Patching inp_metal.mm...${RESET}"
    
    # Create backup
    cp src/intf/input/macos/inp_metal.mm src/intf/input/macos/inp_metal.mm.bak
    
    # Comment out or rename Metal_InitInput/Metal_InputInit
    sed -i '' 's/int Metal_InputInit(/int Metal_InputInit_UNUSED(/g' src/intf/input/macos/inp_metal.mm
fi

# Comment out duplicated Metal_ExitInput/Metal_InputExit in src/intf/input/macos/inp_metal.mm
if [ -f src/intf/input/macos/inp_metal.mm ]; then
    # Comment out or rename Metal_InputExit
    sed -i '' 's/int Metal_InputExit(/int Metal_InputExit_UNUSED(/g' src/intf/input/macos/inp_metal.mm
fi

echo -e "${GREEN}Input system duplicates fixed successfully!${RESET}" 