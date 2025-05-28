#pragma once

#include <vector>
#include <memory>
#include <random>
#include <string>
#include <functional>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "ai_input_frame.h"
#include "ai_output_action.h"
#include "ai_definitions.h"

namespace fbneo {
namespace ai {

// Forward declarations
class AITorchPolicy;
class AIMemoryMapping;

/**
 * @brief Base class for experience data used by RL algorithms
 */
struct Experience {
    std::vector<float> state;        // State vector
    std::vector<float> action;       // Action vector
    float reward;                   // Reward received
    std::vector<float> next_state;   // Next state vector
    bool done;                      // Whether episode is done
    float value;                    // Value estimate (for some algorithms)
    float advantage;                // Advantage estimate (for some algorithms)
    float log_prob;                 // Log probability of action (for some algorithms)
    
    Experience() : reward(0.0f), done(false), value(0.0f), advantage(0.0f), log_prob(0.0f) {}
};

/**
 * @brief Experience buffer for storing training data
 */
class ExperienceBuffer {
public:
    ExperienceBuffer(size_t capacity = 10000);
    ~ExperienceBuffer();
    
    // Add an experience to the buffer
    void add(const Experience& exp);
    
    // Sample a batch of experiences
    std::vector<Experience> sample(size_t batch_size);
    
    // Clear the buffer
    void clear();
    
    // Get the current size of the buffer
    size_t size() const;
    
    // Check if the buffer is empty
    bool empty() const;
    
    // Set priority for prioritized experience replay
    void setPriority(size_t index, float priority);
    
    // Enable/disable prioritized experience replay
    void setPrioritizedReplay(bool enabled, float alpha = 0.6f, float beta = 0.4f);
    
private:
    std::vector<Experience> buffer;
    size_t capacity;
    std::vector<float> priorities;
    bool prioritizedReplay;
    float priorityAlpha;
    float priorityBeta;
    std::mt19937 rng;
};

/**
 * @brief Reinforcement Learning algorithm base class
 */
class RLAlgorithm {
public:
    RLAlgorithm(AITorchPolicy* policy);
    virtual ~RLAlgorithm();
    
    // Train on a batch of experiences
    virtual void train(const std::vector<Experience>& batch) = 0;
    
    // Process a step and add to experience buffer
    virtual void processStep(const AIInputFrame& state, const AIOutputAction& action, 
                           float reward, const AIInputFrame& next_state, bool done);
    
    // Set hyperparameters
    virtual void setHyperparameters(const std::unordered_map<std::string, float>& params);
    
    // Save the algorithm state
    virtual bool save(const std::string& path) = 0;
    
    // Load the algorithm state
    virtual bool load(const std::string& path) = 0;
    
    // Get the policy
    AITorchPolicy* getPolicy() const;
    
    // Get the experience buffer
    ExperienceBuffer& getBuffer();
    
    // Set the learning rate
    void setLearningRate(float lr);
    
    // Get the learning rate
    float getLearningRate() const;
    
    // Set gamma (discount factor)
    void setGamma(float gamma);
    
    // Get gamma
    float getGamma() const;
    
protected:
    AITorchPolicy* policy;
    ExperienceBuffer buffer;
    float learningRate;
    float gamma;                    // Discount factor
    int updateFrequency;            // How often to update the network
    int steps;                      // Step counter
    float clipEpsilon;              // For PPO clipping
};

/**
 * @brief Proximal Policy Optimization (PPO) algorithm
 */
class PPOAlgorithm : public RLAlgorithm {
public:
    PPOAlgorithm(AITorchPolicy* policy);
    ~PPOAlgorithm() override;
    
    // Train on a batch of experiences
    void train(const std::vector<Experience>& batch) override;
    
    // Process a step with PPO-specific logic
    void processStep(const AIInputFrame& state, const AIOutputAction& action, 
                   float reward, const AIInputFrame& next_state, bool done) override;
    
    // Set PPO-specific hyperparameters
    void setHyperparameters(const std::unordered_map<std::string, float>& params) override;
    
    // Save the PPO state
    bool save(const std::string& path) override;
    
    // Load the PPO state
    bool load(const std::string& path) override;
    
    // Compute GAE (Generalized Advantage Estimation)
    void computeGAE(std::vector<Experience>& trajectory, float lambda = 0.95f);
    
    // Run multiple epochs of PPO training on a trajectory
    void trainEpochs(std::vector<Experience>& trajectory, int epochs = 4);
    
private:
    float clipEpsilon;              // PPO clipping parameter
    float vfCoeff;                  // Value function coefficient
    float entropyCoeff;             // Entropy coefficient for exploration
    float lambda;                   // GAE lambda parameter
    int epochs;                     // Number of epochs to train on each batch
    
