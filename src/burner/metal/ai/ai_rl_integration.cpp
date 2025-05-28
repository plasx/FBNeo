#include "ai_rl_integration.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <algorithm>

namespace fbneo {
namespace ai {

// Singleton instance
RLIntegration& RLIntegration::getInstance() {
    static RLIntegration instance;
    return instance;
}

RLIntegration::RLIntegration() 
    : policy(nullptr)
    , ownPolicy(false)
    , algorithmType("ppo")
    , trainingEnabled(false)
    , icmEnabled(false)
    , episodeCount(0)
    , episodeTotalReward(0.0f)
    , episodeSteps(0) {
    
    // Set default reward function
    rewardFunc = std::bind(&RLIntegration::defaultReward, this, 
                          std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

RLIntegration::~RLIntegration() {
    shutdown();
}

bool RLIntegration::initialize(const std::string& configPath) {
    // Create a default policy if none exists
    if (!policy) {
        // In a real implementation, this would create a policy with suitable architecture
        // policy = new AITorchPolicy();
        // ownPolicy = true;
    }
    
    // Load configuration if provided
    if (!configPath.empty()) {
        std::ifstream configFile(configPath);
        if (configFile) {
            std::unordered_map<std::string, float> hyperparams;
            std::string line;
            
            while (std::getline(configFile, line)) {
                // Skip empty lines and comments
                if (line.empty() || line[0] == '#') {
                    continue;
                }
                
                std::istringstream iss(line);
                std::string key, value;
                
                if (std::getline(iss, key, '=') && std::getline(iss, value)) {
                    // Trim whitespace
                    key.erase(0, key.find_first_not_of(" \t"));
                    key.erase(key.find_last_not_of(" \t") + 1);
                    value.erase(0, value.find_first_not_of(" \t"));
                    value.erase(value.find_last_not_of(" \t") + 1);
                    
                    // Special case for algorithm type
                    if (key == "algorithm") {
                        setAlgorithm(value);
                    }
                    // Special case for ICM
                    else if (key == "icm_enabled") {
                        icmEnabled = (value == "true" || value == "1");
                    }
                    else if (key == "icm_scale") {
                        float scale = std::stof(value);
                        if (icmEnabled) {
                            enableICM(true, scale);
                        }
                    }
                    // Special case for training
                    else if (key == "training_enabled") {
                        trainingEnabled = (value == "true" || value == "1");
                    }
                    // Process numerical parameters
                    else {
                        try {
                            hyperparams[key] = std::stof(value);
                        } catch (const std::exception& e) {
                            std::cerr << "Error parsing parameter " << key << ": " << e.what() << std::endl;
                        }
                    }
                }
            }
            
            // Set hyperparameters
            if (!hyperparams.empty()) {
                setHyperparameters(hyperparams);
            }
        }
    }
    
    // Create algorithm if not already created
    if (!algorithm) {
        setAlgorithm(algorithmType);
    }
    
    // Create ICM if enabled
    if (icmEnabled && !icm) {
        icm = std::make_unique<ICMModule>(policy);
        icm->initialize();
    }
    
    // Reset statistics
    resetStatistics();
    
    return true;
}

void RLIntegration::shutdown() {
    // Clean up algorithm
    algorithm.reset();
    
    // Clean up ICM
    icm.reset();
    
    // Clean up policy if we own it
    if (ownPolicy && policy) {
        delete policy;
        policy = nullptr;
        ownPolicy = false;
    }
}

bool RLIntegration::setAlgorithm(const std::string& type) {
    // If we already have this algorithm type, return
    if (algorithm && type == algorithmType) {
        return true;
    }
    
    algorithmType = type;
    
    // Create algorithm
    if (policy) {
        algorithm = RLAlgorithmFactory::create(type, policy);
        
        // Set default hyperparameters
        auto params = RLAlgorithmFactory::getDefaultHyperparameters(type);
        if (!params.empty()) {
            algorithm->setHyperparameters(params);
        }
        
        return algorithm != nullptr;
    }
    
    return false;
}

std::string RLIntegration::getAlgorithmType() const {
    return algorithmType;
}

void RLIntegration::setHyperparameters(const std::unordered_map<std::string, float>& params) {
    if (algorithm) {
        algorithm->setHyperparameters(params);
    }
}

std::unordered_map<std::string, float> RLIntegration::getHyperparameters() const {
    // This is a simplification, as we don't have a real way to extract params
    return RLAlgorithmFactory::getDefaultHyperparameters(algorithmType);
}

void RLIntegration::setPolicy(AITorchPolicy* newPolicy, bool takeOwnership) {
    // Clean up old policy if we own it
    if (ownPolicy && policy) {
        delete policy;
    }
    
    policy = newPolicy;
    ownPolicy = takeOwnership;
    
    // Update algorithm with new policy
    if (policy && !algorithm) {
        setAlgorithm(algorithmType);
    } else if (policy && algorithm) {
        // This is a simplification, as we'd need to update the algorithm's policy
        algorithm = RLAlgorithmFactory::create(algorithmType, policy);
    }
    
    // Update ICM with new policy
    if (policy && icmEnabled) {
        icm = std::make_unique<ICMModule>(policy);
        icm->initialize();
    }
}

AITorchPolicy* RLIntegration::getPolicy() const {
    return policy;
}

void RLIntegration::setRewardFunction(RewardFunction func) {
    if (func) {
        rewardFunc = func;
    }
}

void RLIntegration::resetRewardFunction() {
    rewardFunc = std::bind(&RLIntegration::defaultReward, this, 
                          std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

float RLIntegration::processStep(const AIInputFrame& prevState, const AIOutputAction& action, 
                                const AIInputFrame& currState, bool done) {
    // Skip if no algorithm
    if (!algorithm) {
        return 0.0f;
    }
    
    // Process game memory for observation
    processGameMemory(currState);
    
    // Calculate reward
    float reward = rewardFunc(prevState, currState, action);
    
    // Add intrinsic reward from ICM if enabled
    if (icmEnabled && icm) {
        float intrinsicReward = icm->calculateIntrinsicReward(prevState, action, currState);
        reward += intrinsicReward;
        
        // Update ICM
        if (trainingEnabled) {
            // In a real implementation, we'd collect batches and update periodically
            // For now, we'll skip actual updates since ICM is a stub
        }
    }
    
    // Update episode stats
    episodeTotalReward += reward;
    episodeSteps++;
    
    // Update statistics
    statistics["total_reward"] += reward;
    statistics["steps"]++;
    
    // Process step in algorithm if training
    if (trainingEnabled) {
        algorithm->processStep(prevState, action, reward, currState, done);
    }
    
    return reward;
}

bool RLIntegration::getAction(const AIInputFrame& state, AIOutputAction& actionOut, bool exploit) {
    if (!policy) {
        // Generate random action if no policy exists
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        
        // Random directional input
        actionOut.up = dist(gen) < 0.25f;
        actionOut.down = !actionOut.up && dist(gen) < 0.33f;
        actionOut.left = dist(gen) < 0.25f;
        actionOut.right = !actionOut.left && dist(gen) < 0.33f;
        
        // Random button presses
        for (int i = 0; i < 6; i++) {
            actionOut.buttons[i] = dist(gen) < 0.1f;
        }
        
        return true;
    }
    
    // Use policy to get action
    return policy->predict(state, actionOut, exploit);
}

void RLIntegration::enableTraining(bool enable) {
    trainingEnabled = enable;
    
    // Reset statistics if enabling training
    if (enable) {
        resetStatistics();
    }
}

bool RLIntegration::isTrainingEnabled() const {
    return trainingEnabled;
}

void RLIntegration::enableICM(bool enable, float scale) {
    icmEnabled = enable;
    
    if (enable) {
        // Create ICM if it doesn't exist
        if (!icm && policy) {
            icm = std::make_unique<ICMModule>(policy);
            icm->initialize();
        }
        
        // Set reward scale
        if (icm) {
            icm->setRewardScale(scale);
        }
    } else {
        // Clean up ICM
        icm.reset();
    }
}

bool RLIntegration::isICMEnabled() const {
    return icmEnabled && icm != nullptr;
}

bool RLIntegration::saveState(const std::string& path) {
    if (!policy) {
        std::cerr << "Cannot save state: No policy exists" << std::endl;
        return false;
    }
    
    bool success = true;
    
    // Save policy
    success &= policy->save(path);
    
    // Save algorithm state if it exists
    if (algorithm) {
        success &= algorithm->save(path + ".alg");
    }
    
    // Save ICM state if it exists
    if (icm) {
        success &= icm->save(path + ".icm");
    }
    
    // Save configuration
    std::ofstream configFile(path + ".config");
    if (configFile) {
        configFile << "algorithm=" << algorithmType << std::endl;
        configFile << "training_enabled=" << (trainingEnabled ? "true" : "false") << std::endl;
        configFile << "icm_enabled=" << (icmEnabled ? "true" : "false") << std::endl;
        
        // Save hyperparameters
        if (algorithm) {
            auto hyperparams = algorithm->getHyperparameters();
            for (const auto& param : hyperparams) {
                configFile << param.first << "=" << param.second << std::endl;
            }
        }
        
        configFile.close();
    } else {
        success = false;
    }
    
    return success;
}

bool RLIntegration::loadState(const std::string& path) {
    if (!policy) {
        std::cerr << "Cannot load state: No policy exists" << std::endl;
        return false;
    }
    
    bool success = true;
    
    // Load policy
    success &= policy->load(path);
    
    // Load configuration
    std::ifstream configFile(path + ".config");
    if (configFile) {
        std::string line;
        std::unordered_map<std::string, float> hyperparams;
        
        while (std::getline(configFile, line)) {
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') {
                continue;
            }
            
            std::istringstream iss(line);
            std::string key, value;
            
            if (std::getline(iss, key, '=') && std::getline(iss, value)) {
                // Trim whitespace
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
                
                // Process configuration keys
                if (key == "algorithm") {
                    setAlgorithm(value);
                }
                else if (key == "training_enabled") {
                    trainingEnabled = (value == "true" || value == "1");
                }
                else if (key == "icm_enabled") {
                    icmEnabled = (value == "true" || value == "1");
                }
                // Process numerical parameters
                else {
                    try {
                        hyperparams[key] = std::stof(value);
                    } catch (const std::exception& e) {
                        std::cerr << "Error parsing parameter " << key << ": " << e.what() << std::endl;
                    }
                }
            }
        }
        
        // Set hyperparameters
        if (!hyperparams.empty() && algorithm) {
            algorithm->setHyperparameters(hyperparams);
        }
        
        configFile.close();
    }
    
    // Load algorithm state if it exists
    if (algorithm) {
        // Check if algorithm state file exists
        std::ifstream checkFile(path + ".alg");
        if (checkFile.good()) {
            checkFile.close();
            success &= algorithm->load(path + ".alg");
        }
    }
    
    // Create ICM if enabled
    if (icmEnabled && !icm && policy) {
        icm = std::make_unique<ICMModule>(policy);
        icm->initialize();
        
        // Load ICM state if it exists
        std::ifstream checkICMFile(path + ".icm");
        if (checkICMFile.good()) {
            checkICMFile.close();
            success &= icm->load(path + ".icm");
        }
    }
    
    return success;
}

void RLIntegration::startEpisode() {
    // Reset episode stats
    episodeTotalReward = 0.0f;
    episodeSteps = 0;
    
    // Update statistics
    statistics["episodes"]++;
}

float RLIntegration::endEpisode(bool success) {
    // Update episode statistics
    episodeCount++;
    float avgReward = episodeTotalReward / (episodeSteps > 0 ? episodeSteps : 1);
    
    // Store episode statistics
    episodeStatistics.push_back({episodeCount, episodeSteps, episodeTotalReward, avgReward, success});
    
    // Update global statistics
    statistics["episodes"]++;
    statistics["success_episodes"] += success ? 1 : 0;
    
    // Calculate success rate
    statistics["success_rate"] = statistics["success_episodes"] / statistics["episodes"];
    
    // Train the policy if training is enabled
    if (trainingEnabled && algorithm) {
        algorithm->endEpisode(success);
        
        // Periodically update the policy (actual implementation would be more sophisticated)
        if (episodeCount % 5 == 0) {
            algorithm->updatePolicy();
        }
    }
    
    // Log episode results
    std::cout << "Episode " << episodeCount << " completed: " 
              << episodeSteps << " steps, " 
              << episodeTotalReward << " total reward, "
              << avgReward << " avg reward, "
              << (success ? "success" : "failure") << std::endl;
    
    // Reset episode variables for next episode
    float finalReward = episodeTotalReward;
    episodeTotalReward = 0.0f;
    episodeSteps = 0;
    
    return finalReward;
}

void RLIntegration::startDistributedTraining(int numWorkers) {
    if (numWorkers <= 0) {
        std::cerr << "Invalid number of workers: " << numWorkers << std::endl;
        return;
    }
    
    // Ensure we have a policy
    if (!policy) {
        std::cerr << "Cannot start distributed training without a policy" << std::endl;
        return;
    }
    
    // Stop previous training if any
    if (distributedTrainer) {
        stopDistributedTraining();
    }
    
    // Create distributed trainer
    distributedTrainer = std::make_unique<DistributedTrainer>(policy, numWorkers);
    
    // Set algorithm type from current algorithm
    distributedTrainer->setAlgorithm(algorithmType);
    
    // Set hyperparameters
    std::unordered_map<std::string, float> hyperparams;
    if (algorithm) {
        hyperparams = algorithm->getHyperparameters();
    } else {
        hyperparams = RLAlgorithmFactory::getDefaultHyperparameters(algorithmType);
    }
    distributedTrainer->setHyperparameters(hyperparams);
    
    // Configure experience sharing
    if (algorithmType == "a3c") {
        // A3C typically doesn't use experience sharing
        distributedTrainer->setExperienceSharing(false);
    } else {
        // PPO and other algorithms benefit from experience sharing
        distributedTrainer->setExperienceSharing(true, 10000);
    }
    
    // Start distributed training with 1000 episodes per worker
    distributedTrainer->startTraining(1000);
    
    std::cout << "Started distributed training with " << numWorkers << " workers" << std::endl;
}

void RLIntegration::stopDistributedTraining() {
    if (distributedTrainer) {
        distributedTrainer->stopTraining();
        
        // Copy the global policy back from the distributed trainer
        AITorchPolicy* globalPolicy = distributedTrainer->getGlobalPolicy();
        if (globalPolicy && policy) {
            policy->copyFrom(globalPolicy);
        }
        
        // Update statistics
        statistics["distributed_episodes"] = distributedTrainer->getTotalEpisodes();
        
        // Clean up
        distributedTrainer.reset();
        
        std::cout << "Stopped distributed training" << std::endl;
    }
}

RewardFunction RLIntegration::createRewardFunction(const std::string& gameType) {
    if (gameType == "fighting") {
        return createFightingGameReward();
    } else if (gameType == "platformer") {
        return createPlatformerReward();
    } else if (gameType == "puzzle") {
        return createPuzzleGameReward();
    } else if (gameType == "shooter") {
        return createShooterReward();
    } else {
        // Default reward function
        return [](const AIInputFrame& prevState, const AIInputFrame& currState, const AIOutputAction& action) {
            // Simple default reward: 0.1 per frame to encourage survival
            return 0.1f;
        };
    }
}

const std::unordered_map<std::string, float>& RLIntegration::getStatistics() const {
    return statistics;
}

const std::vector<EpisodeStats>& RLIntegration::getEpisodeStatistics() const {
    return episodeStatistics;
}

void RLIntegration::resetStatistics() {
    statistics.clear();
    statistics["episodes"] = 0;
    statistics["steps"] = 0;
    statistics["total_reward"] = 0.0f;
    statistics["success_episodes"] = 0;
    statistics["success_rate"] = 0.0f;
    
    // Reset episode variables
    episodeCount = 0;
    episodeTotalReward = 0.0f;
    episodeSteps = 0;
    
    // Clear episode statistics
    episodeStatistics.clear();
}

float RLIntegration::defaultReward(const AIInputFrame& prevState, const AIInputFrame& currState, 
                                 const AIOutputAction& action) {
    // Default reward function provides a small negative reward to encourage faster completion
    return -0.01f;
}

void RLIntegration::processGameMemory(const AIInputFrame& state) {
    // In a real implementation, this would extract game-specific information from memory
    // For now, we just store the state for reference
    lastState = state;
}

//------------------------------------------------------------------------------
// Game-specific reward functions
//------------------------------------------------------------------------------

RewardFunction createFightingGameReward() {
    return [](const AIInputFrame& prevState, const AIInputFrame& currState, const AIOutputAction& action) {
        // Initialize reward
        float reward = -0.01f; // Small negative reward per step to encourage faster completion
        
        // This is a stub that would extract game-specific memory for a fighting game
        // In a real implementation, we'd check health, position, hits landed, etc.
        
        // Get memory access (simulated for this stub)
        uint32_t prevPlayerHealth = 100; // Would be extracted from prevState
        uint32_t currPlayerHealth = 100; // Would be extracted from currState
        uint32_t prevOpponentHealth = 100; // Would be extracted from prevState
        uint32_t currOpponentHealth = 100; // Would be extracted from currState
        
        // Health change rewards
        if (prevPlayerHealth > currPlayerHealth) {
            // Player lost health, negative reward
            reward -= (prevPlayerHealth - currPlayerHealth) * 0.1f;
        }
        
        if (prevOpponentHealth > currOpponentHealth) {
            // Opponent lost health, positive reward
            reward += (prevOpponentHealth - currOpponentHealth) * 0.2f;
        }
        
        // Reward for landing special moves (would be detected from game memory)
        bool landedSpecialMove = false; // Would be detected
        if (landedSpecialMove) {
            reward += 1.0f;
        }
        
        // Reward for blocking (would be detected from game memory)
        bool blockedAttack = false; // Would be detected
        if (blockedAttack) {
            reward += 0.5f;
        }
        
        // Reward for winning a round (would be detected from game memory)
        bool wonRound = false; // Would be detected
        if (wonRound) {
            reward += 10.0f;
        }
        
        // Penalty for losing a round (would be detected from game memory)
        bool lostRound = false; // Would be detected
        if (lostRound) {
            reward -= 5.0f;
        }
        
        return reward;
    };
}

RewardFunction createPlatformerReward() {
    return [](const AIInputFrame& prevState, const AIInputFrame& currState, const AIOutputAction& action) {
        // Initialize reward
        float reward = -0.01f; // Small negative reward per step to encourage faster completion
        
        // This is a stub that would extract game-specific memory for a platformer
        // In a real implementation, we'd check position, coins collected, enemies defeated, etc.
        
        // Simulate extracting position from memory
        float prevX = 0.0f; // Would be extracted from prevState
        float currX = 0.0f; // Would be extracted from currState
        float prevY = 0.0f; // Would be extracted from prevState
        float currY = 0.0f; // Would be extracted from currState
        
        // Reward for moving right (progress)
        if (currX > prevX) {
            reward += (currX - prevX) * 0.1f;
        }
        
        // Additional reward for collecting coins
        int prevCoins = 0; // Would be extracted from prevState
        int currCoins = 0; // Would be extracted from currState
        if (currCoins > prevCoins) {
            reward += (currCoins - prevCoins) * 0.5f;
        }
        
        // Reward for defeating enemies
        int prevEnemiesDefeated = 0; // Would be extracted from prevState
        int currEnemiesDefeated = 0; // Would be extracted from currState
        if (currEnemiesDefeated > prevEnemiesDefeated) {
            reward += (currEnemiesDefeated - prevEnemiesDefeated) * 1.0f;
        }
        
        // Penalty for taking damage
        int prevLives = 3; // Would be extracted from prevState
        int currLives = 3; // Would be extracted from currState
        if (currLives < prevLives) {
            reward -= (prevLives - currLives) * 3.0f;
        }
        
        // Reward for completing a level
        bool levelCompleted = false; // Would be detected from game memory
        if (levelCompleted) {
            reward += 20.0f;
        }
        
        return reward;
    };
}

RewardFunction createPuzzleGameReward() {
    return [](const AIInputFrame& prevState, const AIInputFrame& currState, const AIOutputAction& action) {
        // Initialize reward
        float reward = -0.02f; // Slightly higher negative reward to encourage efficient solutions
        
        // This is a stub that would extract game-specific memory for a puzzle game
        // In a real implementation, we'd check score, pieces matched, level progress, etc.
        
        // Reward for increasing score
        int prevScore = 0; // Would be extracted from prevState
        int currScore = 0; // Would be extracted from currState
        if (currScore > prevScore) {
            reward += (currScore - prevScore) * 0.01f;
        }
        
        // Reward for matching pieces
        int prevMatches = 0; // Would be extracted from prevState
        int currMatches = 0; // Would be extracted from currState
        if (currMatches > prevMatches) {
            reward += (currMatches - prevMatches) * 0.5f;
        }
        
        // Reward for combos
        int comboCounter = 0; // Would be extracted from currState
        if (comboCounter > 1) {
            reward += comboCounter * 0.2f;
        }
        
        // Reward for clearing special items
        int prevSpecialItems = 0; // Would be extracted from prevState
        int currSpecialItems = 0; // Would be extracted from currState
        if (currSpecialItems > prevSpecialItems) {
            reward += (currSpecialItems - prevSpecialItems) * 1.0f;
        }
        
        // Reward for completing a level
        bool levelCompleted = false; // Would be detected from game memory
        if (levelCompleted) {
            reward += 10.0f;
        }
        
        // Penalty for game over
        bool gameOver = false; // Would be detected from game memory
        if (gameOver) {
            reward -= 5.0f;
        }
        
        return reward;
    };
}

RewardFunction createShooterReward() {
    return [](const AIInputFrame& prevState, const AIInputFrame& currState, const AIOutputAction& action) {
        // Initialize reward
        float reward = -0.01f; // Small negative reward per step to encourage faster completion
        
        // This is a stub that would extract game-specific memory for a shooter game
        // In a real implementation, we'd check health, ammo, enemies defeated, position, etc.
        
        // Reward for defeating enemies
        int prevEnemiesDefeated = 0; // Would be extracted from prevState
        int currEnemiesDefeated = 0; // Would be extracted from currState
        if (currEnemiesDefeated > prevEnemiesDefeated) {
            reward += (currEnemiesDefeated - prevEnemiesDefeated) * 1.0f;
        }
        
        // Penalty for taking damage
        int prevHealth = 100; // Would be extracted from prevState
        int currHealth = 100; // Would be extracted from currState
        if (currHealth < prevHealth) {
            reward -= (prevHealth - currHealth) * 0.05f;
        }
        
        // Reward for collecting items
        int prevItems = 0; // Would be extracted from prevState
        int currItems = 0; // Would be extracted from currState
        if (currItems > prevItems) {
            reward += (currItems - prevItems) * 0.5f;
        }
        
        // Reward for accuracy
        int prevShotsFired = 0; // Would be extracted from prevState
        int currShotsFired = 0; // Would be extracted from currState
        int prevHits = 0; // Would be extracted from prevState
        int currHits = 0; // Would be extracted from currState
        
        if (currShotsFired > prevShotsFired) {
            int newShots = currShotsFired - prevShotsFired;
            int newHits = currHits - prevHits;
            
            if (newShots > 0) {
                float accuracy = static_cast<float>(newHits) / newShots;
                reward += accuracy * 0.3f;
            }
        }
        
        // Reward for completing an objective
        bool objectiveCompleted = false; // Would be detected from game memory
        if (objectiveCompleted) {
            reward += 5.0f;
        }
        
        // Reward for completing a level
        bool levelCompleted = false; // Would be detected from game memory
        if (levelCompleted) {
            reward += 20.0f;
        }
        
        // Penalty for game over
        bool gameOver = false; // Would be detected from game memory
        if (gameOver) {
            reward -= 10.0f;
        }
        
        return reward;
    };
}

//------------------------------------------------------------------------------
// C API Implementation
//------------------------------------------------------------------------------

extern "C" {

void* FBNEO_RL_Create() {
    return new fbneo::ai::RLIntegration();
}

void FBNEO_RL_Destroy(void* handle) {
    if (handle) {
        delete static_cast<fbneo::ai::RLIntegration*>(handle);
    }
}

int FBNEO_RL_Initialize(void* handle, const char* configPath) {
    if (!handle) return 0;
    
    fbneo::ai::RLIntegration* rl = static_cast<fbneo::ai::RLIntegration*>(handle);
    
    std::string path = configPath ? configPath : "";
    return rl->initialize(path) ? 1 : 0;
}

int FBNEO_RL_SetAlgorithm(void* handle, const char* type) {
    if (!handle || !type) return 0;
    
    fbneo::ai::RLIntegration* rl = static_cast<fbneo::ai::RLIntegration*>(handle);
    
    return rl->setAlgorithm(type) ? 1 : 0;
}

const char* FBNEO_RL_GetAlgorithmType(void* handle) {
    if (!handle) {
        return "";
    }
    
    // Note: This is not memory safe in real usage - would need a static buffer
    fbneo::ai::RLIntegration* rl = static_cast<fbneo::ai::RLIntegration*>(handle);
    static std::string type;
    type = rl->getAlgorithmType();
    return type.c_str();
}

void FBNEO_RL_EnableTraining(void* handle, int enable) {
    if (!handle) return;
    
    fbneo::ai::RLIntegration* rl = static_cast<fbneo::ai::RLIntegration*>(handle);
    
    rl->enableTraining(enable != 0);
}

int FBNEO_RL_IsTrainingEnabled(void* handle) {
    if (!handle) return 0;
    
    fbneo::ai::RLIntegration* rl = static_cast<fbneo::ai::RLIntegration*>(handle);
    
    return rl->isTrainingEnabled() ? 1 : 0;
}

void FBNEO_RL_StartEpisode(void* handle) {
    if (!handle) return;
    
    fbneo::ai::RLIntegration* rl = static_cast<fbneo::ai::RLIntegration*>(handle);
    
    rl->startEpisode();
}

float FBNEO_RL_EndEpisode(void* handle, int success) {
    if (!handle) return 0.0f;
    
    fbneo::ai::RLIntegration* rl = static_cast<fbneo::ai::RLIntegration*>(handle);
    
    return rl->endEpisode(success != 0);
}

float FBNEO_RL_ProcessStep(void* handle, const void* prevState, const void* action,
                         const void* currState, int done) {
    if (!handle || !prevState || !action || !currState) return 0.0f;
    
    fbneo::ai::RLIntegration* rl = static_cast<fbneo::ai::RLIntegration*>(handle);
    
    const AIInputFrame* prev = static_cast<const AIInputFrame*>(prevState);
    const AIOutputAction* act = static_cast<const AIOutputAction*>(action);
    const AIInputFrame* curr = static_cast<const AIInputFrame*>(currState);
    
    return rl->processStep(*prev, *act, *curr, done != 0);
}

int FBNEO_RL_GetAction(void* handle, const void* state, void* actionOut, int exploit) {
    if (!handle || !state || !actionOut) return 0;
    
    fbneo::ai::RLIntegration* rl = static_cast<fbneo::ai::RLIntegration*>(handle);
    
    const AIInputFrame* statePtr = static_cast<const AIInputFrame*>(state);
    AIOutputAction* actionPtr = static_cast<AIOutputAction*>(actionOut);
    
    return rl->getAction(*statePtr, *actionPtr, exploit != 0) ? 1 : 0;
}

int FBNEO_RL_Save(void* handle, const char* path) {
    if (!handle || !path) return 0;
    
    fbneo::ai::RLIntegration* rl = static_cast<fbneo::ai::RLIntegration*>(handle);
    
    return rl->saveState(path) ? 1 : 0;
}

int FBNEO_RL_Load(void* handle, const char* path) {
    if (!handle || !path) return 0;
    
    fbneo::ai::RLIntegration* rl = static_cast<fbneo::ai::RLIntegration*>(handle);
    
    return rl->loadState(path) ? 1 : 0;
}

void FBNEO_RL_EnableICM(void* handle, int enable, float scale) {
    if (!handle) return;
    
    fbneo::ai::RLIntegration* rl = static_cast<fbneo::ai::RLIntegration*>(handle);
    
    rl->enableICM(enable != 0, scale);
}

int FBNEO_RL_IsICMEnabled(void* handle) {
    if (!handle) return 0;
    
    fbneo::ai::RLIntegration* rl = static_cast<fbneo::ai::RLIntegration*>(handle);
    
    return rl->isICMEnabled() ? 1 : 0;
}

void FBNEO_RL_StartDistributedTraining(void* handle, int numWorkers) {
    if (!handle) return;
    
    fbneo::ai::RLIntegration* rl = static_cast<fbneo::ai::RLIntegration*>(handle);
    
    rl->startDistributedTraining(numWorkers);
}

void FBNEO_RL_StopDistributedTraining(void* handle) {
    if (!handle) return;
    
    fbneo::ai::RLIntegration* rl = static_cast<fbneo::ai::RLIntegration*>(handle);
    
    rl->stopDistributedTraining();
}

} // extern "C" 