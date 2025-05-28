#include "neural_ai_controller.h"
#include "ai_input_frame.h"
#include "ai_output_action.h"
#include "ai_memory_mapping.h"
#include "ai_torch_policy_model.h"
#include "../burner/burner.h"

#include <chrono>
#include <thread>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <random>
#include <cmath>

namespace AI {

// Global instance
NeuralAIController* g_pAIController = nullptr;

NeuralAIController::NeuralAIController()
    : m_initialized(false)
    , m_active(false)
    , m_memoryMapping(nullptr)
    , m_policyModel(nullptr)
    , m_currentGameName("")
    , m_frameCount(0)
    , m_aiPlayers{false, false}
    , m_randomActionProb(0.05f)
    , m_reactionDelayFrames(2)
    , m_rng(std::random_device{}())
    , m_uniformDist(0.0f, 1.0f)
    , m_pendingActions{}
    , m_debugOverlayEnabled(false)
{
}

NeuralAIController::~NeuralAIController()
{
    shutdown();
}

bool NeuralAIController::initialize()
{
    if (m_initialized) {
        return true;
    }
    
    std::cout << "Initializing NeuralAIController..." << std::endl;
    
    // Create memory mapping
    m_memoryMapping = std::make_shared<AIMemoryMapping>();
    
    // Create policy model
    m_policyModel = std::make_shared<AITorchPolicyModel>();
    
    m_initialized = true;
    m_active = false;
    m_frameCount = 0;
    
    std::cout << "NeuralAIController initialized successfully" << std::endl;
    return true;
}

void NeuralAIController::shutdown()
{
    if (!m_initialized) {
        return;
    }
    
    std::cout << "Shutting down NeuralAIController..." << std::endl;
    
    // Clear policy models
    for (auto& model : m_playerModels) {
        model.reset();
    }
    m_policyModel.reset();
    
    // Clear memory mapping
    m_memoryMapping.reset();
    
    m_initialized = false;
    m_active = false;
    
    std::cout << "NeuralAIController shutdown complete" << std::endl;
}

void NeuralAIController::update()
{
    if (!m_initialized || !m_active) {
        return;
    }
    
    m_frameCount++;
    
    // Skip processing on some frames for performance if needed
    if (m_frameCount % 1 != 0) {
        return;
    }
    
    // Process each player
    for (int player = 0; player < MAX_PLAYERS; player++) {
        if (!m_aiPlayers[player]) {
            continue;
        }
        
        // Check if we have a model for this player
        if (!m_playerModels[player]) {
            continue;
        }
        
        // Extract game state
        auto inputFrame = extractGameState(player);
        
        // Decide action
        auto action = decideAction(inputFrame, player);
        
        // Apply the action with potential delay
        scheduleAction(action, player);
    }
    
    // Apply any pending actions that have reached their delay
    applyPendingActions();
    
    // Update debug overlay if enabled
    if (m_debugOverlayEnabled) {
        updateDebugOverlay();
    }
}

void NeuralAIController::onGameLoaded(const std::string& gameName)
{
    if (!m_initialized) {
        return;
    }
    
    std::cout << "NeuralAIController: Game loaded - " << gameName << std::endl;
    
    m_currentGameName = gameName;
    
    // Load memory mapping for this game
    if (m_memoryMapping) {
        bool mappingLoaded = m_memoryMapping->loadMappingsFromFile("mappings/" + gameName + ".json");
        if (!mappingLoaded) {
            std::cout << "Failed to load memory mapping for " << gameName << std::endl;
            // Try alternate paths
            mappingLoaded = m_memoryMapping->loadMappingsFromFile("src/ai/mappings/" + gameName + ".json");
        }
        
        if (mappingLoaded) {
            m_active = true;
            std::cout << "Memory mapping loaded for " << gameName << std::endl;
        } else {
            m_active = false;
            std::cout << "No memory mapping available for " << gameName << std::endl;
        }
    }
    
    // Reset frame counter
    m_frameCount = 0;
    
    // Clear pending actions
    m_pendingActions.clear();
}

void NeuralAIController::setPlayerAIEnabled(int player, bool enabled)
{
    if (player < 0 || player >= MAX_PLAYERS) {
        return;
    }
    
    m_aiPlayers[player] = enabled;
    std::cout << "AI for player " << (player + 1) << " is now " << (enabled ? "enabled" : "disabled") << std::endl;
}

bool NeuralAIController::isPlayerAIEnabled(int player) const
{
    if (player < 0 || player >= MAX_PLAYERS) {
        return false;
    }
    
    return m_aiPlayers[player];
}

bool NeuralAIController::loadModelForPlayer(int player, const std::string& modelPath)
{
    if (player < 0 || player >= MAX_PLAYERS) {
        return false;
    }
    
    if (!m_initialized) {
        std::cout << "NeuralAIController not initialized" << std::endl;
        return false;
    }
    
    // Create a new policy model for this player if needed
    if (!m_playerModels[player]) {
        m_playerModels[player] = std::make_shared<AITorchPolicyModel>();
    }
    
    // Load the model
    bool success = m_playerModels[player]->loadModel(modelPath);
    if (success) {
        std::cout << "Model loaded for player " << (player + 1) << ": " << modelPath << std::endl;
        return true;
    } else {
        std::cout << "Failed to load model for player " << (player + 1) << ": " << modelPath << std::endl;
        return false;
    }
}

void NeuralAIController::setRandomActionProbability(float probability)
{
    m_randomActionProb = std::clamp(probability, 0.0f, 1.0f);
}

float NeuralAIController::getRandomActionProbability() const
{
    return m_randomActionProb;
}

void NeuralAIController::setReactionDelay(int frames)
{
    m_reactionDelayFrames = std::max(0, frames);
}

int NeuralAIController::getReactionDelay() const
{
    return m_reactionDelayFrames;
}

void NeuralAIController::enableDebugOverlay(bool enabled)
{
    m_debugOverlayEnabled = enabled;
    
    // Initialize the overlay if needed
    if (enabled && !m_debugOverlay) {
        initializeDebugOverlay();
    }
}

bool NeuralAIController::isDebugOverlayEnabled() const
{
    return m_debugOverlayEnabled;
}

AIInputFrame NeuralAIController::extractGameState(int player)
{
    AIInputFrame frame;
    
    // Set basic frame information
    frame.setFrameNumber(m_frameCount);
    frame.setGameId(m_currentGameName);
    frame.setPlayerIndex(player);
    
    // If we have valid memory mapping, extract state from it
    if (m_memoryMapping) {
        m_memoryMapping->refreshValues();
        
        // Extract player information from memory mapping
        updatePlayerStateFromMemory(frame, player);
        
        // Get opponent information if relevant
        int opponent = (player == 0) ? 1 : 0;
        updateOpponentStateFromMemory(frame, opponent);
    }
    
    return frame;
}

AIOutputAction NeuralAIController::decideAction(const AIInputFrame& inputFrame, int player)
{
    // Default action (no input)
    AIOutputAction action;
    
    // Check if we should take a random action
    bool takeRandomAction = (m_uniformDist(m_rng) < m_randomActionProb);
    
    if (takeRandomAction) {
        // Generate a random action
        action = generateRandomAction();
    } else if (m_playerModels[player]) {
        // Use the neural network model to predict the action
        action = m_playerModels[player]->predict(inputFrame);
    }
    
    return action;
}

void NeuralAIController::scheduleAction(const AIOutputAction& action, int player)
{
    if (m_reactionDelayFrames <= 0) {
        // Apply immediately
        applyAction(action, player);
    } else {
        // Schedule for future application
        PendingAction pending;
        pending.action = action;
        pending.player = player;
        pending.targetFrame = m_frameCount + m_reactionDelayFrames;
        
        m_pendingActions.push_back(pending);
    }
}

void NeuralAIController::applyPendingActions()
{
    // Check for actions that need to be applied this frame
    auto it = m_pendingActions.begin();
    while (it != m_pendingActions.end()) {
        if (it->targetFrame <= m_frameCount) {
            // Apply this action
            applyAction(it->action, it->player);
            
            // Remove from pending list
            it = m_pendingActions.erase(it);
        } else {
            ++it;
        }
    }
}

void NeuralAIController::applyAction(const AIOutputAction& action, int player)
{
    // Convert action to input format expected by FBNeo
    unsigned int inputBits = action.toBitmask();
    
    // Apply to the appropriate player's inputs
    // This will need to interface with the FBNeo input system
    // The exact implementation depends on how FBNeo handles inputs
    
    // Example implementation (pseudocode)
    // FBNeoSetPlayerInputs(player, inputBits);
}

AIOutputAction NeuralAIController::generateRandomAction()
{
    AIOutputAction action;
    
    // Random distribution for each button
    std::bernoulli_distribution buttonDist(0.2); // 20% chance for each button
    
    // Randomly set some buttons
    action.up = buttonDist(m_rng);
    action.down = buttonDist(m_rng) && !action.up; // Avoid conflicting directions
    action.left = buttonDist(m_rng);
    action.right = buttonDist(m_rng) && !action.left; // Avoid conflicting directions
    action.button1 = buttonDist(m_rng);
    action.button2 = buttonDist(m_rng);
    action.button3 = buttonDist(m_rng);
    action.button4 = buttonDist(m_rng);
    action.button5 = buttonDist(m_rng);
    action.button6 = buttonDist(m_rng);
    
    return action;
}

void NeuralAIController::updatePlayerStateFromMemory(AIInputFrame& frame, int player)
{
    // This would extract player state from memory mapping
    // The exact implementation depends on the game and memory mapping
    
    // Example implementation
    const auto& groups = m_memoryMapping->getGroups();
    
    for (const auto& group : groups) {
        // Extract player information from memory
        for (const auto& entry : group.getEntries()) {
            const std::string& name = entry.getName();
            
            // Match player-specific entries
            if (name.find("p" + std::to_string(player + 1)) != std::string::npos) {
                if (name.find("health") != std::string::npos || name.find("life") != std::string::npos) {
                    if (std::holds_alternative<int>(entry.getValue())) {
                        frame.setPlayerHealth(player, std::get<int>(entry.getValue()));
                    }
                } else if (name.find("x_pos") != std::string::npos || name.find("position_x") != std::string::npos) {
                    if (std::holds_alternative<int>(entry.getValue())) {
                        frame.setPlayerX(player, std::get<int>(entry.getValue()));
                    }
                } else if (name.find("y_pos") != std::string::npos || name.find("position_y") != std::string::npos) {
                    if (std::holds_alternative<int>(entry.getValue())) {
                        frame.setPlayerY(player, std::get<int>(entry.getValue()));
                    }
                }
            }
        }
    }
}

void NeuralAIController::updateOpponentStateFromMemory(AIInputFrame& frame, int opponent)
{
    // Similar to updatePlayerStateFromMemory, but for the opponent
    // This would extract opponent state from memory mapping
    
    // Example implementation
    const auto& groups = m_memoryMapping->getGroups();
    
    for (const auto& group : groups) {
        // Extract opponent information from memory
        for (const auto& entry : group.getEntries()) {
            const std::string& name = entry.getName();
            
            // Match opponent-specific entries
            if (name.find("p" + std::to_string(opponent + 1)) != std::string::npos) {
                if (name.find("health") != std::string::npos || name.find("life") != std::string::npos) {
                    if (std::holds_alternative<int>(entry.getValue())) {
                        frame.setOpponentHealth(std::get<int>(entry.getValue()));
                    }
                } else if (name.find("x_pos") != std::string::npos || name.find("position_x") != std::string::npos) {
                    if (std::holds_alternative<int>(entry.getValue())) {
                        frame.setOpponentX(std::get<int>(entry.getValue()));
                    }
                } else if (name.find("y_pos") != std::string::npos || name.find("position_y") != std::string::npos) {
                    if (std::holds_alternative<int>(entry.getValue())) {
                        frame.setOpponentY(std::get<int>(entry.getValue()));
                    }
                }
            }
        }
    }
}

void NeuralAIController::initializeDebugOverlay()
{
    // Initialize debug overlay
    m_debugOverlay = std::make_unique<MetalDebugOverlay>();
    m_debugOverlay->initialize();
}

void NeuralAIController::updateDebugOverlay()
{
    if (!m_debugOverlay) {
        return;
    }
    
    // Update overlay with current state information
    m_debugOverlay->beginFrame();
    
    // Draw AI status
    m_debugOverlay->drawText(10, 30, "AI Status: " + std::string(m_active ? "Active" : "Inactive"));
    m_debugOverlay->drawText(10, 50, "Game: " + m_currentGameName);
    m_debugOverlay->drawText(10, 70, "Frame: " + std::to_string(m_frameCount));
    
    // Draw player AI status
    for (int player = 0; player < MAX_PLAYERS; player++) {
        std::string status = "Player " + std::to_string(player + 1) + ": ";
        status += m_aiPlayers[player] ? "AI" : "Human";
        m_debugOverlay->drawText(10, 100 + player * 20, status);
    }
    
    // Draw model confidence if available
    for (int player = 0; player < MAX_PLAYERS; player++) {
        if (m_aiPlayers[player] && !m_pendingActions.empty()) {
            // Find the most recent action for this player
            for (auto it = m_pendingActions.rbegin(); it != m_pendingActions.rend(); ++it) {
                if (it->player == player) {
                    std::string actionText = "Action: " + it->action.toString();
                    m_debugOverlay->drawText(200, 100 + player * 20, actionText);
                    break;
                }
            }
        }
    }
    
    m_debugOverlay->endFrame();
}

// Initialize static instance
NeuralAIController* NeuralAIController::instance()
{
    if (!g_pAIController) {
        g_pAIController = new NeuralAIController();
    }
    return g_pAIController;
}

// Global initialization
void NeuralAIController::Initialize()
{
    if (!g_pAIController) {
        g_pAIController = new NeuralAIController();
        g_pAIController->initialize();
    }
}

// Global shutdown
void NeuralAIController::Shutdown()
{
    if (g_pAIController) {
        g_pAIController->shutdown();
        delete g_pAIController;
        g_pAIController = nullptr;
    }
}

} // namespace AI 