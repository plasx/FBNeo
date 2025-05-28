#include "ai_torch_policy.h"
#include "ai_input_frame.h"
#include "ai_output_action.h"
#include <iostream>
#include <filesystem>
#include <chrono>
#include <sstream>
#include <unordered_map>

// LibTorch includes
#include <torch/script.h>
#include <torch/torch.h>

namespace fs = std::filesystem;

namespace AI {

// Global instance
AITorchPolicy* g_pAITorchPolicy = nullptr;

// Private implementation
class AITorchPolicy::Impl {
public:
    Impl() 
        : m_bInitialized(false) 
    {
        // Initialize arrays to hold per-player data
        m_lastInferenceTimes.resize(MAX_PLAYERS, 0.0f);
        m_modelPaths.resize(MAX_PLAYERS);
        m_modelInfos.resize(MAX_PLAYERS);
        
#ifdef USE_LIBTORCH
        // Reserve models for each player
        m_models.resize(MAX_PLAYERS);
#endif
    }
    
    ~Impl() {
        // Clean up resources
        Shutdown();
    }
    
    bool Initialize() {
        // Already initialized?
        if (m_bInitialized) {
            return true;
        }
        
#ifdef USE_LIBTORCH
        try {
            // Set torch settings
            torch::NoGradGuard no_grad;
            
            m_bInitialized = true;
            return true;
        }
        catch (const std::exception& e) {
            // Log error and consider initialization failed
            std::cerr << "LibTorch initialization error: " << e.what() << std::endl;
            return false;
        }
#else
        // When LibTorch is not available
        std::cerr << "LibTorch is not available. Torch policy will not be functional." << std::endl;
        return false;
#endif
    }
    
    bool LoadModel(const char* modelPath, int playerIndex) {
        // Validate input
        if (!modelPath || playerIndex < 0 || playerIndex >= MAX_PLAYERS) {
            return false;
        }
        
        // Not initialized?
        if (!m_bInitialized && !Initialize()) {
            return false;
        }
        
#ifdef USE_LIBTORCH
        try {
            // Load the model
            torch::jit::script::Module model = torch::jit::load(modelPath);
            model.eval();
            
            // Store the model
            m_models[playerIndex] = std::make_shared<torch::jit::script::Module>(std::move(model));
            
            // Store path for reference
            m_modelPaths[playerIndex] = modelPath;
            
            // Get and store model info if possible
            try {
                // Create a small dummy input to get model info
                std::vector<torch::jit::IValue> inputs;
                inputs.push_back(torch::ones({1, 20})); // Assuming 20 input dimensions
                
                // Run a test inference
                auto output = m_models[playerIndex]->forward(inputs);
                
                // Get output shape for info
                if (output.isTensor()) {
                    auto outputTensor = output.toTensor();
                    std::stringstream ss;
                    ss << "Model: " << modelPath << " | ";
                    ss << "Input: " << 20 << " | "; // Hardcoded for now
                    ss << "Output: " << outputTensor.sizes()[1];
                    m_modelInfos[playerIndex] = ss.str();
                }
            }
            catch (const std::exception& e) {
                // Not critical - just set basic info
                m_modelInfos[playerIndex] = std::string("Model: ") + modelPath;
            }
            
            return true;
        }
        catch (const std::exception& e) {
            // Log error
            std::cerr << "Error loading model: " << e.what() << std::endl;
            return false;
        }
#else
        // When LibTorch is not available
        std::cerr << "LibTorch is not available. Cannot load model." << std::endl;
        return false;
#endif
    }
    
    bool IsModelLoaded(int playerIndex) const {
        if (playerIndex < 0 || playerIndex >= MAX_PLAYERS) {
            return false;
        }
        
#ifdef USE_LIBTORCH
        return m_models[playerIndex] != nullptr;
#else
        return false;
#endif
    }
    
