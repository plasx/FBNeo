#!/bin/bash
# Build script for FBNeo Metal with frame loop implementation

echo "Building FBNeo Metal with frame loop implementation..."
echo "Target: Marvel vs Capcom (CPS2)"

# Create build directories if they don't exist
mkdir -p build/metal/obj
mkdir -p build/metal/obj/src/burner/metal
mkdir -p build/metal/obj/src/burn/drv/capcom

# Clean previous build
make -f makefile.metal clean

# Build with the updated frame loop code
make -f makefile.metal -j4

# Check if build was successful
if [ -f "fbneo_metal" ]; then
    echo "====================================="
    echo "Build successful!"
    echo "You can run the emulator with:"
    echo "./fbneo_metal roms/mvsc.zip"
    echo "====================================="
    
    # Make sure roms directory exists
    mkdir -p roms
    
    # Check if ROM file exists
    if [ -f "roms/mvsc.zip" ]; then
        echo "ROM file found: roms/mvsc.zip"
    else
        echo "NOTE: You need to copy the Marvel vs Capcom ROM file (mvsc.zip) to the roms directory"
        echo "See ROM_SETUP.md for details"
    fi
else
    echo "Build failed!"
    exit 1
fi 