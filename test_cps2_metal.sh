#!/bin/bash

# FBNeo CPS2 Metal Test Script
# This script builds and tests the CPS2 emulator with Metal renderer

set -e  # Exit on error

echo "==================================="
echo "FBNeo CPS2 Metal Build & Test"
echo "==================================="

# Clean previous build
echo "🧹 Cleaning previous build..."
make -f makefile.metal clean

# Build the emulator
echo "🔨 Building FBNeo Metal..."
make -f makefile.metal -j$(sysctl -n hw.ncpu)

# Check if build succeeded
if [ ! -f "./fbneo_metal" ]; then
    echo "❌ Build failed! fbneo_metal binary not found."
    exit 1
fi

echo "✅ Build successful!"

# Make the binary executable
chmod +x ./fbneo_metal

# Check for ROM file
ROM_PATH=""
if [ -n "$1" ]; then
    ROM_PATH="$1"
else
    # Look for mvsc.zip in common locations
    for path in "./mvsc.zip" "./roms/mvsc.zip" "$HOME/roms/mvsc.zip" "/usr/local/share/fbneo/roms/mvsc.zip"; do
        if [ -f "$path" ]; then
            ROM_PATH="$path"
            break
        fi
    done
fi

if [ -z "$ROM_PATH" ]; then
    echo "⚠️  No ROM file specified or found!"
    echo "Usage: $0 /path/to/mvsc.zip"
    echo ""
    echo "The emulator will run in demo mode without a ROM."
    echo ""
    read -p "Press Enter to continue in demo mode..."
    ./fbneo_metal
else
    echo "🎮 Running with ROM: $ROM_PATH"
    echo ""
    echo "Controls:"
    echo "  Player 1: Arrow keys + Z/X/C/V/B/N"
    echo "  Player 2: W/A/S/D + Q/E/R/T/Y/H"
    echo "  System: 1/2 (Start), 5/6 (Coin), F3 (Reset)"
    echo "  Debug: F1 (Toggle overlay)"
    echo ""
    ./fbneo_metal "$ROM_PATH"
fi

echo ""
echo "Test completed." 