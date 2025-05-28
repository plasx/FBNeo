#!/bin/bash

# Pipe debug output to less for better display
# This lets you scroll through all debug output easily

ROM_PATH="/Users/plasx/dev/ROMs/mvsc.zip"

# Use command-line argument if provided
if [ $# -gt 0 ]; then
    ROM_PATH="$1"
fi

# Use this method for running the actual emulator but seeing all output
echo "Running emulator with debug output piped to less..."
echo "Press 'q' to exit the viewer once you've reviewed the output."
echo
sleep 1

# Run the emulator with debug format and pipe to less
./build/metal/fbneo_metal "$ROM_PATH" --enhanced-debug-only 2>&1 | less

# Alternative: If you want to save to a file AND view it:
# ./build/metal/fbneo_metal "$ROM_PATH" --enhanced-debug-only > debug_output.txt 2>&1
# less debug_output.txt 