    // Target networks (for stability)
    std::unique_ptr<AITorchPolicy> targetPolicy;
    
    // Update the target network
    void updateTargetNetwork();
};

/**
 * @brief Asynchronous Advantage Actor-Critic (A3C) algorithm
 */
class A3CAlgorithm : public RLAlgorithm {
public:
    A3CAlgorithm(AITorchPolicy* globalPolicy, int numWorkers = 4);
    ~A3CAlgorithm() override;
    
    // Train on a batch of experiences
    void train(const std::vector<Experience>& batch) override;
    
    // Process a step with A3C-specific logic
    void processStep(const AIInputFrame& state, const AIOutputAction& action, 
                   float reward, const AIInputFrame& next_state, bool done) override;
    
    // Set A3C-specific hyperparameters
    void setHyperparameters(const std::unordered_map<std::string, float>& params) override;
    
    // Save the A3C state
    bool save(const std::string& path) override;
    
    // Load the A3C state
    bool load(const std::string& path) override;
    
    // Start worker threads
    void startWorkers();
    
    // Stop worker threads
    void stopWorkers();
    
private:
    struct WorkerState {
        std::unique_ptr<AITorchPolicy> policy;
        ExperienceBuffer buffer;
        std::thread thread;
        std::atomic<bool> running;
        int id;
    };
    
    AITorchPolicy* globalPolicy;
    std::vector<std::unique_ptr<WorkerState>> workers;
    std::mutex globalMutex;
    int numWorkers;
    std::atomic<bool> shouldStop;
    
    // Worker thread function
    void workerFunction(WorkerState* state);
    
    // Compute gradients and update global network
    void updateGlobalNetwork(WorkerState* state, const std::vector<Experience>& trajectory);
    
    // Helper method to convert vector to AIInputFrame for policy operations
    AIInputFrame convertVectorToInputFrame(const std::vector<float>& vec);
    
    // Helper method to convert vector to AIOutputAction for policy operations
    AIOutputAction convertVectorToOutputAction(const std::vector<float>& vec);
    
    // Synchronize all worker policies with the global policy
    void synchronizeWorkers();
};

/**
 * @brief Intrinsic Curiosity Module (ICM) for exploration
 */
class ICMModule {
public:
    ICMModule(AITorchPolicy* policy);
    ~ICMModule();
    
    // Initialize the ICM
    bool initialize();
    
    // Calculate intrinsic reward
    float calculateIntrinsicReward(const AIInputFrame& state, const AIOutputAction& action, 
                                  const AIInputFrame& next_state);
    
    // Update the ICM model
    void update(const std::vector<Experience>& batch);
    
    // Set the intrinsic reward scale
    void setRewardScale(float scale);
    
    // Get the intrinsic reward scale
    float getRewardScale() const;
    
    // Save the ICM state
    bool save(const std::string& path);
    
    // Load the ICM state
    bool load(const std::string& path);
    
private:
    struct ICMState {
        std::vector<float> encoded;
        std::vector<float> predicted;
        float forwardLoss;
        float inverseLoss;
    };
    
    AITorchPolicy* policy;
    std::unique_ptr<AITorchPolicy> featureNetwork;
    std::unique_ptr<AITorchPolicy> forwardModel;
    std::unique_ptr<AITorchPolicy> inverseModel;
    float rewardScale;
    float forwardLossWeight;
    float inverseLossWeight;
    
    // Encode state to features
    std::vector<float> encodeState(const AIInputFrame& state);
    
    // Predict next state features
    std::vector<float> predictNextState(const std::vector<float>& stateFeatures, 
                                      const AIOutputAction& action);
    
    // Predict action from states
    std::vector<float> predictAction(const std::vector<float>& stateFeatures, 
                                   const std::vector<float>& nextStateFeatures);
};

/**
 * @brief Factory for creating RL algorithms
 */
class RLAlgorithmFactory {
public:
    // Create an RL algorithm of the specified type
    static std::unique_ptr<RLAlgorithm> create(const std::string& type, AITorchPolicy* policy);
    
    // Get available algorithm types
    static std::vector<std::string> getAvailableAlgorithms();
    
    // Get default hyperparameters for an algorithm
    static std::unordered_map<std::string, float> getDefaultHyperparameters(const std::string& type);
};

} // namespace ai
} // namespace fbneo 