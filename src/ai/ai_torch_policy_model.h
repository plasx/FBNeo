#pragma once

#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include <iostream>

// Forward declarations
namespace torch {
    namespace jit {
        class Module;
    }
}

namespace fs = std::filesystem;

#include "ai_input_frame.h"
#include "ai_output_action.h"

namespace AI {

/**
 * @brief Neural network policy model implementation using LibTorch
 * 
 * This class provides a C++ interface to load and run inference
 * with TorchScript models exported from PyTorch.
 */
class AITorchPolicyModel {
public:
    /**
     * @brief Constructor
     */
    AITorchPolicyModel();
    
    /**
     * @brief Destructor
     */
    ~AITorchPolicyModel();
    
    /**
     * @brief Load a TorchScript model from file
     * 
     * @param path Path to the model file (.pt format)
     * @return True if model loaded successfully
     */
    bool loadModel(const std::string& path);
    
    /**
     * @brief Run inference with the input frame
     * 
     * @param frame Input frame with game state
     * @return Predicted action to take
     */
    AIOutputAction predict(const AIInputFrame& frame);
    
    /**
     * @brief Run inference with raw feature vector
     * 
     * @param features Preprocessed input features
     * @return Predicted action to take
     */
    AIOutputAction predict(const std::vector<float>& features);
    
    /**
     * @brief Update model with new experience (online learning)
     * 
     * @param states Observed states
     * @param actions Actions taken
     * @param rewards Rewards received
     * @return True if update successful
     */
    bool update(const std::vector<AIInputFrame>& states,
                const std::vector<AIOutputAction>& actions,
                const std::vector<float>& rewards);
    
    /**
     * @brief Save model to file
     * 
     * @param output_path Path to save the model
     * @return True if save successful
     */
    bool saveModel(const std::string& output_path) const;
    
    /**
     * @brief Check if a model is loaded
     * 
     * @return True if model is loaded
     */
    bool isLoaded() const;
    
    /**
     * @brief Get the input size required by the model
     * 
     * @return Number of input features
     */
    size_t getInputSize() const;
    
    /**
     * @brief Get the output size produced by the model
     * 
     * @return Number of output values
     */
    size_t getOutputSize() const;
    
private:
    /**
     * @brief Preprocess an input frame to feature vector
     * 
     * @param frame Input frame to preprocess
     * @return Feature vector
     */
    std::vector<float> preprocess(const AIInputFrame& frame) const;
    
    /**
     * @brief Postprocess model output to action
     * 
     * @param output Raw model output
     * @return Processed action
     */
    AIOutputAction postprocess(const std::vector<float>& output) const;
    
    // Model and related fields
    std::shared_ptr<torch::jit::Module> model;
    std::string model_path;
    size_t input_size;
    size_t output_size;
};

} // namespace AI 