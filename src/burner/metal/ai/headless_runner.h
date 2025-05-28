#pragma once

#include <cstdint>
#include <string>
#include <functional>
#include <memory>

namespace fbneo {
namespace ai {

// Forward declarations
class HeadlessRunnerImpl;

/**
 * Configuration for headless emulation
 */
struct HeadlessConfig {
    int width = 320;           // Output width
    int height = 240;          // Output height
    int skipFrames = 0;        // Number of frames to skip between frames sent to callback
    int frameRate = 60;        // Target frame rate (frames per second)
    int audioRate = 44100;     // Audio sample rate
    int audioChannels = 2;     // Number of audio channels
    bool useAudio = false;     // Whether to enable audio
    bool useVideo = true;      // Whether to enable video
    int stepsPerFrame = 1;     // Number of emulation steps per frame
    bool disableThrottling = true; // Whether to disable frame rate throttling
    int threadAffinity = -1;   // CPU thread affinity (-1 = no affinity)

    // Additional options for ROM loading
    bool autoLoadConfig = true;    // Whether to automatically load configuration
    std::string configPath = "";   // Path to configuration file
    bool ignoreSaveRam = true;     // Whether to ignore save RAM files
    bool ignoreSaveState = true;   // Whether to ignore save state files
};

// Callback types
using ActionCallback = std::function<void(uint32_t* inputs)>;
using FrameCallback = std::function<void(const uint8_t* buffer, int width, int height, int stride)>;
using AudioCallback = std::function<void(const float* samples, int count)>;
using RewardCallback = std::function<float()>;
using EpisodeCompleteCallback = std::function<void(int episodeId, float totalReward)>;

/**
 * HeadlessRunner provides headless emulation functionality for training AI
 */
class HeadlessRunner {
public:
    /**
     * Constructor
     */
    HeadlessRunner();

    /**
     * Destructor
     */
    ~HeadlessRunner();

    /**
     * Initialize the headless runner
     * @param config Configuration for the headless runner
     * @return True if initialization successful
     */
    bool initialize(const HeadlessConfig& config);

    /**
     * Start emulation
     * @param romPath Path to the ROM directory
     * @param romName Name of the ROM to load
     * @return True if startup successful
     */
    bool start(const char* romPath, const char* romName);

    /**
     * Stop emulation
     */
    void stop();

    /**
     * Reset emulation
     */
    void reset();

    /**
     * Step one frame forward
     * @return True if step successful
     */
    bool stepFrame();

    /**
     * Run for a specified number of frames
     * @param numFrames Number of frames to run
     * @return Number of frames actually run
     */
    int runFrames(int numFrames);

    /**
     * Run for a specified number of episodes
     * @param numEpisodes Number of episodes to run
     * @return Number of episodes actually run
     */
    int runEpisodes(int numEpisodes);

    /**
     * Set the action callback
     * @param callback Function to call to get input actions
     */
    void setActionCallback(ActionCallback callback);

    /**
     * Set the frame callback
     * @param callback Function to call with rendered frame
     */
    void setFrameCallback(FrameCallback callback);

    /**
     * Set the audio callback
     * @param callback Function to call with audio samples
     */
    void setAudioCallback(AudioCallback callback);

    /**
     * Set the reward callback
     * @param callback Function to call to calculate reward
     */
    void setRewardCallback(RewardCallback callback);

    /**
     * Set the episode complete callback
     * @param callback Function to call when an episode completes
     */
    void setEpisodeCompleteCallback(EpisodeCompleteCallback callback);

    /**
     * Get the current frame count
     * @return Frame count
     */
    int getFrameCount() const;

    /**
     * Get the current episode count
     * @return Episode count
     */
    int getEpisodeCount() const;

    /**
     * Get the current reward for the current episode
     * @return Current reward
     */
    float getCurrentReward() const;

    /**
     * Get the total reward for the current episode
     * @return Total reward
     */
    float getTotalReward() const;

    /**
     * Get a screenshot of the current frame
     * @param width Output width
     * @param height Output height
     * @param pixels Output buffer for pixels (RGBA format)
     * @return True if screenshot successful
     */
    bool getScreenshot(int* width, int* height, uint8_t* pixels);

private:
    // Implementation pointer
    std::unique_ptr<HeadlessRunnerImpl> m_impl;
};

} // namespace ai
} // namespace fbneo 