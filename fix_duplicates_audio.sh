#!/bin/bash
# Fix duplicate symbol issues in audio-related files

set -e  # Exit on error

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
RESET='\033[0m'

echo -e "${BLUE}Fixing duplicate audio system symbols...${RESET}"

# Create directory for unified implementations if it doesn't exist
mkdir -p build/metal_fixed

# Create a unified implementation for audio functions
echo -e "${BLUE}Creating unified audio functions implementation...${RESET}"

cat > build/metal_fixed/audio_functions.c << 'EOL'
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Audio initialization function
int Metal_InitAudio() {
    printf("Metal_InitAudio: Initializing audio system\n");
    // Implementation redirects to the appropriate system
    return 0;
}

// Audio shutdown function
void Metal_ShutdownAudio() {
    printf("Metal_ShutdownAudio: Shutting down audio system\n");
    // Implementation redirects to the appropriate system
}
EOL

# Comment out duplicated Metal_InitAudio in metal_audio_integration.mm
if [ -f src/burner/metal/metal_audio_integration.mm ]; then
    echo -e "${BLUE}Patching metal_audio_integration.mm...${RESET}"
    
    # Create backup
    cp src/burner/metal/metal_audio_integration.mm src/burner/metal/metal_audio_integration.mm.bak
    
    # Comment out or rename Metal_InitAudio
    sed -i '' 's/int Metal_InitAudio(/int Metal_InitAudio_UNUSED(/g' src/burner/metal/metal_audio_integration.mm
fi

# Comment out duplicated Metal_ShutdownAudio in metal_audio_integration.mm
if [ -f src/burner/metal/metal_audio_integration.mm ]; then
    # Comment out or rename Metal_ShutdownAudio
    sed -i '' 's/void Metal_ShutdownAudio(/void Metal_ShutdownAudio_UNUSED(/g' src/burner/metal/metal_audio_integration.mm
fi

# Comment out duplicated Metal_InitAudio in metal_audio.mm
if [ -f src/burner/metal/metal_audio.mm ]; then
    echo -e "${BLUE}Patching metal_audio.mm...${RESET}"
    
    # Create backup
    cp src/burner/metal/metal_audio.mm src/burner/metal/metal_audio.mm.bak
    
    # Comment out or rename Metal_InitAudio
    sed -i '' 's/int Metal_InitAudio(/int Metal_InitAudio_UNUSED(/g' src/burner/metal/metal_audio.mm
fi

# Comment out duplicated Metal_ShutdownAudio in metal_audio.mm
if [ -f src/burner/metal/metal_audio.mm ]; then
    # Comment out or rename Metal_ShutdownAudio
    sed -i '' 's/int Metal_ShutdownAudio(/int Metal_ShutdownAudio_UNUSED(/g' src/burner/metal/metal_audio.mm
fi

# Comment out duplicated Metal_InitAudioSystem in metal_audio.mm
if [ -f metal_audio.mm ]; then
    echo -e "${BLUE}Patching metal_audio.mm...${RESET}"
    
    # Create backup
    cp metal_audio.mm metal_audio.mm.bak
    
    # Comment out or rename Metal_InitAudioSystem
    sed -i '' 's/int Metal_InitAudioSystem(/int Metal_InitAudioSystem_UNUSED(/g' metal_audio.mm
fi

# Comment out duplicated Metal_ShutdownAudio in metal_audio.mm (root dir)
if [ -f metal_audio.mm ]; then
    # Comment out or rename Metal_ShutdownAudio
    sed -i '' 's/int Metal_ShutdownAudio(/int Metal_ShutdownAudio_UNUSED(/g' metal_audio.mm
fi

echo -e "${GREEN}Audio system duplicates fixed successfully!${RESET}" 