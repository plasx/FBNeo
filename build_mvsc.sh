#!/bin/bash
# Build script for FBNeo Metal with Marvel vs Capcom focus

set -e # Exit on any error

echo "Building FBNeo Metal for Marvel vs Capcom..."

# Clean build
make -f makefile.metal.mvsc clean

# Build the emulator
make -f makefile.metal.mvsc -j4

# Check if build was successful
if [ -f "fbneo_metal_core" ]; then
    echo "Build successful!"
    echo "You can run the emulator with:"
    echo "./fbneo_metal_core /path/to/mvsc.zip"
else
    echo "Build failed!"
    exit 1
fi

# Check for ROM file and provide instructions
if [ ! -d "roms" ]; then
    mkdir -p roms
fi

if [ ! -f "roms/mvsc.zip" ]; then
    echo ""
    echo "NOTE: You need to copy the Marvel vs Capcom ROM file (mvsc.zip) to the roms directory."
    echo "Please refer to ROM_SETUP.md for detailed instructions."
fi

echo ""
echo "Build complete!" 