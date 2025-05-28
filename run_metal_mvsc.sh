#!/bin/bash

echo "Starting FBNeo Metal with Marvel vs. Capcom"

# Ensure roms directory exists
mkdir -p roms

# Check if MvC ROM is present
if [ ! -f "roms/mvsc.zip" ]; then
    echo "WARNING: Marvel vs. Capcom ROM (mvsc.zip) not found in roms directory!"
    echo "The emulator may not run correctly without the ROM."
    touch roms/mvsc.zip  # Create empty placeholder for testing
fi

# Check if the binary exists
if [ ! -f "bin/metal/fbneo_metal_core" ]; then
    echo "Error: FBNeo Metal executable not found!"
    echo "Please run build_metal_mvsc.sh first."
    exit 1
fi

# Run the emulator
echo "Launching FBNeo Metal..."
cd bin/metal && ./fbneo_metal_core

# Handle exit status
if [ $? -ne 0 ]; then
    echo "FBNeo Metal exited with an error."
    exit 1
fi

echo "FBNeo Metal session ended." 