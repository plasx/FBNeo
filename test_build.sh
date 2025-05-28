#!/bin/bash
# Simple test build script for FBNeo Metal

set -e  # Exit on error

# Detect architecture
ARCH=$(uname -m)
if [ "$ARCH" == "arm64" ]; then
    ARCH_FLAG="-arch arm64"
else
    ARCH_FLAG="-arch x86_64"
fi

# Compiler flags
CXXFLAGS="$ARCH_FLAG -std=c++17 -g -O0 -Wall -DMETAL_BUILD -fcommon -Wno-deprecated-declarations"
INCLUDES="-Isrc -Isrc/burn -Isrc/burn/devices -Isrc/burn/snd -Isrc/burn/drv -Isrc/burn/drv/capcom -Isrc/burner -Isrc/burner/metal -Isrc/burner/metal/app -Isrc/burner/metal/fixes -Isrc/cpu -Isrc/cpu/m68k -Isrc/cpu/z80 -Isrc/dep/generated -Isrc/intf/video -Isrc/intf/audio -Isrc/intf/input -I/opt/homebrew/include"

# Build files one by one
echo "Compiling metal_bridge.cpp..."
clang++ $CXXFLAGS $INCLUDES -c src/burner/metal/metal_bridge.cpp -o metal_bridge.o

echo "Compiling metal_stubs.cpp..."
clang++ $CXXFLAGS $INCLUDES -c src/burner/metal/metal_stubs.cpp -o metal_stubs.o 

echo "Compiling metal_game_stubs.cpp..."
clang++ $CXXFLAGS $INCLUDES -c src/burner/metal/metal_game_stubs.cpp -o metal_game_stubs.o

echo "Compiling rom_fixes.cpp..."
clang++ $CXXFLAGS $INCLUDES -c src/burner/metal/rom_fixes.cpp -o rom_fixes.o

echo "Compiling cps2_fixes.cpp..."
clang++ $CXXFLAGS $INCLUDES -c src/burner/metal/fixes/cps2_fixes.cpp -o cps2_fixes.o

echo "Build completed" 