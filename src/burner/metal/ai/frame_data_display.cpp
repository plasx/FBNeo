#include "frame_data_display.h"
#include "overlay_renderer.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cmath>

FrameDataDisplay::FrameDataDisplay()
    : m_renderer(nullptr)
    , m_memoryMapping(nullptr)
    , m_enabled(true)
    , m_detailedView(false)
    , m_posX(10)
    , m_posY(400)
    , m_opacity(0.8f)
{
    // Initialize frame data structures
    m_p1FrameData = {0, 0, 0, 0, false, 0, ""};
    m_p2FrameData = {0, 0, 0, 0, false, 0, ""};
}

FrameDataDisplay::~FrameDataDisplay()
{
    // Nothing to clean up
}

bool FrameDataDisplay::initialize(OverlayRenderer* renderer, AIMemoryMapping* memoryMapping)
{
    if (!renderer || !memoryMapping) {
        std::cerr << "FrameDataDisplay: Invalid renderer or memory mapping" << std::endl;
        return false;
    }
    
    m_renderer = renderer;
    m_memoryMapping = memoryMapping;
    
    // Try to load settings
    loadSettings();
    
    return true;
}

void FrameDataDisplay::update(float deltaTime)
{
    if (!m_enabled || !m_memoryMapping) {
        return;
    }
    
    // Get frame data for both players
    m_p1FrameData = getFrameData(1);
    m_p2FrameData = getFrameData(2);
    
    // Calculate frame advantage if applicable
    if (m_p1FrameData.isAttacking && !m_p2FrameData.isAttacking) {
        m_p1FrameData.advantage = calculateFrameAdvantage(m_p1FrameData, m_p2FrameData);
    } else if (m_p2FrameData.isAttacking && !m_p1FrameData.isAttacking) {
        m_p2FrameData.advantage = calculateFrameAdvantage(m_p2FrameData, m_p1FrameData);
    }
}

void FrameDataDisplay::render(int width, int height, float opacity)
{
    if (!m_enabled || !m_renderer) {
        return;
    }
    
    float actualOpacity = m_opacity * opacity;
    
    // Draw header
    float headerHeight = 30.0f;
    float panelWidth = 300.0f;
    float panelHeight = 150.0f;
    float x = m_posX;
    float y = height - panelHeight - m_posY;
    
    // Draw background
    m_renderer->drawRect(x, y, panelWidth, headerHeight, 0.1f, 0.1f, 0.2f, actualOpacity * 0.8f);
    m_renderer->drawRect(x, y + headerHeight, panelWidth, panelHeight - headerHeight, 0.1f, 0.1f, 0.1f, actualOpacity * 0.7f);
    
    // Draw title
    m_renderer->drawTextWithShadow(
        x + 10, y + 7,
        "Frame Data",
        1.0f, 1.0f, 1.0f,
        actualOpacity,
        16.0f
    );
    
    // Draw frame data for both players
    drawFrameData(x + 10, y + headerHeight + 10, 1, m_p1FrameData, actualOpacity);
    drawFrameData(x + 160, y + headerHeight + 10, 2, m_p2FrameData, actualOpacity);
}

void FrameDataDisplay::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

bool FrameDataDisplay::isEnabled() const
{
    return m_enabled;
}

void FrameDataDisplay::setPosition(float x, float y)
{
    m_posX = x;
    m_posY = y;
}

void FrameDataDisplay::getPosition(float& x, float& y) const
{
    x = m_posX;
    y = m_posY;
}

void FrameDataDisplay::setOpacity(float opacity)
{
    m_opacity = std::max(0.0f, std::min(1.0f, opacity));
}

float FrameDataDisplay::getOpacity() const
{
    return m_opacity;
}

void FrameDataDisplay::setDetailedView(bool detailed)
{
    m_detailedView = detailed;
}

bool FrameDataDisplay::isDetailedView() const
{
    return m_detailedView;
}

