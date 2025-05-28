#pragma once

#include "ai_input_frame.h"
#include "ai_output_action.h"
#include <string>
#include <vector>
#include <memory>

namespace fbneo {
namespace ai {

/**
 * @brief PyTorch-based policy model for the AI system
 * 
 * This class encapsulates a neural network policy that takes game state
 * as input and produces actions as output, along with value estimates.
 * It handles both inference and training using PyTorch as the backend.
 */
class AITorchPolicy {
public:
    /**
     * @brief Default constructor
     */
    AITorchPolicy();
    
    /**
     * @brief Constructor with model path
     * 
     * @param modelPath Path to load model from
     */
    explicit AITorchPolicy(const std::string& modelPath);
    
    /**
     * @brief Destructor
     */
    ~AITorchPolicy();
    
    /**
     * @brief Initialize the policy with input and output dimensions
     * 
     * @param inputDims Dimensions of the input tensor
     * @param numActions Number of possible actions
     * @return True if initialization was successful
     */
    bool initialize(const std::vector<int>& inputDims, int numActions);
    
    /**
     * @brief Load a model from disk
     * 
     * @param path Path to load model from
     * @return True if load was successful
     */
    bool load(const std::string& path);
    
    /**
     * @brief Save the model to disk
     * 
     * @param path Path to save model to
     * @return True if save was successful
     */
    bool save(const std::string& path);
    
    /**
     * @brief Predict an action for a given state
     * 
     * @param state Input state to process
     * @param action Output action to fill
     * @param exploit Whether to exploit (true) or explore (false)
     * @return True if prediction was successful
     */
    bool predict(const AIInputFrame& state, AIOutputAction& action, bool exploit = true);
    
    /**
     * @brief Get the value estimate for a state
     * 
     * @param state Input state to process
     * @return The value estimate
     */
    float getValue(const AIInputFrame& state);
    
    /**
     * @brief Get action probabilities for a state
     * 
     * @param state Input state to process
     * @return Vector of action probabilities
     */
    std::vector<float> getActionProbabilities(const AIInputFrame& state);
    
    /**
     * @brief Update the model with a batch of experiences
     * 
     * @param states Batch of states
     * @param actions Batch of actions
     * @param oldLogProbs Batch of old log probabilities
     * @param advantages Batch of advantages
     * @param returns Batch of returns
     * @param learningRate Learning rate to use
     * @return The loss value
     */
    float update(const std::vector<std::vector<float>>& states,
                const std::vector<std::vector<float>>& actions,
                const std::vector<float>& oldLogProbs,
                const std::vector<float>& advantages,
                const std::vector<float>& returns,
                float learningRate);
    
    /**
     * @brief Create a copy of this policy
     * 
     * @return A new policy object that is a copy of this one
     */
    AITorchPolicy* clone();
    
    /**
     * @brief Copy weights from another policy
     * 
     * @param other Policy to copy from
     */
    void copyFrom(const AITorchPolicy* other);
    
    /**
     * @brief Export the model to a specific format
     * 
     * @param path Path to export to
     * @param format Format to export to (e.g., "onnx", "coreml")
     * @return True if export was successful
     */
    bool exportTo(const std::string& path, const std::string& format);
    
    /**
     * @brief Set the model architecture
     * 
     * @param architecture Architecture description (e.g., "cnn", "lstm")
     * @param params Parameters for the architecture
     * @return True if architecture was set successfully
     */
    bool setArchitecture(const std::string& architecture, const std::vector<int>& params);
    
    /**
     * @brief Enable or disable training mode
     * 
     * @param trainable Whether the model should be trainable
     */
    void setTrainable(bool trainable);
    
    /**
     * @brief Check if the model is trainable
     * 
     * @return True if the model is trainable
     */
    bool isTrainable() const;
    
    /**
     * @brief Get the input dimensions
     * 
     * @return The input dimensions
     */
    const std::vector<int>& getInputDims() const;
    
    /**
     * @brief Get the number of actions
     * 
     * @return The number of actions
     */
    int getNumActions() const;
    
private:
    // This would be implemented with actual PyTorch integration
    // For this stub, we'll just have some placeholders
    
    struct PolicyImpl;
    std::unique_ptr<PolicyImpl> m_impl;
    
    std::vector<int> m_inputDims;
    int m_numActions;
    bool m_trainable;
};

} // namespace ai
} // namespace fbneo 