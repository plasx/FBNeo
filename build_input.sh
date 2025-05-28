#!/bin/bash
# Build script for FBNeo Metal with input implementation

echo "Building FBNeo Metal with keyboard and gamepad input..."
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
    
    echo "Input Controls:"
    echo "- Player 1: Arrow Keys + Z,X,C,V,B,N (6 buttons)"
    echo "- Player 2: W,A,S,D + Q,W,E,R,T,Y"
    echo "- Start: Enter (P1), Tab (P2)"
    echo "- Coin: Space (P1), Caps Lock (P2)"
    echo "- Special: F5 (Save), F8 (Load), Esc (Quit)"
    echo "- Command key shortcuts: Cmd+S (Save), Cmd+L (Load)"
    
    # Make sure roms directory exists
    mkdir -p roms
    
    # Check if ROM file exists
    if [ -f "roms/mvsc.zip" ]; then
        echo "ROM file found: roms/mvsc.zip"
    else
        echo "NOTE: You need to copy the Marvel vs Capcom ROM file (mvsc.zip) to the roms directory"
    fi
else
    echo "Build failed!"
    exit 1
fi 