bool FrameDataDisplay::saveSettings(const std::string& filename)
{
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "FrameDataDisplay: Failed to open settings file for writing: " << filename << std::endl;
            return false;
        }
        
        file << "{\n";
        file << "  \"enabled\": " << (m_enabled ? "true" : "false") << ",\n";
        file << "  \"detailedView\": " << (m_detailedView ? "true" : "false") << ",\n";
        file << "  \"posX\": " << m_posX << ",\n";
        file << "  \"posY\": " << m_posY << ",\n";
        file << "  \"opacity\": " << m_opacity << "\n";
        file << "}\n";
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "FrameDataDisplay: Error saving settings: " << e.what() << std::endl;
        return false;
    }
}

bool FrameDataDisplay::loadSettings(const std::string& filename)
{
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            // File doesn't exist yet, which is fine
            return false;
        }
        
        // Very basic JSON parsing
        std::string line;
        while (std::getline(file, line)) {
            size_t pos;
            
            // Check for enabled setting
            if ((pos = line.find("\"enabled\":")) != std::string::npos) {
                pos = line.find("true", pos);
                m_enabled = (pos != std::string::npos);
            }
            
            // Check for detailed view setting
            else if ((pos = line.find("\"detailedView\":")) != std::string::npos) {
                pos = line.find("true", pos);
                m_detailedView = (pos != std::string::npos);
            }
            
            // Check for posX setting
            else if ((pos = line.find("\"posX\":")) != std::string::npos) {
                pos = line.find(":", pos);
                if (pos != std::string::npos) {
                    // Extract the number
                    pos += 1;
                    size_t end = line.find_first_of(",}\n", pos);
                    if (end != std::string::npos) {
                        std::string valueStr = line.substr(pos, end - pos);
                        try {
                            float value = std::stof(valueStr);
                            m_posX = value;
                        } catch (...) {
                            // Ignore parsing errors
                        }
                    }
                }
            }
            
            // Check for posY setting
            else if ((pos = line.find("\"posY\":")) != std::string::npos) {
                pos = line.find(":", pos);
                if (pos != std::string::npos) {
                    // Extract the number
                    pos += 1;
                    size_t end = line.find_first_of(",}\n", pos);
                    if (end != std::string::npos) {
                        std::string valueStr = line.substr(pos, end - pos);
                        try {
                            float value = std::stof(valueStr);
                            m_posY = value;
                        } catch (...) {
                            // Ignore parsing errors
                        }
                    }
                }
            }
            
            // Check for opacity setting
            else if ((pos = line.find("\"opacity\":")) != std::string::npos) {
                pos = line.find(":", pos);
                if (pos != std::string::npos) {
                    // Extract the number
                    pos += 1;
                    size_t end = line.find_first_of(",}\n", pos);
                    if (end != std::string::npos) {
                        std::string valueStr = line.substr(pos, end - pos);
                        try {
                            float value = std::stof(valueStr);
                            m_opacity = std::max(0.0f, std::min(1.0f, value));
                        } catch (...) {
                            // Ignore parsing errors
                        }
                    }
                }
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "FrameDataDisplay: Error loading settings: " << e.what() << std::endl;
        return false;
    }
}

FrameDataDisplay::FrameData FrameDataDisplay::getFrameData(int playerIndex)
{
    FrameData data = {0, 0, 0, 0, false, 0, ""};
    
    if (!m_memoryMapping) {
        return data;
    }
    
    std::string prefix = "p" + std::to_string(playerIndex) + "_";
    
    try {
        // Check if the player is attacking
        if (m_memoryMapping->hasMapping(prefix + "is_attacking")) {
            data.isAttacking = m_memoryMapping->readBool(prefix + "is_attacking");
        }
        
        // Get current animation frame
        if (m_memoryMapping->hasMapping(prefix + "animation_frame")) {
            data.currentFrame = m_memoryMapping->readInt(prefix + "animation_frame");
        }
        
        // Get attack data if available
        if (m_memoryMapping->hasMapping(prefix + "attack_startup")) {
            data.startup = m_memoryMapping->readInt(prefix + "attack_startup");
        }
        
        if (m_memoryMapping->hasMapping(prefix + "attack_active")) {
            data.active = m_memoryMapping->readInt(prefix + "attack_active");
        }
        
        if (m_memoryMapping->hasMapping(prefix + "attack_recovery")) {
            data.recovery = m_memoryMapping->readInt(prefix + "attack_recovery");
        }
        
        // Get move name if available
        if (m_memoryMapping->hasMapping(prefix + "move_name")) {
            data.moveName = m_memoryMapping->getStringValue(prefix + "move_name");
        }
        
    } catch (const std::exception& e) {
        std::cerr << "FrameDataDisplay: Error reading frame data: " << e.what() << std::endl;
    }
    
    return data;
}

