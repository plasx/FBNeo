#!/bin/bash
# Build and run script for FBNeo Metal with all fixes

set -e

echo "====================================================="
echo "FBNeo Metal Build and Run Script (Fixed Version)"
echo "====================================================="
echo "This script builds and runs the Metal implementation with all fixes applied"
echo ""

# Step 1: Create necessary directories
echo "Creating necessary directories..."
mkdir -p src/dep/generated
mkdir -p build/metal/obj/src/burn/devices
mkdir -p build/metal/obj/src/burn/drv/megadrive
mkdir -p src/burner/metal/fixes
mkdir -p src/burner/metal

# Step 2: Apply byteswap fixes
echo "Applying BurnByteswap and memory management fixes..."
./fix_burn_byteswap.sh

# Step 3: Apply window creation fixes
echo "Applying window creation fixes..."
chmod +x fix_metal_window.sh
./fix_metal_window.sh

# Step 4: Build with the makefile
echo "Building FBNeo Metal implementation..."
chmod +x build_metal_window.sh
./build_metal_window.sh

# Step 5: Make run script executable
echo "Setting up run script..."
chmod +x run_fbneo_metal.sh

# Step 6: Run FBNeo Metal
echo ""
echo "FBNeo Metal built successfully!"
echo "Running FBNeo Metal with debugging enabled..."
echo ""
./fbneo_metal "$@" 2>&1 | tee fbneo_metal_debug.log

echo ""
echo "FBNeo Metal execution complete. Check the log for details."
echo "" 