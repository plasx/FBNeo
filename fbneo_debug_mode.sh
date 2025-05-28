#!/bin/bash

# FBNeo Debug Mode Launcher
# This script launches FBNeo with the standardized debug output format

echo "FBNeo Debug Mode Launcher"
echo "-----------------------------"
echo

# Default ROM path
ROM_PATH="/Users/plasx/dev/ROMs/mvsc.zip"

# Check if a ROM path was provided
if [ $# -gt 0 ]; then
  ROM_PATH="$1"
fi

echo "ROM Path: $ROM_PATH"
echo

# Launch options
#  1. Just show the debug format and exit
#  2. Run emulator with debug format
#  3. Show colored debug format with fbneo_debug_display.sh
#  4. Run debug preview program (exact format)
#  5. Run with unbuffered direct terminal output

echo "Launch options:"
echo "  1. Show debug format only (quick preview)"
echo "  2. Run emulator with debug format"
echo "  3. Show colored debug format with fbneo_debug_display.sh"
echo "  4. Run debug preview program (exact format)"
echo "  5. NEW: Run with unbuffered direct terminal output"
echo
read -p "Enter option (1-5): " OPTION

case $OPTION in
  1)
    echo "Showing debug format only..."
    ./build/metal/fbneo_metal "$ROM_PATH" --debug-format
    ;;
  2)
    echo "Running emulator with debug format..."
    ./build/metal/fbneo_metal "$ROM_PATH" --standardized-output
    ;;
  3)
    echo "Showing colored debug format..."
    ./fbneo_debug_display.sh
    ;;
  4)
    echo "Running debug preview program..."
    ./debug_preview "$ROM_PATH"
    ;;
  5)
    echo "Running with unbuffered direct terminal output..."
    script -F -q /dev/null ./build/metal/fbneo_metal "$ROM_PATH" --debug-format --unbuffered-output
    ;;
  *)
    echo "Invalid option. Exiting."
    exit 1
    ;;
esac

exit 0 