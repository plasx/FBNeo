#include "model_optimization.h"
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <chrono>
#include <cmath>

namespace fbneo {
namespace ai {

ModelOptimizer::ModelOptimizer() 
    : m_quantizationBits(0), 
      m_pruningThreshold(0.0f), 
      m_useNeuralEngine(true),
      m_compressionLevel(0) {
}

ModelOptimizer::~ModelOptimizer() {
}

bool ModelOptimizer::optimizeModel(const std::string& inputModelPath, 
                                  const std::string& outputModelPath,
                                  const OptimizationConfig& config) {
    // Validate paths
    if (inputModelPath.empty() || outputModelPath.empty()) {
        std::cerr << "Error: Invalid input or output paths for model optimization" << std::endl;
        return false;
    }
    
    // Check if the input file exists
    std::ifstream checkFile(inputModelPath);
    if (!checkFile.good()) {
        std::cerr << "Error: Input model file does not exist: " << inputModelPath << std::endl;
        return false;
    }
    checkFile.close();
    
    // Copy configuration
    m_quantizationBits = config.quantizationBits;
    m_pruningThreshold = config.pruningThreshold;
    m_useNeuralEngine = config.useNeuralEngine;
    m_compressionLevel = config.compressionLevel;
    
    // Create a temporary Python script for optimization
    std::string scriptPath = createOptimizationScript();
    if (scriptPath.empty()) {
        std::cerr << "Error: Failed to create optimization script" << std::endl;
        return false;
    }
    
    // Build command
    std::string cmd = "python \"" + scriptPath + "\"" +
                     " --input \"" + inputModelPath + "\"" +
                     " --output \"" + outputModelPath + "\"";
    
    // Add optimization parameters
    if (m_quantizationBits > 0) {
        cmd += " --quantize " + std::to_string(m_quantizationBits);
    }
    
    if (m_pruningThreshold > 0.0f) {
        cmd += " --prune " + std::to_string(m_pruningThreshold);
    }
    
    if (m_useNeuralEngine) {
        cmd += " --use-neural-engine";
    }
    
    if (m_compressionLevel > 0) {
        cmd += " --compress " + std::to_string(m_compressionLevel);
    }
    
    // Execute command
    std::cout << "Running model optimization: " << cmd << std::endl;
    int result = system(cmd.c_str());
    
    // Cleanup
    std::remove(scriptPath.c_str());
    
    if (result != 0) {
        std::cerr << "Error: Model optimization failed with code " << result << std::endl;
        return false;
    }
    
    std::cout << "Model optimization completed successfully" << std::endl;
    return true;
}

void ModelOptimizer::setQuantizationBits(int bits) {
    m_quantizationBits = bits;
}

void ModelOptimizer::setPruningThreshold(float threshold) {
    m_pruningThreshold = threshold;
}

void ModelOptimizer::setUseNeuralEngine(bool useNeuralEngine) {
    m_useNeuralEngine = useNeuralEngine;
}

void ModelOptimizer::setCompressionLevel(int level) {
    m_compressionLevel = level;
}

std::string ModelOptimizer::createOptimizationScript() {
    // Create a temporary directory for the script
    std::string tempDir = "/tmp"; // Unix-based temp directory
#ifdef _WIN32
    tempDir = std::getenv("TEMP") ? std::getenv("TEMP") : "C:/Temp";
#endif

    // Create a unique filename for the script
    std::string scriptPath = tempDir + "/model_optimizer_" + 
                           std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + 
                           ".py";
    
    // Create the script file
    std::ofstream scriptFile(scriptPath);
    if (!scriptFile.is_open()) {
        std::cerr << "Error: Could not create optimization script: " << scriptPath << std::endl;
        return "";
    }
    
    // Write script content
    scriptFile << "#!/usr/bin/env python3\n";
    scriptFile << "# FBNeo AI Model Optimizer\n\n";
    
    scriptFile << "import argparse\n";
    scriptFile << "import sys\n";
    scriptFile << "import os\n";
    scriptFile << "import numpy as np\n";
    scriptFile << "import coremltools as ct\n\n";
    
    scriptFile << "def main():\n";
    scriptFile << "    parser = argparse.ArgumentParser(description='Optimize CoreML models for FBNeo')\n";
    scriptFile << "    parser.add_argument('--input', required=True, help='Input model path')\n";
    scriptFile << "    parser.add_argument('--output', required=True, help='Output model path')\n";
    scriptFile << "    parser.add_argument('--quantize', type=int, default=0, help='Quantization bits (0=none, 8=8bit, 16=fp16)')\n";
    scriptFile << "    parser.add_argument('--prune', type=float, default=0.0, help='Pruning threshold (0.0=none)')\n";
    scriptFile << "    parser.add_argument('--use-neural-engine', action='store_true', help='Optimize for Neural Engine')\n";
    scriptFile << "    parser.add_argument('--compress', type=int, default=0, help='Compression level (0-9)')\n";
    scriptFile << "    args = parser.parse_args()\n\n";
    
    scriptFile << "    try:\n";
    scriptFile << "        print(f'Loading model from {args.input}')\n";
    scriptFile << "        model = ct.models.MLModel(args.input)\n";
    scriptFile << "        spec = model.get_spec()\n\n";
    
    scriptFile << "        # Apply optimizations\n";
    scriptFile << "        print('Applying optimizations...')\n\n";
    
    scriptFile << "        # Quantization\n";
    scriptFile << "        if args.quantize > 0:\n";
    scriptFile << "            print(f'Applying {args.quantize}-bit quantization')\n";
    scriptFile << "            if args.quantize == 8:\n";
    scriptFile << "                model = ct.models.neural_network.quantization_utils.quantize_weights(model, nbits=8)\n";
    scriptFile << "            elif args.quantize == 16:\n";
    scriptFile << "                model = ct.models.neural_network.quantization_utils.quantize_weights(model, dtype=np.float16)\n";
    scriptFile << "            else:\n";
    scriptFile << "                print(f'Warning: Unsupported quantization bits: {args.quantize}, using 8-bit')\n";
    scriptFile << "                model = ct.models.neural_network.quantization_utils.quantize_weights(model, nbits=8)\n";
    scriptFile << "        \n";
    scriptFile << "        # Weight pruning\n";
    scriptFile << "        if args.prune > 0.0:\n";
    scriptFile << "            print(f'Pruning weights with threshold {args.prune}')\n";
    scriptFile << "            # Implementation of weight pruning for CoreML models\n";
    scriptFile << "            try:\n";
    scriptFile << "                # Get model architecture and weights\n";
    scriptFile << "                nn_spec = spec.neuralNetwork\n";
    scriptFile << "                layers = nn_spec.layers\n";
    scriptFile << "                pruned_weight_count = 0\n";
    scriptFile << "                total_weight_count = 0\n\n";
    
    scriptFile << "                # Iterate through each layer to prune weights\n";
    scriptFile << "                for i, layer in enumerate(layers):\n";
    scriptFile << "                    if hasattr(layer, 'convolution'):\n";
    scriptFile << "                        # Handle convolution layers\n";
    scriptFile << "                        if hasattr(layer.convolution, 'weights') and len(layer.convolution.weights.floatValue) > 0:\n";
    scriptFile << "                            # Get weights as numpy array\n";
    scriptFile << "                            weights = np.array(layer.convolution.weights.floatValue)\n";
    scriptFile << "                            total_weight_count += len(weights)\n";
    scriptFile << "                            \n";
    scriptFile << "                            # Create mask for pruning (keep only weights with abs value > threshold)\n";
    scriptFile << "                            mask = np.abs(weights) > args.prune\n";
    scriptFile << "                            pruned_weight_count += np.sum(~mask)\n";
    scriptFile << "                            \n";
    scriptFile << "                            # Apply mask (set pruned weights to 0)\n";
    scriptFile << "                            weights[~mask] = 0.0\n";
    scriptFile << "                            \n";
    scriptFile << "                            # Update the weights in the model\n";
    scriptFile << "                            layer.convolution.weights.ClearField('floatValue')\n";
    scriptFile << "                            layer.convolution.weights.floatValue.extend(list(weights.flatten()))\n";
    scriptFile << "                            \n";
    scriptFile << "                    elif hasattr(layer, 'innerProduct'):\n";
    scriptFile << "                        # Handle fully connected layers\n";
    scriptFile << "                        if hasattr(layer.innerProduct, 'weights') and len(layer.innerProduct.weights.floatValue) > 0:\n";
    scriptFile << "                            # Get weights as numpy array\n";
    scriptFile << "                            weights = np.array(layer.innerProduct.weights.floatValue)\n";
    scriptFile << "                            total_weight_count += len(weights)\n";
    scriptFile << "                            \n";
    scriptFile << "                            # Create mask for pruning (keep only weights with abs value > threshold)\n";
    scriptFile << "                            mask = np.abs(weights) > args.prune\n";
    scriptFile << "                            pruned_weight_count += np.sum(~mask)\n";
    scriptFile << "                            \n";
    scriptFile << "                            # Apply mask (set pruned weights to 0)\n";
    scriptFile << "                            weights[~mask] = 0.0\n";
    scriptFile << "                            \n";
    scriptFile << "                            # Update the weights in the model\n";
    scriptFile << "                            layer.innerProduct.weights.ClearField('floatValue')\n";
    scriptFile << "                            layer.innerProduct.weights.floatValue.extend(list(weights.flatten()))\n";
    scriptFile << "                    \n";
    scriptFile << "                    # Handle other layer types that might have weights\n";
    scriptFile << "                    elif hasattr(layer, 'batchnorm'):\n";
    scriptFile << "                        if hasattr(layer.batchnorm, 'gamma') and len(layer.batchnorm.gamma.floatValue) > 0:\n";
    scriptFile << "                            weights = np.array(layer.batchnorm.gamma.floatValue)\n";
    scriptFile << "                            total_weight_count += len(weights)\n";
    scriptFile << "                            mask = np.abs(weights) > args.prune\n";
    scriptFile << "                            pruned_weight_count += np.sum(~mask)\n";
    scriptFile << "                            weights[~mask] = 0.0\n";
    scriptFile << "                            layer.batchnorm.gamma.ClearField('floatValue')\n";
    scriptFile << "                            layer.batchnorm.gamma.floatValue.extend(list(weights.flatten()))\n";
    
    scriptFile << "                # Calculate and print pruning statistics\n";
    scriptFile << "                if total_weight_count > 0:\n";
    scriptFile << "                    pruning_percentage = (pruned_weight_count / total_weight_count) * 100.0\n";
    scriptFile << "                    print(f'Pruned {pruned_weight_count} of {total_weight_count} weights ({pruning_percentage:.2f}%)')\n";
    scriptFile << "                else:\n";
    scriptFile << "                    print('No weights found to prune')\n";
    scriptFile << "                    \n";
    scriptFile << "                # Create a new model from the modified spec\n";
    scriptFile << "                model = ct.models.MLModel(spec)\n";
    scriptFile << "                \n";
    scriptFile << "            except Exception as e:\n";
    scriptFile << "                print(f'Error during weight pruning: {e}')\n";
    scriptFile << "                print('Continuing with unpruned model')\n\n";
    
    scriptFile << "        # Neural Engine optimizations\n";
    scriptFile << "        if args.use_neural_engine:\n";
    scriptFile << "            print('Optimizing for Neural Engine')\n";
    scriptFile << "            # Set compute units to use Neural Engine\n";
    scriptFile << "            ct_config = ct.ComputeConfig()\n";
    scriptFile << "            ct_config.compute_units = ct.ComputeUnit.ALL\n\n";
    
    scriptFile << "            # Enable memory reuse optimization\n";
    scriptFile << "            if hasattr(spec, 'neuralNetwork'):\n";
    scriptFile << "                spec.neuralNetwork.preferences.memoryOptimization = ct.neural_network.NeuralNetworkPreferences.MemoryOptimizationStatus.MEMORY_OPTIMIZATION_STATUS_OPTIMIZE_FOR_EXECUTION\n";
    scriptFile << "            \n";
    scriptFile << "            # Create optimized model\n";
    scriptFile << "            model = ct.models.MLModel(spec, compute_units=ct.ComputeUnit.ALL)\n\n";
    
    scriptFile << "        # Model compression\n";
    scriptFile << "        if args.compress > 0:\n";
    scriptFile << "            print(f'Applying compression level {args.compress}')\n";
    scriptFile << "            # Note: Direct model compression is not supported in coremltools\n";
    scriptFile << "            # We could implement model compression at the file level\n";
    scriptFile << "            print('WARNING: Model compression is not fully implemented')\n\n";

    // Replace the stub implementation with actual model compression logic
    scriptFile << "        # Model compression\n";
    scriptFile << "        if args.compress > 0:\n";
    scriptFile << "            print(f'Applying compression level {args.compress}')\n";
    scriptFile << "            # Use weight quantization as one form of compression\n";
    scriptFile << "            if args.compress >= 1 and args.compress <= 3:\n";
    scriptFile << "                # Light compression: use FP16 precision\n";
    scriptFile << "                print('Applying FP16 quantization as part of compression')\n";
    scriptFile << "                model = ct.models.neural_network.quantization_utils.quantize_weights(model, dtype=np.float16)\n";
    scriptFile << "            elif args.compress >= 4 and args.compress <= 6:\n";
    scriptFile << "                # Medium compression: use 8-bit quantization\n";
    scriptFile << "                print('Applying 8-bit quantization as part of compression')\n";
    scriptFile << "                model = ct.models.neural_network.quantization_utils.quantize_weights(model, nbits=8)\n";
    scriptFile << "            elif args.compress >= 7 and args.compress <= 9:\n";
    scriptFile << "                # Heavy compression: use 8-bit quantization and weight pruning\n";
    scriptFile << "                print('Applying 8-bit quantization and weight pruning as part of compression')\n";
    scriptFile << "                model = ct.models.neural_network.quantization_utils.quantize_weights(model, nbits=8)\n";
    scriptFile << "                \n";
    scriptFile << "                # Also apply aggressive pruning if not already applied\n";
    scriptFile << "                if args.prune <= 0.0:\n";
    scriptFile << "                    print('Adding aggressive weight pruning (threshold=0.01)')\n";
    scriptFile << "                    # Get the spec again after quantization\n";
    scriptFile << "                    spec = model.get_spec()\n";
    scriptFile << "                    try:\n";
    scriptFile << "                        # Get model architecture and weights\n";
    scriptFile << "                        nn_spec = spec.neuralNetwork\n";
    scriptFile << "                        layers = nn_spec.layers\n";
    scriptFile << "                        pruned_weight_count = 0\n";
    scriptFile << "                        total_weight_count = 0\n\n";
    scriptFile << "                        \n";
    scriptFile << "                        # Set pruning threshold based on compression level\n";
    scriptFile << "                        prune_threshold = 0.01\n";
    scriptFile << "                        if args.compress == 8:\n";
    scriptFile << "                            prune_threshold = 0.02\n";
    scriptFile << "                        elif args.compress == 9:\n";
    scriptFile << "                            prune_threshold = 0.05\n";
    scriptFile << "                        \n";
    scriptFile << "                        # Iterate through each layer to prune weights\n";
    scriptFile << "                        for i, layer in enumerate(layers):\n";
    scriptFile << "                            if hasattr(layer, 'convolution'):\n";
    scriptFile << "                                # Handle convolution layers\n";
    scriptFile << "                                if hasattr(layer.convolution, 'weights') and len(layer.convolution.weights.floatValue) > 0:\n";
    scriptFile << "                                    # Get weights as numpy array\n";
    scriptFile << "                                    weights = np.array(layer.convolution.weights.floatValue)\n";
    scriptFile << "                                    total_weight_count += len(weights)\n";
    scriptFile << "                                    \n";
    scriptFile << "                                    # Create mask for pruning\n";
    scriptFile << "                                    mask = np.abs(weights) > prune_threshold\n";
    scriptFile << "                                    pruned_weight_count += np.sum(~mask)\n";
    scriptFile << "                                    \n";
    scriptFile << "                                    # Apply mask (set pruned weights to 0)\n";
    scriptFile << "                                    weights[~mask] = 0.0\n";
    scriptFile << "                                    \n";
    scriptFile << "                                    # Update the weights in the model\n";
    scriptFile << "                                    layer.convolution.weights.ClearField('floatValue')\n";
    scriptFile << "                                    layer.convolution.weights.floatValue.extend(list(weights.flatten()))\n";
    scriptFile << "                            \n";
    scriptFile << "                            elif hasattr(layer, 'innerProduct'):\n";
    scriptFile << "                                # Handle fully connected layers\n";
    scriptFile << "                                if hasattr(layer.innerProduct, 'weights') and len(layer.innerProduct.weights.floatValue) > 0:\n";
    scriptFile << "                                    weights = np.array(layer.innerProduct.weights.floatValue)\n";
    scriptFile << "                                    total_weight_count += len(weights)\n";
    scriptFile << "                                    mask = np.abs(weights) > prune_threshold\n";
    scriptFile << "                                    pruned_weight_count += np.sum(~mask)\n";
    scriptFile << "                                    weights[~mask] = 0.0\n";
    scriptFile << "                                    layer.innerProduct.weights.ClearField('floatValue')\n";
    scriptFile << "                                    layer.innerProduct.weights.floatValue.extend(list(weights.flatten()))\n";
    scriptFile << "                        \n";
    scriptFile << "                        # Calculate and print pruning statistics\n";
    scriptFile << "                        if total_weight_count > 0:\n";
    scriptFile << "                            pruning_percentage = (pruned_weight_count / total_weight_count) * 100.0\n";
    scriptFile << "                            print(f'Compression pruned {pruned_weight_count} of {total_weight_count} weights ({pruning_percentage:.2f}%)')\n";
    scriptFile << "                    \n";
    scriptFile << "                        # Create a new model from the modified spec\n";
    scriptFile << "                        model = ct.models.MLModel(spec)\n";
    scriptFile << "                    \n";
    scriptFile << "                    except Exception as e:\n";
    scriptFile << "                        print(f'Error during weight pruning for compression: {e}')\n";
    scriptFile << "                        print('Continuing with quantized but unpruned model')\n";
    scriptFile << "            \n";
    scriptFile << "            # File-level compression (apply gzip compression to the final model)\n";
    scriptFile << "            print('Will apply file-level compression after saving')\n";
    scriptFile << "            # Set a flag to apply file compression after saving\n";
    scriptFile << "            apply_file_compression = True\n\n";
    
    scriptFile << "        # Save the optimized model\n";
    scriptFile << "        print(f'Saving optimized model to {args.output}')\n";
    scriptFile << "        model.save(args.output)\n";
    scriptFile << "        \n";
    scriptFile << "        # Add metadata about optimizations\n";
    scriptFile << "        optimized_model = ct.models.MLModel(args.output)\n";
    scriptFile << "        if not hasattr(optimized_model, 'user_defined_metadata'):\n";
    scriptFile << "            optimized_model.user_defined_metadata = {}\n";
    scriptFile << "        optimized_model.user_defined_metadata['optimized'] = 'true'\n";
    scriptFile << "        optimized_model.user_defined_metadata['optimization_date'] = str(np.datetime64('now'))\n";
    scriptFile << "        if args.quantize > 0:\n";
    scriptFile << "            optimized_model.user_defined_metadata['quantization'] = str(args.quantize)\n";
    scriptFile << "        if args.prune > 0.0:\n";
    scriptFile << "            optimized_model.user_defined_metadata['pruning'] = str(args.prune)\n";
    scriptFile << "        if args.use_neural_engine:\n";
    scriptFile << "            optimized_model.user_defined_metadata['neural_engine'] = 'true'\n";
    scriptFile << "        if args.compress > 0:\n";
    scriptFile << "            optimized_model.user_defined_metadata['compression'] = str(args.compress)\n";
    scriptFile << "        optimized_model.save(args.output)\n";
    scriptFile << "        \n";
    scriptFile << "        print('Optimization completed successfully')\n";
    scriptFile << "        return 0\n";

    // Add file-level compression code before the exception handling
    scriptFile << "        # Apply file-level compression if specified\n";
    scriptFile << "        if args.compress > 0:\n";
    scriptFile << "            try:\n";
    scriptFile << "                import gzip\n";
    scriptFile << "                import shutil\n";
    scriptFile << "                import os\n";
    scriptFile << "                \n";
    scriptFile << "                print(f'Applying file-level compression (level {args.compress}) to {args.output}')\n";
    scriptFile << "                \n";
    scriptFile << "                # Create a temporary file for compressed output\n";
    scriptFile << "                compressed_path = args.output + '.compressed'\n";
    scriptFile << "                \n";
    scriptFile << "                # Read the saved model file\n";
    scriptFile << "                with open(args.output, 'rb') as f_in:\n";
    scriptFile << "                    # Write compressed model file\n";
    scriptFile << "                    with gzip.open(compressed_path, 'wb', compresslevel=min(9, args.compress)) as f_out:\n";
    scriptFile << "                        shutil.copyfileobj(f_in, f_out)\n";
    scriptFile << "                \n";
    scriptFile << "                # Get file sizes for comparison\n";
    scriptFile << "                original_size = os.path.getsize(args.output)\n";
    scriptFile << "                compressed_size = os.path.getsize(compressed_path)\n";
    scriptFile << "                compression_ratio = (1 - (compressed_size / original_size)) * 100\n";
    scriptFile << "                \n";
    scriptFile << "                print(f'Original size: {original_size:,} bytes')\n";
    scriptFile << "                print(f'Compressed size: {compressed_size:,} bytes')\n";
    scriptFile << "                print(f'Compression ratio: {compression_ratio:.2f}%')\n";
    scriptFile << "                \n";
    scriptFile << "                # Replace original with compressed version\n";
    scriptFile << "                os.remove(args.output)\n";
    scriptFile << "                shutil.move(compressed_path, args.output)\n";
    scriptFile << "                \n";
    scriptFile << "                print('File-level compression completed successfully')\n";
    scriptFile << "                \n";
    scriptFile << "            except Exception as e:\n";
    scriptFile << "                print(f'Error during file-level compression: {e}')\n";
    scriptFile << "                print('Continuing with uncompressed model file')\n";
    scriptFile << "        \n";
    scriptFile << "        print('Optimization completed successfully')\n";
    scriptFile << "        return 0\n";
    scriptFile << "    except Exception as e:\n";
    scriptFile << "        print(f'Error during model optimization: {e}')\n";
    scriptFile << "        return 1\n\n";
    
    scriptFile << "if __name__ == '__main__':\n";
    scriptFile << "    sys.exit(main())\n";
    
    scriptFile.close();
    
    // Make script executable on Unix-like systems
#ifndef _WIN32
    std::string chmodCmd = "chmod +x " + scriptPath;
    system(chmodCmd.c_str());
#endif
    
    return scriptPath;
}

bool ModelOptimizer::pruneWeights(const std::string& inputModelPath, 
                                const std::string& outputModelPath, 
                                float pruningThreshold) {
    // Validate inputs
    if (inputModelPath.empty() || outputModelPath.empty()) {
        std::cerr << "Error: Invalid input or output paths for weight pruning" << std::endl;
        return false;
    }
    
    if (pruningThreshold <= 0.0f || pruningThreshold >= 1.0f) {
        std::cerr << "Warning: Pruning threshold should be between 0.0 and 1.0. Got " 
                 << pruningThreshold << ". Clamping to valid range." << std::endl;
        pruningThreshold = std::max(0.001f, std::min(pruningThreshold, 0.999f));
    }
    
    // Check if the input file exists
    std::ifstream checkFile(inputModelPath);
    if (!checkFile.good()) {
        std::cerr << "Error: Input model file does not exist: " << inputModelPath << std::endl;
        return false;
    }
    checkFile.close();
    
    std::cout << "Starting weight pruning with threshold: " << pruningThreshold << std::endl;
    
    // Create a Python script to handle the pruning process
    std::string scriptPath = "/tmp/prune_weights_" + 
                           std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + 
                           ".py";
    
    // Create the script file
    std::ofstream scriptFile(scriptPath);
    if (!scriptFile.is_open()) {
        std::cerr << "Error: Could not create pruning script" << std::endl;
        return false;
    }
    
    // Write Python script for weight pruning
    scriptFile << "#!/usr/bin/env python3\n";
    scriptFile << "# FBNeo AI Weight Pruning Script\n\n";
    
    scriptFile << "import sys\n";
    scriptFile << "import os\n";
    scriptFile << "import argparse\n";
    scriptFile << "import numpy as np\n";
    scriptFile << "import torch\n";
    scriptFile << "import coremltools as ct\n";
    scriptFile << "from collections import OrderedDict\n\n";
    
    scriptFile << "def prune_torch_model(model_path, output_path, threshold):\n";
    scriptFile << "    try:\n";
    scriptFile << "        print(f'Loading PyTorch model from {model_path}')\n";
    scriptFile << "        model = torch.load(model_path, map_location=torch.device('cpu'))\n";
    scriptFile << "        \n";
    scriptFile << "        # Handle different model formats\n";
    scriptFile << "        if isinstance(model, dict) and 'state_dict' in model:\n";
    scriptFile << "            state_dict = model['state_dict']\n";
    scriptFile << "        elif isinstance(model, OrderedDict):\n";
    scriptFile << "            state_dict = model\n";
    scriptFile << "        else:\n";
    scriptFile << "            state_dict = model.state_dict()\n";
    scriptFile << "        \n";
    scriptFile << "        # Track statistics\n";
    scriptFile << "        total_params = 0\n";
    scriptFile << "        pruned_params = 0\n";
    scriptFile << "        \n";
    scriptFile << "        # Create a new state dict with pruned weights\n";
    scriptFile << "        pruned_state_dict = OrderedDict()\n";
    scriptFile << "        \n";
    scriptFile << "        # Prune weights in each layer\n";
    scriptFile << "        for name, param in state_dict.items():\n";
    scriptFile << "            # Only prune weight tensors, not bias or batch norm\n";
    scriptFile << "            if 'weight' in name and len(param.shape) > 1:  # Typically weights have dim > 1\n";
    scriptFile << "                # Get absolute values of weights\n";
    scriptFile << "                abs_weights = torch.abs(param)\n";
    scriptFile << "                \n";
    scriptFile << "                # Determine pruning mask\n";
    scriptFile << "                mask = abs_weights > threshold\n";
    scriptFile << "                \n";
    scriptFile << "                # Apply pruning mask\n";
    scriptFile << "                pruned_param = param.clone()\n";
    scriptFile << "                pruned_param[~mask] = 0.0\n";
    scriptFile << "                \n";
    scriptFile << "                # Update statistics\n";
    scriptFile << "                layer_total = param.numel()\n";
    scriptFile << "                layer_pruned = torch.sum(~mask).item()\n";
    scriptFile << "                pruned_pct = (layer_pruned / layer_total) * 100 if layer_total > 0 else 0\n";
    scriptFile << "                \n";
    scriptFile << "                print(f'Layer {name}: pruned {layer_pruned}/{layer_total} parameters ({pruned_pct:.2f}%)')\n";
    scriptFile << "                \n";
    scriptFile << "                # Add to totals\n";
    scriptFile << "                total_params += layer_total\n";
    scriptFile << "                pruned_params += layer_pruned\n";
    scriptFile << "                \n";
    scriptFile << "                # Store pruned tensor\n";
    scriptFile << "                pruned_state_dict[name] = pruned_param\n";
    scriptFile << "            else:\n";
    scriptFile << "                # Keep other parameters unchanged\n";
    scriptFile << "                pruned_state_dict[name] = param.clone()\n";
    scriptFile << "                \n";
    scriptFile << "                # Add to total params count if it's a parameter tensor\n";
    scriptFile << "                if isinstance(param, torch.Tensor):\n";
    scriptFile << "                    total_params += param.numel()\n";
    scriptFile << "        \n";
    scriptFile << "        # Calculate overall pruning statistics\n";
    scriptFile << "        overall_pruned_pct = (pruned_params / total_params) * 100 if total_params > 0 else 0\n";
    scriptFile << "        print(f'\\nOverall: pruned {pruned_params}/{total_params} parameters ({overall_pruned_pct:.2f}%)')\n";
    scriptFile << "        \n";
    scriptFile << "        # Create pruned model\n";
    scriptFile << "        if isinstance(model, dict):\n";
    scriptFile << "            model['state_dict'] = pruned_state_dict\n";
    scriptFile << "            pruned_model = model\n";
    scriptFile << "        else:\n";
    scriptFile << "            # Load the pruned state dict into the model\n";
    scriptFile << "            pruned_model = model\n";
    scriptFile << "            pruned_model.load_state_dict(pruned_state_dict)\n";
    scriptFile << "        \n";
    scriptFile << "        # Save pruned model\n";
    scriptFile << "        print(f'Saving pruned model to {output_path}')\n";
    scriptFile << "        torch.save(pruned_model, output_path)\n";
    scriptFile << "        \n";
    scriptFile << "        return True, overall_pruned_pct\n";
    scriptFile << "    except Exception as e:\n";
    scriptFile << "        print(f'Error during PyTorch model pruning: {e}')\n";
    scriptFile << "        return False, 0\n\n";
    
    scriptFile << "def prune_coreml_model(model_path, output_path, threshold):\n";
    scriptFile << "    try:\n";
    scriptFile << "        print(f'Loading CoreML model from {model_path}')\n";
    scriptFile << "        model = ct.models.MLModel(model_path)\n";
    scriptFile << "        spec = model.get_spec()\n";
    scriptFile << "        \n";
    scriptFile << "        # Track statistics\n";
    scriptFile << "        total_params = 0\n";
    scriptFile << "        pruned_params = 0\n";
    scriptFile << "        \n";
    scriptFile << "        # Determine model type\n";
    scriptFile << "        if hasattr(spec, 'neuralNetwork'):\n";
    scriptFile << "            nn_spec = spec.neuralNetwork\n";
    scriptFile << "            network_type = 'neural_network'\n";
    scriptFile << "        elif hasattr(spec, 'neuralNetworkClassifier'):\n";
    scriptFile << "            nn_spec = spec.neuralNetworkClassifier\n";
    scriptFile << "            network_type = 'classifier'\n";
    scriptFile << "        elif hasattr(spec, 'neuralNetworkRegressor'):\n";
    scriptFile << "            nn_spec = spec.neuralNetworkRegressor\n";
    scriptFile << "            network_type = 'regressor'\n";
    scriptFile << "        else:\n";
    scriptFile << "            print('Error: Unsupported CoreML model type')\n";
    scriptFile << "            return False, 0\n";
    scriptFile << "        \n";
    scriptFile << "        # Process layers\n";
    scriptFile << "        for layer in nn_spec.layers:\n";
    scriptFile << "            # Handle convolutional layers\n";
    scriptFile << "            if hasattr(layer, 'convolution'):\n";
    scriptFile << "                if hasattr(layer.convolution, 'weights') and len(layer.convolution.weights.floatValue) > 0:\n";
    scriptFile << "                    # Get weights\n";
    scriptFile << "                    weights = np.array(layer.convolution.weights.floatValue)\n";
    scriptFile << "                    \n";
    scriptFile << "                    # Prune weights\n";
    scriptFile << "                    mask = np.abs(weights) > threshold\n";
    scriptFile << "                    pruned_weights = weights.copy()\n";
    scriptFile << "                    pruned_weights[~mask] = 0.0\n";
    scriptFile << "                    \n";
    scriptFile << "                    # Update statistics\n";
    scriptFile << "                    layer_total = len(weights)\n";
    scriptFile << "                    layer_pruned = np.sum(~mask)\n";
    scriptFile << "                    pruned_pct = (layer_pruned / layer_total) * 100 if layer_total > 0 else 0\n";
    scriptFile << "                    \n";
    scriptFile << "                    print(f'Layer {layer.name} (Conv): pruned {layer_pruned}/{layer_total} parameters ({pruned_pct:.2f}%)')\n";
    scriptFile << "                    \n";
    scriptFile << "                    # Add to totals\n";
    scriptFile << "                    total_params += layer_total\n";
    scriptFile << "                    pruned_params += layer_pruned\n";
    scriptFile << "                    \n";
    scriptFile << "                    # Update weights in the model\n";
    scriptFile << "                    layer.convolution.weights.ClearField('floatValue')\n";
    scriptFile << "                    layer.convolution.weights.floatValue.extend(list(pruned_weights.flatten()))\n";
    scriptFile << "            \n";
    scriptFile << "            # Handle fully connected layers\n";
    scriptFile << "            elif hasattr(layer, 'innerProduct'):\n";
    scriptFile << "                if hasattr(layer.innerProduct, 'weights') and len(layer.innerProduct.weights.floatValue) > 0:\n";
    scriptFile << "                    # Get weights\n";
    scriptFile << "                    weights = np.array(layer.innerProduct.weights.floatValue)\n";
    scriptFile << "                    \n";
    scriptFile << "                    # Prune weights\n";
    scriptFile << "                    mask = np.abs(weights) > threshold\n";
    scriptFile << "                    pruned_weights = weights.copy()\n";
    scriptFile << "                    pruned_weights[~mask] = 0.0\n";
    scriptFile << "                    \n";
    scriptFile << "                    # Update statistics\n";
    scriptFile << "                    layer_total = len(weights)\n";
    scriptFile << "                    layer_pruned = np.sum(~mask)\n";
    scriptFile << "                    pruned_pct = (layer_pruned / layer_total) * 100 if layer_total > 0 else 0\n";
    scriptFile << "                    \n";
    scriptFile << "                    print(f'Layer {layer.name} (FC): pruned {layer_pruned}/{layer_total} parameters ({pruned_pct:.2f}%)')\n";
    scriptFile << "                    \n";
    scriptFile << "                    # Add to totals\n";
    scriptFile << "                    total_params += layer_total\n";
    scriptFile << "                    pruned_params += layer_pruned\n";
    scriptFile << "                    \n";
    scriptFile << "                    # Update weights in the model\n";
    scriptFile << "                    layer.innerProduct.weights.ClearField('floatValue')\n";
    scriptFile << "                    layer.innerProduct.weights.floatValue.extend(list(pruned_weights.flatten()))\n";
    scriptFile << "        \n";
    scriptFile << "        # Calculate overall pruning statistics\n";
    scriptFile << "        overall_pruned_pct = (pruned_params / total_params) * 100 if total_params > 0 else 0\n";
    scriptFile << "        print(f'\\nOverall: pruned {pruned_params}/{total_params} parameters ({overall_pruned_pct:.2f}%)')\n";
    scriptFile << "        \n";
    scriptFile << "        # Add metadata about pruning\n";
    scriptFile << "        if not hasattr(spec, 'metadata'):\n";
    scriptFile << "            spec.metadata = {}\n";
    scriptFile << "        if not hasattr(spec.metadata, 'userDefined'):\n";
    scriptFile << "            spec.metadata.userDefined = {}\n";
    scriptFile << "        \n";
    scriptFile << "        spec.metadata.userDefined['pruned'] = 'true'\n";
    scriptFile << "        spec.metadata.userDefined['pruning_threshold'] = str(threshold)\n";
    scriptFile << "        spec.metadata.userDefined['pruning_date'] = str(np.datetime64('now'))\n";
    scriptFile << "        spec.metadata.userDefined['pruned_percentage'] = str(overall_pruned_pct)\n";
    scriptFile << "        \n";
    scriptFile << "        # Save pruned model\n";
    scriptFile << "        print(f'Saving pruned CoreML model to {output_path}')\n";
    scriptFile << "        ct.models.MLModel(spec).save(output_path)\n";
    scriptFile << "        \n";
    scriptFile << "        return True, overall_pruned_pct\n";
    scriptFile << "    except Exception as e:\n";
    scriptFile << "        print(f'Error during CoreML model pruning: {e}')\n";
    scriptFile << "        return False, 0\n\n";
    
    scriptFile << "def main():\n";
    scriptFile << "    parser = argparse.ArgumentParser(description='Prune weights in neural network models')\n";
    scriptFile << "    parser.add_argument('--input', required=True, help='Input model path')\n";
    scriptFile << "    parser.add_argument('--output', required=True, help='Output model path')\n";
    scriptFile << "    parser.add_argument('--threshold', type=float, required=True, help='Pruning threshold')\n";
    scriptFile << "    args = parser.parse_args()\n\n";
    
    scriptFile << "    # Determine model type by file extension\n";
    scriptFile << "    _, ext = os.path.splitext(args.input)\n";
    scriptFile << "    ext = ext.lower()\n";
    scriptFile << "    \n";
    scriptFile << "    if ext in ['.pt', '.pth']:\n";
    scriptFile << "        # PyTorch model\n";
    scriptFile << "        print('Detected PyTorch model')\n";
    scriptFile << "        success, pruned_pct = prune_torch_model(args.input, args.output, args.threshold)\n";
    scriptFile << "    elif ext in ['.mlmodel']:\n";
    scriptFile << "        # CoreML model\n";
    scriptFile << "        print('Detected CoreML model')\n";
    scriptFile << "        success, pruned_pct = prune_coreml_model(args.input, args.output, args.threshold)\n";
    scriptFile << "    else:\n";
    scriptFile << "        print(f'Unrecognized model format: {ext}')\n";
    scriptFile << "        return 1\n";
    scriptFile << "    \n";
    scriptFile << "    if success:\n";
    scriptFile << "        print(f'Weight pruning complete: {pruned_pct:.2f}% of weights pruned')\n";
    scriptFile << "        return 0\n";
    scriptFile << "    else:\n";
    scriptFile << "        print('Weight pruning failed')\n";
    scriptFile << "        return 1\n\n";
    
    scriptFile << "if __name__ == '__main__':\n";
    scriptFile << "    sys.exit(main())\n";
    
    scriptFile.close();
    
    // Make script executable on Unix-like systems
#ifndef _WIN32
    std::string chmodCmd = "chmod +x " + scriptPath;
    system(chmodCmd.c_str());
#endif
    
    // Build command line
    std::string cmd = "python \"" + scriptPath + "\"" +
                     " --input \"" + inputModelPath + "\"" +
                     " --output \"" + outputModelPath + "\"" +
                     " --threshold " + std::to_string(pruningThreshold);
    
    // Execute pruning script
    std::cout << "Running weight pruning: " << cmd << std::endl;
    int result = system(cmd.c_str());
    
    // Cleanup
    std::remove(scriptPath.c_str());
    
    if (result != 0) {
        std::cerr << "Error: Weight pruning failed with code " << result << std::endl;
        return false;
    }
    
    std::cout << "Weight pruning completed successfully" << std::endl;
    return true;
}

bool ModelOptimizer::compressModel(const std::string& inputModelPath, 
                                  const std::string& outputModelPath, 
                                  int compressionLevel) {
    // Validate inputs
    if (inputModelPath.empty() || outputModelPath.empty()) {
        std::cerr << "Error: Invalid input or output paths for model compression" << std::endl;
        return false;
    }
    
    if (compressionLevel < 0 || compressionLevel > 9) {
        std::cerr << "Warning: Compression level should be between 0 and 9. Got " 
                 << compressionLevel << ". Clamping to valid range." << std::endl;
        compressionLevel = std::max(0, std::min(compressionLevel, 9));
    }
    
    // Check if the input file exists
    std::ifstream checkFile(inputModelPath);
    if (!checkFile.good()) {
        std::cerr << "Error: Input model file does not exist: " << inputModelPath << std::endl;
        return false;
    }
    checkFile.close();
    
    std::cout << "Starting model compression with level: " << compressionLevel << std::endl;
    
    // Create a Python script to handle the compression process
    std::string scriptPath = "/tmp/compress_model_" + 
                           std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + 
                           ".py";
    
    // Create the script file
    std::ofstream scriptFile(scriptPath);
    if (!scriptFile.is_open()) {
        std::cerr << "Error: Could not create compression script" << std::endl;
        return false;
    }
    
    // Write Python script for model compression
    scriptFile << "#!/usr/bin/env python3\n";
    scriptFile << "# FBNeo AI Model Compression Script\n\n";
    
    scriptFile << "import sys\n";
    scriptFile << "import os\n";
    scriptFile << "import argparse\n";
    scriptFile << "import numpy as np\n";
    scriptFile << "import gzip\n";
    scriptFile << "import shutil\n";
    scriptFile << "import tempfile\n";
    scriptFile << "import torch\n";
    scriptFile << "import coremltools as ct\n\n";
    
    // Function for basic file-level compression
    scriptFile << "def compress_file(input_path, output_path, compression_level):\n";
    scriptFile << "    try:\n";
    scriptFile << "        print(f'Applying file-level compression (level {compression_level}) to {input_path}')\n";
    scriptFile << "        \n";
    scriptFile << "        # Read the input file\n";
    scriptFile << "        with open(input_path, 'rb') as f_in:\n";
    scriptFile << "            # Write compressed file\n";
    scriptFile << "            with gzip.open(output_path, 'wb', compresslevel=compression_level) as f_out:\n";
    scriptFile << "                shutil.copyfileobj(f_in, f_out)\n";
    scriptFile << "        \n";
    scriptFile << "        # Get file sizes for comparison\n";
    scriptFile << "        original_size = os.path.getsize(input_path)\n";
    scriptFile << "        compressed_size = os.path.getsize(output_path)\n";
    scriptFile << "        compression_ratio = (1 - (compressed_size / original_size)) * 100\n";
    scriptFile << "        \n";
    scriptFile << "        print(f'Original size: {original_size:,} bytes')\n";
    scriptFile << "        print(f'Compressed size: {compressed_size:,} bytes')\n";
    scriptFile << "        print(f'Compression ratio: {compression_ratio:.2f}%')\n";
    scriptFile << "        \n";
    scriptFile << "        return True, compression_ratio\n";
    scriptFile << "    except Exception as e:\n";
    scriptFile << "        print(f'Error during file-level compression: {e}')\n";
    scriptFile << "        return False, 0\n\n";
    
    // Function for advanced PyTorch model compression
    scriptFile << "def compress_torch_model(input_path, output_path, compression_level):\n";
    scriptFile << "    try:\n";
    scriptFile << "        print(f'Loading PyTorch model from {input_path}')\n";
    scriptFile << "        model = torch.load(input_path, map_location=torch.device('cpu'))\n";
    scriptFile << "        \n";
    scriptFile << "        # Get original model size\n";
    scriptFile << "        original_size = os.path.getsize(input_path)\n";
    scriptFile << "        \n";
    scriptFile << "        # Apply compression techniques based on level\n";
    scriptFile << "        if compression_level >= 1:\n";
    scriptFile << "            # Light compression: quantize to float16\n";
    scriptFile << "            print('Applying FP16 quantization')\n";
    scriptFile << "            for key in list(model.keys()):\n";
    scriptFile << "                if isinstance(model[key], torch.Tensor):\n";
    scriptFile << "                    model[key] = model[key].half()\n";
    scriptFile << "        \n";
    scriptFile << "        if compression_level >= 3:\n";
    scriptFile << "            # Medium compression: small weight pruning (remove near-zero weights)\n";
    scriptFile << "            pruning_threshold = 0.01  # Prune weights smaller than 0.01\n";
    scriptFile << "            print(f'Applying weight pruning with threshold {pruning_threshold}')\n";
    scriptFile << "            pruned_count = 0\n";
    scriptFile << "            total_count = 0\n";
    scriptFile << "            \n";
    scriptFile << "            for key in list(model.keys()):\n";
    scriptFile << "                if isinstance(model[key], torch.Tensor) and len(model[key].shape) > 1:\n";
    scriptFile << "                    weights = model[key]\n";
    scriptFile << "                    mask = torch.abs(weights) > pruning_threshold\n";
    scriptFile << "                    pruned_weights = weights.clone()\n";
    scriptFile << "                    pruned_weights[~mask] = 0.0\n";
    scriptFile << "                    model[key] = pruned_weights\n";
    scriptFile << "                    \n";
    scriptFile << "                    # Update statistics\n";
    scriptFile << "                    layer_total = weights.numel()\n";
    scriptFile << "                    layer_pruned = torch.sum(~mask).item()\n";
    scriptFile << "                    pruned_count += layer_pruned\n";
    scriptFile << "                    total_count += layer_total\n";
    scriptFile << "            \n";
    scriptFile << "            if total_count > 0:\n";
    scriptFile << "                print(f'Pruned {pruned_count}/{total_count} weights ({pruned_count/total_count*100:.2f}%)')\n";
    scriptFile << "        \n";
    scriptFile << "        if compression_level >= 6:\n";
    scriptFile << "            # High compression: aggressive pruning and 8-bit quantization\n";
    scriptFile << "            pruning_threshold = 0.05  # More aggressive pruning\n";
    scriptFile << "            print(f'Applying aggressive weight pruning with threshold {pruning_threshold}')\n";
    scriptFile << "            pruned_count = 0\n";
    scriptFile << "            total_count = 0\n";
    scriptFile << "            \n";
    scriptFile << "            for key in list(model.keys()):\n";
    scriptFile << "                if isinstance(model[key], torch.Tensor) and len(model[key].shape) > 1:\n";
    scriptFile << "                    weights = model[key]\n";
    scriptFile << "                    mask = torch.abs(weights) > pruning_threshold\n";
    scriptFile << "                    pruned_weights = weights.clone()\n";
    scriptFile << "                    pruned_weights[~mask] = 0.0\n";
    scriptFile << "                    model[key] = pruned_weights\n";
    scriptFile << "                    \n";
    scriptFile << "                    # Update statistics\n";
    scriptFile << "                    layer_total = weights.numel()\n";
    scriptFile << "                    layer_pruned = torch.sum(~mask).item()\n";
    scriptFile << "                    pruned_count += layer_pruned\n";
    scriptFile << "                    total_count += layer_total\n";
    scriptFile << "            \n";
    scriptFile << "            if total_count > 0:\n";
    scriptFile << "                print(f'Aggressively pruned {pruned_count}/{total_count} weights ({pruned_count/total_count*100:.2f}%)')\n";
    scriptFile << "            \n";
    scriptFile << "            # Custom 8-bit quantization \n";
    scriptFile << "            print('Applying 8-bit quantization')\n";
    scriptFile << "            for key in list(model.keys()):\n";
    scriptFile << "                if isinstance(model[key], torch.Tensor):\n";
    scriptFile << "                    tensor = model[key]\n";
    scriptFile << "                    if tensor.numel() > 0:\n";
    scriptFile << "                        # Get min and max values\n";
    scriptFile << "                        min_val = torch.min(tensor).item()\n";
    scriptFile << "                        max_val = torch.max(tensor).item()\n";
    scriptFile << "                        \n";
    scriptFile << "                        # Skip if all values are the same\n";
    scriptFile << "                        if min_val == max_val:\n";
    scriptFile << "                            continue\n";
    scriptFile << "                        \n";
    scriptFile << "                        # Scale to 0-255 range\n";
    scriptFile << "                        scale = 255.0 / (max_val - min_val)\n";
    scriptFile << "                        zero_point = -min_val * scale\n";
    scriptFile << "                        \n";
    scriptFile << "                        # Quantize to 8-bit\n";
    scriptFile << "                        quantized = torch.round(tensor * scale + zero_point).clamp(0, 255).byte()\n";
    scriptFile << "                        \n";
    scriptFile << "                        # Store quantized tensor and scale factors\n";
    scriptFile << "                        model[key + '_quantized'] = quantized\n";
    scriptFile << "                        model[key + '_scale'] = scale\n";
    scriptFile << "                        model[key + '_zero_point'] = zero_point\n";
    scriptFile << "                        \n";
    scriptFile << "                        # Remove original tensor\n";
    scriptFile << "                        del model[key]\n";
    scriptFile << "        \n";
    scriptFile << "        # Save the compressed model\n";
    scriptFile << "        print(f'Saving compressed model to {output_path}')\n";
    scriptFile << "        # Create metadata about compression\n";
    scriptFile << "        if not hasattr(model, 'metadata'):\n";
    scriptFile << "            model.metadata = {}\n";
    scriptFile << "        model.metadata['compressed'] = True\n";
    scriptFile << "        model.metadata['compression_level'] = compression_level\n";
    scriptFile << "        model.metadata['compression_date'] = str(np.datetime64('now'))\n";
    scriptFile << "        \n";
    scriptFile << "        # Save model\n";
    scriptFile << "        torch.save(model, output_path)\n";
    scriptFile << "        \n";
    scriptFile << "        # Apply file-level compression if level > 7\n";
    scriptFile << "        if compression_level >= 8:\n";
    scriptFile << "            # Create a temporary file for intermediate step\n";
    scriptFile << "            temp_output = output_path + '.tmp'\n";
    scriptFile << "            shutil.move(output_path, temp_output)\n";
    scriptFile << "            \n";
    scriptFile << "            # Apply file-level compression\n";
    scriptFile << "            file_success, _ = compress_file(temp_output, output_path, min(9, compression_level))\n";
    scriptFile << "            \n";
    scriptFile << "            # Clean up temporary file\n";
    scriptFile << "            if file_success:\n";
    scriptFile << "                os.remove(temp_output)\n";
    scriptFile << "            else:\n";
    scriptFile << "                # If file compression failed, keep the original compressed model\n";
    scriptFile << "                shutil.move(temp_output, output_path)\n";
    scriptFile << "        \n";
    scriptFile << "        # Get compressed size\n";
    scriptFile << "        compressed_size = os.path.getsize(output_path)\n";
    scriptFile << "        compression_ratio = (1 - (compressed_size / original_size)) * 100\n";
    scriptFile << "        \n";
    scriptFile << "        print(f'Original size: {original_size:,} bytes')\n";
    scriptFile << "        print(f'Compressed size: {compressed_size:,} bytes')\n";
    scriptFile << "        print(f'Compression ratio: {compression_ratio:.2f}%')\n";
    scriptFile << "        \n";
    scriptFile << "        return True, compression_ratio\n";
    scriptFile << "    except Exception as e:\n";
    scriptFile << "        print(f'Error during PyTorch model compression: {e}')\n";
    scriptFile << "        import traceback\n";
    scriptFile << "        traceback.print_exc()\n";
    scriptFile << "        return False, 0\n\n";
    
    // Function for CoreML model compression
    scriptFile << "def compress_coreml_model(input_path, output_path, compression_level):\n";
    scriptFile << "    try:\n";
    scriptFile << "        print(f'Loading CoreML model from {input_path}')\n";
    scriptFile << "        model = ct.models.MLModel(input_path)\n";
    scriptFile << "        original_spec = model.get_spec()\n";
    scriptFile << "        \n";
    scriptFile << "        # Get original model size\n";
    scriptFile << "        original_size = os.path.getsize(input_path)\n";
    scriptFile << "        \n";
    scriptFile << "        # Apply compression techniques based on level\n";
    scriptFile << "        if compression_level >= 1:\n";
    scriptFile << "            # Light compression: quantize to float16\n";
    scriptFile << "            print('Applying float16 weight quantization')\n";
    scriptFile << "            model = ct.models.neural_network.quantization_utils.quantize_weights(model, dtype=np.float16)\n";
    scriptFile << "        \n";
    scriptFile << "        if compression_level >= 4:\n";
    scriptFile << "            # Medium compression: quantize to 8-bit\n";
    scriptFile << "            print('Applying 8-bit weight quantization')\n";
    scriptFile << "            model = ct.models.neural_network.quantization_utils.quantize_weights(model, nbits=8)\n";
    scriptFile << "        \n";
    scriptFile << "        if compression_level >= 7:\n";
    scriptFile << "            # High compression: weight pruning (if supported)\n";
    scriptFile << "            pruning_threshold = 0.01 * (compression_level - 6)  # Scale based on level\n";
    scriptFile << "            print(f'Applying weight pruning with threshold {pruning_threshold}')\n";
    scriptFile << "            \n";
    scriptFile << "            # Get spec after quantization\n";
    scriptFile << "            spec = model.get_spec()\n";
    scriptFile << "            \n";
    scriptFile << "            # Determine model type\n";
    scriptFile << "            if hasattr(spec, 'neuralNetwork'):\n";
    scriptFile << "                nn_spec = spec.neuralNetwork\n";
    scriptFile << "            elif hasattr(spec, 'neuralNetworkClassifier'):\n";
    scriptFile << "                nn_spec = spec.neuralNetworkClassifier\n";
    scriptFile << "            elif hasattr(spec, 'neuralNetworkRegressor'):\n";
    scriptFile << "                nn_spec = spec.neuralNetworkRegressor\n";
    scriptFile << "            else:\n";
    scriptFile << "                print('Warning: Unable to determine neural network type for pruning')\n";
    scriptFile << "                nn_spec = None\n";
    scriptFile << "            \n";
    scriptFile << "            # Apply pruning if we can access the network spec\n";
    scriptFile << "            if nn_spec is not None:\n";
    scriptFile << "                pruned_count = 0\n";
    scriptFile << "                total_count = 0\n";
    scriptFile << "                \n";
    scriptFile << "                # Process layers\n";
    scriptFile << "                for layer in nn_spec.layers:\n";
    scriptFile << "                    # Handle convolutional layers\n";
    scriptFile << "                    if hasattr(layer, 'convolution'):\n";
    scriptFile << "                        if hasattr(layer.convolution, 'weights') and len(layer.convolution.weights.floatValue) > 0:\n";
    scriptFile << "                            # Get weights\n";
    scriptFile << "                            weights = np.array(layer.convolution.weights.floatValue)\n";
    scriptFile << "                            \n";
    scriptFile << "                            # Prune weights\n";
    scriptFile << "                            mask = np.abs(weights) > pruning_threshold\n";
    scriptFile << "                            pruned_weights = weights.copy()\n";
    scriptFile << "                            pruned_weights[~mask] = 0.0\n";
    scriptFile << "                            \n";
    scriptFile << "                            # Update statistics\n";
    scriptFile << "                            layer_total = len(weights)\n";
    scriptFile << "                            layer_pruned = np.sum(~mask)\n";
    scriptFile << "                            \n";
    scriptFile << "                            # Add to totals\n";
    scriptFile << "                            total_count += layer_total\n";
    scriptFile << "                            pruned_count += layer_pruned\n";
    scriptFile << "                            \n";
    scriptFile << "                            # Update weights in the model\n";
    scriptFile << "                            layer.convolution.weights.ClearField('floatValue')\n";
    scriptFile << "                            layer.convolution.weights.floatValue.extend(list(pruned_weights.flatten()))\n";
    scriptFile << "                    \n";
    scriptFile << "                    # Handle fully connected layers\n";
    scriptFile << "                    elif hasattr(layer, 'innerProduct'):\n";
    scriptFile << "                        if hasattr(layer.innerProduct, 'weights') and len(layer.innerProduct.weights.floatValue) > 0:\n";
    scriptFile << "                            # Get weights\n";
    scriptFile << "                            weights = np.array(layer.innerProduct.weights.floatValue)\n";
    scriptFile << "                            \n";
    scriptFile << "                            # Prune weights\n";
    scriptFile << "                            mask = np.abs(weights) > pruning_threshold\n";
    scriptFile << "                            pruned_weights = weights.copy()\n";
    scriptFile << "                            pruned_weights[~mask] = 0.0\n";
    scriptFile << "                            \n";
    scriptFile << "                            # Update statistics\n";
    scriptFile << "                            layer_total = len(weights)\n";
    scriptFile << "                            layer_pruned = np.sum(~mask)\n";
    scriptFile << "                            \n";
    scriptFile << "                            # Add to totals\n";
    scriptFile << "                            total_count += layer_total\n";
    scriptFile << "                            pruned_count += layer_pruned\n";
    scriptFile << "                            \n";
    scriptFile << "                            # Update weights in the model\n";
    scriptFile << "                            layer.innerProduct.weights.ClearField('floatValue')\n";
    scriptFile << "                            layer.innerProduct.weights.floatValue.extend(list(pruned_weights.flatten()))\n";
    scriptFile << "                \n";
    scriptFile << "                if total_count > 0:\n";
    scriptFile << "                    pruned_pct = (pruned_count / total_count) * 100\n";
    scriptFile << "                    print(f'Pruned {pruned_count}/{total_count} weights ({pruned_pct:.2f}%)')\n";
    scriptFile << "                \n";
    scriptFile << "                # Update model with pruned weights\n";
    scriptFile << "                model = ct.models.MLModel(spec)\n";
    scriptFile << "        \n";
    scriptFile << "        # Add metadata about compression\n";
    scriptFile << "        spec = model.get_spec()\n";
    scriptFile << "        if not hasattr(spec, 'metadata'):\n";
    scriptFile << "            spec.metadata = {}\n";
    scriptFile << "        if not hasattr(spec.metadata, 'userDefined'):\n";
    scriptFile << "            spec.metadata.userDefined = {}\n";
    scriptFile << "        \n";
    scriptFile << "        spec.metadata.userDefined['compressed'] = 'true'\n";
    scriptFile << "        spec.metadata.userDefined['compression_level'] = str(compression_level)\n";
    scriptFile << "        spec.metadata.userDefined['compression_date'] = str(np.datetime64('now'))\n";
    scriptFile << "        \n";
    scriptFile << "        # Save the compressed model\n";
    scriptFile << "        print(f'Saving compressed CoreML model to {output_path}')\n";
    scriptFile << "        ct.models.MLModel(spec).save(output_path)\n";
    scriptFile << "        \n";
    scriptFile << "        # Apply file-level compression if level >= 8\n";
    scriptFile << "        if compression_level >= 8:\n";
    scriptFile << "            # Create a temporary file for intermediate step\n";
    scriptFile << "            temp_output = output_path + '.tmp'\n";
    scriptFile << "            shutil.move(output_path, temp_output)\n";
    scriptFile << "            \n";
    scriptFile << "            # Apply file-level compression\n";
    scriptFile << "            file_success, _ = compress_file(temp_output, output_path, min(9, compression_level))\n";
    scriptFile << "            \n";
    scriptFile << "            # Clean up temporary file\n";
    scriptFile << "            if file_success:\n";
    scriptFile << "                os.remove(temp_output)\n";
    scriptFile << "            else:\n";
    scriptFile << "                # If file compression failed, keep the original compressed model\n";
    scriptFile << "                shutil.move(temp_output, output_path)\n";
    scriptFile << "        \n";
    scriptFile << "        # Get compressed size\n";
    scriptFile << "        compressed_size = os.path.getsize(output_path)\n";
    scriptFile << "        compression_ratio = (1 - (compressed_size / original_size)) * 100\n";
    scriptFile << "        \n";
    scriptFile << "        print(f'Original size: {original_size:,} bytes')\n";
    scriptFile << "        print(f'Compressed size: {compressed_size:,} bytes')\n";
    scriptFile << "        print(f'Compression ratio: {compression_ratio:.2f}%')\n";
    scriptFile << "        \n";
    scriptFile << "        return True, compression_ratio\n";
    scriptFile << "    except Exception as e:\n";
    scriptFile << "        print(f'Error during CoreML model compression: {e}')\n";
    scriptFile << "        import traceback\n";
    scriptFile << "        traceback.print_exc()\n";
    scriptFile << "        return False, 0\n\n";
    
    // Main function to parse arguments and call appropriate compression functions
    scriptFile << "def main():\n";
    scriptFile << "    parser = argparse.ArgumentParser(description='Compress neural network models')\n";
    scriptFile << "    parser.add_argument('--input', required=True, help='Input model path')\n";
    scriptFile << "    parser.add_argument('--output', required=True, help='Output model path')\n";
    scriptFile << "    parser.add_argument('--level', type=int, required=True, help='Compression level (1-9)')\n";
    scriptFile << "    args = parser.parse_args()\n\n";
    
    scriptFile << "    # Validate compression level\n";
    scriptFile << "    if args.level < 1 or args.level > 9:\n";
    scriptFile << "        print(f'Warning: Compression level should be between 1-9. Got {args.level}, clamping.')\n";
    scriptFile << "        args.level = max(1, min(args.level, 9))\n\n";
    
    scriptFile << "    # Determine model type by file extension\n";
    scriptFile << "    _, ext = os.path.splitext(args.input)\n";
    scriptFile << "    ext = ext.lower()\n";
    scriptFile << "    \n";
    scriptFile << "    if ext in ['.pt', '.pth']:\n";
    scriptFile << "        # PyTorch model\n";
    scriptFile << "        print('Detected PyTorch model')\n";
    scriptFile << "        success, ratio = compress_torch_model(args.input, args.output, args.level)\n";
    scriptFile << "    elif ext in ['.mlmodel']:\n";
    scriptFile << "        # CoreML model\n";
    scriptFile << "        print('Detected CoreML model')\n";
    scriptFile << "        success, ratio = compress_coreml_model(args.input, args.output, args.level)\n";
    scriptFile << "    else:\n";
    scriptFile << "        # Fallback to basic file compression\n";
    scriptFile << "        print(f'Unknown model format {ext}, applying file-level compression only')\n";
    scriptFile << "        success, ratio = compress_file(args.input, args.output, args.level)\n";
    scriptFile << "    \n";
    scriptFile << "    if success:\n";
    scriptFile << "        print(f'Model compression complete: {ratio:.2f}% reduction in size')\n";
    scriptFile << "        return 0\n";
    scriptFile << "    else:\n";
    scriptFile << "        print('Model compression failed')\n";
    scriptFile << "        return 1\n\n";
    
    scriptFile << "if __name__ == '__main__':\n";
    scriptFile << "    sys.exit(main())\n";
    
    scriptFile.close();
    
    // Make script executable on Unix-like systems
#ifndef _WIN32
    std::string chmodCmd = "chmod +x " + scriptPath;
    system(chmodCmd.c_str());
#endif
    
    // Build command line
    std::string cmd = "python \"" + scriptPath + "\"" +
                     " --input \"" + inputModelPath + "\"" +
                     " --output \"" + outputModelPath + "\"" +
                     " --level " + std::to_string(compressionLevel);
    
    // Execute compression script
    std::cout << "Running model compression: " << cmd << std::endl;
    int result = system(cmd.c_str());
    
    // Cleanup
    std::remove(scriptPath.c_str());
    
    if (result != 0) {
        std::cerr << "Error: Model compression failed with code " << result << std::endl;
        return false;
    }
    
    std::cout << "Model compression completed successfully" << std::endl;
    return true;
}

// Standalone optimization functions

bool optimizeModelForSpeed(const std::string& inputModelPath, const std::string& outputModelPath) {
    ModelOptimizer optimizer;
    OptimizationConfig config;
    
    // Configure for speed optimization
    config.quantizationBits = 16;  // Use FP16 for faster computation
    config.pruningThreshold = 0.01f; // Light pruning to remove near-zero weights
    config.useNeuralEngine = true;  // Use Neural Engine for acceleration
    config.compressionLevel = 0;    // No compression to avoid decompression overhead
    
    return optimizer.optimizeModel(inputModelPath, outputModelPath, config);
}

bool optimizeModelForSize(const std::string& inputModelPath, const std::string& outputModelPath) {
    ModelOptimizer optimizer;
    OptimizationConfig config;
    
    // Configure for size optimization
    config.quantizationBits = 8;     // Use 8-bit quantization for smaller model size
    config.pruningThreshold = 0.03f; // More aggressive pruning to reduce model size
    config.useNeuralEngine = false;  // Don't optimize for Neural Engine (focus on size)
    config.compressionLevel = 8;     // High compression level
    
    return optimizer.optimizeModel(inputModelPath, outputModelPath, config);
}

bool optimizeModelForAccuracy(const std::string& inputModelPath, const std::string& outputModelPath) {
    ModelOptimizer optimizer;
    OptimizationConfig config;
    
    // Configure for accuracy optimization
    config.quantizationBits = 0;    // No quantization to maintain full precision
    config.pruningThreshold = 0.0f; // No pruning to maintain accuracy
    config.useNeuralEngine = true;  // Use Neural Engine
    config.compressionLevel = 0;    // No compression
    
    return optimizer.optimizeModel(inputModelPath, outputModelPath, config);
}

// External C interface for model optimization
extern "C" {
    int FBNEO_OptimizeModel_ForSpeed(const char* inputPath, const char* outputPath) {
        return optimizeModelForSpeed(inputPath, outputPath) ? 1 : 0;
    }
    
    int FBNEO_OptimizeModel_ForSize(const char* inputPath, const char* outputPath) {
        return optimizeModelForSize(inputPath, outputPath) ? 1 : 0;
    }
    
    int FBNEO_OptimizeModel_ForAccuracy(const char* inputPath, const char* outputPath) {
        return optimizeModelForAccuracy(inputPath, outputPath) ? 1 : 0;
    }
    
    int FBNEO_OptimizeModel_Custom(const char* inputPath, const char* outputPath, 
                                int quantizeBits, float pruneThreshold, 
                                int useNeuralEngine, int compressionLevel) {
        ModelOptimizer optimizer;
        OptimizationConfig config;
        
        config.quantizationBits = quantizeBits;
        config.pruningThreshold = pruneThreshold;
        config.useNeuralEngine = (useNeuralEngine != 0);
        config.compressionLevel = compressionLevel;
        
        return optimizer.optimizeModel(inputPath, outputPath, config) ? 1 : 0;
    }
    
    // Add new dedicated functions for pruning and compression
    int FBNEO_PruneModelWeights(const char* inputPath, const char* outputPath, float threshold) {
        ModelOptimizer optimizer;
        return optimizer.pruneWeights(inputPath, outputPath, threshold) ? 1 : 0;
    }
    
    int FBNEO_CompressModel(const char* inputPath, const char* outputPath, int compressionLevel) {
        ModelOptimizer optimizer;
        return optimizer.compressModel(inputPath, outputPath, compressionLevel) ? 1 : 0;
    }
}

} // namespace ai
} // namespace fbneo 