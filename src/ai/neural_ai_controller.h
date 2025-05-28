#pragma once

#include <memory>
#include <string>
#include <vector>
#include <random>
#include <mutex>

namespace AI {

// Forward declarations
class AIInputFrame;
class AIOutputAction;
class AIMemoryMapping;
class AITorchPolicyModel;
class MetalDebugOverlay;

// Maximum number of players
constexpr int MAX_PLAYERS = 2;

// Neural AI Controller class
class NeuralAIController {
public:
    // Singleton access
    static NeuralAIController* instance();
    
    // Global initialization and shutdown
    static void Initialize();
    static void Shutdown();
    
    // Constructor and destructor
    NeuralAIController();
    virtual ~NeuralAIController();
    
    // Initialize and shutdown
    bool initialize();
    void shutdown();
    
    // Update AI state (called once per frame)
    void update();
    
    // Called when a new game is loaded
    void onGameLoaded(const std::string& gameName);
    
    // Player control
    void setPlayerAIEnabled(int player, bool enabled);
    bool isPlayerAIEnabled(int player) const;
    
    // Model loading
    bool loadModelForPlayer(int player, const std::string& modelPath);
    
    // AI parameters
    void setRandomActionProbability(float probability);
    float getRandomActionProbability() const;
    
    void setReactionDelay(int frames);
    int getReactionDelay() const;
    
    // Debug overlay
    void enableDebugOverlay(bool enabled);
    bool isDebugOverlayEnabled() const;
    
private:
    // Game state extraction
    AIInputFrame extractGameState(int player);
    
    // Action decision
    AIOutputAction decideAction(const AIInputFrame& inputFrame, int player);
    void scheduleAction(const AIOutputAction& action, int player);
    void applyPendingActions();
    void applyAction(const AIOutputAction& action, int player);
    
    // Helper methods
    AIOutputAction generateRandomAction();
    void updatePlayerStateFromMemory(AIInputFrame& frame, int player);
    void updateOpponentStateFromMemory(AIInputFrame& frame, int opponent);
    
    // Debug overlay
    void initializeDebugOverlay();
    void updateDebugOverlay();
    
    // Pending action structure
    struct PendingAction {
        AIOutputAction action;
        int player;
        int targetFrame;
    };
    
    // Member variables
    bool m_initialized;
    bool m_active;
    std::shared_ptr<AIMemoryMapping> m_memoryMapping;
    std::shared_ptr<AITorchPolicyModel> m_policyModel;
    std::shared_ptr<AITorchPolicyModel> m_playerModels[MAX_PLAYERS];
    std::string m_currentGameName;
    int m_frameCount;
    bool m_aiPlayers[MAX_PLAYERS];
    
    // AI parameters
    float m_randomActionProb;
    int m_reactionDelayFrames;
    
    // Random number generation
    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_uniformDist;
    
    // Action management
    std::vector<PendingAction> m_pendingActions;
    
    // Debug overlay
    bool m_debugOverlayEnabled;
    std::unique_ptr<MetalDebugOverlay> m_debugOverlay;
};

} // namespace AI 