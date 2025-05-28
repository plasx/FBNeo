#include "headless_runner.h"

#include "burner.h"
#include "burnint.h"
#include "state.h"
#include "input/input.h"
#include "intf/video/vid_support.h"

#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>

namespace fbneo {
namespace ai {

class HeadlessRunnerImpl {
public:
    HeadlessRunnerImpl() 
        : m_initialized(false)
        , m_running(false)
        , m_frameCount(0)
        , m_episodeCount(0)
        , m_currentReward(0.0f)
        , m_totalReward(0.0f) {
    }
    
    ~HeadlessRunnerImpl() {
        stop();
    }
    
    bool initialize(const HeadlessConfig& config) {
        m_config = config;
        
        // Initialize DipSwitches
        DIPInfo.nDIP = 0;
        
        // Setup video callbacks but don't render
        VidSelect(0);
        VidInit();
        
        if (m_config.useVideo) {
            m_frameBuffer.resize(m_config.width * m_config.height * 4);
        }
        
        if (m_config.useAudio) {
            // Setup audio but don't output
            if (!AudSelect(0)) {
                AudSetCallback(audioCallbackBridge, this);
                AudInit();
                AudSetVolume(1.0);
            }
        }
        
        m_initialized = true;
        return true;
    }
    
    bool start(const char* romPath, const char* romName) {
        if (!m_initialized) {
            return false;
        }
        
        if (m_running) {
            stop();
        }
        
        m_frameCount = 0;
        m_episodeCount = 0;
        m_currentReward = 0.0f;
        m_totalReward = 0.0f;
        
        // Set ROM path
        strcpy(szAppRomPaths[0], romPath);
        
        // Load game
        if (BurnDrvSelect(BurnDrvGetIndexByName(romName)) < 0) {
            return false;
        }
        
        // Create input info
        if (!m_config.autoLoadConfig) {
            InputInit();
        } else {
            if (!m_config.configPath.empty()) {
                ConfigGameLoad(m_config.configPath.c_str());
            }
        }
        
        // Load the game ROM
        int nRet = BurnDrvInit();
        if (nRet != 0) {
            return false;
        }
        
        // Additional game initialization
        InputMake(true);
        
        m_running = true;
        return true;
    }
    
    void stop() {
        if (!m_running) {
            return;
        }
        
        BurnDrvExit();
        m_running = false;
    }
    
    void reset() {
        if (!m_running) {
            return;
        }
        
        BurnDrvReset();
        m_frameCount = 0;
        m_currentReward = 0.0f;
        m_totalReward = 0.0f;
    }
    
    bool stepFrame() {
        if (!m_running) {
            return false;
        }
        
        // Get input from action callback if set
        if (m_actionCallback) {
            uint32_t inputs[4] = {0};
            m_actionCallback(inputs);
            
            // Apply inputs to game
            for (int i = 0; i < 4; i++) {
                InputSetCooperativeLevel(i, inputs[i], 0);
            }
        }
        
        // Run emulation steps
        for (int i = 0; i < m_config.stepsPerFrame; i++) {
            BurnDrvFrame();
        }
        
        // If we have a reward callback, update rewards
        if (m_rewardCallback) {
            m_currentReward = m_rewardCallback();
            m_totalReward += m_currentReward;
        }
        
        // Call frame callback with frame buffer if set and not skipping this frame
        if (m_frameCallback && m_config.useVideo && (m_frameCount % (m_config.skipFrames + 1) == 0)) {
            // Render to the frame buffer
            VidRenderFrame(m_config.disableThrottling);
            
            // Get the pixels
            VidCopyScreen(&m_frameBuffer[0], m_config.width * 4);
            
            // Call the callback
            m_frameCallback(&m_frameBuffer[0], m_config.width, m_config.height, m_config.width * 4);
        }
        
        m_frameCount++;
        return true;
    }
    
    int runFrames(int numFrames) {
        if (!m_running) {
            return 0;
        }
        
        int framesRun = 0;
        for (int i = 0; i < numFrames; i++) {
            if (!stepFrame()) {
                break;
            }
            framesRun++;
        }
        
        return framesRun;
    }
    
    int runEpisodes(int numEpisodes) {
        if (!m_running) {
            return 0;
        }
        
        int episodesRun = 0;
        for (int i = 0; i < numEpisodes; i++) {
            // Reset the game for a new episode
            BurnDrvReset();
            m_frameCount = 0;
            m_currentReward = 0.0f;
            m_totalReward = 0.0f;
            
            bool episodeComplete = false;
            while (!episodeComplete) {
                if (!stepFrame()) {
                    break;
                }
                
                // Check for episode end condition
                // For now, just assume the episode ends after 3600 frames (1 minute at 60fps)
                if (m_frameCount >= 3600) {
                    episodeComplete = true;
                }
            }
            
            // Call episode complete callback if set
            if (m_episodeCompleteCallback) {
                m_episodeCompleteCallback(m_episodeCount, m_totalReward);
            }
            
            m_episodeCount++;
            episodesRun++;
        }
        
        return episodesRun;
    }
    