    std::vector<float> RunInference(const float* inputValues, size_t inputSize, int playerIndex) {
        // Validate input
        if (!inputValues || inputSize == 0 || playerIndex < 0 || playerIndex >= MAX_PLAYERS) {
            return std::vector<float>(); // Empty result
        }
        
        // Check if model is loaded
        if (!IsModelLoaded(playerIndex)) {
            std::cerr << "No model loaded for player " << playerIndex << std::endl;
            return std::vector<float>(); // Empty result
        }
        
#ifdef USE_LIBTORCH
        try {
            // Measure inference time
            auto startTime = std::chrono::high_resolution_clock::now();
            
            // No gradient computation for inference
            torch::NoGradGuard no_grad;
            
            // Create input tensor
            torch::Tensor inputTensor = torch::from_blob(
                const_cast<float*>(inputValues), 
                {1, static_cast<long>(inputSize)}, 
                torch::kFloat32
            );
            
            // Create inputs list
            std::vector<torch::jit::IValue> inputs;
            inputs.push_back(inputTensor);
            
            // Run inference
            auto output = m_models[playerIndex]->forward(inputs);
            
            // Process output
            torch::Tensor outputTensor;
            if (output.isTensor()) {
                outputTensor = output.toTensor();
            }
            else if (output.isTuple()) {
                // Handle tuple output (common in some model architectures)
                auto outputTuple = output.toTuple();
                outputTensor = outputTuple->elements()[0].toTensor();
            }
            
            // Apply sigmoid to get probabilities (if needed)
            // Assuming model outputs logits rather than probabilities
            outputTensor = torch::sigmoid(outputTensor);
            
            // Convert to vector
            std::vector<float> result(outputTensor.data_ptr<float>(),
                                    outputTensor.data_ptr<float>() + outputTensor.numel());
            
            // Update inference time
            auto endTime = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float, std::milli> duration = endTime - startTime;
            m_lastInferenceTimes[playerIndex] = duration.count();
            
            return result;
        }
        catch (const std::exception& e) {
            // Log error
            std::cerr << "Inference error: " << e.what() << std::endl;
            return std::vector<float>(); // Empty result
        }
#else
        // When LibTorch is not available
        std::cerr << "LibTorch is not available. Cannot run inference." << std::endl;
        return std::vector<float>(); // Empty result
#endif
    }
    
    float GetLastInferenceTime(int playerIndex) const {
        if (playerIndex < 0 || playerIndex >= MAX_PLAYERS) {
            return 0.0f;
        }
        
        return m_lastInferenceTimes[playerIndex];
    }
    
    std::string GetModelInfo(int playerIndex) const {
        if (playerIndex < 0 || playerIndex >= MAX_PLAYERS) {
            return "Invalid player index";
        }
        
        if (!IsModelLoaded(playerIndex)) {
            return "No model loaded";
        }
        
        return m_modelInfos[playerIndex];
    }
    
    void Shutdown() {
        if (!m_bInitialized) {
            return;
        }
        
#ifdef USE_LIBTORCH
        // Clear models
        for (auto& model : m_models) {
            model.reset();
        }
#endif
        
        m_bInitialized = false;
    }
    
private:
    static const int MAX_PLAYERS = 4;
    bool m_bInitialized;
    
