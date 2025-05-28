#include "ai_controller.h"
#include "ai_memory_mapping.h"
#include "ai_torch_policy.h"
#include "ai_input_frame.h"
#include "burner.h"  // For input handling
#include <chrono>
#include <sstream>
#include <algorithm>
#include <iomanip>

// Global instance
AIController* g_pAIController = nullptr;

// Private implementation
class AIController::Impl {
public:
    Impl() 
        : m_bEnabled(false)
        , m_eMode(AIControllerMode::Disabled)
        , m_fAssistThreshold(0.7f)
        , m_nPlayerIndex(0)
        , m_pMemoryMapping(nullptr)
        , m_pTorchPolicy(nullptr)
        , m_fLastInferenceTime(0.0f)
        , m_nConsecutiveFrames(0)
    {
        // Initialize vectors with default values
        m_vPredictedActions.resize(16, 0.0f);
        m_vAppliedActions.resize(16, 0.0f);
        
        // Get references to global objects
        m_pMemoryMapping = g_pAIMemoryMapping;
        m_pTorchPolicy = g_pAITorchPolicy;
    }
    
    ~Impl() {
        // Nothing to do - we don't own the pointers
    }
    
    // Settings
    bool m_bEnabled;
    AIControllerMode m_eMode;
    float m_fAssistThreshold;
    int m_nPlayerIndex;
    
    // Components
    AIMemoryMapping* m_pMemoryMapping;
    AITorchPolicy* m_pTorchPolicy;
    
    // State
    std::string m_strLoadedModelPath;
    std::vector<float> m_vPredictedActions;
    std::vector<float> m_vAppliedActions;
    float m_fLastInferenceTime;
    int m_nConsecutiveFrames;
    
    // Performance monitoring
    std::chrono::time_point<std::chrono::high_resolution_clock> m_lastUpdateTime;
    
    // Callbacks
    std::function<void(const std::vector<float>&)> m_decisionCallback;
    
    // Model loading
    bool LoadModel(const std::string& modelPath) {
        if (!m_pTorchPolicy) {
            printf("Error: AITorchPolicy not initialized\n");
            return false;
        }
        
        // Try to load the model
        bool success = m_pTorchPolicy->LoadModel(modelPath.c_str(), m_nPlayerIndex);
        
        if (success) {
            m_strLoadedModelPath = modelPath;
            printf("Successfully loaded AI model: %s\n", modelPath.c_str());
        } else {
            printf("Failed to load AI model: %s\n", modelPath.c_str());
        }
        
        return success;
    }
    
    // Game state extraction
    AIInputFrame ExtractGameState() const {
        // Make sure memory mapping is available
        if (!m_pMemoryMapping) {
            return AIInputFrame(); // Return empty frame
        }
        
        // Extract state using memory mapping
        return AIInputFrame::extractFromMemory(*m_pMemoryMapping);
    }
    
    // Inference
    std::vector<float> RunInference(const AIInputFrame& inputFrame) {
        if (!m_pTorchPolicy || !m_bEnabled) {
            return std::vector<float>(16, 0.0f); // Return empty actions
        }
        
        // Track time for performance monitoring
        auto startTime = std::chrono::high_resolution_clock::now();
        
        // Convert input frame to tensor and run inference
        std::vector<float> inputVector = inputFrame.toVector();
        std::vector<float> outputVector = m_pTorchPolicy->RunInference(
            inputVector.data(), inputVector.size(), m_nPlayerIndex);
        
        // Calculate inference time
        auto endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float, std::milli> duration = endTime - startTime;
        m_fLastInferenceTime = duration.count();
        
        // Call decision callback if registered
        if (m_decisionCallback) {
            m_decisionCallback(outputVector);
        }
        
        return outputVector;
    }
    
    // Action application
    void ApplyActions(const std::vector<float>& actions) {
        if (!m_bEnabled || m_eMode == AIControllerMode::Disabled || m_eMode == AIControllerMode::WatchOnly) {
            return;
        }
        
        // Store actions for debugging/display
        m_vPredictedActions = actions;
        
        // Determine which actions to apply based on confidence
        std::vector<float> appliedActions = actions;
        
        // In assist mode, only apply actions that meet the threshold
        if (m_eMode == AIControllerMode::AssistMode) {
            for (auto& action : appliedActions) {
                if (action < m_fAssistThreshold) {
                    action = 0.0f;
                }
            }
        }
        
        // Convert actions to input bits
        uint32_t inputBits = 0;
        
        // Mapping from action index to input bit
        // This depends on the exact action encoding from the model
        struct ActionMapping {
            int actionIndex;
            uint32_t inputBit;
        };
        
        // Define mappings (adjust based on actual game input system)
        ActionMapping mappings[] = {
            { 0, 0x0001 }, // UP
            { 1, 0x0002 }, // DOWN
            { 2, 0x0004 }, // LEFT
            { 3, 0x0008 }, // RIGHT
            { 4, 0x0010 }, // BUTTON 1
            { 5, 0x0020 }, // BUTTON 2
            { 6, 0x0040 }, // BUTTON 3
            { 7, 0x0080 }, // BUTTON 4
            { 8, 0x0100 }, // BUTTON 5
            { 9, 0x0200 }, // BUTTON 6
        };
        
        // Set input bits based on action values
        for (const auto& mapping : mappings) {
            if (mapping.actionIndex < appliedActions.size() && 
                appliedActions[mapping.actionIndex] > 0.5f) {
                inputBits |= mapping.inputBit;
            }
        }
        
        // Apply to the correct player's inputs
        // This depends on the FBNeo input system structure
        if (m_nPlayerIndex == 0) {
            // Apply to player 1 inputs
            // Example (may need adjustment based on actual FBNeo input system):
            nInputs[0] = inputBits;
        } else if (m_nPlayerIndex == 1) {
            // Apply to player 2 inputs
            nInputs[1] = inputBits;
        }
        
        // Store applied actions for debugging
        m_vAppliedActions = appliedActions;
    }
    
