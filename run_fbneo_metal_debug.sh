#!/bin/bash
set -e
echo "Cleaning and rebuilding..."
make -f makefile.metal clean
make -f makefile.metal -j10
# Enable Metal debug output
export MTL_DEBUG_LAYER=1
export MTL_HUD_ENABLED=1
echo "Running with debug output enabled"
ROM_PATH="/Users/plasx/dev/ROMs/mvsc.zip"
./fbneo_metal "$ROM_PATH"
