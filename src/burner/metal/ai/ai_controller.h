#pragma once

#include <vector>
#include <string>
#include <memory>
#include "ai_definitions.h"

// AI Controller Interface for FBNeo
// This defines the interface for AI agents to interact with the emulator

namespace fbneo {
namespace ai {

// Forward declarations
class GameState;
class AIController;

// AI Player Types
enum class AIPlayerType {
    NONE,           // No AI (human player)
    RULE_BASED,     // Simple rule-based AI
    REPLAY,         // Replay recorded inputs
    NEURAL_NETWORK, // Neural network based AI
    CUSTOM          // Custom AI implementation
};

// Game observation data
struct GameObservation {
    unsigned char* screenBuffer;  // Raw screen buffer
    int width;                    // Screen width
    int height;                   // Screen height
    int pitch;                    // Screen pitch
    float* gameVariables;         // Game-specific variables (health, position, etc.)
    int numVariables;             // Number of game variables
    uint64_t frameNumber;         // Current frame number
};

// Input action representation
struct InputAction {
    bool up;
    bool down;
    bool left;
    bool right;
    bool button1;
    bool button2;
    bool button3;
    bool button4;
    bool button5;
    bool button6;
    bool start;
    bool coin;
    
    InputAction() : up(false), down(false), left(false), right(false),
                   button1(false), button2(false), button3(false),
                   button4(false), button5(false), button6(false),
                   start(false), coin(false) {}
};

// Training mode options
struct TrainingModeOptions {
    bool infiniteHealth;
    bool infiniteTime;
    bool showHitboxes;
    bool showFrameData;
    bool showInputDisplay;
    bool recordReplay;
    
    TrainingModeOptions() : infiniteHealth(false), infiniteTime(false),
                           showHitboxes(false), showFrameData(false),
                           showInputDisplay(false), recordReplay(false) {}
};

/**
 * @brief Main AI controller class for FBNeo Metal implementation
 * 
 * This class manages the AI functionality for the Metal backend,
 * including model loading, inference, and game state processing.
 * It uses Metal's latest features to accelerate AI processing.
 */
class AIController {
public:
    /**
     * Constructor
     */
    AIController();
    
    /**
     * Destructor
     */
    ~AIController();
    
    /**
     * Initialize the AI controller
     * @return True if initialization was successful
     */
    bool initialize();
    
    /**
     * Process a frame and generate AI actions
     * @param gameState Current game state
     * @return AI output actions for the current frame
     */
    AIOutputState processFrame(const GameState& gameState);
    
    /**
     * Load an AI model from the specified path
     * @param modelPath Path to the model file (.pt, .mlpackage, or .mpsgraphpackage)
     * @return True if the model was loaded successfully
     */
    bool loadModel(const char* modelPath);
    
    /**
     * Enable or disable the AI
     * @param active True to enable, false to disable
     */
    void setActive(bool active);
    
    /**
     * Check if the AI is active
     * @return True if the AI is active
     */
    bool isActive() const;
    
    /**
     * Set the AI difficulty level
     * @param difficulty Level from 0 (easiest) to 10 (hardest)
     */
    void setDifficulty(int difficulty);
    
    /**
     * Get the current AI difficulty level
     * @return Current difficulty level
     */
    int getDifficulty() const;
    
    /**
     * Set which player(s) the AI should control
     * @param player 0=none, 1=P1, 2=P2, 3=both
     */
    void setPlayerControlled(int player);
    
    /**
     * Get which player(s) the AI is controlling
     * @return Player control setting
     */
    int getPlayerControlled() const;
    
    /**
     * Enable or disable training mode
     * @param enabled True to enable training mode
     */
    void setTrainingMode(bool enabled);
    
    /**
     * Check if training mode is enabled
     * @return True if training mode is enabled
     */
    bool isTrainingMode() const;
    
    /**
     * Enable or disable debug overlay
     * @param enabled True to enable debug overlay
     */
    void setDebugOverlay(bool enabled);
    
    /**
     * Check if debug overlay is enabled
     * @return True if debug overlay is enabled
     */
    bool isDebugOverlay() const;
    
    /**
     * Enable or disable mixed precision (bfloat16/float16)
     * Requires Metal 3 support and macOS 14+
     * @param enabled True to enable mixed precision
     */
    void setMixedPrecision(bool enabled);
    
    /**
     * Check if mixed precision is enabled
     * @return True if mixed precision is enabled
     */
    bool isMixedPrecisionEnabled() const;
    
    /**
     * Enable or disable quantization (int8)
     * Requires Metal 3 support
     * @param enabled True to enable quantization
     */
    void setQuantizationEnabled(bool enabled);
    
    /**
     * Check if quantization is enabled
     * @return True if quantization is enabled
     */
    bool isQuantizationEnabled() const;
    
    // Get controller name
    const char* getName() const;
    
    // Get controller type
    AIPlayerType getType() const;
    
    // Provide feedback/reward to the agent
    void provideReward(float reward);
    
    // Notification of end of episode
    void episodeComplete();
    
    // Save model or parameters
    bool saveModel(const char* modelPath);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

// Factory function to create AI controller
AIController* createAIController(AIPlayerType type);

// Get list of available AI controllers
std::vector<std::string> getAvailableControllers();

} // namespace ai
} // namespace fbneo 