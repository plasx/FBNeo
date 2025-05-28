#pragma once

#include <string>
#include <vector>
#include <memory>

/**
 * @brief Interface for torch-based neural network policy
 * 
 * This class provides an interface for loading and running inference
 * with PyTorch models (via LibTorch) for AI decision making.
 */
class AITorchPolicy {
public:
    /**
     * @brief Constructor
     */
    AITorchPolicy();
    
    /**
     * @brief Destructor
     */
    ~AITorchPolicy();
    
    /**
     * @brief Initialize the policy
     * @return True if initialization succeeded
     */
    bool Initialize();
    
    /**
     * @brief Load a model from file
     * 
     * @param modelPath Path to the model file (.pt or .pth format)
     * @param playerIndex The player this model will control (0-based)
     * @return True if model loaded successfully
     */
    bool LoadModel(const char* modelPath, int playerIndex = 0);
    
    /**
     * @brief Check if a model is loaded
     * 
     * @param playerIndex The player to check (0-based)
     * @return True if a model is loaded for this player
     */
    bool IsModelLoaded(int playerIndex = 0) const;
    
    /**
     * @brief Run inference with the loaded model
     * 
     * @param inputValues Pointer to input values array
     * @param inputSize Size of input array
     * @param playerIndex Player index for model selection (0-based)
     * @return Vector of output action probabilities
     */
    std::vector<float> RunInference(const float* inputValues, size_t inputSize, int playerIndex = 0);
    
    /**
     * @brief Get the last inference time in milliseconds
     * 
     * @param playerIndex Player index (0-based)
     * @return Inference time in milliseconds
     */
    float GetLastInferenceTime(int playerIndex = 0) const;
    
    /**
     * @brief Get information about the loaded model
     * 
     * @param playerIndex Player index (0-based)
     * @return String with model information
     */
    std::string GetModelInfo(int playerIndex = 0) const;
    
    /**
     * @brief Shutdown the policy and release resources
     */
    void Shutdown();
    
private:
    // Private implementation
    class Impl;
    std::unique_ptr<Impl> m_pImpl;
};

// Global instance
extern AITorchPolicy* g_pAITorchPolicy; 