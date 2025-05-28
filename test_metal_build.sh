#!/bin/bash
# Test script for Metal build fixes

set -e
echo "Testing Metal build with C/C++ compatibility fixes..."

# Make sure directories exist
mkdir -p src/burner/metal/fixes
mkdir -p build/metal/obj/src/burner/metal

# Check if our fix files exist
if [ ! -f "src/burner/metal/fixes/c_cpp_fixes.h" ]; then
    echo "Error: Fix files not found!"
    exit 1
fi

# Test compiling metal_stubs.c with our fixes
echo "Testing compilation of metal_stubs.c..."
clang -g -Wall -I./src/dep/generated -I./src -I./src/burn -I./src/burner -I./src/burner/metal -DMETAL_BUILD -include ./src/burner/metal/fixes/c_cpp_fixes.h -c src/burner/metal/metal_stubs.c -o build/metal/obj/src/burner/metal/metal_stubs.o

if [ $? -eq 0 ]; then
    echo "✅ metal_stubs.c compiled successfully!"
else
    echo "❌ metal_stubs.c compilation failed!"
    exit 1
fi

# Test compiling a C++ file
echo "Testing compilation of metal_bridge.cpp..."
clang++ -g -Wall -I./src/dep/generated -I./src -I./src/burn -I./src/burner -I./src/burner/metal -std=c++17 -DMETAL_BUILD -c src/burner/metal/metal_bridge.cpp -o build/metal/obj/src/burner/metal/metal_bridge.o

if [ $? -eq 0 ]; then
    echo "✅ metal_bridge.cpp compiled successfully!"
else
    echo "❌ metal_bridge.cpp compilation failed!"
    exit 1
fi

echo "✅ All tests passed! The C/C++ compatibility fixes are working properly."
echo "Run './build_metal_fixed.sh' to build the full project with these fixes." 