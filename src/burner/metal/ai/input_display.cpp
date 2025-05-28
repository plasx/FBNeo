#include "input_display.h"
#include <iostream>
#include <sstream>
#include <algorithm>

// Implementation of InputState comparison operators
bool InputDisplay::InputState::operator==(const InputState& other) const {
    return up == other.up &&
           down == other.down &&
           left == other.left &&
           right == other.right &&
           attack1 == other.attack1 &&
           attack2 == other.attack2 &&
           attack3 == other.attack3 &&
           attack4 == other.attack4 &&
           attack5 == other.attack5 &&
           attack6 == other.attack6 &&
           start == other.start &&
           select == other.select;
}

bool InputDisplay::InputState::operator!=(const InputState& other) const {
    return !(*this == other);
}

bool InputDisplay::InputState::anyButton() const {
    return attack1 || attack2 || attack3 || attack4 || attack5 || attack6 || start || select;
}

bool InputDisplay::InputState::anyDirection() const {
    return up || down || left || right;
}

std::string InputDisplay::InputState::toNotation() const {
    std::string notation;
    
    // Add directional input
    if (up && !left && !right) notation += "↑";
    if (up && right) notation += "↗";
    if (!up && !down && right) notation += "→";
    if (down && right) notation += "↘";
    if (down && !left && !right) notation += "↓";
    if (down && left) notation += "↙";
    if (!up && !down && left) notation += "←";
    if (up && left) notation += "↖";
    
    // Add button inputs
    std::string buttons;
    if (attack1) buttons += "LP+";
    if (attack2) buttons += "MP+";
    if (attack3) buttons += "HP+";
    if (attack4) buttons += "LK+";
    if (attack5) buttons += "MK+";
    if (attack6) buttons += "HK+";
    
    // Remove trailing + if present
    if (!buttons.empty()) {
        buttons.pop_back();
        if (!notation.empty()) {
            notation += "+";
        }
        notation += buttons;
    }
    
    return notation;
}

InputDisplay::InputDisplay()
    : m_renderer(nullptr)
    , m_memoryMapping(nullptr)
    , m_p1X(50)
    , m_p1Y(50)
    , m_p2X(400)
    , m_p2Y(50)
    , m_historySize(60)  // 1 second at 60fps
    , m_historyEnabled(true)
    , m_iconsEnabled(true)
    , m_frameAdvantageEnabled(true)
    , m_opacity(0.8f)
    , m_currentFrame(0)
    , m_initialized(false)
{
    // Initialize empty input states
    m_p1CurrentState = {};
    m_p2CurrentState = {};
    
    // Initialize common motion patterns for fighting games
    m_motionPatterns.push_back({"Fireball", {"↓", "↘", "→"}, 20});
    m_motionPatterns.push_back({"Dragon Punch", {"→", "↓", "↘"}, 15});
    m_motionPatterns.push_back({"Charge Back-Forward", {"←", "→"}, 30});
    m_motionPatterns.push_back({"Charge Down-Up", {"↓", "↑"}, 30});
    m_motionPatterns.push_back({"Half Circle Forward", {"↓", "↘", "→", "↗", "↑"}, 25});
    m_motionPatterns.push_back({"Half Circle Back", {"↓", "↙", "←", "↖", "↑"}, 25});
    m_motionPatterns.push_back({"360", {"←", "↓", "→", "↑"}, 30});
}

InputDisplay::~InputDisplay() {
    // Nothing to clean up
}

bool InputDisplay::initialize(OverlayRenderer* renderer, AIMemoryMapping* memoryMapping) {
    if (!renderer || !memoryMapping) {
        std::cerr << "InputDisplay: Invalid renderer or memory mapping" << std::endl;
        return false;
    }
    
    m_renderer = renderer;
    m_memoryMapping = memoryMapping;
    
    // Check if memory mapping is loaded
    if (!m_memoryMapping->isLoaded()) {
        std::cerr << "InputDisplay: Memory mapping not loaded" << std::endl;
        return false;
    }
    
    // Clear input history
    m_p1History.clear();
    m_p2History.clear();
    
    // Reset frame counter
    m_currentFrame = 0;
    
    m_initialized = true;
    return true;
}

