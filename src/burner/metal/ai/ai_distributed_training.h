#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>
#include "ai_rl_algorithms.h"
#include "ai_torch_policy.h"

namespace fbneo {
namespace ai {

/**
 * @brief Class for managing distributed training of AI models
 * 
 * This class provides functionality for:
 * 1. Distribution of model parameters to workers
 * 2. Collection and aggregation of gradients from workers
 * 3. Experience sharing between workers
 * 4. Synchronization of models across workers
 */
class DistributedTrainer {
public:
    /**
     * @brief Constructor
     * 
     * @param globalPolicy The global policy to be shared across workers
     * @param numWorkers Number of worker threads to spawn
     */
    DistributedTrainer(AITorchPolicy* globalPolicy, int numWorkers = 4);
    
    /**
     * @brief Destructor
     */
    ~DistributedTrainer();
    
    /**
     * @brief Start distributed training
     * 
     * @param episodesPerWorker Number of episodes for each worker to run
     * @return True if training was successful
     */
    bool startTraining(int episodesPerWorker = 100);
    
    /**
     * @brief Stop distributed training
     */
    void stopTraining();
    
    /**
     * @brief Save the trained model
     * 
     * @param path Path to save the model to
     * @return True if save was successful
     */
    bool saveModel(const std::string& path);
    
    /**
     * @brief Load a model for continued training
     * 
     * @param path Path to load the model from
     * @return True if load was successful
     */
    bool loadModel(const std::string& path);
    
    /**
     * @brief Set hyperparameters for training
     * 
     * @param params Map of hyperparameter names to values
     */
    void setHyperparameters(const std::unordered_map<std::string, float>& params);
    
    /**
     * @brief Set the training algorithm
     * 
     * @param algorithm Algorithm to use (e.g., "ppo", "a3c")
     * @return True if algorithm was set successfully
     */
    bool setAlgorithm(const std::string& algorithm);
    
    /**
     * @brief Enable or disable experience sharing between workers
     * 
     * @param enable Whether to enable experience sharing
     * @param bufferSize Size of the shared experience buffer
     */
    void setExperienceSharing(bool enable, int bufferSize = 10000);
    
    /**
     * @brief Set model synchronization frequency
     * 
     * @param frequency How often to synchronize models (in steps)
     */
    void setSynchronizationFrequency(int frequency);
    
    /**
     * @brief Get the current training status
     * 
     * @return A string describing the current training status
     */
    std::string getStatus() const;
    
private:
    // Worker thread function
    struct WorkerState {
        int id;
        std::unique_ptr<AITorchPolicy> policy;
        std::thread thread;
        std::atomic<bool> running;
        std::atomic<int> episodesCompleted;
        std::atomic<float> totalReward;
        std::atomic<int> stepsCompleted;
    };
    
    // Global policy and worker states
    AITorchPolicy* globalPolicy;
    std::vector<std::unique_ptr<WorkerState>> workers;
    std::mutex globalMutex;
    std::atomic<bool> shouldStop;
    std::atomic<int> totalEpisodesCompleted;
    std::atomic<float> totalTrainingReward;
    std::atomic<int> totalTrainingSteps;
    
    // Hyperparameters
    float learningRate;
    float gamma;
    int syncFrequency;
    bool useExperienceSharing;
    int sharedBufferSize;
    std::string algorithmType;
    
    // Shared experience buffer
    ExperienceBuffer sharedBuffer;
    std::mutex bufferMutex;
    
    // Worker thread function
    void workerFunction(WorkerState* state, int episodesPerWorker);
    
    // Add experiences to the shared buffer
    void addToSharedBuffer(const std::vector<Experience>& experiences);
    
    // Sample experiences from the shared buffer
    std::vector<Experience> sampleFromSharedBuffer(int batchSize);
    
    // Synchronize worker policy with global policy
    void synchronizeWorker(WorkerState* state);
    
    // Update global policy with worker gradients
    void updateGlobalPolicy(const std::vector<Experience>& batch, WorkerState* state);
};

} // namespace ai
} // namespace fbneo 