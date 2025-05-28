#!/bin/bash
# Build and Test AI Module Script

echo "Building FBNeo Metal with AI Module..."

# Create test directories
mkdir -p tests
mkdir -p models

# Set compiler flags
export CFLAGS="-g -O2 -I/opt/homebrew/include"
export CXXFLAGS="-g -O2 -std=c++17 -I/opt/homebrew/include -Isrc/burner/metal -Isrc/burner/metal/ai -Isrc"
export LDFLAGS="-framework Metal -framework MetalKit -framework Cocoa -framework GameController -framework CoreVideo -framework AudioToolbox -framework CoreGraphics -framework CoreML -framework Vision -framework MetalPerformanceShaders -framework MetalPerformanceShadersGraph -framework Compression"

# Build options
BUILD_TYPE="ai"  # Options: all, ai, test

# Build the full emulator with AI support
if [ "$BUILD_TYPE" = "all" ] || [ "$BUILD_TYPE" = "ai" ]; then
    echo "Building full emulator with AI module..."
    make -f makefile.metal clean
    make -f makefile.metal -j$(sysctl -n hw.ncpu)
    
    if [ $? -ne 0 ]; then
        echo "Error: Failed to build FBNeo with AI module"
        exit 1
    fi
    
    echo "FBNeo with AI module built successfully!"
fi

# Build and run the AI test only
if [ "$BUILD_TYPE" = "test" ]; then
    echo "Building AI test application..."
    
    # Create build directory for test
    mkdir -p build/test
    
    # Compile test application
    clang++ $CXXFLAGS $LDFLAGS -o build/test/ai_test tests/ai_test.cpp \
        src/burner/metal/ai/metal_ai_module.cpp \
        src/burner/metal/ai/ai_core_bridge.cpp \
        src/burner/metal/ai/ai_rl_algorithms.cpp \
        src/burner/metal/ai/ai_rl_integration.cpp \
        src/burner/metal/ai/ai_torch_policy.cpp \
        src/burner/metal/ai/pytorch_to_coreml.cpp
    
    if [ $? -ne 0 ]; then
        echo "Error: Failed to build AI test application"
        exit 1
    fi
    
    echo "Running AI test application..."
    ./build/test/ai_test
fi

echo "All done!"
exit 0 