void InputDisplay::update() {
    if (!m_initialized) {
        return;
    }
    
    // Increment frame counter
    m_currentFrame++;
    
    // Read current input states
    InputState p1State = readInputState(0);
    InputState p2State = readInputState(1);
    
    // Set frame number
    p1State.frame = m_currentFrame;
    p2State.frame = m_currentFrame;
    
    // Update current states
    m_p1CurrentState = p1State;
    m_p2CurrentState = p2State;
    
    // Add to history if anything changed or if there's active input
    if (m_p1History.empty() || m_p1CurrentState != m_p1History.back() || 
        m_p1CurrentState.anyButton() || m_p1CurrentState.anyDirection()) {
        m_p1History.push_back(m_p1CurrentState);
    }
    
    if (m_p2History.empty() || m_p2CurrentState != m_p2History.back() ||
        m_p2CurrentState.anyButton() || m_p2CurrentState.anyDirection()) {
        m_p2History.push_back(m_p2CurrentState);
    }
    
    // Trim history to max size
    while (m_p1History.size() > m_historySize) {
        m_p1History.pop_front();
    }
    
    while (m_p2History.size() > m_historySize) {
        m_p2History.pop_front();
    }
}

void InputDisplay::render() {
    if (!m_initialized || !m_renderer) {
        return;
    }
    
    // Render current input state for both players
    renderInputState(m_p1CurrentState, m_p1X, m_p1Y, 0);
    renderInputState(m_p2CurrentState, m_p2X, m_p2Y, 1);
    
    // Render input history if enabled
    if (m_historyEnabled) {
        renderInputHistory(m_p1History, m_p1X, m_p1Y + 70, 0);
        renderInputHistory(m_p2History, m_p2X, m_p2Y + 70, 1);
    }
    
    // Render frame advantage if enabled
    if (m_frameAdvantageEnabled) {
        renderFrameAdvantage(m_p1History, m_p2History);
    }
}

InputDisplay::InputState InputDisplay::readInputState(int playerIndex) {
    InputState state = {};
    
    // Mapping prefixes for each player
    std::string prefix = (playerIndex == 0) ? "p1_" : "p2_";
    
    // Try to read digital inputs from memory mapping
    // These depend on the game's memory layout, so we try different common naming conventions
    
    // First try: direct input mappings
    if (m_memoryMapping->hasMapping(prefix + "input_up")) {
        state.up = m_memoryMapping->readInt(prefix + "input_up") != 0;
        state.down = m_memoryMapping->readInt(prefix + "input_down") != 0;
        state.left = m_memoryMapping->readInt(prefix + "input_left") != 0;
        state.right = m_memoryMapping->readInt(prefix + "input_right") != 0;
        state.attack1 = m_memoryMapping->readInt(prefix + "input_button1") != 0;
        state.attack2 = m_memoryMapping->readInt(prefix + "input_button2") != 0;
        state.attack3 = m_memoryMapping->readInt(prefix + "input_button3") != 0;
        state.attack4 = m_memoryMapping->readInt(prefix + "input_button4") != 0;
        state.attack5 = m_memoryMapping->readInt(prefix + "input_button5") != 0;
        state.attack6 = m_memoryMapping->readInt(prefix + "input_button6") != 0;
        state.start = m_memoryMapping->readInt(prefix + "input_start") != 0;
        state.select = m_memoryMapping->readInt(prefix + "input_select") != 0;
    }
    // Second try: input as a single bitfield
    else if (m_memoryMapping->hasMapping(prefix + "input")) {
        int input = m_memoryMapping->readInt(prefix + "input");
        // Typical arcade-style bit mapping:
        // Bit 0: Up
        // Bit 1: Down
        // Bit 2: Left
        // Bit 3: Right
        // Bit 4-9: Buttons 1-6
        // Bit 10: Start
        // Bit 11: Select/Coin
        state.up = (input & 0x01) != 0;
        state.down = (input & 0x02) != 0;
        state.left = (input & 0x04) != 0;
        state.right = (input & 0x08) != 0;
        state.attack1 = (input & 0x10) != 0;
        state.attack2 = (input & 0x20) != 0;
        state.attack3 = (input & 0x40) != 0;
        state.attack4 = (input & 0x80) != 0;
        state.attack5 = (input & 0x100) != 0;
        state.attack6 = (input & 0x200) != 0;
        state.start = (input & 0x400) != 0;
        state.select = (input & 0x800) != 0;
    }
    // Third try: CPS-style input mapping
    else if (m_memoryMapping->hasMapping(prefix + "joystick")) {
        int joystick = m_memoryMapping->readInt(prefix + "joystick");
        int buttons = 0;
        if (m_memoryMapping->hasMapping(prefix + "buttons")) {
            buttons = m_memoryMapping->readInt(prefix + "buttons");
        }
        
        // CPS and Neo Geo games often use a different bit mapping
        state.up = (joystick & 0x01) != 0;
        state.down = (joystick & 0x02) != 0;
        state.left = (joystick & 0x04) != 0;
        state.right = (joystick & 0x08) != 0;
        state.attack1 = (buttons & 0x01) != 0;
        state.attack2 = (buttons & 0x02) != 0;
        state.attack3 = (buttons & 0x04) != 0;
        state.attack4 = (buttons & 0x08) != 0;
        state.attack5 = (buttons & 0x10) != 0;
        state.attack6 = (buttons & 0x20) != 0;
        state.start = (buttons & 0x40) != 0;
        state.select = (buttons & 0x80) != 0;
    }
    
    return state;
}

