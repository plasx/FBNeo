#pragma once

#include "ai_rl_algorithms.h"
#include "ai_torch_policy.h"
#include "ai_input_frame.h"
#include "ai_output_action.h"
#include "../metal_declarations.h"
#include <string>
#include <memory>
#include <functional>
#include <unordered_map>

namespace fbneo {
namespace ai {

/**
 * @brief Function type for custom reward calculation
 */
using RewardFunction = std::function<float(const AIInputFrame&, const AIInputFrame&, const AIOutputAction&)>;

/**
 * @brief Reinforcement Learning integration module for FBNeo
 * 
 * This class provides the interface between the FBNeo core and the 
 * reinforcement learning algorithms, handling training, inference,
 * and reward calculation.
 */
class RLIntegration {
public:
    /**
     * @brief Get the singleton instance
     * @return Reference to the singleton instance
     */
    static RLIntegration& getInstance();

    /**
     * @brief Initialize the RL integration
     * @param configPath Path to configuration file (optional)
     * @return True if initialization was successful
     */
    bool initialize(const std::string& configPath = "");
    
    /**
     * @brief Shutdown the RL integration
     */
    void shutdown();
    
    /**
     * @brief Set the active algorithm
     * @param type Algorithm type (e.g., "ppo", "a3c")
     * @return True if successful
     */
    bool setAlgorithm(const std::string& type);
    
    /**
     * @brief Get the active algorithm type
     * @return The active algorithm type
     */
    std::string getAlgorithmType() const;
    
    /**
     * @brief Set algorithm hyperparameters
     * @param params Map of parameter names to values
     */
    void setHyperparameters(const std::unordered_map<std::string, float>& params);
    
    /**
     * @brief Get current hyperparameters
     * @return Map of parameter names to values
     */
    std::unordered_map<std::string, float> getHyperparameters() const;
    
    /**
     * @brief Set the policy model
     * @param policy Pointer to the policy model
     * @param takeOwnership Whether to take ownership of the policy
     */
    void setPolicy(AITorchPolicy* policy, bool takeOwnership = false);
    
    /**
     * @brief Get the policy model
     * @return Pointer to the policy model
     */
    AITorchPolicy* getPolicy() const;
    
    /**
     * @brief Set the reward function
     * @param func Custom reward function
     */
    void setRewardFunction(RewardFunction func);
    
    /**
     * @brief Reset the reward function to the default
     */
    void resetRewardFunction();
    
    /**
     * @brief Process a game step
     * @param prevState Previous state
     * @param action Action taken
     * @param currState Current state
     * @param done Whether the episode is done
     * @return The calculated reward
     */
    float processStep(const AIInputFrame& prevState, const AIOutputAction& action, 
                     const AIInputFrame& currState, bool done);
    
    /**
     * @brief Get an action for the current state
     * @param state Current state
     * @param actionOut Output action
     * @param exploit Whether to exploit (true) or explore (false)
     * @return True if successful
     */
    bool getAction(const AIInputFrame& state, AIOutputAction& actionOut, bool exploit = false);
    
    /**
     * @brief Enable/disable training
     * @param enable Whether to enable training
     */
    void enableTraining(bool enable);
    
    /**
     * @brief Check if training is enabled
     * @return True if training is enabled
     */
    bool isTrainingEnabled() const;
    
    /**
     * @brief Enable/disable intrinsic curiosity module
     * @param enable Whether to enable ICM
     * @param scale Reward scale for ICM (if enabled)
     */
    void enableICM(bool enable, float scale = 0.01f);
    
    /**
     * @brief Check if ICM is enabled
     * @return True if ICM is enabled
     */
    bool isICMEnabled() const;
    
    /**
     * @brief Save the current model and algorithm state
     * @param path Path to save to
     * @return True if successful
     */
    bool save(const std::string& path);
    
    /**
     * @brief Load a model and algorithm state
     * @param path Path to load from
     * @return True if successful
     */
    bool load(const std::string& path);
    
    /**
     * @brief Start a new training episode
     */
    void startEpisode();
    
    /**
     * @brief End the current training episode
     * @param success Whether the episode was successful
     * @return The total reward for the episode
     */
    float endEpisode(bool success);
    
