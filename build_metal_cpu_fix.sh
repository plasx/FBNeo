#!/bin/bash
# Build script for Metal with CPU struct fixes

# Make sure we're in the right directory
cd "$(dirname "$0")"

# Create necessary directories
mkdir -p src/dep/generated
mkdir -p obj/metal/burner/metal/debug
mkdir -p obj/metal/burner/metal/ai
mkdir -p obj/metal/burner/metal/replay
mkdir -p build/metal/obj

# Create symlinks for missing headers if needed
if [ ! -L src/dep/generated/tiles_generic.h ]; then
    ln -sf ../../../burn/tiles_generic.h src/dep/generated/tiles_generic.h
fi

# Use our patched burnint.h
if [ ! -L src/dep/generated/burnint.h ]; then
    # Instead of symlinking to the original burnint.h, use our patched version
    ln -sf ../../../burner/metal/fixes/burnint_metal.h src/dep/generated/burnint.h
fi

# Use our fixed makefile
make -f makefile.metal.fixed

# Print status
if [ $? -eq 0 ]; then
    echo "Build successful!"
    echo "Binary location: ./fbneo_metal"
else
    echo "Build failed!"
fi 