int FrameDataDisplay::calculateFrameAdvantage(const FrameData& attacker, const FrameData& defender)
{
    // This is a simplified calculation and would need to be adapted for each game
    if (attacker.startup <= 0 || attacker.active <= 0 || attacker.recovery <= 0) {
        return 0;
    }
    
    // Basic formula: active frames + recovery frames - defender's reaction frames
    int totalFrames = attacker.startup + attacker.active + attacker.recovery;
    int frameAdvantage = 0;
    
    // In a real implementation, this would depend on hit/block state and game-specific logic
    if (m_memoryMapping->hasMapping("hit_state")) {
        std::string hitState = m_memoryMapping->getStringValue("hit_state");
        
        if (hitState == "hit") {
            // On hit, generally more advantageous
            frameAdvantage = attacker.active - attacker.recovery;
        } else if (hitState == "block") {
            // On block, generally less advantageous
            frameAdvantage = attacker.active - attacker.recovery - 2;
        }
    } else {
        // Default approximation
        frameAdvantage = attacker.active - attacker.recovery;
    }
    
    return frameAdvantage;
}

void FrameDataDisplay::drawFrameData(float x, float y, int playerIndex, const FrameData& data, float opacity)
{
    if (!m_renderer) {
        return;
    }
    
    // Set colors based on player
    float r, g, b;
    if (playerIndex == 1) {
        r = 0.2f;
        g = 0.6f;
        b = 0.9f;
    } else {
        r = 0.9f;
        g = 0.2f;
        b = 0.2f;
    }
    
    // Draw player header
    std::string playerText = "P" + std::to_string(playerIndex);
    m_renderer->drawText(
        x, y,
        playerText.c_str(),
        r, g, b,
        opacity,
        14.0f
    );
    
    y += 20;
    
    // Draw frame data
    std::string frameText = "Frame: " + std::to_string(data.currentFrame);
    m_renderer->drawText(
        x, y,
        frameText.c_str(),
        1.0f, 1.0f, 1.0f,
        opacity,
        12.0f
    );
    
    y += 16;
    
    // Draw startup/active/recovery
    std::string framesText = "S/A/R: ";
    if (data.startup > 0 || data.active > 0 || data.recovery > 0) {
        framesText += std::to_string(data.startup) + "/" + 
                     std::to_string(data.active) + "/" + 
                     std::to_string(data.recovery);
    } else {
        framesText += "N/A";
    }
    
    m_renderer->drawText(
        x, y,
        framesText.c_str(),
        1.0f, 1.0f, 1.0f,
        opacity,
        12.0f
    );
    
    y += 16;
    
    // Draw advantage with color coding
    std::string advText = "Adv: ";
    float advR = 1.0f, advG = 1.0f, advB = 1.0f;
    
    if (data.advantage != 0) {
        advText += (data.advantage > 0 ? "+" : "") + std::to_string(data.advantage);
        
        // Color code advantage
        if (data.advantage > 0) {
            // Positive advantage - green
            advR = 0.2f;
            advG = 0.9f;
            advB = 0.2f;
        } else if (data.advantage < -2) {
            // Very negative advantage - red
            advR = 0.9f;
            advG = 0.2f;
            advB = 0.2f;
        } else {
            // Slightly negative advantage - yellow
            advR = 0.9f;
            advG = 0.9f;
            advB = 0.2f;
        }
    } else {
        advText += "N/A";
    }
    
    m_renderer->drawText(
        x, y,
        advText.c_str(),
        advR, advG, advB,
        opacity,
        12.0f
    );
    
    y += 16;
    
    // Draw move name if available and in detailed view
    if (m_detailedView && !data.moveName.empty()) {
        m_renderer->drawText(
            x, y,
            data.moveName.c_str(),
            1.0f, 1.0f, 1.0f,
            opacity,
            12.0f
        );
    }
} 