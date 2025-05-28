#!/bin/bash
# Simple script to build FBNeo Metal

echo "Quick build script for FBNeo Metal..."

# Ensure g_frameBuffer is available
echo "Compiling metal_globals.cpp..."
mkdir -p build/metal/obj/src/burner/metal
clang++ -std=c++17 -O3 -Wall -DMACOSX -DUSE_METAL_FIXES -DMETAL_IMPLEMENTATION_FILE -DUSE_METAL_RENDERER -DDARWIN -arch arm64 -DENHANCED_DEBUG_MODE=1 -Isrc -Isrc/burn -Isrc/burner -Isrc/cpu -Isrc/dep -Isrc/burner/metal -c src/burner/metal/metal_globals.cpp -o build/metal/obj/src/burner/metal/metal_globals.o

# Now run the main build
echo "Running main build..."
make -f makefile.metal

# Move to executable to current directory if successful
if [ -f build/metal/fbneo_metal ]; then
    cp build/metal/fbneo_metal .
    chmod +x fbneo_metal
    echo "Success! Binary copied to ./fbneo_metal"
else
    echo "Build failed."
fi 