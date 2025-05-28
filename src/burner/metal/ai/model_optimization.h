#pragma once

#include <string>
#include <vector>

namespace fbneo {
namespace ai {

/**
 * @brief Configuration for model optimization
 */
struct OptimizationConfig {
    int quantizationBits;     // Quantization bits (0=none, 8=8bit, 16=fp16)
    float pruningThreshold;   // Threshold for weight pruning (0.0=no pruning)
    bool useNeuralEngine;     // Whether to optimize for Neural Engine
    int compressionLevel;     // Compression level (0-9, 0=none)
    
    OptimizationConfig() 
        : quantizationBits(0), 
          pruningThreshold(0.0f), 
          useNeuralEngine(true),
          compressionLevel(0) {}
};

/**
 * @brief Model optimizer for FBNeo AI models
 * 
 * This class provides techniques for optimizing neural network models:
 * - Quantization: Reduces precision of weights to decrease model size
 * - Pruning: Removes small weights to decrease model size and increase inference speed
 * - Neural Engine optimization: Adapts the model for efficient execution on Apple Neural Engine
 * - Compression: Reduces model file size
 */
class ModelOptimizer {
public:
    /**
     * @brief Constructor
     */
    ModelOptimizer();
    
    /**
     * @brief Destructor
     */
    ~ModelOptimizer();
    
    /**
     * @brief Optimize a model with given configuration
     * 
     * @param inputModelPath Path to the input model
     * @param outputModelPath Path to save the optimized model
     * @param config Optimization configuration
     * @return True if optimization was successful
     */
    bool optimizeModel(const std::string& inputModelPath, 
                      const std::string& outputModelPath,
                      const OptimizationConfig& config);
    
    /**
     * @brief Prune weights in a model
     * 
     * This function removes weights below the specified threshold to reduce
     * model size and potentially improve inference speed.
     * 
     * @param inputModelPath Path to the input model
     * @param outputModelPath Path to save the pruned model
     * @param pruningThreshold Threshold for weight pruning (weights with absolute value below this are zeroed)
     * @return True if pruning was successful
     */
    bool pruneWeights(const std::string& inputModelPath, 
                     const std::string& outputModelPath, 
                     float pruningThreshold);
    
    /**
     * @brief Compress a model to reduce file size
     * 
     * This function applies various compression techniques to reduce model file size,
     * including quantization, pruning, and file-level compression depending on the
     * compression level.
     * 
     * @param inputModelPath Path to the input model
     * @param outputModelPath Path to save the compressed model
     * @param compressionLevel Compression level (1-9, higher = more compression but potentially lower accuracy)
     * @return True if compression was successful
     */
    bool compressModel(const std::string& inputModelPath, 
                      const std::string& outputModelPath, 
                      int compressionLevel);
    
    /**
     * @brief Set quantization bits
     * 
     * @param bits Quantization bits (0=none, 8=8bit, 16=fp16)
     */
    void setQuantizationBits(int bits);
    
    /**
     * @brief Set pruning threshold
     * 
     * @param threshold Threshold for weight pruning (0.0=no pruning)
     */
    void setPruningThreshold(float threshold);
    
    /**
     * @brief Set whether to optimize for Neural Engine
     * 
     * @param useNeuralEngine Whether to optimize for Neural Engine
     */
    void setUseNeuralEngine(bool useNeuralEngine);
    
    /**
     * @brief Set compression level
     * 
     * @param level Compression level (0-9, 0=none)
     */
    void setCompressionLevel(int level);
    
private:
    /**
     * @brief Create the optimization script
     * 
     * @return Path to the created script, or empty string on failure
     */
    std::string createOptimizationScript();
    