void InputDisplay::renderInputState(const InputState& state, float x, float y, int playerIndex) {
    if (!m_renderer) {
        return;
    }
    
    // Background for input display
    m_renderer->drawRect(x, y, 150, 60, 0.1f, 0.1f, 0.1f, m_opacity * 0.7f);
    
    // Draw player label
    std::string playerLabel = (playerIndex == 0) ? "P1" : "P2";
    m_renderer->drawTextWithShadow(
        x + 5, y + 5, 
        playerLabel.c_str(),
        (playerIndex == 0) ? 0.2f : 0.8f, 
        0.6f, 
        (playerIndex == 0) ? 0.9f : 0.2f, 
        m_opacity,
        16.0f
    );
    
    // Draw directional pad
    float centerX = x + 35;
    float centerY = y + 30;
    float size = 10.0f;
    
    // Draw d-pad background
    m_renderer->drawRect(centerX - size, centerY - size, size * 2, size * 2, 0.3f, 0.3f, 0.3f, m_opacity * 0.8f);
    
    // Draw directions based on input state
    if (state.up) {
        m_renderer->drawRect(centerX - size/2, centerY - size, size, size, 0.9f, 0.9f, 0.9f, m_opacity);
    }
    if (state.down) {
        m_renderer->drawRect(centerX - size/2, centerY, size, size, 0.9f, 0.9f, 0.9f, m_opacity);
    }
    if (state.left) {
        m_renderer->drawRect(centerX - size, centerY - size/2, size, size, 0.9f, 0.9f, 0.9f, m_opacity);
    }
    if (state.right) {
        m_renderer->drawRect(centerX, centerY - size/2, size, size, 0.9f, 0.9f, 0.9f, m_opacity);
    }
    
    // Draw buttons if icons are enabled
    if (m_iconsEnabled) {
        // Draw button layout (6-button style, like Street Fighter)
        float buttonStartX = x + 70;
        float buttonStartY = y + 15;
        float buttonSize = 12.0f;
        float buttonGap = 5.0f;
        
        // Light Punch / Light Kick (top row)
        m_renderer->drawRect(
            buttonStartX, buttonStartY, 
            buttonSize, buttonSize, 
            state.attack1 ? 0.9f : 0.3f, 
            state.attack1 ? 0.2f : 0.3f, 
            state.attack1 ? 0.2f : 0.3f, 
            m_opacity
        );
        m_renderer->drawRect(
            buttonStartX + buttonSize + buttonGap, buttonStartY, 
            buttonSize, buttonSize, 
            state.attack4 ? 0.2f : 0.3f, 
            state.attack4 ? 0.9f : 0.3f, 
            state.attack4 ? 0.2f : 0.3f, 
            m_opacity
        );
        
        // Medium Punch / Medium Kick (middle row)
        m_renderer->drawRect(
            buttonStartX, buttonStartY + buttonSize + buttonGap, 
            buttonSize, buttonSize, 
            state.attack2 ? 0.9f : 0.3f, 
            state.attack2 ? 0.5f : 0.3f, 
            state.attack2 ? 0.2f : 0.3f, 
            m_opacity
        );
        m_renderer->drawRect(
            buttonStartX + buttonSize + buttonGap, buttonStartY + buttonSize + buttonGap, 
            buttonSize, buttonSize, 
            state.attack5 ? 0.2f : 0.3f, 
            state.attack5 ? 0.9f : 0.3f, 
            state.attack5 ? 0.5f : 0.3f, 
            m_opacity
        );
        
        // Heavy Punch / Heavy Kick (bottom row)
        m_renderer->drawRect(
            buttonStartX, buttonStartY + (buttonSize + buttonGap) * 2, 
            buttonSize, buttonSize, 
            state.attack3 ? 0.9f : 0.3f, 
            state.attack3 ? 0.7f : 0.3f, 
            state.attack3 ? 0.2f : 0.3f, 
            m_opacity
        );
        m_renderer->drawRect(
            buttonStartX + buttonSize + buttonGap, buttonStartY + (buttonSize + buttonGap) * 2, 
            buttonSize, buttonSize, 
            state.attack6 ? 0.2f : 0.3f, 
            state.attack6 ? 0.7f : 0.3f, 
            state.attack6 ? 0.9f : 0.3f, 
            m_opacity
        );
        
        // Button labels
        m_renderer->drawText(buttonStartX + buttonSize/2 - 3, buttonStartY + buttonSize/2 - 5, 
                            "LP", 1.0f, 1.0f, 1.0f, m_opacity * 0.9f, 8.0f);
        m_renderer->drawText(buttonStartX + buttonSize + buttonGap + buttonSize/2 - 3, buttonStartY + buttonSize/2 - 5, 
                            "LK", 1.0f, 1.0f, 1.0f, m_opacity * 0.9f, 8.0f);
        m_renderer->drawText(buttonStartX + buttonSize/2 - 3, buttonStartY + buttonSize + buttonGap + buttonSize/2 - 5, 
                            "MP", 1.0f, 1.0f, 1.0f, m_opacity * 0.9f, 8.0f);
        m_renderer->drawText(buttonStartX + buttonSize + buttonGap + buttonSize/2 - 3, buttonStartY + buttonSize + buttonGap + buttonSize/2 - 5, 
                            "MK", 1.0f, 1.0f, 1.0f, m_opacity * 0.9f, 8.0f);
        m_renderer->drawText(buttonStartX + buttonSize/2 - 3, buttonStartY + (buttonSize + buttonGap) * 2 + buttonSize/2 - 5, 
                            "HP", 1.0f, 1.0f, 1.0f, m_opacity * 0.9f, 8.0f);
        m_renderer->drawText(buttonStartX + buttonSize + buttonGap + buttonSize/2 - 3, buttonStartY + (buttonSize + buttonGap) * 2 + buttonSize/2 - 5, 
                            "HK", 1.0f, 1.0f, 1.0f, m_opacity * 0.9f, 8.0f);
    }
    
    // Draw current input in notation form
    std::string notation = state.toNotation();
    if (!notation.empty()) {
        m_renderer->drawTextWithShadow(x + 5, y + 45, notation.c_str(), 1.0f, 1.0f, 1.0f, m_opacity, 12.0f);
    }
}

