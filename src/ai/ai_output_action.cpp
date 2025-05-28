#include "ai_output_action.h"
#include <sstream>
#include <algorithm>
#include <nlohmann/json.hpp>

namespace AI {

AIOutputAction::AIOutputAction() 
    : m_buttons(), m_frameNumber(0) 
{
}

AIOutputAction::AIOutputAction(unsigned int frameNumber) 
    : m_buttons(), m_frameNumber(frameNumber) 
{
}

AIOutputAction::AIOutputAction(const AIOutputAction& other) 
    : m_buttons(other.m_buttons), m_frameNumber(other.m_frameNumber) 
{
}

AIOutputAction::~AIOutputAction() 
{
    // Nothing to clean up for now
}

void AIOutputAction::setButton(ButtonMapping button, bool pressed) 
{
    if (button >= 0 && button < MAX_BUTTONS) {
        m_buttons.set(button, pressed);
    }
}

bool AIOutputAction::isButtonPressed(ButtonMapping button) const 
{
    if (button >= 0 && button < MAX_BUTTONS) {
        return m_buttons.test(button);
    }
    return false;
}

void AIOutputAction::clearAllButtons() 
{
    m_buttons.reset();
}

bool AIOutputAction::isIdle() const 
{
    // Idle means no buttons are pressed
    return m_buttons.none();
}

bool AIOutputAction::isJumping() const 
{
    // Jumping usually involves the UP button
    return m_buttons.test(UP);
}

bool AIOutputAction::isPunching() const 
{
    // Punching usually involves BUTTON1 or BUTTON2
    return m_buttons.test(BUTTON1) || m_buttons.test(BUTTON2);
}

bool AIOutputAction::isKicking() const 
{
    // Kicking usually involves BUTTON3 or BUTTON4
    return m_buttons.test(BUTTON3) || m_buttons.test(BUTTON4);
}

bool AIOutputAction::isBlocking() const 
{
    // Blocking usually involves holding back (LEFT or RIGHT depending on facing direction)
    // This is a simplified check - in a real game you would need to know player facing
    return m_buttons.test(LEFT) || m_buttons.test(RIGHT);
}

void AIOutputAction::setFrameNumber(unsigned int frameNumber) 
{
    m_frameNumber = frameNumber;
}

unsigned int AIOutputAction::getFrameNumber() const 
{
    return m_frameNumber;
}

std::string AIOutputAction::toString() const 
{
    std::stringstream ss;
    ss << "Frame: " << m_frameNumber << " Buttons: [";
    
    if (m_buttons.test(UP)) ss << "UP ";
    if (m_buttons.test(DOWN)) ss << "DOWN ";
    if (m_buttons.test(LEFT)) ss << "LEFT ";
    if (m_buttons.test(RIGHT)) ss << "RIGHT ";
    if (m_buttons.test(BUTTON1)) ss << "B1 ";
    if (m_buttons.test(BUTTON2)) ss << "B2 ";
    if (m_buttons.test(BUTTON3)) ss << "B3 ";
    if (m_buttons.test(BUTTON4)) ss << "B4 ";
    if (m_buttons.test(BUTTON5)) ss << "B5 ";
    if (m_buttons.test(BUTTON6)) ss << "B6 ";
    if (m_buttons.test(START)) ss << "START ";
    if (m_buttons.test(COIN)) ss << "COIN ";
    
    ss << "]";
    return ss.str();
}

std::string AIOutputAction::toJson() const 
{
    nlohmann::json j;
    j["frame_number"] = m_frameNumber;
    j["buttons"] = {
        {"up", m_buttons.test(UP)},
        {"down", m_buttons.test(DOWN)},
        {"left", m_buttons.test(LEFT)},
        {"right", m_buttons.test(RIGHT)},
        {"button1", m_buttons.test(BUTTON1)},
        {"button2", m_buttons.test(BUTTON2)},
        {"button3", m_buttons.test(BUTTON3)},
        {"button4", m_buttons.test(BUTTON4)},
        {"button5", m_buttons.test(BUTTON5)},
        {"button6", m_buttons.test(BUTTON6)},
        {"start", m_buttons.test(START)},
        {"coin", m_buttons.test(COIN)}
    };
    return j.dump();
}

AIOutputAction AIOutputAction::fromJson(const std::string& json) 
{
    AIOutputAction action;
    
    try {
        nlohmann::json j = nlohmann::json::parse(json);
        action.setFrameNumber(j["frame_number"]);
        
        const auto& buttons = j["buttons"];
        action.setButton(UP, buttons["up"]);
        action.setButton(DOWN, buttons["down"]);
        action.setButton(LEFT, buttons["left"]);
        action.setButton(RIGHT, buttons["right"]);
        action.setButton(BUTTON1, buttons["button1"]);
        action.setButton(BUTTON2, buttons["button2"]);
        action.setButton(BUTTON3, buttons["button3"]);
        action.setButton(BUTTON4, buttons["button4"]);
        action.setButton(BUTTON5, buttons["button5"]);
        action.setButton(BUTTON6, buttons["button6"]);
        action.setButton(START, buttons["start"]);
        action.setButton(COIN, buttons["coin"]);
    } catch (const std::exception& e) {
        // Handle parsing errors
    }
    
    return action;
}

AIOutputAction& AIOutputAction::operator=(const AIOutputAction& other) 
{
    if (this != &other) {
        m_buttons = other.m_buttons;
        m_frameNumber = other.m_frameNumber;
    }
    return *this;
}

bool AIOutputAction::operator==(const AIOutputAction& other) const 
{
    return (m_buttons == other.m_buttons) && 
           (m_frameNumber == other.m_frameNumber);
}

bool AIOutputAction::operator!=(const AIOutputAction& other) const 
{
    return !(*this == other);
}

std::bitset<AIOutputAction::MAX_BUTTONS> AIOutputAction::getButtonBitset() const 
{
    return m_buttons;
}

void AIOutputAction::setButtonBitset(const std::bitset<MAX_BUTTONS>& buttons) 
{
    m_buttons = buttons;
}

} // namespace AI