#!/bin/bash

# Create directories
mkdir -p build/metal/test

# Compile the AI system test
clang++ -std=c++17 -o build/metal/test/ai_system_test \
  -Isrc -Isrc/burner -Isrc/burner/metal -Isrc/burner/metal/ai -Isrc/burner/metal/fixes \
  src/burner/metal/tests/ai_system_test.cpp \
  src/burner/metal/tests/ai_system_test_mock.cpp \
  -DMETAL_BUILD

# Check if compilation succeeded
if [ $? -eq 0 ]; then
  echo "Build successful! Running test..."
  ./build/metal/test/ai_system_test
else
  echo "Build failed!"
  exit 1
fi 