    // Debug info
    std::string GetDebugInfo() const {
        std::stringstream ss;
        
        // Format basic info
        ss << "AI Controller - ";
        ss << (m_bEnabled ? "Enabled" : "Disabled") << " | ";
        
        // Mode
        switch (m_eMode) {
            case AIControllerMode::Disabled:
                ss << "Mode: Disabled";
                break;
            case AIControllerMode::FullControl:
                ss << "Mode: Full Control";
                break;
            case AIControllerMode::AssistMode:
                ss << "Mode: Assist (Threshold: " << m_fAssistThreshold << ")";
                break;
            case AIControllerMode::WatchOnly:
                ss << "Mode: Watch Only";
                break;
        }
        
        ss << " | Player: " << (m_nPlayerIndex + 1) << "\n";
        
        // Model info
        ss << "Model: " << (!m_strLoadedModelPath.empty() ? m_strLoadedModelPath : "None") << "\n";
        
        // Performance
        ss << "Inference time: " << std::fixed << std::setprecision(2) << m_fLastInferenceTime << "ms\n";
        
        // Action predictions
        ss << "Actions: ";
        for (size_t i = 0; i < std::min(size_t(10), m_vPredictedActions.size()); ++i) {
            float val = m_vPredictedActions[i];
            ss << std::fixed << std::setprecision(2) << val << " ";
        }
        
        return ss.str();
    }
};

// Constructor & destructor
AIController::AIController() : m_pImpl(new Impl()) {
}

AIController::~AIController() {
}

// Static initialization & access
void AIController::Initialize() {
    if (!g_pAIController) {
        g_pAIController = new AIController();
    }
}

void AIController::Shutdown() {
    if (g_pAIController) {
        delete g_pAIController;
        g_pAIController = nullptr;
    }
}

AIController& AIController::GetInstance() {
    if (!g_pAIController) {
        Initialize();
    }
    return *g_pAIController;
}

// Public interface methods
void AIController::SetEnabled(bool enabled) {
    m_pImpl->m_bEnabled = enabled;
    
    // If disabling, reset the mode
    if (!enabled) {
        m_pImpl->m_eMode = AIControllerMode::Disabled;
    } else if (m_pImpl->m_eMode == AIControllerMode::Disabled) {
        // If enabling, set a default mode if currently disabled
        m_pImpl->m_eMode = AIControllerMode::FullControl;
    }
}

bool AIController::IsEnabled() const {
    return m_pImpl->m_bEnabled;
}

void AIController::SetMode(AIControllerMode mode) {
    m_pImpl->m_eMode = mode;
}

AIControllerMode AIController::GetMode() const {
    return m_pImpl->m_eMode;
}

void AIController::SetAssistThreshold(float threshold) {
    // Clamp the threshold to valid range
    m_pImpl->m_fAssistThreshold = std::max(0.0f, std::min(1.0f, threshold));
}

void AIController::SetPlayerIndex(int playerIndex) {
    m_pImpl->m_nPlayerIndex = playerIndex;
}

int AIController::GetPlayerIndex() const {
    return m_pImpl->m_nPlayerIndex;
}

bool AIController::LoadModel(const std::string& modelPath) {
    return m_pImpl->LoadModel(modelPath);
}

void AIController::Update() {
    // Skip if disabled
    if (!m_pImpl->m_bEnabled) {
        return;
    }
    
    // Track update time for performance monitoring
    auto currentTime = std::chrono::high_resolution_clock::now();
    m_pImpl->m_lastUpdateTime = currentTime;
    
    // Extract the current game state
    AIInputFrame inputFrame = ExtractGameState();
    
    // Run inference
    std::vector<float> actions = RunInference(inputFrame);
    
    // Apply actions
    ApplyActions(actions);
    
    // Update consecutive frames counter
    m_pImpl->m_nConsecutiveFrames++;
}

std::string AIController::GetDebugInfo() const {
    return m_pImpl->GetDebugInfo();
}

void AIController::RegisterDecisionCallback(std::function<void(const std::vector<float>&)> callback) {
    m_pImpl->m_decisionCallback = callback;
}

// Private methods
AIInputFrame AIController::ExtractGameState() const {
    return m_pImpl->ExtractGameState();
}

std::vector<float> AIController::RunInference(const AIInputFrame& inputFrame) {
    return m_pImpl->RunInference(inputFrame);
}

void AIController::ApplyActions(const std::vector<float>& actions) {
    m_pImpl->ApplyActions(actions);
} 