void InputDisplay::renderInputHistory(const std::deque<InputState>& history, float x, float y, int playerIndex) {
    if (history.empty() || !m_renderer) {
        return;
    }
    
    // Background for history display
    float width = 200.0f;
    float height = 100.0f;
    m_renderer->drawRect(x, y, width, height, 0.1f, 0.1f, 0.1f, m_opacity * 0.7f);
    
    // Draw header
    std::string header = (playerIndex == 0) ? "P1 Input History" : "P2 Input History";
    m_renderer->drawTextWithShadow(
        x + 5, y + 5, 
        header.c_str(),
        (playerIndex == 0) ? 0.2f : 0.8f, 
        0.6f, 
        (playerIndex == 0) ? 0.9f : 0.2f, 
        m_opacity,
        12.0f
    );
    
    // Draw the most recent inputs from bottom to top
    float lineHeight = 14.0f;
    float startY = y + height - lineHeight - 5;
    int maxLines = static_cast<int>((height - 25) / lineHeight);
    
    // Get the most recent inputs
    std::vector<InputState> recentInputs;
    for (auto it = history.rbegin(); it != history.rend() && recentInputs.size() < maxLines; ++it) {
        if (it->anyDirection() || it->anyButton()) {
            // Only add inputs that have some activity
            if (recentInputs.empty() || *it != recentInputs.back()) {
                recentInputs.push_back(*it);
            }
        }
    }
    
    // Draw the input history
    for (size_t i = 0; i < recentInputs.size(); ++i) {
        const auto& state = recentInputs[i];
        std::string notation = state.toNotation();
        if (!notation.empty()) {
            // Frame number (relative to current frame)
            int frameOffset = m_currentFrame - state.frame;
            std::stringstream frameText;
            frameText << "F-" << frameOffset << ": ";
            
            m_renderer->drawText(
                x + 5, startY - i * lineHeight, 
                frameText.str().c_str(),
                0.8f, 0.8f, 0.8f, 
                m_opacity * 0.8f,
                10.0f
            );
            
            // Input notation
            m_renderer->drawText(
                x + 50, startY - i * lineHeight, 
                notation.c_str(),
                1.0f, 1.0f, 1.0f, 
                m_opacity,
                10.0f
            );
        }
    }
    
    // Detect and display any recognized motion patterns
    auto detectedMotions = detectMotions(history);
    if (!detectedMotions.empty()) {
        float motionY = y + 25;
        m_renderer->drawText(
            x + 5, motionY, 
            "Detected:", 
            0.9f, 0.9f, 0.3f, 
            m_opacity,
            10.0f
        );
        
        for (size_t i = 0; i < detectedMotions.size() && i < 3; ++i) {
            m_renderer->drawText(
                x + 5, motionY + (i + 1) * 12, 
                detectedMotions[i].c_str(), 
                1.0f, 1.0f, 0.5f, 
                m_opacity,
                10.0f
            );
        }
    }
}

