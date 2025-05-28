#!/bin/bash

# Show the exact debug format using the debug_preview program
# This displays the standardized debug output without running the actual emulator

ROM_PATH="/Users/plasx/dev/ROMs/mvsc.zip"

# Use command-line argument if provided
if [ $# -gt 0 ]; then
    ROM_PATH="$1"
fi

# Display a header
echo "================== FBNeo Debug Format =================="
echo "ROM: $ROM_PATH"
echo "======================================================"
echo

# Run the debug_preview program with the specified ROM path
./debug_preview "$ROM_PATH" 