#!/bin/bash

# Fix vid_interface_fixes.cpp and compile it

mkdir -p obj/metal/burner/metal/fixes
rm -f src/burner/metal/fixes/vid_interface_fixes.cpp

# Create video interface fixes with correct implementation
echo "Creating video interface fixes..."
cat > src/burner/metal/fixes/vid_interface_fixes.cpp << 'EOF'
#include "burnint.h"

// Define MAX_HARDCODED_FX if not defined
#ifndef MAX_HARDCODED_FX
#define MAX_HARDCODED_FX 10
#endif

// External variables we need
extern bool bDrvOkay;

// Metal video interface - simplified version with stubs
INT32 VidSelect = 0;
INT32 nVidSelect = 0;
INT32 nVidWidth  = 0;
INT32 nVidHeight = 0;
INT32 nVidRefresh = 0;
INT32 nVidDepth  = 0;
INT32 nVidBlitterOpt[4] = {0, 0, 0, 0};
bool bVidOkay = false;

// Define other needed variables
struct hardfx_config { 
    char title[128];
    int version; 
    int preset;
    float saturation;
    float brightness;
};

struct hardfx_config HardFXConfigs[MAX_HARDCODED_FX];

// Video core functions (stubs)
INT32 VidInit() { return 0; }
INT32 VidExit() { return 0; }
INT32 VidRedraw() { return 0; }
INT32 VidFrame() { return 0; }
INT32 VidPaint(INT32 bValidate) { return 0; }
EOF

cat src/burner/metal/fixes/vid_interface_fixes.cpp

# Compile the vid_interface_fixes.cpp file
echo "Compiling vid_interface_fixes.cpp..."
clang++ -O2 -fomit-frame-pointer -Wno-write-strings -std=c++17 -DUSE_METAL -DLSB_FIRST -DTCHAR_IS_CHAR \
    -I./src/dep/generated -I./src/burn -I./src/burn/devices -I./src/burn/sound -I./src/burn/snd \
    -I./src/cpu -I./src/cpu/m68k -I./src/cpu/z80 -I./src/intf -I./src/intf/video -I./src/intf/audio \
    -I./src/burner -I./src/burner/metal -I./src/burner/metal/fixes \
    -c src/burner/metal/fixes/vid_interface_fixes.cpp -o obj/metal/burner/metal/fixes/vid_interface_fixes.o

if [ -f obj/metal/burner/metal/fixes/vid_interface_fixes.o ]; then
    echo "Successfully compiled vid_interface_fixes.cpp"
else
    echo "Failed to compile vid_interface_fixes.cpp"
fi 