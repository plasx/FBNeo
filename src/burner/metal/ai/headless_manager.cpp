#include "headless_manager.h"
#include <algorithm>
#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#endif

namespace fbneo {
namespace ai {

// Singleton instance
static std::unique_ptr<HeadlessManager> s_instance = nullptr;

// Get singleton instance
HeadlessManager& HeadlessManager::getInstance() {
    if (!s_instance) {
        s_instance = std::unique_ptr<HeadlessManager>(new HeadlessManager());
    }
    return *s_instance;
}

// Constructor
HeadlessManager::HeadlessManager() 
    : m_nextInstanceId(1), m_maxInstances(16), m_initialized(false) {
}

// Destructor
HeadlessManager::~HeadlessManager() {
    shutdown();
}

// Initialize the manager
bool HeadlessManager::initialize(int maxInstances) {
    if (m_initialized) {
        return true;
    }
    
    m_maxInstances = maxInstances;
    m_nextInstanceId = 1;
    m_initialized = true;
    
    return true;
}

// Shutdown the manager
void HeadlessManager::shutdown() {
    if (!m_initialized) {
        return;
    }
    
    stopAll();
    
    // Clear all instances
    std::lock_guard<std::mutex> lock(m_instancesMutex);
    m_instances.clear();
    
    m_initialized = false;
}

// Create a new headless instance
int HeadlessManager::createInstance(const HeadlessConfig& config) {
    if (!m_initialized) {
        std::cerr << "HeadlessManager not initialized" << std::endl;
        return -1;
    }
    
    std::lock_guard<std::mutex> lock(m_instancesMutex);
    
    // Check if we've reached max instances
    if (m_instances.size() >= m_maxInstances) {
        std::cerr << "Maximum number of instances reached" << std::endl;
        return -1;
    }
    
    // Get next instance ID
    int instanceId = m_nextInstanceId++;
    
    // Create a new instance
    auto instance = std::make_unique<HeadlessRunner>();
    if (!instance->initialize(config)) {
        std::cerr << "Failed to initialize headless instance" << std::endl;
        return -1;
    }
    
    // Add to map
    m_instances[instanceId] = std::move(instance);
    
    return instanceId;
}

// Get a headless instance by ID
HeadlessRunner* HeadlessManager::getInstance(int instanceId) {
    std::lock_guard<std::mutex> lock(m_instancesMutex);
    
    auto it = m_instances.find(instanceId);
    if (it == m_instances.end()) {
        return nullptr;
    }
    
    return it->second.get();
}

// Remove an instance by ID
bool HeadlessManager::removeInstance(int instanceId) {
    std::lock_guard<std::mutex> lock(m_instancesMutex);
    
    auto it = m_instances.find(instanceId);
    if (it == m_instances.end()) {
        return false;
    }
    
    // Stop the instance first
    it->second->stop();
    
    // Remove from map
    m_instances.erase(it);
    
    return true;
}

// Get all active instance IDs
std::vector<int> HeadlessManager::getInstanceIds() const {
    std::lock_guard<std::mutex> lock(m_instancesMutex);
    
    std::vector<int> ids;
    ids.reserve(m_instances.size());
    
    for (const auto& pair : m_instances) {
        ids.push_back(pair.first);
    }
    
    return ids;
}

// Get the number of active instances
int HeadlessManager::getInstanceCount() const {
    std::lock_guard<std::mutex> lock(m_instancesMutex);
    return static_cast<int>(m_instances.size());
}

// Start all instances
int HeadlessManager::startAll(const char* romPath, const char* romName) {
    if (!m_initialized) {
        std::cerr << "HeadlessManager not initialized" << std::endl;
        return 0;
    }
    
    int startedCount = 0;
    
    std::lock_guard<std::mutex> lock(m_instancesMutex);
    
    for (auto& pair : m_instances) {
        if (pair.second->start(romPath, romName)) {
            startedCount++;
        } else {
            std::cerr << "Failed to start instance " << pair.first << std::endl;
        }
    }
    
    return startedCount;
}

// Stop all instances
void HeadlessManager::stopAll() {
    if (!m_initialized) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_instancesMutex);
    
