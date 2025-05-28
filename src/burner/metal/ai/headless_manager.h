#pragma once

#include "headless_mode.h"
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <thread>
#include <condition_variable>
#include <queue>

namespace fbneo {
namespace ai {

// Forward declarations
class HeadlessInstance;

/**
 * @brief Manages multiple headless instances for parallel training
 * 
 * This class provides thread-safe management of multiple headless instances,
 * allowing parallel execution of different ROMs or configurations.
 */
class HeadlessManager {
public:
    /**
     * @brief Get the singleton instance
     * 
     * @return Reference to singleton instance
     */
    static HeadlessManager& getInstance();
    
    /**
     * @brief Initialize the manager
     * 
     * @param maxInstances Maximum number of instances to support
     * @return True if initialized successfully
     */
    bool initialize(int maxInstances = 16);
    
    /**
     * @brief Shutdown the manager
     */
    void shutdown();
    
    /**
     * @brief Create a new headless instance
     * 
     * @param config Configuration for the instance
     * @return Instance ID, or -1 on failure
     */
    int createInstance(const HeadlessConfig& config);
    
    /**
     * @brief Get a headless instance by ID
     * 
     * @param instanceId Instance ID to retrieve
     * @return Pointer to instance, or nullptr if not found
     */
    HeadlessRunner* getInstance(int instanceId);
    
    /**
     * @brief Remove an instance by ID
     * 
     * @param instanceId Instance ID to remove
     * @return True if removed successfully
     */
    bool removeInstance(int instanceId);
    
    /**
     * @brief Get all active instance IDs
     * 
     * @return Vector of instance IDs
     */
    std::vector<int> getInstanceIds() const;
    
    /**
     * @brief Get the number of active instances
     * 
     * @return Number of active instances
     */
    int getInstanceCount() const;
    
    /**
     * @brief Start all instances
     * 
     * @param romPath Path to ROM file
     * @param romName Name of ROM
     * @return Number of instances started successfully
     */
    int startAll(const char* romPath, const char* romName);
    
    /**
     * @brief Stop all instances
     */
    void stopAll();
    
    /**
     * @brief Run a function on all instances
     * 
     * @param func Function to run on each instance
     */
    void forEachInstance(std::function<void(HeadlessRunner*)> func);
    
    /**
     * @brief Run episodes on all instances in parallel
     * 
     * @param numEpisodes Number of episodes to run
     * @return Number of completed episodes
     */
    int runEpisodesParallel(int numEpisodes);
    
private:
    // Private constructor for singleton
    HeadlessManager();
    // Destructor
    ~HeadlessManager();
    
    // Deleted copy/move constructors and assignment operators
    HeadlessManager(const HeadlessManager&) = delete;
    HeadlessManager& operator=(const HeadlessManager&) = delete;
    HeadlessManager(HeadlessManager&&) = delete;
    HeadlessManager& operator=(HeadlessManager&&) = delete;
    
    // Instance map
    std::unordered_map<int, std::unique_ptr<HeadlessRunner>> m_instances;
    
    // Thread-safety
    mutable std::mutex m_instancesMutex;
    
    // Next instance ID
    std::atomic<int> m_nextInstanceId;
    
    // Maximum number of instances
    int m_maxInstances;
    
    // Manager state
    std::atomic<bool> m_initialized;
};

/**
 * @class HeadlessInstance
 * @brief Wraps a headless runner with process isolation
 * 
 * This class provides a way to run headless instances in separate
 * processes for true parallelism and better isolation.
 */
class HeadlessInstance {
public:
    /**
     * @brief Constructor
     * 
     * @param instanceId Unique identifier for this instance
     * @param config Configuration for the instance
     */
    HeadlessInstance(int instanceId, const HeadlessConfig& config);
    
    /**
     * @brief Destructor
     */
    ~HeadlessInstance();
    
    /**
     * @brief Start the instance
     * 
     * @param romPath Path to ROM file
     * @param romName Name of ROM
     * @return True if started successfully
     */
    bool start(const char* romPath, const char* romName);
    
    /**
     * @brief Stop the instance
     */
    void stop();
    
    /**
     * @brief Run episodes
     * 
     * @param numEpisodes Number of episodes to run
     * @return Number of completed episodes
     */
    int runEpisodes(int numEpisodes);
    
    /**
     * @brief Set action callback
     * 
     * @param callback Function to call when action is needed
     */
    void setActionCallback(ActionCallback callback);
    
    /**
     * @brief Set frame callback
     * 
     * @param callback Function to call when frame is processed
     */
    void setFrameCallback(FrameCallback callback);
    
    /**
     * @brief Set reward callback
     * 
     * @param callback Function to call to calculate reward
     */
    void setRewardCallback(RewardCallback callback);
    
    /**
     * @brief Set episode complete callback
     * 
     * @param callback Function to call when episode completes
     */
    void setEpisodeCompleteCallback(EpisodeCompleteCallback callback);
    
    /**
     * @brief Get instance ID
     * 
     * @return Instance ID
     */
    int getInstanceId() const { return m_instanceId; }
    
    /**
     * @brief Get episode count
     * 
     * @return Number of completed episodes
     */
    int getEpisodeCount() const;
    
    /**
     * @brief Get frame count
     * 
     * @return Number of processed frames
     */
    int getFrameCount() const;
    
    /**
     * @brief Get runner state
     * 
     * @return True if running
     */
    bool isRunning() const { return m_running; }
    
private:
    // Instance ID
    int m_instanceId;
    
    // Configuration
    HeadlessConfig m_config;
    
    // Headless runner
    std::unique_ptr<HeadlessRunner> m_runner;
    
    // State
    std::atomic<bool> m_running;
    
    // Process ID (if using process isolation)
    int m_processId;
    
    // For process isolation
    void runInChildProcess();
    bool isChildProcess() const { return m_processId == 0; }
    
    // Callbacks
    ActionCallback m_actionCallback;
    FrameCallback m_frameCallback;
    RewardCallback m_rewardCallback;
    EpisodeCompleteCallback m_episodeCallback;
    
    // IPC structures
    // These would be used for process isolation if implemented
    struct SharedMemory {
        std::atomic<int> episodeCount;
        std::atomic<int> frameCount;
        std::atomic<bool> running;
        
        // Would have more fields for sharing observations, actions, etc.
    };
    
    // Shared memory for IPC
    std::shared_ptr<SharedMemory> m_sharedMemory;
};

} // namespace ai
} // namespace fbneo 