void InputDisplay::renderFrameAdvantage(const std::deque<InputState>& p1History, const std::deque<InputState>& p2History) {
    if (p1History.empty() || p2History.empty() || !m_renderer) {
        return;
    }
    
    // This is a simplified frame advantage calculation
    // In a real implementation, we would look at game state to detect hits, blocks, etc.
    // For now, we'll just look for button press patterns
    
    // Find the most recent attack button press for each player
    int p1LastAttackFrame = -1;
    int p2LastAttackFrame = -1;
    
    for (auto it = p1History.rbegin(); it != p1History.rend() && p1LastAttackFrame == -1; ++it) {
        if (it->attack1 || it->attack2 || it->attack3 || it->attack4 || it->attack5 || it->attack6) {
            p1LastAttackFrame = it->frame;
        }
    }
    
    for (auto it = p2History.rbegin(); it != p2History.rend() && p2LastAttackFrame == -1; ++it) {
        if (it->attack1 || it->attack2 || it->attack3 || it->attack4 || it->attack5 || it->attack6) {
            p2LastAttackFrame = it->frame;
        }
    }
    
    // If both players pressed an attack button within the last 60 frames, calculate frame advantage
    if (p1LastAttackFrame != -1 && p2LastAttackFrame != -1 && 
        m_currentFrame - p1LastAttackFrame < 60 && m_currentFrame - p2LastAttackFrame < 60) {
        
        int frameAdvantage = p2LastAttackFrame - p1LastAttackFrame;
        
        // Display frame advantage
        std::stringstream ss;
        ss << "Frame Advantage: ";
        if (frameAdvantage > 0) {
            ss << "P1 +" << frameAdvantage;
        } else if (frameAdvantage < 0) {
            ss << "P2 +" << (-frameAdvantage);
        } else {
            ss << "Even";
        }
        
        // Draw at the center of the screen
        float viewportWidth = static_cast<float>(m_renderer->getViewportWidth());
        float viewportHeight = static_cast<float>(m_renderer->getViewportHeight());
        
        m_renderer->drawRect(
            viewportWidth / 2 - 100, viewportHeight - 40, 
            200, 30, 
            0.1f, 0.1f, 0.1f, 
            m_opacity * 0.8f
        );
        
        m_renderer->drawTextWithShadow(
            viewportWidth / 2 - 80, viewportHeight - 35, 
            ss.str().c_str(), 
            1.0f, 1.0f, 0.0f, 
            m_opacity,
            14.0f
        );
    }
}

