#!/bin/bash

# Install AI dependencies for FBNeo Metal build
echo "Installing AI dependencies for FBNeo Metal build..."

# Create directories
mkdir -p deps/include/nlohmann
mkdir -p deps/include/torch
mkdir -p deps/lib

# Install nlohmann/json
echo "Downloading nlohmann/json.hpp..."
curl -o deps/include/nlohmann/json.hpp https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp

# Check if we need to download LibTorch
if [ ! -d "deps/libtorch" ]; then
    echo "Downloading LibTorch for MacOS..."
    
    # Check architecture
    if [ "$(uname -m)" == "arm64" ]; then
        echo "Detected ARM64 architecture (Apple Silicon)"
        LIBTORCH_URL="https://download.pytorch.org/libtorch/cpu/libtorch-macos-arm64-2.1.2.zip"
    else
        echo "Detected x86_64 architecture (Intel)"
        LIBTORCH_URL="https://download.pytorch.org/libtorch/cpu/libtorch-macos-2.1.2.zip"
    fi
    
    # Download and extract LibTorch
    curl -L $LIBTORCH_URL -o deps/libtorch.zip
    unzip deps/libtorch.zip -d deps/
    rm deps/libtorch.zip
    
    # Create symlinks to important headers
    echo "Creating symlinks for LibTorch headers..."
    ln -sf $(pwd)/deps/libtorch/include/torch deps/include/torch
    
    # Create symlinks to lib files
    echo "Creating symlinks for LibTorch libraries..."
    ln -sf $(pwd)/deps/libtorch/lib/* deps/lib/
fi

# Update makefiles to include our local dependencies
echo "Updating makefiles to include dependencies..."

# Create a fake LibTorch compatibility header for conditional compilation
cat > deps/include/torch_compat.h << 'EOF'
#pragma once

// This header provides compatibility for conditional LibTorch compilation
#ifdef USE_LIBTORCH
#include <torch/script.h>
#include <torch/torch.h>
#else
// Forward declarations
namespace torch {
    namespace jit {
        class Module;
        namespace script {
            class Module;
        }
    }
}
#endif
EOF

echo "Dependencies installed successfully!"
echo "To use locally installed dependencies, add these flags to your build:"
echo "  -I$(pwd)/deps/include -L$(pwd)/deps/lib" 