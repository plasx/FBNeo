#!/bin/bash
# Replace the broken metal_bridge.cpp with our fixed version

cp -f src/burner/metal/metal_bridge.cpp src/burner/metal/metal_bridge.cpp.bak
cp -f src/burner/metal/fixed_bridge.cpp src/burner/metal/metal_bridge.cpp

echo "Bridge file replaced. Original saved as metal_bridge.cpp.bak" 