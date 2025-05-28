#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <bitset>

namespace AI {

/**
 * @brief Represents an action output by the AI controller
 * 
 * This class encapsulates the button state for a single frame
 * that will be sent to the emulator as input.
 */
class AIOutputAction {
public:
    // Button mappings that correspond to FBNeo's input system
    enum ButtonMapping {
        UP = 0,
        DOWN = 1,
        LEFT = 2,
        RIGHT = 3,
        BUTTON1 = 4,
        BUTTON2 = 5,
        BUTTON3 = 6,
        BUTTON4 = 7,
        BUTTON5 = 8,
        BUTTON6 = 9,
        START = 10,
        COIN = 11,
        MAX_BUTTONS = 12
    };

    // Constructor/destructor
    AIOutputAction();
    AIOutputAction(unsigned int frameNumber);
    AIOutputAction(const AIOutputAction& other);
    ~AIOutputAction();
    
    // Button state methods
    void setButton(ButtonMapping button, bool pressed);
    bool isButtonPressed(ButtonMapping button) const;
    void clearAllButtons();
    
    // Convenience methods for commonly used button combinations
    bool isIdle() const;
    bool isJumping() const;
    bool isPunching() const;
    bool isKicking() const;
    bool isBlocking() const;
    
    // Frame information
    void setFrameNumber(unsigned int frameNumber);
    unsigned int getFrameNumber() const;
    
    // Conversion methods
    std::string toString() const;
    std::string toJson() const;
    static AIOutputAction fromJson(const std::string& json);
    
    // Copy and assignment
    AIOutputAction& operator=(const AIOutputAction& other);
    bool operator==(const AIOutputAction& other) const;
    bool operator!=(const AIOutputAction& other) const;
    
    // Utility methods
    std::bitset<MAX_BUTTONS> getButtonBitset() const;
    void setButtonBitset(const std::bitset<MAX_BUTTONS>& buttons);
    
private:
    // Button state storage
    std::bitset<MAX_BUTTONS> m_buttons;
    
    // Frame information
    unsigned int m_frameNumber;
};

} // namespace AI 