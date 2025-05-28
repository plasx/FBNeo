#pragma once

#include <cstdint>
#include <array>
#include <vector>

namespace fbneo {
namespace ai {

// Maximum number of buttons supported
constexpr int MAX_BUTTONS = 8;

/**
 * @brief Output action structure for AI model
 */
struct AIOutputAction {
    // Directional inputs
    float up;
    float down;
    float left;
    float right;
    
    // Button inputs (variable size)
    std::vector<float> buttons;
    
    // Action confidence/probability
    float confidence;
    
    // Value estimation (for actor-critic methods)
    float valueEstimate;
    
    // Constructor
    AIOutputAction() : up(0.0f), down(0.0f), left(0.0f), right(0.0f), confidence(0.0f), valueEstimate(0.0f) {
        buttons.resize(MAX_BUTTONS, 0.0f);
    }
    
    // Reset all actions to zero
    void reset() {
        up = down = left = right = 0.0f;
        confidence = valueEstimate = 0.0f;
        for (size_t i = 0; i < buttons.size(); ++i) {
            buttons[i] = 0.0f;
        }
    }
    
    // Set button value
    void setButton(int index, float value) {
        if (index >= 0 && index < static_cast<int>(buttons.size())) {
            buttons[index] = value;
        }
    }
    
    // Get button value
    float getButton(int index) const {
        if (index >= 0 && index < static_cast<int>(buttons.size())) {
            return buttons[index];
        }
        return 0.0f;
    }
    
    // Resize button array
    void resizeButtons(size_t size) {
        buttons.resize(size, 0.0f);
    }
    
    // Convert to binary inputs (0 or 1)
    void toBinary(float threshold = 0.5f) {
        up = (up >= threshold) ? 1.0f : 0.0f;
        down = (down >= threshold) ? 1.0f : 0.0f;
        left = (left >= threshold) ? 1.0f : 0.0f;
        right = (right >= threshold) ? 1.0f : 0.0f;
        
        for (size_t i = 0; i < buttons.size(); ++i) {
            buttons[i] = (buttons[i] >= threshold) ? 1.0f : 0.0f;
        }
    }
};

} // namespace ai
} // namespace fbneo 