#!/bin/bash
# Build script for FBNeo Metal input test application
# This can be used to test input handling on macOS/ARM64

set -e  # Exit on error

# Ensure we're in the right directory
if [ ! -f input_test.mm ]; then
    echo "Error: Please run this script from the src/burner/metal/input directory"
    exit 1
fi

# Include paths
FBNEO_ROOT="../../../.."  # Path to FBNeo root from the input directory
INCLUDES="-I$FBNEO_ROOT/src -I$FBNEO_ROOT/src/burn -I$FBNEO_ROOT/src/burner -I$FBNEO_ROOT/src/cpu -I.."

# Output directory
OUTPUT_DIR="./build"
mkdir -p $OUTPUT_DIR

# Compiler flags
CXXFLAGS="-std=c++17 -O2 -DSTANDALONE_INPUT_TEST=1"

# Frameworks
FRAMEWORKS="-framework Foundation -framework Cocoa -framework GameController"

# Source files
SOURCES="input_test.mm metal_input.mm ../fixes/cps_input_full.c"

echo "Building FBNeo Metal input test application..."

# Build for ARM64 (Apple Silicon)
echo "Building for ARM64..."
clang++ $CXXFLAGS -target arm64-apple-macos11 $INCLUDES $SOURCES $FRAMEWORKS -o $OUTPUT_DIR/input_test_arm64

# Build for x86_64 (Intel)
echo "Building for x86_64..."
clang++ $CXXFLAGS -target x86_64-apple-macos10.15 $INCLUDES $SOURCES $FRAMEWORKS -o $OUTPUT_DIR/input_test_x86_64

# Create a Universal Binary
echo "Creating universal binary..."
lipo -create $OUTPUT_DIR/input_test_arm64 $OUTPUT_DIR/input_test_x86_64 -output $OUTPUT_DIR/input_test

# Make it executable
chmod +x $OUTPUT_DIR/input_test

echo "Build complete. The test application is at $OUTPUT_DIR/input_test"
echo "Run it with: $OUTPUT_DIR/input_test" 