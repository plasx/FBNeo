#pragma once

#include "ai_controller.h"
#include <vector>
#include <string>
#include <functional>

namespace fbneo {
namespace ai {

// Callback types for headless mode
using FrameCallback = std::function<void(const GameObservation&)>;
using RewardCallback = std::function<float(const GameObservation&)>;
using ActionCallback = std::function<InputAction(const GameObservation&)>;
using EpisodeCompleteCallback = std::function<void(int, float)>; // episodeNum, totalReward

// Headless mode configuration
struct HeadlessConfig {
    int numInstances;           // Number of parallel instances
    int stepsPerAction;         // How many frames to run per action
    int maxEpisodeLength;       // Max frames per episode
    bool renderFrames;          // Whether to render frames (even in headless)
    bool saveReplayBuffer;      // Whether to save replay buffer
    int replayBufferSize;       // Size of replay buffer
    std::string outputDir;      // Directory for outputs
    
    HeadlessConfig() 
        : numInstances(1), stepsPerAction(1), maxEpisodeLength(10000),
          renderFrames(false), saveReplayBuffer(false), replayBufferSize(10000),
          outputDir("./output") {}
};

// Headless runner class
class HeadlessRunner {
private:
    HeadlessConfig m_config;
    std::vector<GameObservation> m_replayBuffer;
    std::vector<InputAction> m_actionBuffer;
    FrameCallback m_frameCallback;
    RewardCallback m_rewardCallback;
    ActionCallback m_actionCallback;
    EpisodeCompleteCallback m_episodeCallback;
    int m_episodeCount;
    int m_frameCount;
    float m_totalReward;
    bool m_isRunning;
    
public:
    HeadlessRunner();
    ~HeadlessRunner();
    
    // Initialize headless runner
    bool initialize(const HeadlessConfig& config);
    
    // Set callbacks
    void setFrameCallback(FrameCallback callback);
    void setRewardCallback(RewardCallback callback);
    void setActionCallback(ActionCallback callback);
    void setEpisodeCompleteCallback(EpisodeCompleteCallback callback);
    
    // Start headless execution
    bool start(const char* romPath, const char* romName);
    
    // Stop headless execution
    void stop();
    
    // Step a single frame
    bool step();
    
    // Run until episode complete
    bool runEpisode();
    
    // Run multiple episodes
    bool runEpisodes(int numEpisodes);
    
    // Reset the current environment
    bool reset();
    
    // Save replay buffer
    bool saveReplay(const char* filename);
    
    // Load replay buffer
    bool loadReplay(const char* filename);
    
    // Get current episode count
    int getEpisodeCount() const;
    
    // Get current frame count
    int getFrameCount() const;
    
    // Get total reward for current episode
    float getTotalReward() const;
    
    // Get replay buffer
    const std::vector<GameObservation>& getReplayBuffer() const;
    
    // Get action buffer
    const std::vector<InputAction>& getActionBuffer() const;
};

// C API for external tools (Python bindings, etc.)
extern "C" {
    // Create headless runner
    void* fbneo_headless_create();
    
    // Destroy headless runner
    void fbneo_headless_destroy(void* runner);
    
    // Initialize headless runner
    int fbneo_headless_init(void* runner, const char* configJson);
    
    // Start headless execution
    int fbneo_headless_start(void* runner, const char* romPath, const char* romName);
    
    // Stop headless execution
    void fbneo_headless_stop(void* runner);
    
    // Step a single frame
    int fbneo_headless_step(void* runner);
    
    // Run episode
    int fbneo_headless_run_episode(void* runner);
    
    // Reset environment
    int fbneo_headless_reset(void* runner);
    
    // Get observation
    int fbneo_headless_get_observation(void* runner, void* buffer, int* width, int* height);
    
    // Set action
    void fbneo_headless_set_action(void* runner, int up, int down, int left, int right,
                                  int b1, int b2, int b3, int b4, int b5, int b6,
                                  int start, int coin);
    
    // Get reward
    float fbneo_headless_get_reward(void* runner);
    
    // Save state
    int fbneo_headless_save_state(void* runner, const char* filename);
    
    // Load state
    int fbneo_headless_load_state(void* runner, const char* filename);
}

} // namespace ai
} // namespace fbneo 