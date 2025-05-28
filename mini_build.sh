#!/bin/bash
# Simple script to build the minimal Metal test app

set -e

# Target directories
OUTPUT_DIR="../../../bin/metal"
mkdir -p $OUTPUT_DIR

# Check architecture
ARCH=$(uname -m)
if [ "$ARCH" == "arm64" ]; then
    ARCH_FLAG="-arch arm64"
else
    ARCH_FLAG="-arch x86_64"
fi

# Frameworks
FRAMEWORKS="-framework Metal -framework MetalKit -framework Cocoa -framework AppKit"

# Compiler flags
CXXFLAGS="$ARCH_FLAG -O2"

# Build minimal test app
echo "Building minimal Metal test app..."
clang++ $CXXFLAGS minimal_metal.mm -o $OUTPUT_DIR/minimal_metal $FRAMEWORKS

if [ $? -eq 0 ]; then
    echo "Minimal Metal test app build successful!"
    echo "Binary location: $OUTPUT_DIR/minimal_metal"
    
    # Make executable
    chmod +x $OUTPUT_DIR/minimal_metal
    
    echo "Running the app..."
    $OUTPUT_DIR/minimal_metal &
else
    echo "Error: Build failed."
    exit 1
fi

exit 0 