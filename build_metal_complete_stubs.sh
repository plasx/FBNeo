#!/bin/bash
# Build script for Metal implementation with complete stubs

set -e

echo "====================================================="
echo "FBNeo Metal Complete Stubs Build Script"
echo "====================================================="
echo "This script builds the Metal implementation with all required stubs"
echo ""

# Step 1: Create necessary directories
echo "Creating necessary directories..."
mkdir -p src/dep/generated
mkdir -p build/metal/obj/src/burn/devices
mkdir -p build/metal/obj/src/burn/drv/megadrive
mkdir -p src/burner/metal/fixes
mkdir -p src/burner/metal

# Step 2: Apply byteswap fixes to maintain compatibility
echo "Applying BurnByteswap and memory management fixes..."
./fix_burn_byteswap.sh

# Step 3: Apply window creation fixes
echo "Applying window creation fixes..."
chmod +x fix_metal_window.sh
./fix_metal_window.sh

# Step 4: Define core globals if needed
echo "Creating global variables stub..."
cat > src/burner/metal/metal_globals.cpp << 'EOL'
// Global variables for Metal implementation
#include <stdint.h>

// Basic types
typedef unsigned char UINT8;
typedef int INT32;
typedef unsigned int UINT32;

// Global variables that are accessed from various files
extern "C" {
    // Core engine variables that need to be accessible
    UINT8* pBurnDraw = NULL;
    INT32 nBurnPitch = 0;
    INT32 nBurnBpp = 32;
    INT32 nBurnDrvCount = 1;
    UINT32 nBurnDrvActive = 0;
    INT32 nBurnSoundRate = 44100;
    INT32 nBurnSoundLen = 0;
    INT16* pBurnSoundOut = NULL;
    
    // Additional variables if needed
}
EOL

# Step 5: Ensure we have the latest makefile
echo "Using updated makefile.metal..."
cp -f makefile.metal makefile.metal.bak  # Backup

# Step 6: Build with our makefile
echo "Building FBNeo Metal with complete stubs..."
make -f makefile.metal

# Step 7: Check if the build was successful
if [ -f "fbneo_metal" ]; then
    echo ""
    echo "Build complete! You can now run ./fbneo_metal"
    chmod +x fbneo_metal
    echo ""
else
    echo ""
    echo "Build failed! Check errors above for details."
    echo ""
    exit 1
fi 