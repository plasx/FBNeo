#!/bin/bash
# Debug script for FBNeo Metal renderer

echo "=== Building FBNeo Metal (Debug Mode) ==="
make -f makefile.metal.simple clean
make -f makefile.metal.simple

echo "=== Setting Debug Environment ==="
export METAL_DEVICE_WRAPPER_TYPE=1
export METAL_DEBUG=1
export METAL_DEBUG_OPTIONS="validation,shader-validation"
export FB_DEBUG=1

# Create debug directory for logs
mkdir -p debug_output

echo "=== Running FBNeo Metal with Test Pattern ==="
echo "You should see a rainbow test pattern in the window."
echo "If you don't see anything, check the console output for errors."
echo "To close the application, press Ctrl+C in the terminal."
echo ""
echo "Starting in 3 seconds..."
sleep 3

# Run the emulator and capture both stdout and stderr to a log file
./fbneo_metal 2>&1 | tee debug_output/metal_debug_log.txt

echo ""
echo "Check debug_output/metal_debug_log.txt for detailed logs." 