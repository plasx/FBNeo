#!/bin/bash
# FBNeo Metal build script with patched files to fix build errors

set -e

# Create necessary directories
mkdir -p src/dep/generated
mkdir -p build/metal/obj/src/burn/devices

# Create symlinks for header files if they don't exist
if [ ! -L src/dep/generated/tiles_generic.h ]; then
    ln -sf ../../../burn/tiles_generic.h src/dep/generated/tiles_generic.h
fi

if [ ! -L src/dep/generated/burnint.h ]; then
    ln -sf ../../../burn/burnint.h src/dep/generated/burnint.h
fi

# Ensure fixes directory exists
mkdir -p src/burner/metal/fixes

# Check if patched files exist
if [ ! -f src/burner/metal/fixes/burn_patched.cpp ]; then
    echo "Error: Patched files missing. Please run this script from the project root."
    exit 1
fi

# Clean build if requested
if [ "$1" == "clean" ]; then
    echo "Cleaning build..."
    make -f makefile.metal clean
fi

# Run the build using main makefile.metal (which now includes our patched files)
echo "Building with patched files..."
make -f makefile.metal

echo "Build complete!" 