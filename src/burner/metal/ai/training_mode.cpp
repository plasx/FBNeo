#include "training_mode.h"
#include <iostream>
#include <algorithm>

namespace fbneo {
namespace ai {

// Training mode implementation
TrainingMode::TrainingMode() : m_enabled(false) {
    // Initialize with default options
    m_options = TrainingModeOptions();
}

TrainingMode::~TrainingMode() {
    // Clean up any resources
}

bool TrainingMode::initialize() {
    // Initialize training mode
    m_currentHitboxes.clear();
    m_inputHistory.clear();
    
    return true;
}

void TrainingMode::setEnabled(bool enabled) {
    m_enabled = enabled;
}

bool TrainingMode::isEnabled() const {
    return m_enabled;
}

void TrainingMode::setOptions(const TrainingModeOptions& options) {
    m_options = options;
}

const TrainingModeOptions& TrainingMode::getOptions() const {
    return m_options;
}

void TrainingMode::updateHitboxes(const uint8_t* gameMemory, int gameType) {
    // Clear existing hitboxes
    m_currentHitboxes.clear();
    
    // Create game-specific training data provider
    GameSpecificTrainingData* provider = createTrainingDataProvider("");
    
    // If no provider available for this game, return
    if (!provider) {
        return;
    }
    
    // Get hitboxes from provider
    m_currentHitboxes = provider->getHitboxes(gameMemory);
    
    // Clean up
    delete provider;
}

const std::vector<Hitbox>& TrainingMode::getHitboxes() const {
    return m_currentHitboxes;
}

void TrainingMode::updateFrameData(const uint8_t* gameMemory, int gameType) {
    // Create game-specific training data provider
    GameSpecificTrainingData* provider = createTrainingDataProvider("");
    
    // If no provider available for this game, return
    if (!provider) {
        return;
    }
    
    // Get frame data from provider
    m_lastMoveData = provider->getFrameData(gameMemory);
    
    // Clean up
    delete provider;
}

const FrameData& TrainingMode::getFrameData() const {
    return m_lastMoveData;
}

void TrainingMode::addInputToHistory(const std::string& input) {
    // Add input to history
    m_inputHistory.push_back(input);
    
    // Keep only the last 10 inputs
    if (m_inputHistory.size() > 10) {
        m_inputHistory.erase(m_inputHistory.begin());
    }
}

const std::vector<std::string>& TrainingMode::getInputHistory() const {
    return m_inputHistory;
}

void TrainingMode::render(unsigned char* screenBuffer, int width, int height, int pitch) {
    if (!m_enabled) {
        return;
    }
    
    // Render hitboxes if enabled
    if (m_options.showHitboxes) {
        renderHitboxes(screenBuffer, width, height, pitch);
    }
    
    // Render frame data if enabled
    if (m_options.showFrameData) {
        renderFrameData(screenBuffer, width, height, pitch);
    }
    
    // Render input display if enabled
    if (m_options.showInputDisplay) {
        renderInputDisplay(screenBuffer, width, height, pitch);
    }
}

void TrainingMode::renderHitboxes(unsigned char* screenBuffer, int width, int height, int pitch) {
    // Draw hitboxes on the screen buffer using Metal rendering
    if (m_currentHitboxes.empty()) {
        return;
    }
    
    // Use Metal renderer to draw hitboxes
    extern void MetalRenderer_BeginOverlay();
    extern void MetalRenderer_DrawRect(int x, int y, int width, int height, float r, float g, float b, float a);
    extern void MetalRenderer_EndOverlay();
    
    // Start overlay rendering
    MetalRenderer_BeginOverlay();
    
    // Draw each hitbox
    for (const auto& hitbox : m_currentHitboxes) {
        // Define colors based on hitbox type
        float r = 1.0f, g = 0.0f, b = 0.0f, a = 0.5f; // Red for attack hitboxes by default
        
        switch (hitbox.type) {
            case HitboxType::ATTACK:
                r = 1.0f; g = 0.0f; b = 0.0f; // Red
                break;
            case HitboxType::VULNERABLE:
                r = 0.0f; g = 0.0f; b = 1.0f; // Blue
                break;
            case HitboxType::COLLISION:
                r = 0.0f; g = 1.0f; b = 0.0f; // Green
                break;
            case HitboxType::THROW:
                r = 1.0f; g = 1.0f; b = 0.0f; // Yellow
                break;
            case HitboxType::CUSTOM:
                r = 1.0f; g = 0.0f; b = 1.0f; // Magenta
                break;
        }
        
        // Draw hitbox rectangle with outline
        // First draw filled transparent rectangle
        MetalRenderer_DrawRect(hitbox.x, hitbox.y, hitbox.width, hitbox.height, r, g, b, 0.3f);
        
        // Then draw outline with higher opacity
        // Top line
        MetalRenderer_DrawRect(hitbox.x, hitbox.y, hitbox.width, 1, r, g, b, 0.8f);
        // Bottom line
        MetalRenderer_DrawRect(hitbox.x, hitbox.y + hitbox.height - 1, hitbox.width, 1, r, g, b, 0.8f);
        // Left line
        MetalRenderer_DrawRect(hitbox.x, hitbox.y, 1, hitbox.height, r, g, b, 0.8f);
        // Right line
        MetalRenderer_DrawRect(hitbox.x + hitbox.width - 1, hitbox.y, 1, hitbox.height, r, g, b, 0.8f);
        
        // Draw frame/damage info if active
        if (hitbox.type == HitboxType::ATTACK && hitbox.frame_active > 0) {
            char infoText[32];
            snprintf(infoText, sizeof(infoText), "F:%d D:%d", hitbox.frame_active, hitbox.damage);
            
            extern void MetalRenderer_DrawText(int x, int y, const char* text, float r, float g, float b, float a, float scale);
            MetalRenderer_DrawText(hitbox.x, hitbox.y - 15, infoText, 1.0f, 1.0f, 1.0f, 1.0f, 0.8f);
        }
    }
    
    // End overlay rendering
    MetalRenderer_EndOverlay();
}

void TrainingMode::renderFrameData(unsigned char* screenBuffer, int width, int height, int pitch) {
    // Use Metal renderer to draw frame data
    extern void MetalRenderer_BeginOverlay();
    extern void MetalRenderer_DrawRect(int x, int y, int width, int height, float r, float g, float b, float a);
    extern void MetalRenderer_DrawText(int x, int y, const char* text, float r, float g, float b, float a, float scale);
    extern void MetalRenderer_EndOverlay();
    
    // Start overlay rendering
    MetalRenderer_BeginOverlay();
    
    // Determine position for frame data display (top-right corner)
    int x = width - 200;
    int y = 20;
    int panelWidth = 180;
    int panelHeight = 130;
    
    // Draw semi-transparent background panel
    MetalRenderer_DrawRect(x, y, panelWidth, panelHeight, 0.0f, 0.0f, 0.0f, 0.7f);
    
    // Draw title
    MetalRenderer_DrawText(x + 10, y + 10, "FRAME DATA", 1.0f, 1.0f, 0.0f, 1.0f, 1.0f);
    
    // Draw horizontal separator line
    MetalRenderer_DrawRect(x + 5, y + 30, panelWidth - 10, 1, 1.0f, 1.0f, 0.0f, 0.8f);
    
    // Draw frame data details
    char buffer[64];
    
    y += 40;
    snprintf(buffer, sizeof(buffer), "Startup: %d", m_lastMoveData.startup);
    MetalRenderer_DrawText(x + 10, y, buffer, 1.0f, 1.0f, 1.0f, 1.0f, 0.9f);
    
    y += 20;
    snprintf(buffer, sizeof(buffer), "Active: %d", m_lastMoveData.active);
    MetalRenderer_DrawText(x + 10, y, buffer, 1.0f, 1.0f, 1.0f, 1.0f, 0.9f);
    
    y += 20;
    snprintf(buffer, sizeof(buffer), "Recovery: %d", m_lastMoveData.recovery);
    MetalRenderer_DrawText(x + 10, y, buffer, 1.0f, 1.0f, 1.0f, 1.0f, 0.9f);
    
    y += 20;
    // Color advantage value based on whether it's positive or negative
    snprintf(buffer, sizeof(buffer), "Advantage: %+d", m_lastMoveData.advantage);
    float advR = m_lastMoveData.advantage >= 0 ? 0.0f : 1.0f;
    float advG = m_lastMoveData.advantage >= 0 ? 1.0f : 0.0f;
    MetalRenderer_DrawText(x + 10, y, buffer, advR, advG, 0.0f, 1.0f, 0.9f);
    
    y += 20;
    snprintf(buffer, sizeof(buffer), "Damage: %d", m_lastMoveData.damage);
    MetalRenderer_DrawText(x + 10, y, buffer, 1.0f, 1.0f, 1.0f, 1.0f, 0.9f);
    
    // End overlay rendering
    MetalRenderer_EndOverlay();
}

void TrainingMode::renderInputDisplay(unsigned char* screenBuffer, int width, int height, int pitch) {
    // Use Metal renderer to draw input display
    extern void MetalRenderer_BeginOverlay();
    extern void MetalRenderer_DrawRect(int x, int y, int width, int height, float r, float g, float b, float a);
    extern void MetalRenderer_DrawText(int x, int y, const char* text, float r, float g, float b, float a, float scale);
    extern void MetalRenderer_DrawCircle(int x, int y, int radius, float r, float g, float b, float a);
    extern void MetalRenderer_EndOverlay();
    
    // Start overlay rendering
    MetalRenderer_BeginOverlay();
    
    // Determine position for input display (bottom of screen)
    int baseX = width / 2 - 200;
    int baseY = height - 70;
    int panelWidth = 400;
    int panelHeight = 60;
    
    // Draw semi-transparent background panel
    MetalRenderer_DrawRect(baseX, baseY, panelWidth, panelHeight, 0.0f, 0.0f, 0.0f, 0.7f);
    
    // Draw title
    MetalRenderer_DrawText(baseX + 10, baseY + 5, "INPUT HISTORY", 1.0f, 1.0f, 1.0f, 1.0f, 0.8f);
    
    // Draw input history
    int x = baseX + 10;
    int y = baseY + 25;
    int buttonSize = 18;
    int buttonSpacing = 4;
    
    // Map of input symbols to colors
    const struct {
        char symbol;
        float r, g, b;
    } colorMap[] = {
        {'↑', 1.0f, 1.0f, 1.0f}, // up - white
        {'↓', 1.0f, 1.0f, 1.0f}, // down - white
        {'←', 1.0f, 1.0f, 1.0f}, // left - white
        {'→', 1.0f, 1.0f, 1.0f}, // right - white
        {'A', 1.0f, 0.0f, 0.0f}, // button 1 - red
        {'B', 0.0f, 1.0f, 0.0f}, // button 2 - green
        {'C', 0.0f, 0.0f, 1.0f}, // button 3 - blue
        {'D', 1.0f, 1.0f, 0.0f}, // button 4 - yellow
        {'E', 1.0f, 0.0f, 1.0f}, // button 5 - magenta
        {'F', 0.0f, 1.0f, 1.0f}, // button 6 - cyan
        {'S', 1.0f, 0.6f, 0.0f}, // start - orange
        {'C', 0.7f, 0.7f, 0.7f}  // coin - gray
    };
    
    // Draw each input in the history
    for (int i = m_inputHistory.size() - 1; i >= 0; i--) {
        const std::string& input = m_inputHistory[i];
        
        // Parse the input string and visualize it using circles with symbols
        for (char c : input) {
            // Find color for this symbol
            float r = 1.0f, g = 1.0f, b = 1.0f;
            for (const auto& mapping : colorMap) {
                if (mapping.symbol == c) {
                    r = mapping.r;
                    g = mapping.g;
                    b = mapping.b;
                    break;
                }
            }
            
            // Draw button circle
            MetalRenderer_DrawCircle(x + buttonSize/2, y + buttonSize/2, buttonSize/2, r, g, b, 0.8f);
            
            // Draw symbol inside
            char symbolStr[2] = {c, '\0'};
            MetalRenderer_DrawText(x + buttonSize/2 - 5, y + buttonSize/2 - 7, symbolStr, 0.0f, 0.0f, 0.0f, 1.0f, 0.9f);
            
            // Move to next position
            x += buttonSize + buttonSpacing;
        }
        
        // Add a separator between different inputs
        MetalRenderer_DrawRect(x, y, 2, buttonSize, 0.5f, 0.5f, 0.5f, 0.5f);
        x += 10;
        
        // If we run out of space, stop drawing
        if (x > baseX + panelWidth - buttonSize) {
            break;
        }
    }
    
    // End overlay rendering
    MetalRenderer_EndOverlay();
}

// Implementation for Street Fighter III: 3rd Strike
class SF3ThirdStrikeTrainingData : public GameSpecificTrainingData {
public:
    std::vector<Hitbox> getHitboxes(const uint8_t* gameMemory) override {
        // This would be implemented with actual memory addresses for SF3
        std::vector<Hitbox> hitboxes;
        
        // Example hitbox for demonstration
        Hitbox h;
        h.type = HitboxType::ATTACK;
        h.x = 100;
        h.y = 100;
        h.width = 50;
        h.height = 30;
        h.frame_active = 3;
        h.damage = 10;
        hitboxes.push_back(h);
        
        return hitboxes;
    }
    
