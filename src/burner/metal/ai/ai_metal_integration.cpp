#include "ai_metal_integration.h"
#include <memory>
#include <iostream>
#include <chrono>

namespace fbneo {
namespace ai {

// Globals for AI system
namespace {
    std::unique_ptr<AITorchPolicy> g_policy;
    std::unique_ptr<RLAlgorithm> g_algorithm;
    bool g_trainingMode = false;
    AIMemoryMapping g_memoryMapping;
    
    // Previous state for reward calculation
    GameState g_prevState;
    AIInputFrame g_prevFrame;
    AIOutputAction g_prevAction;
    bool g_hasPrevState = false;
    
    // Episode tracking
    int g_episodeCount = 0;
    int g_stepCount = 0;
    float g_episodeReward = 0.0f;
}

bool Metal_InitializeAI(const char* modelPath, const char* algorithmType) {
    // Clean up any existing instances
    Metal_ShutdownAI();
    
    // Create the policy
    g_policy = std::make_unique<AITorchPolicy>();
    
    // Try to load the model if a path is provided
    bool modelLoaded = false;
    if (modelPath && modelPath[0] != '\0') {
        modelLoaded = g_policy->LoadModel(modelPath);
        if (!modelLoaded) {
            std::cerr << "Failed to load AI model from: " << modelPath << std::endl;
            // Continue with a new model
        } else {
            std::cout << "AI model loaded from: " << modelPath << std::endl;
        }
    }
    
    // If no model was loaded, initialize a new one
    if (!modelLoaded) {
        if (!g_policy->InitializeDefaultModel()) {
            std::cerr << "Failed to initialize default AI model" << std::endl;
            return false;
        }
        std::cout << "Initialized new AI model" << std::endl;
    }
    
    // Create the algorithm
    if (algorithmType == nullptr || strcmp(algorithmType, "ppo") == 0) {
        g_algorithm = std::make_unique<PPOAlgorithm>(g_policy.get());
        std::cout << "Using PPO algorithm" << std::endl;
    } else {
        std::cerr << "Unsupported algorithm type: " << algorithmType << std::endl;
        g_algorithm = std::make_unique<PPOAlgorithm>(g_policy.get());
        std::cout << "Falling back to PPO algorithm" << std::endl;
    }
    
    // Reset episode tracking
    g_episodeCount = 0;
    g_stepCount = 0;
    g_episodeReward = 0.0f;
    g_hasPrevState = false;
    
    return true;
}

void Metal_ShutdownAI() {
    // Reset previous state
    g_hasPrevState = false;
    
    // Clean up algorithm and policy
    g_algorithm.reset();
    g_policy.reset();
    
    std::cout << "AI systems shut down" << std::endl;
}

AIOutputAction Metal_ProcessAIFrame(const void* frameBuffer, int width, int height, const GameState& gameState) {
    // Create input frame
    AIInputFrame frame;
    frame.frameBuffer = frameBuffer;
    frame.width = width;
    frame.height = height;
    frame.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    frame.gameState = gameState;
    
    // Get AI action
    AIOutputAction action;
    if (g_policy) {
        g_policy->RunInference(frame, action);
    }
    
    // Process for training if in training mode
    if (g_trainingMode && g_algorithm && g_hasPrevState) {
        // Calculate reward
        float reward = Metal_CalculateReward(g_prevState, gameState);
        g_episodeReward += reward;
        
        // Check if episode is over
        bool isDone = Metal_IsEpisodeOver(gameState);
        
        // Process this step
        g_algorithm->processStep(g_prevFrame, g_prevAction, reward, frame, isDone);
        
        // Increment step count
        g_stepCount++;
        
        // Handle episode end
        if (isDone) {
            std::cout << "Episode " << g_episodeCount << " completed with "
                      << g_stepCount << " steps and reward " << g_episodeReward << std::endl;
            
            // Reset counters
            g_episodeCount++;
            g_stepCount = 0;
            g_episodeReward = 0.0f;
            g_hasPrevState = false;
        }
    }
    
    // Save current state as previous
    g_prevState = gameState;
    g_prevFrame = frame;
    g_prevAction = action;
    g_hasPrevState = true;
    
    return action;
}

bool Metal_SaveAIModel(const char* path) {
    if (!g_policy) {
        std::cerr << "No AI model to save" << std::endl;
        return false;
    }
    
    bool success = g_policy->SaveModel(path);
    if (success) {
        std::cout << "AI model saved to: " << path << std::endl;
    } else {
        std::cerr << "Failed to save AI model to: " << path << std::endl;
    }
    
    // Save algorithm state if training
    if (g_algorithm && g_trainingMode) {
        std::string algorithmPath = std::string(path) + ".algorithm";
        success = g_algorithm->save(algorithmPath) && success;
        
        if (success) {
            std::cout << "Algorithm state saved to: " << algorithmPath << std::endl;
        } else {
            std::cerr << "Failed to save algorithm state" << std::endl;
        }
    }
    
    return success;
}

bool Metal_LoadAIModel(const char* path) {
    if (!g_policy) {
        std::cerr << "AI system not initialized" << std::endl;
        return false;
    }
    
    bool success = g_policy->LoadModel(path);
    if (success) {
        std::cout << "AI model loaded from: " << path << std::endl;
    } else {
        std::cerr << "Failed to load AI model from: " << path << std::endl;
    }
    
    // Load algorithm state if training
    if (g_algorithm && g_trainingMode) {
        std::string algorithmPath = std::string(path) + ".algorithm";
        bool algSuccess = g_algorithm->load(algorithmPath);
        
        if (algSuccess) {
            std::cout << "Algorithm state loaded from: " << algorithmPath << std::endl;
        } else {
            std::cerr << "Failed to load algorithm state (using defaults)" << std::endl;
        }
    }
    
    return success;
}

void Metal_SetAITrainingMode(bool enabled) {
    g_trainingMode = enabled;
    std::cout << "AI training mode " << (enabled ? "enabled" : "disabled") << std::endl;
    
    // Reset counters when enabling training
    if (enabled) {
        g_episodeCount = 0;
        g_stepCount = 0;
        g_episodeReward = 0.0f;
        g_hasPrevState = false;
    }
}

bool Metal_IsAITrainingMode() {
    return g_trainingMode;
}

float Metal_CalculateReward(const GameState& prevState, const GameState& currentState) {
    // Base reward starts at a small negative value to encourage quick completion
    float reward = -0.01f;
    
    // Example: For fighting games, reward dealing damage and penalize taking damage
    if (currentState.playerCount >= 2 && prevState.playerCount >= 2) {
        // Player 1 is AI, Player 2 is opponent
        const auto& prevP1 = prevState.players[0];
        const auto& currentP1 = currentState.players[0];
        const auto& prevP2 = prevState.players[1];
        const auto& currentP2 = currentState.players[1];
        
        // Reward for damaging opponent
        if (currentP2.health < prevP2.health) {
            float damageDone = prevP2.health - currentP2.health;
            reward += damageDone * 0.1f;
        }
        
        // Penalty for taking damage
        if (currentP1.health < prevP1.health) {
            float damageTaken = prevP1.health - currentP1.health;
            reward -= damageTaken * 0.15f;
        }
        
        // Big reward for winning round
        if (currentP2.health <= 0 && prevP2.health > 0) {
            reward += 10.0f;
        }
        
        // Big penalty for losing round
        if (currentP1.health <= 0 && prevP1.health > 0) {
            reward -= 15.0f;
        }
    }
    
    return reward;
}

bool Metal_ExtractGameState(const AIMemoryMapping& gameMapping, GameState& state) {
    // This would typically use the memory mapping to read values from the emulator's memory
    // For now, we'll just return a default state
    state.Reset();
    
    // In a real implementation, we would use gameMapping to read memory addresses
    // For example:
    // state.players[0].health = gameMapping.ReadMemory<int>(0x12345);
    
    return true;
}

bool Metal_IsEpisodeOver(const GameState& state) {
    // An episode is typically over when:
    // 1. A round ends (either player health = 0)
    // 2. Time runs out
    // 3. Match ends
    
    // Simple check - either player's health is 0
    if (state.playerCount >= 2) {
        if (state.players[0].health <= 0 || state.players[1].health <= 0) {
            return true;
        }
    }
    
    // Time ran out
    if (state.timeRemaining <= 0) {
        return true;
    }
    
    return false;
}

void Metal_ApplyAIAction(const AIOutputAction& action, int playerIndex) {
    // This would integrate with the emulator's input system
    // For example:
    // InputMake(playerIndex, action.ToDirectionalBits(), action.ToButtonBits());
    
    // Print action for debugging
    std::cout << "AI Action for Player " << playerIndex << ": ";
    if (action.up) std::cout << "Up ";
    if (action.down) std::cout << "Down ";
    if (action.left) std::cout << "Left ";
    if (action.right) std::cout << "Right ";
    
    for (int i = 0; i < MAX_BUTTONS; i++) {
        if (action.buttons[i]) {
            std::cout << "Button" << i << " ";
        }
    }
    std::cout << std::endl;
}

} // namespace ai
} // namespace fbneo 