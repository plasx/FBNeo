#pragma once

#include <string>
#include <memory>
#include <vector>
#include <functional>

// Forward declarations
class AIMemoryMapping;
class AIInputFrame;
class AITorchPolicy;

/**
 * @brief Enum for AI controller operation modes
 */
enum class AIControllerMode {
    Disabled,    ///< AI is completely disabled
    FullControl, ///< AI has full control over player inputs
    AssistMode,  ///< AI suggests inputs but player maintains control
    WatchOnly    ///< AI predicts inputs but doesn't apply them
};

/**
 * @brief Main AI controller for FBNeo
 * 
 * This class coordinates the AI systems, handling:
 * - Model loading and inference
 * - Game state extraction 
 * - Input application to the emulator
 * - Assist modes and visualization
 */
class AIController {
public:
    /**
     * @brief Initialize the AI controller
     */
    static void Initialize();
    
    /**
     * @brief Shutdown the AI controller
     */
    static void Shutdown();

    /**
     * @brief Get the singleton instance
     * @return Reference to the global AI controller
     */
    static AIController& GetInstance();

    /**
     * @brief Set whether AI is enabled
     * @param enabled True to enable, false to disable
     */
    void SetEnabled(bool enabled);
    
    /**
     * @brief Check if AI is enabled
     * @return True if AI is enabled
     */
    bool IsEnabled() const;
    
    /**
     * @brief Set the operation mode
     * @param mode The controller mode to set
     */
    void SetMode(AIControllerMode mode);
    
    /**
     * @brief Get the current operation mode
     * @return Current controller mode
     */
    AIControllerMode GetMode() const;
    
    /**
     * @brief Set the assist threshold
     * @param threshold Value between 0.0 and 1.0 indicating confidence threshold
     */
    void SetAssistThreshold(float threshold);
    
    /**
     * @brief Set the player index to control
     * @param playerIndex 0-based player index
     */
    void SetPlayerIndex(int playerIndex);
    
    /**
     * @brief Get the controller's player index
     * @return Player index being controlled
     */
    int GetPlayerIndex() const;
    
    /**
     * @brief Load an AI model
     * @param modelPath Path to the model file
     * @return True if loaded successfully
     */
    bool LoadModel(const std::string& modelPath);
    
    /**
     * @brief Update called each frame to run AI inference and apply inputs
     */
    void Update();
    
    /**
     * @brief Get debug visualization info
     * @return String containing debug info
     */
    std::string GetDebugInfo() const;
    
    /**
     * @brief Register callback for when AI makes a decision
     * @param callback Function to call with predicted actions
     */
    void RegisterDecisionCallback(std::function<void(const std::vector<float>&)> callback);

private:
    AIController();
    ~AIController();
    
    // Delete copy/move constructors and assignment operators
    AIController(const AIController&) = delete;
    AIController& operator=(const AIController&) = delete;
    AIController(AIController&&) = delete;
    AIController& operator=(AIController&&) = delete;
    
    /**
     * @brief Extract the current game state
     * @return Input frame containing normalized state
     */
    AIInputFrame ExtractGameState() const;
    
    /**
     * @brief Run inference on the loaded model
     * @param inputFrame The input state to process
     * @return Vector of action probabilities
     */
    std::vector<float> RunInference(const AIInputFrame& inputFrame);
    
    /**
     * @brief Apply predicted actions to the emulator
     * @param actions Vector of action probabilities
     */
    void ApplyActions(const std::vector<float>& actions);
    
    // Private implementation
    class Impl;
    std::unique_ptr<Impl> m_pImpl;
};

// Global instance for easy access
extern AIController* g_pAIController; 