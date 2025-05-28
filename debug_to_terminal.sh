#!/bin/bash

# Script to ensure debug output appears in terminal
# By using script -F, we get unbuffered output directly to the terminal

ROM_PATH="/Users/plasx/dev/ROMs/mvsc.zip"

# Use command-line argument if provided
if [ $# -gt 0 ]; then
    ROM_PATH="$1"
fi

# Use script to capture terminal output with unbuffered mode (-F)
# This ensures all output is immediately displayed
script -F -q /dev/null ./build/metal/fbneo_metal "$ROM_PATH" --debug-format --unbuffered-output

# If you still don't see output, uncomment this alternative method:
# stdbuf -o0 ./build/metal/fbneo_metal "$ROM_PATH" --debug-format --unbuffered-output 