    int m_quantizationBits;      // Quantization bits
    float m_pruningThreshold;    // Pruning threshold
    bool m_useNeuralEngine;      // Whether to optimize for Neural Engine
    int m_compressionLevel;      // Compression level
};

/**
 * @brief Optimize a model for speed
 * 
 * This function applies optimizations focused on inference speed:
 * - FP16 quantization for faster computation
 * - No pruning to maintain accuracy
 * - Neural Engine optimization for hardware acceleration
 * - No compression to avoid decompression overhead
 * 
 * @param inputModelPath Path to the input model
 * @param outputModelPath Path to save the optimized model
 * @return True if optimization was successful
 */
bool optimizeModelForSpeed(const std::string& inputModelPath, const std::string& outputModelPath);

/**
 * @brief Optimize a model for size
 * 
 * This function applies optimizations focused on model size:
 * - 8-bit quantization for smaller weights
 * - Light pruning for smaller size with minimal accuracy loss
 * - Neural Engine optimization for hardware acceleration
 * - Medium compression level
 * 
 * @param inputModelPath Path to the input model
 * @param outputModelPath Path to save the optimized model
 * @return True if optimization was successful
 */
bool optimizeModelForSize(const std::string& inputModelPath, const std::string& outputModelPath);

/**
 * @brief Optimize a model for accuracy
 * 
 * This function applies optimizations focused on inference accuracy:
 * - No quantization to maintain full precision
 * - No pruning to maintain accuracy
 * - Neural Engine optimization for hardware acceleration
 * - No compression
 * 
 * @param inputModelPath Path to the input model
 * @param outputModelPath Path to save the optimized model
 * @return True if optimization was successful
 */
bool optimizeModelForAccuracy(const std::string& inputModelPath, const std::string& outputModelPath);

// External C interface for model optimization
extern "C" {
    /**
     * @brief Optimize a model for speed
     * 
     * @param inputPath Path to the input model
     * @param outputPath Path to save the optimized model
     * @return 1 if successful, 0 if failed
     */
    int FBNEO_OptimizeModel_ForSpeed(const char* inputPath, const char* outputPath);
    
    /**
     * @brief Optimize a model for size
     * 
     * @param inputPath Path to the input model
     * @param outputPath Path to save the optimized model
     * @return 1 if successful, 0 if failed
     */
    int FBNEO_OptimizeModel_ForSize(const char* inputPath, const char* outputPath);
    
    /**
     * @brief Optimize a model for accuracy
     * 
     * @param inputPath Path to the input model
     * @param outputPath Path to save the optimized model
     * @return 1 if successful, 0 if failed
     */
    int FBNEO_OptimizeModel_ForAccuracy(const char* inputPath, const char* outputPath);
    
    /**
     * @brief Optimize a model with custom parameters
     * 
     * @param inputPath Path to the input model
     * @param outputPath Path to save the optimized model
     * @param quantizeBits Quantization bits (0=none, 8=8bit, 16=fp16)
     * @param pruneThreshold Threshold for weight pruning (0.0=no pruning)
     * @param useNeuralEngine Whether to optimize for Neural Engine (0=no, 1=yes)
     * @param compressionLevel Compression level (0-9, 0=none)
     * @return 1 if successful, 0 if failed
     */
    int FBNEO_OptimizeModel_Custom(const char* inputPath, const char* outputPath, 
                                int quantizeBits, float pruneThreshold, 
                                int useNeuralEngine, int compressionLevel);
    
    /**
     * @brief Prune weights in a model
     * 
     * This function removes weights below the specified threshold to reduce
     * model size and potentially improve inference speed.
     * 
     * @param inputPath Path to the input model
     * @param outputPath Path to save the pruned model
     * @param threshold Threshold for weight pruning (weights with absolute value below this are zeroed)
     * @return 1 if successful, 0 if failed
     */
    int FBNEO_PruneModelWeights(const char* inputPath, const char* outputPath, float threshold);
    
    /**
     * @brief Compress a model to reduce file size
     * 
     * This function applies various compression techniques to reduce model file size,
     * including quantization, pruning, and file-level compression depending on the
     * compression level.
     * 
     * @param inputPath Path to the input model
     * @param outputPath Path to save the compressed model
     * @param compressionLevel Compression level (1-9, higher = more compression but potentially lower accuracy)
     * @return 1 if successful, 0 if failed
     */
    int FBNEO_CompressModel(const char* inputPath, const char* outputPath, int compressionLevel);
}

} // namespace ai
} // namespace fbneo 