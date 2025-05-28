#!/bin/bash

# Build the minimal Metal version of FBNeo

echo "Building FBNeo Metal Minimal..."

# Clean up previous build
make -f makefile.metal.minimal clean

# Build the minimal version
make -f makefile.metal.minimal

echo "Build complete! Run ./fbneo_metal_minimal to test" 