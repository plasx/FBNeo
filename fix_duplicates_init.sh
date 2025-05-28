#!/bin/bash
# Fix duplicate symbol issues in initialization functions

set -e  # Exit on error

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
RESET='\033[0m'

echo -e "${BLUE}Fixing duplicate initialization symbols...${RESET}"

# Create a unified initialization functions file that will be the only implementation
mkdir -p build/metal_fixed
cat > build/metal_fixed/init_functions.c << 'EOL'
#include <stdio.h>
#include <stdbool.h>

// Core initialization functions - centralized implementation to avoid duplicates

// Main initialization function
int FBNeoInit(void) {
    printf("FBNeoInit: Initializing FBNeo core\n");
    return 0;
}

// Main exit function
int FBNeoExit(void) {
    printf("FBNeoExit: Shutting down FBNeo core\n");
    return 0;
}

// Run one frame of emulation
int RunFrame(int bDraw) {
    // Just a stub to be replaced with proper implementation
    return 0;
}

// Global variable needed by some parts of FBNeo
bool bDoIpsPatch = false;
EOL

# Comment out duplicate implementations in metal_bridge_stubs.c
if [ -f src/burner/metal/metal_bridge_stubs.c ]; then
    echo -e "${BLUE}Patching metal_bridge_stubs.c for init functions...${RESET}"
    
    # Comment out FBNeoInit
    sed -i '' '/^int FBNeoInit/,/^}/c\
// REMOVED: Duplicate implementation of FBNeoInit\
// Using the implementation from init_functions.c\
int FBNeoInit_UNUSED(void) {\
    // This function has been removed to avoid duplicates\
    return 0;\
}' src/burner/metal/metal_bridge_stubs.c
    
    # Comment out FBNeoExit
    sed -i '' '/^int FBNeoExit/,/^}/c\
// REMOVED: Duplicate implementation of FBNeoExit\
// Using the implementation from init_functions.c\
int FBNeoExit_UNUSED(void) {\
    // This function has been removed to avoid duplicates\
    return 0;\
}' src/burner/metal/metal_bridge_stubs.c
    
    # Comment out RunFrame
    sed -i '' '/^int RunFrame/,/^}/c\
// REMOVED: Duplicate implementation of RunFrame\
// Using the implementation from init_functions.c\
int RunFrame_UNUSED(int bDraw) {\
    // This function has been removed to avoid duplicates\
    return 0;\
}' src/burner/metal/metal_bridge_stubs.c

    # Comment out bDoIpsPatch
    sed -i '' '/^bool bDoIpsPatch/c\
// REMOVED: Duplicate definition of bDoIpsPatch\
// Using the implementation from init_functions.c\
bool bDoIpsPatch_UNUSED = false;' src/burner/metal/metal_bridge_stubs.c
fi

# Fix duplicates in metal_cps_stubs.c
if [ -f src/burner/metal/fixes/metal_cps_stubs.c ]; then
    echo -e "${BLUE}Patching metal_cps_stubs.c for init functions...${RESET}"
    
    # Comment out FBNeoInit
    sed -i '' '/^INT32 FBNeoInit/,/^}/c\
// REMOVED: Duplicate implementation of FBNeoInit\
// Using the implementation from init_functions.c\
INT32 FBNeoInit_UNUSED(void) {\
    // This function has been removed to avoid duplicates\
    return 0;\
}' src/burner/metal/fixes/metal_cps_stubs.c
    
    # Comment out FBNeoExit
    sed -i '' '/^INT32 FBNeoExit/,/^}/c\
// REMOVED: Duplicate implementation of FBNeoExit\
// Using the implementation from init_functions.c\
INT32 FBNeoExit_UNUSED(void) {\
    // This function has been removed to avoid duplicates\
    return 0;\
}' src/burner/metal/fixes/metal_cps_stubs.c
    
    # Comment out RunFrame
    sed -i '' '/^INT32 RunFrame/,/^}/c\
// REMOVED: Duplicate implementation of RunFrame\
// Using the implementation from init_functions.c\
INT32 RunFrame_UNUSED(INT32 bDraw) {\
    // This function has been removed to avoid duplicates\
    return 0;\
}' src/burner/metal/fixes/metal_cps_stubs.c
fi

echo -e "${GREEN}Initialization duplicates fixed successfully!${RESET}" 