#!/bin/bash
# Script to compile Metal shaders for FBNeo

# Set error handling
set -e

# Configuration
SOURCE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SHADERS_FILE="${SOURCE_DIR}/Shaders.metal"
OUTPUT_FILE="${SOURCE_DIR}/fbneo_shaders.metallib"
TEMP_DIR="${SOURCE_DIR}/.shader_build"
METAL_PATH="/usr/bin/xcrun -sdk macosx metal"
METALLIB_PATH="/usr/bin/xcrun -sdk macosx metallib"

# Print configuration
echo "FBNeo Metal Shader Compilation"
echo "Source: ${SHADERS_FILE}"
echo "Output: ${OUTPUT_FILE}"

# Check if source file exists
if [ ! -f "${SHADERS_FILE}" ]; then
    echo "Error: Shader source file not found at ${SHADERS_FILE}"
    exit 1
fi

# Create temporary directory if it doesn't exist
mkdir -p "${TEMP_DIR}"

# Compile shader source to .air file
echo "Compiling Metal shaders..."
$METAL_PATH -c "${SHADERS_FILE}" -o "${TEMP_DIR}/Shaders.air" -D FBNEO_METAL -std=macos-metal2.0

# Create metal library
echo "Creating Metal library..."
$METALLIB_PATH "${TEMP_DIR}/Shaders.air" -o "${OUTPUT_FILE}"

# Clean up
rm -rf "${TEMP_DIR}"

echo "Metal shader compilation complete!"
echo "Library created at: ${OUTPUT_FILE}"
exit 0 