    // Per-player data
    std::vector<float> m_lastInferenceTimes;
    std::vector<std::string> m_modelPaths;
    std::vector<std::string> m_modelInfos;
    
#ifdef USE_LIBTORCH
    std::vector<std::shared_ptr<torch::jit::script::Module>> m_models;
#endif
};

// Constructor & destructor
AITorchPolicy::AITorchPolicy() : m_pImpl(new Impl()) {
}

AITorchPolicy::~AITorchPolicy() {
}

// Public interface methods
bool AITorchPolicy::Initialize() {
    return m_pImpl->Initialize();
}

bool AITorchPolicy::LoadModel(const char* modelPath, int playerIndex) {
    return m_pImpl->LoadModel(modelPath, playerIndex);
}

bool AITorchPolicy::IsModelLoaded(int playerIndex) const {
    return m_pImpl->IsModelLoaded(playerIndex);
}

std::vector<float> AITorchPolicy::RunInference(const float* inputValues, size_t inputSize, int playerIndex) {
    return m_pImpl->RunInference(inputValues, inputSize, playerIndex);
}

float AITorchPolicy::GetLastInferenceTime(int playerIndex) const {
    return m_pImpl->GetLastInferenceTime(playerIndex);
}

std::string AITorchPolicy::GetModelInfo(int playerIndex) const {
    return m_pImpl->GetModelInfo(playerIndex);
}

void AITorchPolicy::Shutdown() {
    m_pImpl->Shutdown();
}

AITorchPolicyModel::AITorchPolicyModel()
    : input_size(0), output_size(0) {
    // Initialize with empty model
}

AITorchPolicyModel::~AITorchPolicyModel() {
    // Clean up resources (LibTorch handles most cleanup via shared_ptr)
}

bool AITorchPolicyModel::loadModel(const std::string& path) {
    try {
        // Check if file exists
        if (!fs::exists(path)) {
            std::cerr << "Model file not found: " << path << std::endl;
            return false;
        }
        
        // Load the TorchScript model
        model = std::make_shared<torch::jit::Module>(torch::jit::load(path));
        model_path = path;
        
        // Model is now loaded in evaluation mode
        model->eval();
        
        // Try to determine input/output sizes by examining model metadata
        // This would typically be stored in the model itself or in a companion file
        
        // Default to reasonable values if not available
        input_size = 20; // Match AIInputFrame::toVector size
        output_size = static_cast<size_t>(AIAction::ACTION_COUNT);
        
        std::cout << "Model loaded from: " << path << std::endl;
        std::cout << "Input size: " << input_size << ", Output size: " << output_size << std::endl;
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading model: " << e.what() << std::endl;
        model.reset();
        return false;
    }
}

AIOutputAction AITorchPolicyModel::predict(const AIInputFrame& frame) {
    // Preprocess frame into features
    std::vector<float> features = preprocess(frame);
    
    // Forward through model
    return predict(features);
}

AIOutputAction AITorchPolicyModel::predict(const std::vector<float>& features) {
    if (!isLoaded()) {
        std::cerr << "Model not loaded" << std::endl;
        return AIOutputAction(); // Return default action
    }
    
    try {
        // Check input size
        if (features.size() != input_size) {
            std::cerr << "Input feature size mismatch. Expected: " 
                      << input_size << ", Got: " << features.size() << std::endl;
            return AIOutputAction(); // Return default action
        }
        
        // Convert features to torch tensor
        torch::Tensor input_tensor = torch::from_blob(
            const_cast<float*>(features.data()),
            {1, static_cast<long>(features.size())},
            torch::kFloat
        );
        
        // Prepare input list (model might expect a list of tensors)
        std::vector<torch::jit::IValue> inputs;
        inputs.push_back(input_tensor);
        
        // Forward pass (with no gradient calculation)
        torch::NoGradGuard no_grad; // Disable gradients for inference
        torch::Tensor output = model->forward(inputs).toTensor();
        
        // Convert output tensor to vector
        std::vector<float> output_vec(output.data_ptr<float>(), 
                                      output.data_ptr<float>() + output.numel());
        
        // Postprocess output to get action
        return postprocess(output_vec);
    } catch (const std::exception& e) {
        std::cerr << "Error in prediction: " << e.what() << std::endl;
        return AIOutputAction(); // Return default action
    }
}

bool AITorchPolicyModel::update(const std::vector<AIInputFrame>& states, 
                                const std::vector<AIOutputAction>& actions, 
                                const std::vector<float>& rewards) {
    // Online learning would be implemented here
    // This is quite complex for a full PPO implementation
    
    // For now, this is a placeholder. Full implementation would require:
    // 1. Setting the model to train mode
    // 2. Creating batches of experience
    // 3. Computing value and policy losses
    // 4. Computing gradients and updating weights
    
    std::cerr << "Online model update not fully implemented" << std::endl;
    return false;
}

bool AITorchPolicyModel::saveModel(const std::string& output_path) const {
    if (!isLoaded()) {
        std::cerr << "No model to save" << std::endl;
        return false;
    }
    
    try {
        // Create directory if needed
        fs::path path(output_path);
        fs::path dir = path.parent_path();
        if (!dir.empty() && !fs::exists(dir)) {
            fs::create_directories(dir);
        }
        
        // Save the model
        model->save(output_path);
        
        std::cout << "Model saved to: " << output_path << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving model: " << e.what() << std::endl;
        return false;
    }
}

bool AITorchPolicyModel::isLoaded() const {
    return model != nullptr;
}

size_t AITorchPolicyModel::getInputSize() const {
    return input_size;
}

size_t AITorchPolicyModel::getOutputSize() const {
    return output_size;
}

std::vector<float> AITorchPolicyModel::preprocess(const AIInputFrame& frame) const {
    // Convert frame to feature vector
    return frame.toVector();
}

AIOutputAction AITorchPolicyModel::postprocess(const std::vector<float>& output) const {
    // Assuming output is a probability distribution over actions
    
    if (output.empty() || output.size() != output_size) {
        std::cerr << "Invalid output size" << std::endl;
        return AIOutputAction(); // Return default action
    }
    
    // Find the action with highest probability
    size_t best_action_idx = 0;
    float best_prob = output[0];
    
    for (size_t i = 1; i < output.size(); ++i) {
        if (output[i] > best_prob) {
            best_prob = output[i];
            best_action_idx = i;
        }
    }
    
    // Convert to AIAction
    AIAction action = static_cast<AIAction>(best_action_idx);
    
    // Create and return action with confidence
    return AIOutputAction(action, best_prob);
}

} // namespace AI 