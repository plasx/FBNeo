#!/bin/bash
# Build script for FBNeo Metal with improved audio implementation

echo "Building FBNeo Metal with improved audio implementation..."
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
    
    # Make the script executable
    chmod +x fbneo_metal_stub
    
    echo "Audio Configuration:"
    echo "- Sample Rate: 44100 Hz"
    echo "- Buffer Size: 735 samples (44100/60 - matches 60fps)"
    echo "- Channels: 2 (stereo)"
    echo "- Format: 16-bit signed integer"
    
    # Make sure roms directory exists
    mkdir -p roms
    
    # Check if ROM file exists
    if [ -f "roms/mvsc.zip" ]; then
        echo "ROM file found: roms/mvsc.zip"
        echo "Running the emulator..."
        ./fbneo_metal_stub roms/mvsc.zip --display
    else
        echo "NOTE: You need to copy the Marvel vs Capcom ROM file (mvsc.zip) to the roms directory"
        echo "Once you have the ROM, run:"
        echo "./fbneo_metal_stub roms/mvsc.zip --display"
    fi
else
    echo "Build failed!"
    exit 1
fi 