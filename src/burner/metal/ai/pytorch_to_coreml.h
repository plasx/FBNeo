#pragma once

#include <string>
#include <vector>

namespace fbneo {
namespace ai {

/**
 * @brief Initialize the PyTorch to CoreML conversion system
 * @return True if initialization was successful
 */
bool initializePyTorchToCoreMLSystem();

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
                           bool useNeuralEngine = true,
                           bool quantize = false);

/**
 * @brief Batch converts multiple PyTorch models to CoreML format
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
                              bool useNeuralEngine = true,
                              bool quantize = false,
                              int numThreads = 0);

/**
 * @brief Optimizes a CoreML model for a specific device
 * @param coreMLModelPath Path to the CoreML model file
 * @param outputPath Path for the optimized model
 * @param targetDevice Target device type (CPU, GPU, ANE)
 * @return True if optimization was successful
 */
bool optimizeCoreMLModel(const std::string& coreMLModelPath,
                        const std::string& outputPath,
                        const std::string& targetDevice = "ANE");

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
                           bool enableANE = true,
                           int quantizeBits = 0);

} // namespace ai
} // namespace fbneo

// C API for integration with non-C++ code
#ifdef __cplusplus
extern "C" {
#endif

int FBNEO_PyTorch_ToCoreML_Init();
    
int FBNEO_PyTorch_ToCoreML_Convert(const char* torchModelPath, 
                                 const char* coreMLOutputPath,
                                 const int* inputShape, 
                                 int shapeLen,
                                 int useNeuralEngine,
                                 int quantize);

int FBNEO_PyTorch_ToCoreML_Optimize(const char* coreMLModelPath,
                                  const char* outputPath,
                                  const char* targetDevice);

int FBNEO_PyTorch_ToCoreML_Enhanced(const char* torchModelPath,
                                  const char* coreMLOutputPath,
                                  const int* inputShape,
                                  int inputShapeLen,
                                  const int* outputShape,
                                  int outputShapeLen,
                                  int enableANE,
                                  int quantizeBits);

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
                                      int numThreads);

int FBNEO_PyTorch_ValidateCoreMLModel(const char* modelPath, 
                                    int iterations, 
                                    const char* computeUnit);

#ifdef __cplusplus
}
#endif 