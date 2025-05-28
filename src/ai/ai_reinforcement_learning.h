#pragma once

#include "ai_input_frame.h"
#include "ai_output_action.h"
#include "ai_torch_policy_model.h"
#include <string>
#include <vector>
#include <deque>
#include <random>
#include <memory>

namespace AI {

/**
 * @struct Experience
 * @brief Represents a single transition in reinforcement learning
 * 
 * Contains state, action, reward, next state, and done flag
 */
struct Experience {
    AIInputFrame state;
    AIOutputAction action;
    float reward;
    AIInputFrame nextState;
    bool done;
    
    Experience(const AIInputFrame& s, const AIOutputAction& a, float r, 
               const AIInputFrame& ns, bool d)
        : state(s), action(a), reward(r), nextState(ns), done(d) {}
};

/**
 * @class AIReinforcementLearning
 * @brief Implementation of reinforcement learning for FBNeo
 * 
 * This class provides the core functionality for training AI agents
 * using reinforcement learning techniques like PPO (Proximal Policy Optimization).
 */
class AIReinforcementLearning {
public:
    /**
     * @brief Constructor
     * @param policyModel Policy model to use for learning
     * @param bufferSize Maximum size of experience buffer
     * @param batchSize Size of batch for training
     * @param gamma Discount factor for future rewards
     * @param learningRate Learning rate for model updates
     */
    AIReinforcementLearning(std::shared_ptr<AITorchPolicyModel> policyModel,
                          size_t bufferSize = 10000,
                          size_t batchSize = 64,
                          float gamma = 0.99f,
                          float learningRate = 0.001f);
    
    /**
     * @brief Destructor
     */
    ~AIReinforcementLearning();
    
    /**
     * @brief Add an experience to the buffer
     * @param state Current state
     * @param action Action taken
     * @param reward Reward received
     * @param nextState Next state
     * @param done Whether episode is done
     */
    void addExperience(const AIInputFrame& state, const AIOutputAction& action,
                      float reward, const AIInputFrame& nextState, bool done);
    
    /**
     * @brief Train the model on the current experience buffer
     * @param epochs Number of training epochs
     * @return Average loss across all batches
     */
    float train(int epochs = 1);
    
    /**
     * @brief Train using Proximal Policy Optimization (PPO) algorithm
     * @param batch Batch of experiences to train on
     * @param clipEpsilon PPO clipping parameter (default=0.2)
     * @param learningRate Learning rate (default=current model learning rate)
     * @return Average loss
     */
    float TrainPPO(const std::vector<Experience>& batch, float clipEpsilon = 0.2f, float learningRate = 0.0f);
    
    /**
     * @brief Train using Asynchronous Advantage Actor-Critic (A3C) algorithm
     * @param batch Batch of experiences to train on
     * @param numWorkers Number of worker threads to use
     * @param learningRate Learning rate (default=current model learning rate)
     * @return Average loss
     */
    float TrainA3C(const std::vector<Experience>& batch, int numWorkers = 4, float learningRate = 0.0f);
    
    /**
     * @brief Train using Intrinsic Curiosity Module (ICM)
     * @param batch Batch of experiences to train on
     * @param forwardScale Scale for forward model loss (default=0.8)
     * @param inverseScale Scale for inverse model loss (default=0.2)
     * @return Combined loss
     */
    float TrainICM(const std::vector<Experience>& batch, float forwardScale = 0.8f, float inverseScale = 0.2f);
    
    /**
     * @brief Get the policy model
     * @return Reference to the policy model
     */
    std::shared_ptr<AITorchPolicyModel> getPolicyModel() const;
    
    /**
     * @brief Save the current model
     * @param path Path to save the model
     * @return Whether the save was successful
     */
    bool saveModel(const std::string& path) const;
    
    /**
     * @brief Load a model
     * @param path Path to load the model from
     * @return Whether the load was successful
     */
    bool loadModel(const std::string& path);
    
    /**
     * @brief Clear the experience buffer
     */
    void clearExperiences();
    
    /**
     * @brief Get the number of experiences in the buffer
     * @return Number of experiences
     */
    size_t getExperienceCount() const;
    
    /**
     * @brief Get the PPO clip ratio
     * @return Current clip ratio
     */
    float getClipRatio() const;
    
    /**
     * @brief Set the PPO clip ratio
     * @param ratio New clip ratio
     */
    void setClipRatio(float ratio);
    
    /**
     * @brief Export experiences to JSON
     * @param filename File to save experiences to
     * @return Whether the export was successful
     */
    bool exportExperiencesToJson(const std::string& filename) const;
    
    /**
     * @brief Import experiences from JSON
     * @param filename File to load experiences from
     * @return Number of experiences imported
     */
    int importExperiencesFromJson(const std::string& filename);
    
private:
    // Policy model
    std::shared_ptr<AITorchPolicyModel> m_policyModel;
    
    // Experience buffer
    std::deque<Experience> m_experienceBuffer;
    
    // Training parameters
    size_t m_bufferSize;
    size_t m_batchSize;
    float m_gamma;
    float m_learningRate;
    float m_clipRatio;
    
    // Random number generator
    std::mt19937 m_rng;
    
    // Private methods
    std::vector<Experience> sampleBatch();
    float calculateGAE(const std::vector<Experience>& batch, std::vector<float>& advantages);
    float calculatePPOLoss(const std::vector<Experience>& batch, const std::vector<float>& advantages);
};

} // namespace AI 