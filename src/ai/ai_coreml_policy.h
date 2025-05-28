#pragma once

#include "ai_input_frame.h"
#include "ai_output_action.h"
#include <string>

namespace AI {

// Forward declaration for CoreML types
typedef void* CoreMLModelRef;

/**
 * @class AICoreMLPolicyModel
 * @brief Interface for Apple CoreML policy models
 *
 * This class provides functionality to load and run inference with CoreML models
 * on macOS and iOS devices. It leverages the Apple Neural Engine for optimized 
 * inference when available.
 */
class AICoreMLPolicyModel {
public:
    /**
     * @brief Constructor
     */
    AICoreMLPolicyModel();
    
    /**
     * @brief Destructor
     */
    ~AICoreMLPolicyModel();
    
    /**
     * @brief Load a CoreML model from a file
     *
     * @param modelPath Path to the .mlmodel or .mlmodelc file
     * @return true if model loaded successfully, false otherwise
     */
    bool loadModel(const std::string& modelPath);
    
    /**
     * @brief Check if a model is loaded
     *
     * @return true if a model is loaded, false otherwise
     */
    bool isModelLoaded() const;
    
    /**
     * @brief Run inference on the model to predict an action
     *
     * @param input The game state input
     * @return Predicted action with confidence
     */
    AIOutputAction predict(const AIInputFrame& input);
    
private:
    // CoreML model reference
    CoreMLModelRef m_model;
    
    // Model path
    std::string m_modelPath;
    
    // Model loaded flag
    bool m_isLoaded;
    
    // Input shape
    int m_inputSize;
};

} // namespace AI 