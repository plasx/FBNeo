#!/bin/bash
# Fix duplicate symbol issues in sound-related files

set -e  # Exit on error

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
RESET='\033[0m'

echo -e "${BLUE}Fixing duplicate sound-related symbols...${RESET}"

# Create a unified sound globals file that will be the only implementation
mkdir -p build/metal_fixed
cat > build/metal_fixed/sound_globals.c << 'EOL'
#include <stdint.h>
#include <stddef.h>
#include <string.h>

// Define the global variables for audio - main implementation for FBNeo Metal
int16_t* pBurnSoundOut = NULL;
int nBurnSoundLen = 0;
int nBurnSoundRate = 44100;
int nBurnSoundPos = 0;
int nBurnSoundBufferSize = 0;

// Single implementation of BurnSoundRender to avoid duplicates
int BurnSoundRender(int16_t* pDest, int nLen) {
    // Just fill with silence in the stub implementation
    if (pDest && nLen > 0) {
        memset(pDest, 0, nLen * 2 * sizeof(int16_t));
    }
    return 0;
}
EOL

# Comment out duplicated BurnSoundRender in metal_cps_stubs.c
if [ -f src/burner/metal/fixes/metal_cps_stubs.c ]; then
    echo -e "${BLUE}Patching metal_cps_stubs.c...${RESET}"
    sed -i '' '/^INT32 BurnSoundRender/,/^}/c\
// REMOVED: Duplicate implementation of BurnSoundRender\
// Now using unified implementation in sound_globals.c' src/burner/metal/fixes/metal_cps_stubs.c
fi

# Comment out duplicated BurnSoundRender in metal_stubs.c
if [ -f src/burner/metal/fixes/metal_stubs.c ]; then
    echo -e "${BLUE}Patching metal_stubs.c...${RESET}"
    sed -i '' '/^INT32 BurnSoundRender/,/^}/c\
// REMOVED: Duplicate implementation of BurnSoundRender\
// Now using unified implementation in sound_globals.c' src/burner/metal/fixes/metal_stubs.c
fi

# Edit the build/metal/sound_stubs.c file to avoid duplicating the globals
echo -e "${BLUE}Patching sound_stubs.c...${RESET}"
cat > build/metal/sound_stubs.c << 'EOL'
// This file is intentionally empty - sound globals now defined in sound_globals.c
// to avoid duplicate symbol errors
EOL

echo -e "${GREEN}Sound-related duplicates fixed successfully!${RESET}" 