    /**
     * @brief Run distributed training (for A3C)
     * @param numWorkers Number of worker threads
     */
    void startDistributedTraining(int numWorkers = 4);
    
    /**
     * @brief Stop distributed training
     */
    void stopDistributedTraining();
    
    /**
     * @brief Generate reward shaping functions for different game types
     * @param gameType Type of game (fighting, platformer, etc.)
     * @return Reward function for the game type
     */
    static RewardFunction createRewardFunction(const std::string& gameType);
    
    /**
     * @brief Get learning statistics
     * @return Map of statistic names to values
     */
    std::unordered_map<std::string, float> getStatistics() const;
    
    /**
     * @brief Reset learning statistics
     */
    void resetStatistics();

private:
    RLIntegration();
    ~RLIntegration();
    
    // Prevent copying and assignment
    RLIntegration(const RLIntegration&) = delete;
    RLIntegration& operator=(const RLIntegration&) = delete;
    
    // Default reward function
    float defaultReward(const AIInputFrame& prevState, const AIInputFrame& currState, 
                       const AIOutputAction& action);
    
    // Process memory for observation
    void processGameMemory(const AIInputFrame& state);
    
    // Member variables
    std::unique_ptr<RLAlgorithm> algorithm;
    AITorchPolicy* policy;
    bool ownPolicy;
    std::string algorithmType;
    bool trainingEnabled;
    std::unique_ptr<ICMModule> icm;
    bool icmEnabled;
    RewardFunction rewardFunc;
    
    // Episode tracking
    int episodeCount;
    float episodeTotalReward;
    int episodeSteps;
    
    // Statistics
    std::unordered_map<std::string, float> statistics;
    
    // Game-specific memory locations
    struct GameMemoryInfo {
        std::string gameName;
        std::vector<std::pair<std::string, uint32_t>> memoryAddresses;
        std::unordered_map<std::string, std::vector<float>> memoryHistory;
    };
    GameMemoryInfo gameMemory;
};

/**
 * @brief Create a reward function specifically for fighting games
 * @return Reward function for fighting games
 */
RewardFunction createFightingGameReward();

/**
 * @brief Create a reward function specifically for platformer games
 * @return Reward function for platformer games
 */
RewardFunction createPlatformerReward();

/**
 * @brief Create a reward function specifically for puzzle games
 * @return Reward function for puzzle games
 */
RewardFunction createPuzzleGameReward();

/**
 * @brief Create a reward function specifically for shooter games
 * @return Reward function for shooter games
 */
RewardFunction createShooterReward();

} // namespace ai
} // namespace fbneo

// C API for integration with non-C++ code
extern "C" {
    // Create and initialize RL system
    void* FBNEO_RL_Create();
    void FBNEO_RL_Destroy(void* handle);
    int FBNEO_RL_Initialize(void* handle, const char* configPath);
    
    // Algorithm selection
    int FBNEO_RL_SetAlgorithm(void* handle, const char* type);
    const char* FBNEO_RL_GetAlgorithmType(void* handle);
    
    // Training control
    void FBNEO_RL_EnableTraining(void* handle, int enable);
    int FBNEO_RL_IsTrainingEnabled(void* handle);
    void FBNEO_RL_StartEpisode(void* handle);
    float FBNEO_RL_EndEpisode(void* handle, int success);
    
    // Process steps and get actions
    float FBNEO_RL_ProcessStep(void* handle, const void* prevState, const void* action,
                             const void* currState, int done);
    int FBNEO_RL_GetAction(void* handle, const void* state, void* actionOut, int exploit);
    
    // Save and load
    int FBNEO_RL_Save(void* handle, const char* path);
    int FBNEO_RL_Load(void* handle, const char* path);
    
    // ICM control
    void FBNEO_RL_EnableICM(void* handle, int enable, float scale);
    int FBNEO_RL_IsICMEnabled(void* handle);
    
    // Distributed training
    void FBNEO_RL_StartDistributedTraining(void* handle, int numWorkers);
    void FBNEO_RL_StopDistributedTraining(void* handle);
} 