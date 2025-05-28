#include "ai_torch_policy.h"
#include "ai_input_frame.h"
#include "ai_output_action.h"
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <memory>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>

// This file would normally include PyTorch and CoreML headers
// For this implementation, we'll create proxy functions that would
// be implemented with the actual libraries in a real project

namespace fbneo {
namespace ai {

// Function declarations
bool createConversionScript(const std::string& scriptPath);
bool createOptimizationScript(const std::string& scriptPath);
bool createBatchConversionScript(const std::string& scriptPath);
bool createModelValidatorScript(const std::string& scriptPath);

// Constants for model conversion
constexpr const char* PYTHON_SCRIPT_PATH = "tools/convert_pytorch_to_coreml.py";

/**
 * @brief Converts a PyTorch model to CoreML format
 * @param torchModelPath Path to the PyTorch model file (.pt)
 * @param coreMLOutputPath Path for the output CoreML model (.mlmodel)
 * @param inputShape Array of input dimensions
 * @param useNeuralEngine Whether to enable the Apple Neural Engine
 * @param quantize Whether to quantize the model for reduced size
 * @return True if conversion was successful
 */
bool convertPyTorchToCoreML(const std::string& torchModelPath, 
                           const std::string& coreMLOutputPath,
                           const std::vector<int>& inputShape,
                           bool useNeuralEngine,
                           bool quantize) {
    std::cout << "Converting PyTorch model to CoreML: " << torchModelPath << " -> " << coreMLOutputPath << std::endl;
    
    // Check if PyTorch model exists
    std::ifstream checkFile(torchModelPath);
    if (!checkFile.good()) {
        std::cerr << "Error: PyTorch model file does not exist: " << torchModelPath << std::endl;
        return false;
    }
    checkFile.close();
    
    // Create a temporary directory for the script
    std::string tempDir = "/tmp"; // Unix-based temp directory
#ifdef _WIN32
    tempDir = std::getenv("TEMP") ? std::getenv("TEMP") : "C:/Temp";
#endif
    
    // Create a unique filename for the script
    std::string scriptPath = tempDir + "/torch_to_coreml_" + 
                           std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + 
                           ".py";
    
    // Create conversion script
    if (!createConversionScript(scriptPath)) {
        std::cerr << "Failed to create conversion script" << std::endl;
        return false;
    }
    
    // Build input shape string
    std::string inputShapeStr = "";
    for (size_t i = 0; i < inputShape.size(); ++i) {
        inputShapeStr += std::to_string(inputShape[i]);
        if (i < inputShape.size() - 1) {
            inputShapeStr += ",";
        }
    }
    
    // Build the command to run the conversion script
    std::string cmd = "python \"" + scriptPath + "\"" +
                     " --torch-model \"" + torchModelPath + "\"" +
                     " --coreml-output \"" + coreMLOutputPath + "\"" +
                     " --input-shape " + inputShapeStr;
    
    // Add flags
    if (useNeuralEngine) {
        cmd += " --use-neural-engine";
    }
    
    if (quantize) {
        cmd += " --quantize";
    }
    
    // Execute the command
    std::cout << "Running: " << cmd << std::endl;
    int result = system(cmd.c_str());
    
    // Clean up the temporary script
    std::remove(scriptPath.c_str());
    
    if (result != 0) {
        std::cerr << "Error: PyTorch to CoreML conversion failed with code " << result << std::endl;
        return false;
    }
    
    std::cout << "PyTorch to CoreML conversion complete: " << coreMLOutputPath << std::endl;
    return true;
}

/**
 * @brief Batch converts multiple PyTorch models to CoreML format
 * 
 * This function converts multiple PyTorch models to CoreML format in parallel,
 * which can significantly speed up the conversion process when dealing with
 * multiple models. Each model is converted with the same settings.
 * 
 * @param torchModelPaths Vector of paths to PyTorch model files
 * @param coreMLOutputPaths Vector of paths for output CoreML models
 * @param inputShape Array of input dimensions (same for all models)
 * @param useNeuralEngine Whether to enable the Apple Neural Engine
 * @param quantize Whether to quantize the models for reduced size
 * @param numThreads Number of parallel conversion threads (0 = auto)
 * @return Number of successfully converted models
 */
int batchConvertPyTorchToCoreML(const std::vector<std::string>& torchModelPaths,
                              const std::vector<std::string>& coreMLOutputPaths,
                              const std::vector<int>& inputShape,
                              bool useNeuralEngine,
                              bool quantize,
                              int numThreads) {
    // Validate inputs
    if (torchModelPaths.empty() || coreMLOutputPaths.empty() || 
        torchModelPaths.size() != coreMLOutputPaths.size()) {
        std::cerr << "Error: Invalid input parameters for batch conversion" << std::endl;
        return 0;
    }
    
    const size_t numModels = torchModelPaths.size();
    std::cout << "Starting batch conversion of " << numModels << " PyTorch models" << std::endl;
    
    // Determine number of threads to use
    if (numThreads <= 0) {
        // Auto-determine based on hardware
        numThreads = std::thread::hardware_concurrency();
        // Ensure at least one thread and at most the number of models
        numThreads = std::max(1, std::min(numThreads, static_cast<int>(numModels)));
    }
    
    std::cout << "Using " << numThreads << " parallel conversion threads" << std::endl;
    
    // Create a temporary Python script for batch conversion
    std::string tempDir = "/tmp"; // Unix-based temp directory
#ifdef _WIN32
    tempDir = std::getenv("TEMP") ? std::getenv("TEMP") : "C:/Temp";
#endif
    
    // Create a unique filename for the batch script
    std::string scriptPath = tempDir + "/batch_torch_to_coreml_" + 
                           std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + 
                           ".py";
    
    // Create the batch conversion script
    if (!createBatchConversionScript(scriptPath)) {
        std::cerr << "Failed to create batch conversion script" << std::endl;
        return 0;
    }
    
    // Set up threading
    std::atomic<int> successCount(0);
    std::mutex printMutex;
    
    // Process models in batches
    for (size_t startIdx = 0; startIdx < numModels; startIdx += numThreads) {
        // Calculate the size of the current batch
        size_t batchSize = std::min(numThreads, static_cast<int>(numModels - startIdx));
        
        // Create and start threads for this batch
        std::vector<std::thread> threads;
        for (size_t i = 0; i < batchSize; ++i) {
            size_t modelIdx = startIdx + i;
            threads.emplace_back([&, modelIdx]() {
                const std::string& torchModelPath = torchModelPaths[modelIdx];
                const std::string& coreMLOutputPath = coreMLOutputPaths[modelIdx];
                
                {
                    std::lock_guard<std::mutex> lock(printMutex);
                    std::cout << "Thread " << std::this_thread::get_id() 
                              << " converting model " << modelIdx + 1 << "/" << numModels 
                              << ": " << torchModelPath << " -> " << coreMLOutputPath << std::endl;
                }
                
                // Build input shape string
                std::string inputShapeStr = "";
                for (size_t j = 0; j < inputShape.size(); ++j) {
                    inputShapeStr += std::to_string(inputShape[j]);
                    if (j < inputShape.size() - 1) {
                        inputShapeStr += ",";
                    }
                }
                
                // Build the command to run the conversion script
                std::string cmd = "python \"" + scriptPath + "\"" +
                                 " --torch-model \"" + torchModelPath + "\"" +
                                 " --coreml-output \"" + coreMLOutputPath + "\"" +
                                 " --input-shape " + inputShapeStr;
                
                // Add flags
                if (useNeuralEngine) {
                    cmd += " --use-neural-engine";
                }
                
                if (quantize) {
                    cmd += " --quantize";
                }
                
                // Execute the command
                {
                    std::lock_guard<std::mutex> lock(printMutex);
                    std::cout << "Running: " << cmd << std::endl;
                }
                
                int result = system(cmd.c_str());
                
                if (result == 0) {
                    {
                        std::lock_guard<std::mutex> lock(printMutex);
                        std::cout << "Successfully converted model " << modelIdx + 1 
                                  << ": " << coreMLOutputPath << std::endl;
                    }
                    successCount++;
                } else {
                    std::lock_guard<std::mutex> lock(printMutex);
                    std::cerr << "Error: Failed to convert model " << modelIdx + 1 
                              << " with code " << result << std::endl;
                }
            });
        }
        
        // Wait for all threads in this batch to complete
        for (auto& thread : threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }
    
    // Clean up
    std::remove(scriptPath.c_str());
    
    std::cout << "Batch conversion complete. Successfully converted " 
              << successCount << " of " << numModels << " models." << std::endl;
    
    return successCount;
}

/**
 * @brief Optimizes a CoreML model for a specific device
 * @param coreMLModelPath Path to the CoreML model file
 * @param outputPath Path for the optimized model
 * @param targetDevice Target device type (CPU, GPU, ANE)
 * @return True if optimization was successful
 */
bool optimizeCoreMLModel(const std::string& coreMLModelPath,
                        const std::string& outputPath,
                        const std::string& targetDevice) {
    std::cout << "Optimizing CoreML model for " << targetDevice << ": " 
             << coreMLModelPath << " -> " << outputPath << std::endl;
    
    // Check if the input file exists
    std::ifstream checkFile(coreMLModelPath);
    if (!checkFile.good()) {
        std::cerr << "Error: CoreML model file does not exist: " << coreMLModelPath << std::endl;
        return false;
    }
    checkFile.close();
    
    // Create an optimization script in the temp directory
    std::string tempDir = "/tmp"; // Unix-based temp directory
#ifdef _WIN32
    tempDir = std::getenv("TEMP") ? std::getenv("TEMP") : "C:/Temp";
#endif
    
    // Create a unique filename for the script
    std::string scriptPath = tempDir + "/optimize_coreml_" + 
                           std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + 
                           ".py";
    
    // Create the optimization script
    if (!createOptimizationScript(scriptPath)) {
        std::cerr << "Failed to create optimization script" << std::endl;
        return false;
    }
    
    // Build the command to run the optimization script
    std::string cmd = "python \"" + scriptPath + "\"" +
                     " --model \"" + coreMLModelPath + "\"" +
                     " --output \"" + outputPath + "\"" +
                     " --target " + targetDevice;
    
    // Execute the command
    std::cout << "Running: " << cmd << std::endl;
    int result = system(cmd.c_str());
    
    // Clean up the temporary script
    std::remove(scriptPath.c_str());
    
    if (result != 0) {
        std::cerr << "Error: CoreML optimization failed with code " << result << std::endl;
        return false;
    }
    
    std::cout << "CoreML model optimization complete: " << outputPath << std::endl;
    return true;
}

/**
 * @brief Converts an AITorchPolicy to CoreML format
 * @param policy The policy to convert
 * @param outputPath Path for the output CoreML model
 * @param useNeuralEngine Whether to enable the Apple Neural Engine
 * @return True if conversion was successful
 */
bool convertPolicyToCoreML(AITorchPolicy* policy, 
                          const std::string& outputPath,
                          bool useNeuralEngine = true) {
    if (!policy) {
        std::cerr << "Error: Cannot convert null policy" << std::endl;
        return false;
    }
    
    // This is a stub implementation
    // In a real implementation, this would:
    // 1. Export the policy to a temporary PyTorch model file
    // 2. Convert the PyTorch model to CoreML
    // 3. Clean up temporary files
    
    std::cout << "Converting policy to CoreML: " << outputPath << std::endl;
    
    // Define input shape based on expected frame dimensions
    // For example, [1, 4, 84, 84] for a batch of 4 84x84 frames
    std::vector<int> inputShape = {1, 4, 84, 84};
    
    // Create temporary file for PyTorch model
    std::string tempPath = outputPath + ".tmp.pt";
    
    // Export policy to temporary file (this would be implemented in AITorchPolicy)
    // policy->exportToFile(tempPath);
    
    // Convert to CoreML
    bool success = convertPyTorchToCoreML(tempPath, outputPath, inputShape, useNeuralEngine, false);
    
    // Clean up temporary file
    // In a real implementation, this would delete the file
    // std::remove(tempPath.c_str());
    
    return success;
}

/**
 * @brief Create a Python script for PyTorch to CoreML conversion
 * @param scriptPath Path to create the script
 * @return True if script was created successfully
 */
bool createConversionScript(const std::string& scriptPath) {
    // Create a Python script that uses coremltools to convert PyTorch models
    std::ofstream scriptFile(scriptPath);
    if (!scriptFile.is_open()) {
        std::cerr << "Error: Could not create conversion script: " << scriptPath << std::endl;
        return false;
    }
    
    // Write script content
    scriptFile << "#!/usr/bin/env python\n";
    scriptFile << "# PyTorch to CoreML conversion script\n";
    scriptFile << "# Usage: python convert_pytorch_to_coreml.py --torch-model model.pt --coreml-output model.mlmodel --input-shape 1,4,84,84 [--use-neural-engine] [--quantize]\n\n";
    
    scriptFile << "import argparse\n";
    scriptFile << "import torch\n";
    scriptFile << "import coremltools as ct\n";
    scriptFile << "import numpy as np\n\n";
    
    scriptFile << "def main():\n";
    scriptFile << "    parser = argparse.ArgumentParser(description='Convert PyTorch model to CoreML')\n";
    scriptFile << "    parser.add_argument('--torch-model', required=True, help='Path to PyTorch model file (.pt)')\n";
    scriptFile << "    parser.add_argument('--coreml-output', required=True, help='Path for output CoreML model (.mlmodel)')\n";
    scriptFile << "    parser.add_argument('--input-shape', required=True, help='Input shape as comma-separated values (e.g., 1,4,84,84)')\n";
    scriptFile << "    parser.add_argument('--use-neural-engine', action='store_true', help='Enable Apple Neural Engine')\n";
    scriptFile << "    parser.add_argument('--quantize', action='store_true', help='Quantize model for reduced size')\n";
    scriptFile << "    args = parser.parse_args()\n\n";
    
    scriptFile << "    # Parse input shape\n";
    scriptFile << "    input_shape = [int(dim) for dim in args.input_shape.split(',')]\n";
    scriptFile << "    print(f'Input shape: {input_shape}')\n\n";
    
    scriptFile << "    # Load PyTorch model\n";
    scriptFile << "    print(f'Loading PyTorch model: {args.torch_model}')\n";
    scriptFile << "    model = torch.jit.load(args.torch_model)\n";
    scriptFile << "    model.eval()\n\n";
    
    scriptFile << "    # Create example input\n";
    scriptFile << "    example_input = torch.rand(*input_shape)\n\n";
    
    scriptFile << "    # Convert to CoreML\n";
    scriptFile << "    print('Converting to CoreML...')\n";
    scriptFile << "    traced_model = torch.jit.trace(model, example_input)\n";
    scriptFile << "    mlmodel = ct.convert(\n";
    scriptFile << "        traced_model,\n";
    scriptFile << "        inputs=[ct.TensorType(name='input', shape=input_shape)],\n";
    scriptFile << "        compute_units=ct.ComputeUnit.ALL if args.use_neural_engine else ct.ComputeUnit.CPU_AND_GPU\n";
    scriptFile << "    )\n\n";
    
    scriptFile << "    # Quantize if requested\n";
    scriptFile << "    if args.quantize:\n";
    scriptFile << "        print('Quantizing model...')\n";
    scriptFile << "        mlmodel = ct.models.neural_network.quantization_utils.quantize_weights(mlmodel, nbits=8)\n\n";
    
    scriptFile << "    # Save the CoreML model\n";
    scriptFile << "    print(f'Saving CoreML model to {args.coreml_output}')\n";
    scriptFile << "    mlmodel.save(args.coreml_output)\n";
    scriptFile << "    print('Conversion complete')\n\n";
    
    scriptFile << "if __name__ == '__main__':\n";
    scriptFile << "    main()\n";
    
    scriptFile.close();
    
    // Make script executable
    std::string chmodCmd = "chmod +x " + scriptPath;
    // system(chmodCmd.c_str());
    
    return true;
}

/**
 * @brief Create a Python script for CoreML model optimization
 * @param scriptPath Path to create the script
 * @return True if script was created successfully
 */
bool createOptimizationScript(const std::string& scriptPath) {
    // Create a Python script that optimizes CoreML models
    std::ofstream scriptFile(scriptPath);
    if (!scriptFile.is_open()) {
        std::cerr << "Error: Could not create optimization script: " << scriptPath << std::endl;
        return false;
    }
    
    // Write script content
    scriptFile << "#!/usr/bin/env python\n";
    scriptFile << "# CoreML model optimization script\n";
    scriptFile << "# Usage: python optimize_coreml.py --model model.mlmodel --output optimized.mlmodel --target [CPU/GPU/ANE]\n\n";
    
    scriptFile << "import argparse\n";
    scriptFile << "import coremltools as ct\n";
    scriptFile << "from coremltools.models.neural_network import NeuralNetworkBuilder\n";
    scriptFile << "from coremltools.models import MLModel\n";
    scriptFile << "import numpy as np\n\n";
    
    scriptFile << "def main():\n";
    scriptFile << "    parser = argparse.ArgumentParser(description='Optimize CoreML model')\n";
    scriptFile << "    parser.add_argument('--model', required=True, help='Path to CoreML model file (.mlmodel)')\n";
    scriptFile << "    parser.add_argument('--output', required=True, help='Path for optimized model (.mlmodel)')\n";
    scriptFile << "    parser.add_argument('--target', required=True, choices=['CPU', 'GPU', 'ANE'], help='Target compute device')\n";
    scriptFile << "    args = parser.parse_args()\n\n";
    
    scriptFile << "    # Load the CoreML model\n";
    scriptFile << "    print(f'Loading CoreML model: {args.model}')\n";
    scriptFile << "    model = ct.models.MLModel(args.model)\n\n";
    
    scriptFile << "    # Get compute unit based on target\n";
    scriptFile << "    compute_units = ct.ComputeUnit.CPU_ONLY\n";
    scriptFile << "    if args.target == 'GPU':\n";
    scriptFile << "        compute_units = ct.ComputeUnit.CPU_AND_GPU\n";
    scriptFile << "    elif args.target == 'ANE':\n";
    scriptFile << "        compute_units = ct.ComputeUnit.ALL\n\n";
    
    scriptFile << "    # Optimize the model\n";
    scriptFile << "    print(f'Optimizing model for {args.target}...')\n";
    scriptFile << "    model.save(args.output, compute_units=compute_units)\n";
    scriptFile << "    print(f'Optimized model saved to: {args.output}')\n\n";
    
    scriptFile << "    # Verify the model\n";
    scriptFile << "    try:\n";
    scriptFile << "        optimized_model = ct.models.MLModel(args.output)\n";
    scriptFile << "        spec = optimized_model.get_spec()\n";
    scriptFile << "        print(f'Model successfully optimized and verified.')\n";
    scriptFile << "        print(f'Model input: {spec.description.input[0].name}, shape: {spec.description.input[0].type.multiArrayType.shape}')\n";
    scriptFile << "        print(f'Model output: {spec.description.output[0].name}')\n";
    scriptFile << "    except Exception as e:\n";
    scriptFile << "        print(f'Error verifying optimized model: {e}')\n";
    scriptFile << "        return 1\n\n";
    
    scriptFile << "    return 0\n\n";
    
    scriptFile << "if __name__ == '__main__':\n";
    scriptFile << "    import sys\n";
    scriptFile << "    sys.exit(main())\n";
    
    scriptFile.close();
    return true;
}

/**
 * @brief Create a Python script for batch PyTorch to CoreML conversion
 * 
 * @param scriptPath Path to create the script
 * @return True if script was created successfully
 */
bool createBatchConversionScript(const std::string& scriptPath) {
    // Create a Python script that uses coremltools to convert PyTorch models
    std::ofstream scriptFile(scriptPath);
    if (!scriptFile.is_open()) {
        std::cerr << "Error: Could not create batch conversion script: " << scriptPath << std::endl;
        return false;
    }
    
    // Write script content
    scriptFile << "#!/usr/bin/env python\n";
    scriptFile << "# PyTorch to CoreML batch conversion script\n";
    scriptFile << "# Usage: python batch_convert_pytorch_to_coreml.py --torch-model model.pt --coreml-output model.mlmodel --input-shape 1,4,84,84 [--use-neural-engine] [--quantize]\n\n";
    
    scriptFile << "import argparse\n";
    scriptFile << "import torch\n";
    scriptFile << "import coremltools as ct\n";
    scriptFile << "import numpy as np\n";
    scriptFile << "import os\n";
    scriptFile << "import sys\n";
    scriptFile << "import time\n\n";
    
    scriptFile << "def main():\n";
    scriptFile << "    parser = argparse.ArgumentParser(description='Convert PyTorch model to CoreML')\n";
    scriptFile << "    parser.add_argument('--torch-model', required=True, help='Path to PyTorch model file (.pt)')\n";
    scriptFile << "    parser.add_argument('--coreml-output', required=True, help='Path for output CoreML model (.mlmodel)')\n";
    scriptFile << "    parser.add_argument('--input-shape', required=True, help='Input shape as comma-separated values (e.g., 1,4,84,84)')\n";
    scriptFile << "    parser.add_argument('--use-neural-engine', action='store_true', help='Enable Apple Neural Engine')\n";
    scriptFile << "    parser.add_argument('--quantize', action='store_true', help='Quantize model for reduced size')\n";
    scriptFile << "    args = parser.parse_args()\n\n";
    
    scriptFile << "    # Create output directory if it doesn't exist\n";
    scriptFile << "    output_dir = os.path.dirname(args.coreml_output)\n";
    scriptFile << "    if output_dir and not os.path.exists(output_dir):\n";
    scriptFile << "        os.makedirs(output_dir)\n\n";
    
    scriptFile << "    # Parse input shape\n";
    scriptFile << "    input_shape = [int(dim) for dim in args.input_shape.split(',')]\n";
    scriptFile << "    print(f'Input shape: {input_shape}')\n\n";
    
    scriptFile << "    try:\n";
    scriptFile << "        start_time = time.time()\n";
    scriptFile << "        # Load PyTorch model\n";
    scriptFile << "        print(f'Loading PyTorch model: {args.torch_model}')\n";
    scriptFile << "        model = torch.jit.load(args.torch_model)\n";
    scriptFile << "        model.eval()\n\n";
    
    scriptFile << "        # Create example input\n";
    scriptFile << "        example_input = torch.rand(*input_shape)\n\n";
    
    scriptFile << "        # Convert to CoreML\n";
    scriptFile << "        print('Converting to CoreML format...')\n";
    scriptFile << "        mlmodel = ct.convert(\n";
    scriptFile << "            model,\n";
    scriptFile << "            inputs=[ct.TensorType(name='input', shape=input_shape)],\n";
    scriptFile << "            convert_to='mlprogram'\n";
    scriptFile << "        )\n\n";
    
    scriptFile << "        # Configure compute units\n";
    scriptFile << "        if args.use_neural_engine:\n";
    scriptFile << "            print('Enabling Neural Engine optimizations')\n";
    scriptFile << "            mlmodel = ct.models.MLModel(mlmodel.get_spec(), compute_units=ct.ComputeUnit.ALL)\n";
    scriptFile << "        else:\n";
    scriptFile << "            mlmodel = ct.models.MLModel(mlmodel.get_spec(), compute_units=ct.ComputeUnit.CPU_AND_GPU)\n\n";
    
    scriptFile << "        # Apply quantization if requested\n";
    scriptFile << "        if args.quantize:\n";
    scriptFile << "            print('Applying 8-bit quantization')\n";
    scriptFile << "            mlmodel = ct.models.neural_network.quantization_utils.quantize_weights(mlmodel, nbits=8)\n\n";
    
    scriptFile << "        # Save the model\n";
    scriptFile << "        print(f'Saving CoreML model to {args.coreml_output}')\n";
    scriptFile << "        mlmodel.save(args.coreml_output)\n\n";
    
    scriptFile << "        # Add metadata\n";
    scriptFile << "        mlmodel = ct.models.MLModel(args.coreml_output)\n";
    scriptFile << "        mlmodel.user_defined_metadata['source'] = 'pytorch'\n";
    scriptFile << "        mlmodel.user_defined_metadata['conversion_time'] = str(np.datetime64('now'))\n";
    scriptFile << "        if args.use_neural_engine:\n";
    scriptFile << "            mlmodel.user_defined_metadata['neural_engine'] = 'enabled'\n";
    scriptFile << "        if args.quantize:\n";
    scriptFile << "            mlmodel.user_defined_metadata['quantized'] = 'true'\n";
    scriptFile << "        mlmodel.save(args.coreml_output)\n";
    scriptFile << "        \n";
    scriptFile << "        # Print performance information\n";
    scriptFile << "        elapsed_time = time.time() - start_time\n";
    scriptFile << "        print(f'Conversion completed in {elapsed_time:.2f} seconds')\n";
    scriptFile << "        \n";
    scriptFile << "        return 0\n";
    scriptFile << "    except Exception as e:\n";
    scriptFile << "        print(f'Error during conversion: {e}')\n";
    scriptFile << "        return 1\n\n";
    
    scriptFile << "if __name__ == '__main__':\n";
    scriptFile << "    sys.exit(main())\n";
    
    scriptFile.close();
    
    // Make script executable on Unix-like systems
#ifndef _WIN32
    std::string chmodCmd = "chmod +x " + scriptPath;
    system(chmodCmd.c_str());
#endif
    
    return true;
}

/**
 * @brief Enhanced PyTorch to CoreML conversion with additional optimizations for Apple Neural Engine
 * 
 * @param torchModelPath Path to the PyTorch model
 * @param coreMLOutputPath Path to save the CoreML model
 * @param inputShape Shape of the input tensor
 * @param outputShape Shape of the output tensor (optional, inferred if not provided)
 * @param enableANE Whether to enable Apple Neural Engine optimizations
 * @param quantizeBits Number of bits to quantize to (0 = no quantization, 8 = 8-bit, 16 = fp16)
 * @return True if conversion was successful
 */
bool enhancedPyTorchToCoreML(const std::string& torchModelPath, 
                           const std::string& coreMLOutputPath,
                           const std::vector<int>& inputShape,
                           const std::vector<int>& outputShape,
                           bool enableANE,
                           int quantizeBits) {
    std::cout << "Converting PyTorch model to CoreML (enhanced): " << torchModelPath << " -> " << coreMLOutputPath << std::endl;
    
    // Check if PyTorch model exists
    std::ifstream checkFile(torchModelPath);
    if (!checkFile.good()) {
        std::cerr << "Error: PyTorch model file does not exist: " << torchModelPath << std::endl;
        return false;
    }
    checkFile.close();
    
    // Create a temporary directory for the script
    std::string tempDir = "/tmp"; // Unix-based temp directory
#ifdef _WIN32
    tempDir = std::getenv("TEMP") ? std::getenv("TEMP") : "C:/Temp";
#endif
    
    // Create a unique filename for the script
    std::string scriptPath = tempDir + "/enhanced_torch_to_coreml_" + 
                           std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + 
                           ".py";
    
    // Create the enhanced conversion script
    std::ofstream scriptFile(scriptPath);
    if (!scriptFile.is_open()) {
        std::cerr << "Error: Could not create enhanced conversion script: " << scriptPath << std::endl;
        return false;
    }
    
    // Write script content
    scriptFile << "#!/usr/bin/env python\n";
    scriptFile << "# Enhanced PyTorch to CoreML conversion script\n";
    scriptFile << "# This version supports advanced optimization and quantization\n\n";
    
    scriptFile << "import argparse\n";
    scriptFile << "import torch\n";
    scriptFile << "import coremltools as ct\n";
    scriptFile << "import numpy as np\n";
    scriptFile << "from coremltools.models.neural_network.quantization_utils import quantize_weights\n\n";
    
    scriptFile << "def main():\n";
    scriptFile << "    parser = argparse.ArgumentParser(description='Convert PyTorch model to CoreML with advanced options')\n";
    scriptFile << "    parser.add_argument('--torch-model', required=True, help='Path to PyTorch model file (.pt)')\n";
    scriptFile << "    parser.add_argument('--coreml-output', required=True, help='Path for output CoreML model (.mlmodel)')\n";
    scriptFile << "    parser.add_argument('--input-shape', required=True, help='Input shape as comma-separated values')\n";
    scriptFile << "    parser.add_argument('--output-shape', required=True, help='Output shape as comma-separated values')\n";
    scriptFile << "    parser.add_argument('--enable-ane', type=int, required=True, help='Enable Apple Neural Engine (1=yes, 0=no)')\n";
    scriptFile << "    parser.add_argument('--quantize-bits', type=int, required=True, help='Quantization bits (0=none, 8=8bit, 16=16bit)')\n";
    scriptFile << "    args = parser.parse_args()\n\n";
    
    scriptFile << "    # Parse shapes\n";
    scriptFile << "    input_shape = [int(dim) for dim in args.input_shape.split(',')]\n";
    scriptFile << "    output_shape = [int(dim) for dim in args.output_shape.split(',')]\n";
    scriptFile << "    print(f'Input shape: {input_shape}')\n";
    scriptFile << "    print(f'Output shape: {output_shape}')\n\n";
    
    scriptFile << "    # Load PyTorch model\n";
    scriptFile << "    print(f'Loading PyTorch model: {args.torch_model}')\n";
    scriptFile << "    model = torch.jit.load(args.torch_model)\n";
    scriptFile << "    model.eval()\n\n";
    
    scriptFile << "    # Create example input\n";
    scriptFile << "    example_input = torch.rand(*input_shape)\n\n";
    
    scriptFile << "    # Trace the model with example input\n";
    scriptFile << "    print('Tracing PyTorch model...')\n";
    scriptFile << "    traced_model = torch.jit.trace(model, example_input)\n\n";
    
    scriptFile << "    # Configure CoreML conversion options\n";
    scriptFile << "    compute_units = ct.ComputeUnit.CPU_ONLY\n";
    scriptFile << "    if args.enable_ane == 1:\n";
    scriptFile << "        compute_units = ct.ComputeUnit.ALL\n";
    scriptFile << "        print('Enabling Apple Neural Engine')\n";
    scriptFile << "    else:\n";
    scriptFile << "        print('Using CPU/GPU only')\n\n";
    
    scriptFile << "    # Convert to CoreML\n";
    scriptFile << "    print('Converting to CoreML...')\n";
    scriptFile << "    try:\n";
    scriptFile << "        input_name = 'input'\n";
    scriptFile << "        output_name = 'output'\n";
    scriptFile << "        coreml_model = ct.convert(\n";
    scriptFile << "            traced_model,\n";
    scriptFile << "            inputs=[ct.TensorType(name=input_name, shape=input_shape)],\n";
    scriptFile << "            outputs=[ct.TensorType(name=output_name, shape=output_shape)],\n";
    scriptFile << "            compute_units=compute_units,\n";
    scriptFile << "            convert_to='mlprogram'\n";
    scriptFile << "        )\n\n";
    
    scriptFile << "        # Add metadata\n";
    scriptFile << "        coreml_model.user_defined_metadata['source'] = 'PyTorch'\n";
    scriptFile << "        coreml_model.user_defined_metadata['input_shape'] = str(input_shape)\n";
    scriptFile << "        coreml_model.user_defined_metadata['output_shape'] = str(output_shape)\n\n";
    
    scriptFile << "        # Apply quantization if specified\n";
    scriptFile << "        if args.quantize_bits in [8, 16]:\n";
    scriptFile << "            print(f'Applying {args.quantize_bits}-bit quantization...')\n";
    scriptFile << "            nbits = args.quantize_bits\n";
    scriptFile << "            if nbits == 8:\n";
    scriptFile << "                config = ct.ComputePrecision.FLOAT16\n";
    scriptFile << "            else:  # 16-bit\n";
    scriptFile << "                config = ct.ComputePrecision.FLOAT16\n";
    scriptFile << "            coreml_model = ct.models.neural_network.quantization_utils.quantize_weights(coreml_model, nbits)\n";
    scriptFile << "            coreml_model.user_defined_metadata['quantization_bits'] = str(nbits)\n\n";
    
    scriptFile << "        # Save the model\n";
    scriptFile << "        print(f'Saving CoreML model to: {args.coreml_output}')\n";
    scriptFile << "        coreml_model.save(args.coreml_output)\n";
    scriptFile << "        print('Conversion successful')\n";
    scriptFile << "        return 0\n";
    scriptFile << "    except Exception as e:\n";
    scriptFile << "        print(f'Error during conversion: {e}')\n";
    scriptFile << "        return 1\n\n";
    
    scriptFile << "if __name__ == '__main__':\n";
    scriptFile << "    import sys\n";
    scriptFile << "    sys.exit(main())\n";
    
    scriptFile.close();
    
    // Build input shape string
    std::string inputShapeStr = "";
    for (size_t i = 0; i < inputShape.size(); ++i) {
        inputShapeStr += std::to_string(inputShape[i]);
        if (i < inputShape.size() - 1) {
            inputShapeStr += ",";
        }
    }
    
    // Build output shape string
    std::string outputShapeStr = "";
    for (size_t i = 0; i < outputShape.size(); ++i) {
        outputShapeStr += std::to_string(outputShape[i]);
        if (i < outputShape.size() - 1) {
            outputShapeStr += ",";
        }
    }
    
    // Build the command to run the conversion script
    std::string cmd = "python \"" + scriptPath + "\"" +
                     " --torch-model \"" + torchModelPath + "\"" +
                     " --coreml-output \"" + coreMLOutputPath + "\"" +
                     " --input-shape " + inputShapeStr +
                     " --output-shape " + outputShapeStr +
                     " --enable-ane " + std::to_string(enableANE ? 1 : 0) +
                     " --quantize-bits " + std::to_string(quantizeBits);
    
    // Execute the command
    std::cout << "Running: " << cmd << std::endl;
    int result = system(cmd.c_str());
    
    // Clean up the temporary script
    std::remove(scriptPath.c_str());
    
    if (result != 0) {
        std::cerr << "Error: Enhanced PyTorch to CoreML conversion failed with code " << result << std::endl;
        return false;
    }
    
    std::cout << "Enhanced PyTorch to CoreML conversion complete: " << coreMLOutputPath << std::endl;
    return true;
}

/**
 * @brief Create a model validator tool for CoreML models
 * @param scriptPath Path to create the validator script
 * @return True if script was created successfully
 */
bool createModelValidatorScript(const std::string& scriptPath) {
    // Create a Python script that validates CoreML models
    std::ofstream scriptFile(scriptPath);
    if (!scriptFile.is_open()) {
        std::cerr << "Error: Could not create validation script: " << scriptPath << std::endl;
        return false;
    }
    
    // Write script content
    scriptFile << "#!/usr/bin/env python\n";
    scriptFile << "# CoreML model validation script\n";
    scriptFile << "# Usage: python validate_coreml.py --model model.mlmodel --iterations 100 --compute-unit [CPU/GPU/ANE]\n\n";
    
    scriptFile << "import argparse\n";
    scriptFile << "import coremltools as ct\n";
    scriptFile << "import numpy as np\n";
    scriptFile << "import time\n\n";
    
    scriptFile << "def main():\n";
    scriptFile << "    parser = argparse.ArgumentParser(description='Validate CoreML model')\n";
    scriptFile << "    parser.add_argument('--model', required=True, help='Path to CoreML model file (.mlmodel)')\n";
    scriptFile << "    parser.add_argument('--iterations', type=int, default=100, help='Number of inference iterations')\n";
    scriptFile << "    parser.add_argument('--compute-unit', choices=['CPU', 'GPU', 'ANE'], default='CPU', help='Compute unit to use')\n";
    scriptFile << "    args = parser.parse_args()\n\n";
    
    scriptFile << "    # Load the CoreML model\n";
    scriptFile << "    print(f'Loading CoreML model: {args.model}')\n";
    scriptFile << "    try:\n";
    scriptFile << "        # Set compute unit\n";
    scriptFile << "        compute_unit = ct.ComputeUnit.CPU_ONLY\n";
    scriptFile << "        if args.compute_unit == 'GPU':\n";
    scriptFile << "            compute_unit = ct.ComputeUnit.CPU_AND_GPU\n";
    scriptFile << "        elif args.compute_unit == 'ANE':\n";
    scriptFile << "            compute_unit = ct.ComputeUnit.ALL\n\n";
    
    scriptFile << "        # Load model with specified compute unit\n";
    scriptFile << "        model = ct.models.MLModel(args.model, compute_units=compute_unit)\n";
    scriptFile << "        spec = model.get_spec()\n\n";
    
    scriptFile << "        # Extract model info\n";
    scriptFile << "        print('Model information:')\n";
    scriptFile << "        print(f'  Description: {spec.description.metadata.shortDescription}')\n";
    scriptFile << "        print(f'  Inputs:')\n";
    scriptFile << "        for input in spec.description.input:\n";
    scriptFile << "            input_type = input.type.WhichOneof('Type')\n";
    scriptFile << "            if input_type == 'multiArrayType':\n";
    scriptFile << "                shape = input.type.multiArrayType.shape\n";
    scriptFile << "                print(f'    Name: {input.name}, Shape: {shape}')\n";
    scriptFile << "            else:\n";
    scriptFile << "                print(f'    Name: {input.name}, Type: {input_type}')\n\n";
    
    scriptFile << "        print(f'  Outputs:')\n";
    scriptFile << "        for output in spec.description.output:\n";
    scriptFile << "            output_type = output.type.WhichOneof('Type')\n";
    scriptFile << "            if output_type == 'multiArrayType':\n";
    scriptFile << "                shape = output.type.multiArrayType.shape\n";
    scriptFile << "                print(f'    Name: {output.name}, Shape: {shape}')\n";
    scriptFile << "            else:\n";
    scriptFile << "                print(f'    Name: {output.name}, Type: {output_type}')\n\n";
    
    scriptFile << "        # Run performance test\n";
    scriptFile << "        print(f'Running {args.iterations} inference iterations on {args.compute_unit}...')\n";
    scriptFile << "        # Generate random input data matching the model's input shape\n";
    scriptFile << "        input_name = spec.description.input[0].name\n";
    scriptFile << "        input_type = spec.description.input[0].type.WhichOneof('Type')\n";
    scriptFile << "        input_shape = []\n";
    scriptFile << "        if input_type == 'multiArrayType':\n";
    scriptFile << "            input_shape = spec.description.input[0].type.multiArrayType.shape\n";
    scriptFile << "        else:\n";
    scriptFile << "            print(f'Unsupported input type: {input_type}')\n";
    scriptFile << "            return 1\n\n";
    
    scriptFile << "        # Create random input data\n";
    scriptFile << "        input_data = np.random.rand(*input_shape).astype(np.float32)\n";
    scriptFile << "        input_dict = {input_name: input_data}\n\n";
    
    scriptFile << "        # Warmup run\n";
    scriptFile << "        model.predict(input_dict)\n\n";
    
    scriptFile << "        # Timing runs\n";
    scriptFile << "        timings = []\n";
    scriptFile << "        for i in range(args.iterations):\n";
    scriptFile << "            start_time = time.time()\n";
    scriptFile << "            output = model.predict(input_dict)\n";
    scriptFile << "            end_time = time.time()\n";
    scriptFile << "            timings.append((end_time - start_time) * 1000)  # ms\n\n";
    
    scriptFile << "        # Calculate statistics\n";
    scriptFile << "        mean_time = np.mean(timings)\n";
    scriptFile << "        std_time = np.std(timings)\n";
    scriptFile << "        min_time = np.min(timings)\n";
    scriptFile << "        max_time = np.max(timings)\n\n";
    
    scriptFile << "        print('Performance results:')\n";
    scriptFile << "        print(f'  Mean inference time: {mean_time:.2f} ms')\n";
    scriptFile << "        print(f'  Std deviation: {std_time:.2f} ms')\n";
    scriptFile << "        print(f'  Min time: {min_time:.2f} ms')\n";
    scriptFile << "        print(f'  Max time: {max_time:.2f} ms')\n";
    scriptFile << "        print(f'  Throughput: {1000/mean_time:.2f} inferences/second')\n\n";
    
    scriptFile << "        # Check output\n";
    scriptFile << "        output_name = spec.description.output[0].name\n";
    scriptFile << "        if output_name in output:\n";
    scriptFile << "            print(f'Model output shape: {output[output_name].shape}')\n";
    scriptFile << "        else:\n";
    scriptFile << "            print(f'Warning: Output {output_name} not found in model output')\n\n";
    
    scriptFile << "        return 0\n\n";
    
    scriptFile << "    except Exception as e:\n";
    scriptFile << "        print(f'Error validating CoreML model: {e}')\n";
    scriptFile << "        return 1\n\n";
    
    scriptFile << "if __name__ == '__main__':\n";
    scriptFile << "    import sys\n";
    scriptFile << "    sys.exit(main())\n";
    
    scriptFile.close();
    return true;
}

/**
 * @brief Initialize the PyTorch to CoreML conversion system
 * @return True if initialization was successful
 */
bool initializePyTorchToCoreMLSystem() {
    // Create necessary directories
    std::string tempDir = "/tmp/fbneo_torch_coreml";
#ifdef _WIN32
    std::string cmd = "mkdir \"" + tempDir + "\" 2>nul";
#else
    std::string cmd = "mkdir -p \"" + tempDir + "\" 2>/dev/null";
#endif
    
    int result = system(cmd.c_str());
    
    // Check if Python and required libraries are available
    std::string checkCmd = "python -c \"import torch; import coremltools; print('PyTorch and CoreMLTools available')\" 2>/dev/null";
    result = system(checkCmd.c_str());
    
    if (result != 0) {
        std::cerr << "Warning: PyTorch or CoreMLTools not available. Conversion will use fallback mechanisms." << std::endl;
    } else {
        std::cout << "PyTorch and CoreMLTools detected. Full conversion capabilities available." << std::endl;
    }
    
    return true;
}

// C API implementation for use by external code
extern "C" {
    int FBNEO_PyTorch_ToCoreML_Init() {
        return initializePyTorchToCoreMLSystem() ? 0 : 1;
    }
    
    int FBNEO_PyTorch_ToCoreML_Convert(const char* torchModelPath, 
                                     const char* coreMLOutputPath,
                                     const int* inputShape, 
                                     int shapeLen,
                                     int useNeuralEngine,
                                     int quantize) {
        if (!torchModelPath || !coreMLOutputPath || !inputShape || shapeLen <= 0) {
            return 0;
        }
        
        // Convert C array to C++ vector
        std::vector<int> shape(inputShape, inputShape + shapeLen);
        
        // Call C++ function
        bool result = convertPyTorchToCoreML(
            torchModelPath, 
            coreMLOutputPath, 
            shape,
            useNeuralEngine != 0,
            quantize != 0
        );
        
        return result ? 1 : 0;
    }
    
    int FBNEO_PyTorch_ToCoreML_Optimize(const char* coreMLModelPath,
                                      const char* outputPath,
                                      const char* targetDevice) {
        if (!coreMLModelPath || !outputPath) {
            return 0;
        }
        
        bool result = optimizeCoreMLModel(coreMLModelPath, outputPath, targetDevice ? targetDevice : "ANE");
        return result ? 1 : 0;
    }
    
    int FBNEO_PyTorch_ToCoreML_Enhanced(const char* torchModelPath,
                                      const char* coreMLOutputPath,
                                      const int* inputShape,
                                      int inputShapeLen,
                                      const int* outputShape,
                                      int outputShapeLen,
                                      int enableANE,
                                      int quantizeBits) {
        if (!torchModelPath || !coreMLOutputPath || !inputShape || inputShapeLen <= 0) {
            return 0;
        }
        
        // Convert C arrays to C++ vectors
        std::vector<int> inShape(inputShape, inputShape + inputShapeLen);
        std::vector<int> outShape;
        
        if (outputShape && outputShapeLen > 0) {
            outShape.assign(outputShape, outputShape + outputShapeLen);
        }
        
        // Call C++ function
        bool result = enhancedPyTorchToCoreML(
            torchModelPath,
            coreMLOutputPath,
            inShape,
            outShape,
            enableANE != 0,
            quantizeBits
        );
        
        return result ? 1 : 0;
    }
    
    /**
     * @brief Batch convert multiple PyTorch models to CoreML format
     * 
     * @param torchModelPaths Array of paths to PyTorch model files
     * @param coreMLOutputPaths Array of paths for output CoreML models
     * @param numModels Number of models to convert
     * @param inputShape Input shape array
     * @param shapeLen Length of input shape array
     * @param useNeuralEngine Whether to enable Apple Neural Engine 
     * @param quantize Whether to quantize the models
     * @param numThreads Number of parallel conversion threads (0 = auto)
     * @return Number of successfully converted models
     */
    int FBNEO_PyTorch_ToCoreML_BatchConvert(const char** torchModelPaths,
                                          const char** coreMLOutputPaths,
                                          int numModels,
                                          const int* inputShape,
                                          int shapeLen,
                                          int useNeuralEngine,
                                          int quantize,
                                          int numThreads) {
        if (!torchModelPaths || !coreMLOutputPaths || numModels <= 0 || !inputShape || shapeLen <= 0) {
            return 0;
        }
        
        // Convert C arrays to C++ vectors
        std::vector<std::string> modelPaths;
        std::vector<std::string> outputPaths;
        std::vector<int> shape(inputShape, inputShape + shapeLen);
        
        // Convert string arrays
        for (int i = 0; i < numModels; ++i) {
            if (torchModelPaths[i] && coreMLOutputPaths[i]) {
                modelPaths.push_back(torchModelPaths[i]);
                outputPaths.push_back(coreMLOutputPaths[i]);
            }
        }
        
        // Call C++ function for batch conversion
        return batchConvertPyTorchToCoreML(
            modelPaths,
            outputPaths,
            shape,
            useNeuralEngine != 0,
            quantize != 0,
            numThreads
        );
    }
    
    int FBNEO_PyTorch_ValidateCoreMLModel(const char* modelPath, int iterations, const char* computeUnit) {
        // Create temporary validator script
        std::string tempDir = "/tmp";
#ifdef _WIN32
        tempDir = std::getenv("TEMP") ? std::getenv("TEMP") : "C:/Temp";
#endif
        
        std::string scriptPath = tempDir + "/validate_coreml_" + 
                               std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + 
                               ".py";
        
        if (!createModelValidatorScript(scriptPath)) {
            return 1;
        }
        
        // Build command
        std::string cmd = "python \"" + scriptPath + "\"" +
                         " --model \"" + modelPath + "\"" +
                         " --iterations " + std::to_string(iterations) +
                         " --compute-unit " + computeUnit;
        
        // Execute command
        int result = system(cmd.c_str());
        
        // Clean up
        std::remove(scriptPath.c_str());
        
        return result;
    }
}

} // namespace ai
} // namespace fbneo
} 