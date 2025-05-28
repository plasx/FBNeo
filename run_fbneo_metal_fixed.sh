#!/bin/bash
# Build and run the FBNeo Metal implementation with fixes applied
set -e

# Apply the fixes
echo "Applying Metal build fixes..."
./fix_metal_build_targeted.sh

# Build with the fixed Makefile
echo "Building with fixed Makefile..."
make -f makefile.metal.fixed

# Run the emulator
echo "Running FBNeo Metal..."
./fbneo_metal