std::vector<std::string> InputDisplay::detectMotions(const std::deque<InputState>& history) {
    std::vector<std::string> detectedMotions;
    if (history.size() < 3) {
        return detectedMotions;
    }
    
    // This is a simplified motion detection algorithm
    // In a real implementation, we would check the exact sequence with proper timing
    
    // Get the most recent directional inputs
    std::vector<std::string> recentDirections;
    for (auto it = history.rbegin(); it != history.rend() && recentDirections.size() < 10; ++it) {
        std::string direction;
        
        if (it->up && !it->left && !it->right) direction = "↑";
        else if (it->up && it->right) direction = "↗";
        else if (!it->up && !it->down && it->right) direction = "→";
        else if (it->down && it->right) direction = "↘";
        else if (it->down && !it->left && !it->right) direction = "↓";
        else if (it->down && it->left) direction = "↙";
        else if (!it->up && !it->down && it->left) direction = "←";
        else if (it->up && it->left) direction = "↖";
        else continue; // Skip neutral state
        
        // Only add if different from the last direction
        if (recentDirections.empty() || direction != recentDirections.back()) {
            recentDirections.push_back(direction);
        }
    }
    
    // Check each motion pattern
    for (const auto& motion : m_motionPatterns) {
        if (motion.sequence.size() > recentDirections.size()) {
            continue; // Not enough inputs to match this pattern
        }
        
        // Check if the motion sequence appears in the recent directions
        bool found = false;
        for (size_t i = 0; i <= recentDirections.size() - motion.sequence.size(); ++i) {
            bool match = true;
            for (size_t j = 0; j < motion.sequence.size(); ++j) {
                if (recentDirections[i + j] != motion.sequence[j]) {
                    match = false;
                    break;
                }
            }
            if (match) {
                found = true;
                break;
            }
        }
        
        if (found) {
            detectedMotions.push_back(motion.name);
        }
    }
    
    return detectedMotions;
}

void InputDisplay::setP1Position(float x, float y) {
    m_p1X = x;
    m_p1Y = y;
}

void InputDisplay::setP2Position(float x, float y) {
    m_p2X = x;
    m_p2Y = y;
}

void InputDisplay::setHistorySize(int frames) {
    m_historySize = std::max(10, frames); // Ensure at least 10 frames
}

void InputDisplay::setHistoryEnabled(bool enabled) {
    m_historyEnabled = enabled;
}

void InputDisplay::setIconsEnabled(bool enabled) {
    m_iconsEnabled = enabled;
}

void InputDisplay::setFrameAdvantageEnabled(bool enabled) {
    m_frameAdvantageEnabled = enabled;
}

void InputDisplay::setOpacity(float opacity) {
    m_opacity = std::max(0.0f, std::min(1.0f, opacity));
}

float InputDisplay::getOpacity() const {
    return m_opacity;
} 