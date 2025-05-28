#!/bin/bash

# Build and run FBNeo with Metal backend

# Set working directory to script location
cd "$(dirname "$0")"

# Build FBNeo with Metal backend
echo "Building FBNeo with Metal backend..."
make -f makefile.metal clean
make -f makefile.metal

# Check if build was successful
if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

# Create symlink from root directory to binary
echo "Creating symlink to binary..."
ln -sf ./bin/metal/fbneo_metal .

# Make sure the binary is executable
chmod +x ./bin/metal/fbneo_metal

# Run the emulator
echo "Running FBNeo with Metal backend..."
./fbneo_metal "$@" 