#!/bin/bash

# Fix burn_fixes.cpp and compile it

mkdir -p obj/metal/burner/metal/fixes
rm -f src/burner/metal/fixes/burn_fixes.cpp

# Create burn lib fixes with correct types
echo "Creating burn lib fixes with correct types..."
cat > src/burner/metal/fixes/burn_fixes.cpp << 'EOF'
#include "burnint.h"

// Basic burn lib implementation stubs

// Global driver-related variables 
UINT32 nBurnDrvCount = 0;
UINT32 nBurnDrvActive = 0;

// Initialize the Burn library
int BurnLibInit() {
    nBurnDrvCount = 1; // We have one driver
    return 0;
}

// Exit the Burn library 
int BurnLibExit() {
    nBurnDrvCount = 0;
    nBurnDrvActive = 0;
    return 0;
}

// Burn driver functions
INT32 BurnDrvGetFlags() { return 0; }
int BurnDrvGetMaxPlayers() { return 1; }
char* BurnDrvGetTextA(unsigned int i) { return (char*)"Marvel vs. Capcom"; }
EOF

cat src/burner/metal/fixes/burn_fixes.cpp

# Compile the burn_fixes.cpp file
echo "Compiling burn_fixes.cpp..."
clang++ -O2 -fomit-frame-pointer -Wno-write-strings -std=c++17 -DUSE_METAL -DLSB_FIRST -DTCHAR_IS_CHAR \
    -I./src/dep/generated -I./src/burn -I./src/burn/devices -I./src/burn/sound -I./src/burn/snd \
    -I./src/cpu -I./src/cpu/m68k -I./src/cpu/z80 -I./src/intf -I./src/intf/video -I./src/intf/audio \
    -I./src/burner -I./src/burner/metal -I./src/burner/metal/fixes \
    -c src/burner/metal/fixes/burn_fixes.cpp -o obj/metal/burner/metal/fixes/burn_fixes.o

if [ -f obj/metal/burner/metal/fixes/burn_fixes.o ]; then
    echo "Successfully compiled burn_fixes.cpp"
else
    echo "Failed to compile burn_fixes.cpp"
fi 