    void setActionCallback(ActionCallback callback) {
        m_actionCallback = callback;
    }
    
    void setFrameCallback(FrameCallback callback) {
        m_frameCallback = callback;
    }
    
    void setAudioCallback(AudioCallback callback) {
        m_audioCallback = callback;
    }
    
    void setRewardCallback(RewardCallback callback) {
        m_rewardCallback = callback;
    }
    
    void setEpisodeCompleteCallback(EpisodeCompleteCallback callback) {
        m_episodeCompleteCallback = callback;
    }
    
    int getFrameCount() const {
        return m_frameCount;
    }
    
    int getEpisodeCount() const {
        return m_episodeCount;
    }
    
    float getCurrentReward() const {
        return m_currentReward;
    }
    
    float getTotalReward() const {
        return m_totalReward;
    }
    
    bool getScreenshot(int* width, int* height, uint8_t* pixels) {
        if (!m_running || !m_config.useVideo) {
            return false;
        }
        
        // Render to the frame buffer
        VidRenderFrame(m_config.disableThrottling);
        
        // Get the pixels
        VidCopyScreen(pixels, *width * 4);
        
        *width = m_config.width;
        *height = m_config.height;
        
        return true;
    }

private:
    // Audio callback bridge (static method to pass as C callback)
    static void audioCallbackBridge(int16_t* buffer, int samples, void* param) {
        HeadlessRunnerImpl* self = static_cast<HeadlessRunnerImpl*>(param);
        self->handleAudioCallback(buffer, samples);
    }
    
    // Handle audio callback
    void handleAudioCallback(int16_t* buffer, int samples) {
        if (m_audioCallback) {
            // Convert 16-bit integer samples to float
            std::vector<float> floatSamples(samples * m_config.audioChannels);
            for (int i = 0; i < samples * m_config.audioChannels; i++) {
                floatSamples[i] = buffer[i] / 32768.0f;
            }
            
            m_audioCallback(floatSamples.data(), samples);
        }
    }

    HeadlessConfig m_config;
    bool m_initialized;
    bool m_running;
    
    int m_frameCount;
    int m_episodeCount;
    float m_currentReward;
    float m_totalReward;
    
    ActionCallback m_actionCallback;
    FrameCallback m_frameCallback;
    AudioCallback m_audioCallback;
    RewardCallback m_rewardCallback;
    EpisodeCompleteCallback m_episodeCompleteCallback;
    
    std::vector<uint8_t> m_frameBuffer;
};

// HeadlessRunner implementation
HeadlessRunner::HeadlessRunner() : m_impl(std::make_unique<HeadlessRunnerImpl>()) {
}

HeadlessRunner::~HeadlessRunner() {
}

bool HeadlessRunner::initialize(const HeadlessConfig& config) {
    return m_impl->initialize(config);
}

bool HeadlessRunner::start(const char* romPath, const char* romName) {
    return m_impl->start(romPath, romName);
}

void HeadlessRunner::stop() {
    m_impl->stop();
}

void HeadlessRunner::reset() {
    m_impl->reset();
}

bool HeadlessRunner::stepFrame() {
    return m_impl->stepFrame();
}

int HeadlessRunner::runFrames(int numFrames) {
    return m_impl->runFrames(numFrames);
}

int HeadlessRunner::runEpisodes(int numEpisodes) {
    return m_impl->runEpisodes(numEpisodes);
}

void HeadlessRunner::setActionCallback(ActionCallback callback) {
    m_impl->setActionCallback(callback);
}

void HeadlessRunner::setFrameCallback(FrameCallback callback) {
    m_impl->setFrameCallback(callback);
}

void HeadlessRunner::setAudioCallback(AudioCallback callback) {
    m_impl->setAudioCallback(callback);
}

void HeadlessRunner::setRewardCallback(RewardCallback callback) {
    m_impl->setRewardCallback(callback);
}

void HeadlessRunner::setEpisodeCompleteCallback(EpisodeCompleteCallback callback) {
    m_impl->setEpisodeCompleteCallback(callback);
}

int HeadlessRunner::getFrameCount() const {
    return m_impl->getFrameCount();
}

int HeadlessRunner::getEpisodeCount() const {
    return m_impl->getEpisodeCount();
}

float HeadlessRunner::getCurrentReward() const {
    return m_impl->getCurrentReward();
}

float HeadlessRunner::getTotalReward() const {
    return m_impl->getTotalReward();
}

bool HeadlessRunner::getScreenshot(int* width, int* height, uint8_t* pixels) {
    return m_impl->getScreenshot(width, height, pixels);
}

} // namespace ai
} // namespace fbneo 