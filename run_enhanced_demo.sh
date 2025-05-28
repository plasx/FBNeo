#!/bin/bash
# Run the enhanced Metal demo with Marvel vs Capcom

# Set working directory to script location
cd "$(dirname "$0")"

# ROM file path
ROM_PATH="/Users/plasx/dev/ROMs/mvsc.zip"

# Check if ROM exists
if [ ! -f "$ROM_PATH" ]; then
    echo "Error: ROM not found at $ROM_PATH"
    echo "Please update the script with the correct ROM path."
    exit 1
fi

echo "========================================"
echo "FBNeo Enhanced Metal Demo"
echo "========================================"

# Build the Metal backend
echo "Building FBNeo Metal backend..."
make -f makefile.metal clean && make -f makefile.metal -j10

# Check if build was successful by looking for the binary
if [ ! -f "./bin/metal/fbneo_metal" ]; then
    echo "Error: Build failed, binary not found!"
    exit 1
fi

echo "Build successful!"

# Make binary executable
chmod +x ./bin/metal/fbneo_metal

# Create symlink
echo "Creating symlink..."
ln -sf ./bin/metal/fbneo_metal ./fbneo_metal
chmod +x ./fbneo_metal

# Run the enhanced demo
echo "========================================"
echo "Running Enhanced Metal Demo..."
echo "========================================"
echo "The demo will now run 60 frames of emulation with enhanced graphics"
echo "and simulated input to demonstrate the Metal backend."
echo "This is a tech demo using CPS2 emulation patterns."
echo "========================================"

# Run the emulator
./fbneo_metal "$ROM_PATH"

echo "Demo completed! Thank you for trying the FBNeo Metal backend."
exit 0 