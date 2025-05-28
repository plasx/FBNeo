#pragma once

#include <cstdint>
#include <vector>

namespace fbneo {
namespace ai {

/**
 * @brief Represents a frame of input from the game for AI processing
 * 
 * This structure holds the visual and game state information for a single frame
 * that will be processed by the AI system.
 */
struct AIInputFrame {
    /**
     * @brief Frame dimensions
     */
    int width;
    int height;
    int channels;
    
    /**
     * @brief Frame data
     */
    std::vector<uint8_t> data;
    
    /**
     * @brief Game state information
     */
    std::vector<float> stateValues;
    
    /**
     * @brief Frame number in current session
     */
    int frameNumber;
    
    /**
     * @brief Previous action feedback
     */
    float previousReward;
    bool isDone;
    
    /**
     * @brief Default constructor
     */
    AIInputFrame() : width(0), height(0), channels(0), frameNumber(0), previousReward(0.0f), isDone(false) {}
    
    /**
     * @brief Constructor with frame buffer
     * 
     * @param w Width of frame buffer
     * @param h Height of frame buffer
     * @param c Channels of frame buffer
     */
    AIInputFrame(int w, int h, int c) : width(w), height(h), channels(c), frameNumber(0), previousReward(0.0f), isDone(false) {
        data.resize(w * h * c, 0);
    }
    
    /**
     * @brief Clear the frame data
     */
    void clear() {
        std::fill(data.begin(), data.end(), 0);
        std::fill(stateValues.begin(), stateValues.end(), 0.0f);
        frameNumber = 0;
        previousReward = 0.0f;
        isDone = false;
    }
    
    /**
     * @brief Resize the frame data
     * 
     * @param w Width of frame buffer
     * @param h Height of frame buffer
     * @param c Channels of frame buffer
     */
    void resize(int w, int h, int c) {
        width = w;
        height = h;
        channels = c;
        data.resize(w * h * c, 0);
    }
    
    /**
     * @brief Get the size of the frame data in bytes
     * 
     * @return Size of frame data in bytes
     */
    size_t getDataSize() const {
        return data.size();
    }
    
    /**
     * @brief Set state value
     * 
     * @param index Index of state variable
     * @param value Value to set
     */
    void setState(int index, float value) {
        if (index >= static_cast<int>(stateValues.size())) {
            stateValues.resize(index + 1, 0.0f);
        }
        stateValues[index] = value;
    }
    
    /**
     * @brief Get state value
     * 
     * @param index Index of state variable
     * @return Value of state variable
     */
    float getState(int index) const {
        if (index >= static_cast<int>(stateValues.size())) {
            return 0.0f;
        }
        return stateValues[index];
    }
};

} // namespace ai
} // namespace fbneo 