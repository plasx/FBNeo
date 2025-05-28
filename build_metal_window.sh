#!/bin/bash
# Build script for Metal implementation with window fix

set -e

# Create necessary directories
mkdir -p src/dep/generated
mkdir -p build/metal/obj/src/burn/devices
mkdir -p build/metal/obj/src/burn/drv/megadrive
mkdir -p src/burner/metal/fixes
mkdir -p src/burner/metal

echo "====================================================="
echo "FBNeo Metal Window Fixed Build Script"
echo "====================================================="
echo "This script builds the Metal implementation with window fixes"
echo ""

# Clean build if requested
if [ "$1" == "clean" ]; then
    echo "Cleaning build..."
    make -f makefile.metal clean
fi

# Apply window fixes
echo "Applying window creation fixes..."
./fix_metal_window.sh

# Build with the minimal makefile
echo "Building with minimal makefile..."
make -f makefile.metal

echo ""
echo "Build complete! You can now run ./fbneo_metal"
echo "" 