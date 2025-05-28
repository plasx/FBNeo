#!/bin/bash
# Build script for FBNeo Metal with frame loop implementation

echo "Building FBNeo Metal with frame loop implementation..."
echo "Target: Marvel vs Capcom (CPS2)"

# Make sure build directories exist
mkdir -p build/metal_stub/obj
mkdir -p build/metal_stub/obj/src/burner/metal
mkdir -p build/metal_stub/obj/src/burn/drv/capcom

# Clean previous build
make -f makefile.metal.stub clean

# Build with the stub makefile
make -f makefile.metal.stub -j4

# Check if build was successful
if [ -f "fbneo_metal_stub" ]; then
    echo "====================================="
    echo "Build successful!"
    echo "You can run the emulator with:"
    echo "./fbneo_metal_stub roms/mvsc.zip --display"
    echo "====================================="
    
    # Make sure roms directory exists
    mkdir -p roms
    
    # Check if ROM file exists
    if [ -f "roms/mvsc.zip" ]; then
        echo "ROM file found: roms/mvsc.zip"
    else
        echo "NOTE: You need to copy the Marvel vs Capcom ROM file (mvsc.zip) to the roms directory"
    fi
    
    # Make the script executable
    chmod +x fbneo_metal_stub
else
    echo "Build failed!"
    exit 1
fi 