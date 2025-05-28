#!/bin/bash

# Create build directories
mkdir -p build/metal/test

echo "=== Building AI System Test ==="
clang++ -std=c++17 -o build/metal/test/ai_system_test \
  -Isrc -Isrc/burner -Isrc/burner/metal -Isrc/burner/metal/ai -Isrc/burner/metal/fixes \
  src/burner/metal/tests/ai_system_test.cpp \
  src/burner/metal/tests/ai_system_test_mock.cpp \
  -DMETAL_BUILD

echo "=== Building AI Hyperparameter Tuning Test ==="
clang++ -std=c++17 -o build/metal/test/ai_tuning_test \
  -Isrc -Isrc/burner -Isrc/burner/metal -Isrc/burner/metal/ai -Isrc/burner/metal/fixes \
  src/burner/metal/tests/ai_tuning_test.cpp \
  src/burner/metal/ai/ai_hyperparameter_tuning.cpp \
  -DMETAL_BUILD

echo "=== Building AI Distributed Training Test ==="
clang++ -std=c++17 -o build/metal/test/ai_distributed_test \
  -Isrc -Isrc/burner -Isrc/burner/metal -Isrc/burner/metal/ai -Isrc/burner/metal/fixes \
  src/burner/metal/tests/ai_distributed_test.cpp \
  src/burner/metal/ai/ai_distributed_training.cpp \
  src/burner/metal/ai/ai_torch_policy.cpp \
  src/burner/metal/ai/ai_rl_algorithms.cpp \
  -DMETAL_BUILD

echo "=== Building AI Model Export Test ==="
clang++ -std=c++17 -o build/metal/test/ai_model_export_test \
  -Isrc -Isrc/burner -Isrc/burner/metal -Isrc/burner/metal/ai -Isrc/burner/metal/fixes \
  src/burner/metal/ai/test_model_export.cpp \
  src/burner/metal/ai/ai_torch_policy.cpp \
  src/burner/metal/ai/pytorch_to_coreml.cpp \
  src/burner/metal/ai/model_optimization.cpp \
  -DMETAL_BUILD

# Check if all builds succeeded
if [ $? -eq 0 ]; then
  echo -e "\n=== Running AI System Test ==="
  ./build/metal/test/ai_system_test
  
  echo -e "\n=== Running AI Hyperparameter Tuning Test ==="
  ./build/metal/test/ai_tuning_test
  
  echo -e "\n=== Running AI Distributed Training Test ==="
  ./build/metal/test/ai_distributed_test
  
  echo -e "\n=== Running AI Model Export Test ==="
  ./build/metal/test/ai_model_export_test
  
  echo -e "\n=== All tests completed! ==="
else
  echo "Build failed!"
  exit 1
fi 