    FrameData getFrameData(const uint8_t* gameMemory) override {
        // This would be implemented with actual memory addresses for SF3
        FrameData data;
        data.startup = 3;
        data.active = 2;
        data.recovery = 10;
        data.advantage = -2;
        data.damage = 100;
        data.isProjectile = false;
        
        return data;
    }
    
    std::vector<int> getHealthAddresses() override {
        // Return memory addresses for player health in SF3
        return { 0x02068D6B, 0x02069087 };
    }
    
    int getTimerAddress() override {
        // Return memory address for timer in SF3
        return 0x02002E09;
    }
    
    void applyInfiniteHealth(uint8_t* gameMemory) override {
        // Set health to maximum for both players
        auto addresses = getHealthAddresses();
        for (auto addr : addresses) {
            gameMemory[addr] = 0x90; // Example max health value
        }
    }
    
    void applyInfiniteTime(uint8_t* gameMemory) override {
        // Set timer to maximum
        gameMemory[getTimerAddress()] = 0x99; // Example max timer value
    }
};

// Implementation for Marvel vs Capcom
class MarvelVsCapcomTrainingData : public GameSpecificTrainingData {
public:
    std::vector<Hitbox> getHitboxes(const uint8_t* gameMemory) override {
        // This would be implemented with actual memory addresses for MvC
        std::vector<Hitbox> hitboxes;
        
        // Example hitboxes for demonstration
        Hitbox h1;
        h1.type = HitboxType::ATTACK;
        h1.x = 150;
        h1.y = 80;
        h1.width = 60;
        h1.height = 40;
        h1.frame_active = 5;
        h1.damage = 15;
        hitboxes.push_back(h1);
        
        Hitbox h2;
        h2.type = HitboxType::VULNERABLE;
        h2.x = 130;
        h2.y = 70;
        h2.width = 100;
        h2.height = 120;
        h2.frame_active = 0;
        h2.damage = 0;
        hitboxes.push_back(h2);
        
        return hitboxes;
    }
    