    for (auto& pair : m_instances) {
        pair.second->stop();
    }
}

// Run a function on all instances
void HeadlessManager::forEachInstance(std::function<void(HeadlessRunner*)> func) {
    std::lock_guard<std::mutex> lock(m_instancesMutex);
    
    for (auto& pair : m_instances) {
        func(pair.second.get());
    }
}

// Run episodes on all instances in parallel
int HeadlessManager::runEpisodesParallel(int numEpisodes) {
    if (!m_initialized) {
        std::cerr << "HeadlessManager not initialized" << std::endl;
        return 0;
    }
    
    // Get a snapshot of current instances to avoid locking during the entire operation
    std::vector<HeadlessRunner*> instances;
    {
        std::lock_guard<std::mutex> lock(m_instancesMutex);
        instances.reserve(m_instances.size());
        for (auto& pair : m_instances) {
            instances.push_back(pair.second.get());
        }
    }
    
    // No instances to run
    if (instances.empty()) {
        return 0;
    }
    
    // Calculate episodes per instance
    int episodesPerInstance = numEpisodes / static_cast<int>(instances.size());
    int remainder = numEpisodes % static_cast<int>(instances.size());
    
    // Create threads for each instance
    std::vector<std::thread> threads;
    std::vector<int> completedEpisodes(instances.size(), 0);
    
    for (size_t i = 0; i < instances.size(); i++) {
        int episodesToRun = episodesPerInstance + (i < remainder ? 1 : 0);
        
        threads.emplace_back([instance = instances[i], episodesToRun, &completedEpisodes, i]() {
            completedEpisodes[i] = instance->runEpisodes(episodesToRun);
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Calculate total completed episodes
    int totalCompleted = 0;
    for (int completed : completedEpisodes) {
        totalCompleted += completed;
    }
    
    return totalCompleted;
}

// HeadlessInstance implementation
HeadlessInstance::HeadlessInstance(int instanceId, const HeadlessConfig& config)
    : m_instanceId(instanceId), m_config(config), m_running(false), m_processId(-1) {
    
    // Create shared memory for process communication
    m_sharedMemory = std::make_shared<SharedMemory>();
    m_sharedMemory->episodeCount = 0;
    m_sharedMemory->frameCount = 0;
    m_sharedMemory->running = false;
    
    // Create runner
    m_runner = std::make_unique<HeadlessRunner>();
    m_runner->initialize(config);
}

HeadlessInstance::~HeadlessInstance() {
    stop();
}

bool HeadlessInstance::start(const char* romPath, const char* romName) {
    if (m_running) {
        return true; // Already running
    }
    
    // Set up callbacks
    if (m_actionCallback) {
        m_runner->setActionCallback(m_actionCallback);
    }
    
    if (m_frameCallback) {
        m_runner->setFrameCallback(m_frameCallback);
    }
    
    if (m_rewardCallback) {
        m_runner->setRewardCallback(m_rewardCallback);
    }
    
    if (m_episodeCallback) {
        m_runner->setEpisodeCompleteCallback(m_episodeCallback);
    }
    
#ifdef _WIN32
    // Windows doesn't fully support fork, so we'll just use threading
    if (!m_runner->start(romPath, romName)) {
        return false;
    }
#else
    // Use fork for true process isolation on UNIX systems
    m_processId = fork();
    
    if (m_processId < 0) {
        // Fork failed
        std::cerr << "Failed to fork process for headless instance " << m_instanceId << std::endl;
        return false;
    }
    
    if (m_processId == 0) {
        // Child process
        runInChildProcess();
        exit(0);
    } else {
        // Parent process - just track the child
        m_sharedMemory->running = true;
    }
#endif
    
    m_running = true;
    return true;
}

void HeadlessInstance::stop() {
    if (!m_running) {
        return;
    }
    
#ifdef _WIN32
    m_runner->stop();
#else
    if (m_processId > 0) {
        // Send signal to child process
        m_sharedMemory->running = false;
        
        // Wait for child to exit
        int status;
        waitpid(m_processId, &status, 0);
        m_processId = -1;
    }
#endif
    
    m_running = false;
}

int HeadlessInstance::runEpisodes(int numEpisodes) {
    if (!m_running) {
        return 0;
    }
    
#ifdef _WIN32
    return m_runner->runEpisodes(numEpisodes);
#else
    if (m_processId > 0) {
        // In parent process, communicate through shared memory
        int startingEpisodes = m_sharedMemory->episodeCount;
        
        // Send command to run episodes (this would need a more robust IPC mechanism)
        // For now, we'll just check periodically
        while (m_sharedMemory->episodeCount < startingEpisodes + numEpisodes) {
            if (!m_sharedMemory->running) {
                break; // Process stopped
            }
            
            // Sleep to avoid busy waiting
            usleep(10000); // 10ms
        }
        
        return m_sharedMemory->episodeCount - startingEpisodes;
    }
    
    return 0;
#endif
}

void HeadlessInstance::setActionCallback(ActionCallback callback) {
    m_actionCallback = callback;
    if (m_runner) {
        m_runner->setActionCallback(callback);
    }
}

void HeadlessInstance::setFrameCallback(FrameCallback callback) {
    m_frameCallback = callback;
    if (m_runner) {
        m_runner->setFrameCallback(callback);
    }
}

void HeadlessInstance::setRewardCallback(RewardCallback callback) {
    m_rewardCallback = callback;
    if (m_runner) {
        m_runner->setRewardCallback(callback);
    }
}

void HeadlessInstance::setEpisodeCompleteCallback(EpisodeCompleteCallback callback) {
    m_episodeCallback = callback;
    if (m_runner) {
        m_runner->setEpisodeCompleteCallback(callback);
    }
}

int HeadlessInstance::getEpisodeCount() const {
#ifdef _WIN32
    return m_runner ? m_runner->getEpisodeCount() : 0;
#else
    if (m_processId > 0) {
        return m_sharedMemory->episodeCount;
    }
    return m_runner ? m_runner->getEpisodeCount() : 0;
#endif
}

int HeadlessInstance::getFrameCount() const {
#ifdef _WIN32
    return m_runner ? m_runner->getFrameCount() : 0;
#else
    if (m_processId > 0) {
        return m_sharedMemory->frameCount;
    }
    return m_runner ? m_runner->getFrameCount() : 0;
#endif
}

void HeadlessInstance::runInChildProcess() {
    // This runs in the child process after fork
    
    // Set process name for easier identification
    std::stringstream ss;
    ss << "fbneo-headless-" << m_instanceId;
#ifdef _GNU_SOURCE
    // Linux specific - set process name
    prctl(PR_SET_NAME, ss.str().c_str(), 0, 0, 0);
#endif
    
    // Initialize and run
    if (m_runner) {
        // Set up shared memory updates
        m_runner->setFrameCallback([this](const uint8_t* buffer, int width, int height, int stride) {
            // Update frame count in shared memory
            m_sharedMemory->frameCount = m_runner->getFrameCount();
            
            // Forward to original callback if set
            if (m_frameCallback) {
                m_frameCallback(buffer, width, height, stride);
            }
        });
        
        m_runner->setEpisodeCompleteCallback([this](int episodeId, float totalReward) {
            // Update episode count in shared memory
            m_sharedMemory->episodeCount = m_runner->getEpisodeCount();
            
            // Forward to original callback if set
            if (m_episodeCallback) {
                m_episodeCallback(episodeId, totalReward);
            }
        });
        
        // Run until parent signals stop
        while (m_sharedMemory->running) {
            m_runner->stepFrame();
            
            // Sleep to avoid 100% CPU usage
            usleep(1000); // 1ms
        }
    }
}

} // namespace ai
} // namespace fbneo 