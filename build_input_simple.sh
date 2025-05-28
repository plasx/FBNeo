#!/bin/bash
# Simplified build script for the input system

echo "Building simplified FBNeo Metal with keyboard and gamepad input..."

# Ensure output directory exists
mkdir -p build

# Compile metal_input_stub.mm with the test main
clang++ -std=c++17 -O3 -fobjc-arc -DMACOSX -DUSE_METAL_FIXES -DMETAL_IMPLEMENTATION_FILE \
    -DUSE_METAL_RENDERER -DDARWIN -DINCLUDE_CPS_DRIVER=1 -DTCHAR_DEFINED=1 -DMAX_PATH=512 \
    -DMETAL_COMPATIBILITY=1 -arch arm64 \
    -I. -Isrc -Isrc/burn -Isrc/burner -Isrc/cpu -Isrc/dep -Isrc/burner/metal -Isrc/cpu/z80 -Isrc/burn/snd \
    -include src/burner/metal/metal_input_defs.h \
    -framework Cocoa -framework Metal -framework MetalKit -framework GameController \
    src/burner/metal/metal_debug_overlay_stub.mm \
    src/burner/metal/metal_input_stub.mm \
    src/burner/metal/input_test_main.mm \
    -o fbneo_input_test

# Check if build was successful
if [ -f "fbneo_input_test" ]; then
    echo "====================================="
    echo "Build successful!"
    echo "You can test the input system with:"
    echo "./fbneo_input_test"
    echo "====================================="
    
    # Make the executable
    chmod +x fbneo_input_test
    
    echo "Input Controls:"
    echo "- Player 1: Arrow Keys + Z,X,C,V,B,N (6 buttons)"
    echo "- Player 2: W,A,S,D + Q,W,E,R,T,Y"
    echo "- Start: Enter (P1), Tab (P2)"
    echo "- Coin: Space (P1), Caps Lock (P2)"
    echo "- Special: F5 (Save), F8 (Load), Esc (Quit)"
    echo "- Command key shortcuts: Cmd+S (Save), Cmd+L (Load)"
else
    echo "Build failed!"
    exit 1
fi 