    FrameData getFrameData(const uint8_t* gameMemory) override {
        // This would be implemented with actual memory addresses for MvC
        FrameData data;
        data.startup = 4;
        data.active = 3;
        data.recovery = 12;
        data.advantage = 2;
        data.damage = 120;
        data.isProjectile = true;
        
        return data;
    }
    
    std::vector<int> getHealthAddresses() override {
        // Return memory addresses for player health in MvC
        return { 0x0206A123, 0x0206A456 };
    }
    
    int getTimerAddress() override {
        // Return memory address for timer in MvC
        return 0x02003B78;
    }
    
    void applyInfiniteHealth(uint8_t* gameMemory) override {
        // Set health to maximum for both players
        auto addresses = getHealthAddresses();
        for (auto addr : addresses) {
            gameMemory[addr] = 0xC8; // Example max health value
        }
    }
    
    void applyInfiniteTime(uint8_t* gameMemory) override {
        // Set timer to maximum
        gameMemory[getTimerAddress()] = 0x99; // Example max timer value
    }
};

// Factory implementation
GameSpecificTrainingData* createTrainingDataProvider(const char* gameName) {
    std::string game = gameName ? gameName : "";
    
    // Convert to lowercase for case-insensitive comparison
    std::transform(game.begin(), game.end(), game.begin(), ::tolower);
    
    if (game.find("sf3") != std::string::npos || 
        game.find("street fighter iii") != std::string::npos ||
        game.find("3rd strike") != std::string::npos) {
        return new SF3ThirdStrikeTrainingData();
    }
    else if (game.find("marvel") != std::string::npos || 
             game.find("mvc") != std::string::npos ||
             game.find("vs capcom") != std::string::npos) {
        return new MarvelVsCapcomTrainingData();
    }
    
    // No specific provider available
    return nullptr;
}

} // namespace ai
} // namespace fbneo 