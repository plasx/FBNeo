#include "training_overlay.h"
#include "ai_memory_mapping.h"
#include "../burner/metal/metal_renderer.h"
#include "../burner/metal/metal_common.h"
#include <algorithm>
#include <fstream>
#include <json.hpp>
#include <chrono>

// For JSON serialization
using json = nlohmann::json;

// Singleton instance
TrainingOverlay* TrainingOverlay::s_instance = nullptr;

TrainingOverlay* TrainingOverlay::getInstance() {
    if (!s_instance) {
        s_instance = new TrainingOverlay();
    }
    return s_instance;
}

TrainingOverlay::TrainingOverlay() 
    : m_renderer(nullptr)
    , m_memoryMapping(nullptr)
    , m_hitboxEnabled(false)
    , m_frameDataEnabled(false)
    , m_inputHistoryEnabled(false)
    , m_stateInfoEnabled(false)
    , m_hitboxOpacity(0.7f)
    , m_fontSize(16.0f)
    , m_inputHistoryLength(10)
    , m_currentFrame(0)
    , m_currentTimestamp(0)
{
    // Initialize default colors for hitbox types
    m_hitboxColors[HitboxType::Attack] = Color::Red();
    m_hitboxColors[HitboxType::Hurtbox] = Color::Blue();
    m_hitboxColors[HitboxType::Pushbox] = Color::Green();
    m_hitboxColors[HitboxType::Throwbox] = Color::Purple();
    m_hitboxColors[HitboxType::Projectile] = Color::Orange();
    m_hitboxColors[HitboxType::Special] = Color::Yellow();
    m_hitboxColors[HitboxType::Counter] = Color::White();
    
    // Initialize default component positions
    m_componentPositions["frameData"] = {10.0f, 30.0f};
    m_componentPositions["inputHistory"] = {10.0f, 100.0f};
    m_componentPositions["stateInfo"] = {10.0f, 200.0f};
}

TrainingOverlay::~TrainingOverlay() {
    shutdown();
}

bool TrainingOverlay::initialize(MetalRenderer* renderer, AIMemoryMapping* memoryMapping) {
    if (!renderer) {
        return false;
    }
    
    m_renderer = renderer;
    m_memoryMapping = memoryMapping;
    
    // Initialize display components
    m_hitboxDisplay = std::make_unique<HitboxDisplay>(renderer);
    m_frameDataDisplay = std::make_unique<FrameDataDisplay>(renderer);
    m_inputHistoryDisplay = std::make_unique<InputHistoryDisplay>(renderer);
    m_stateInfoDisplay = std::make_unique<StateInfoDisplay>(renderer);
    
    // Load configuration if available
    loadConfiguration("config/training_overlay.json");
    
    return true;
}

void TrainingOverlay::shutdown() {
    // Save configuration
    saveConfiguration("config/training_overlay.json");
    
    // Clean up display components
    m_hitboxDisplay.reset();
    m_frameDataDisplay.reset();
    m_inputHistoryDisplay.reset();
    m_stateInfoDisplay.reset();
    
    m_renderer = nullptr;
    m_memoryMapping = nullptr;
}

void TrainingOverlay::update() {
    if (!m_renderer || !m_memoryMapping) {
        return;
    }
    
    // Update frame counter
    updateFrameCounter();
    
    // Collect data from memory
    if (m_hitboxEnabled) {
        collectHitboxData();
        m_hitboxDisplay->update(m_memoryMapping, m_hitboxes);
    }
    
    if (m_frameDataEnabled) {
        collectFrameData();
        m_frameDataDisplay->update(m_memoryMapping, m_frameData);
    }
    
    if (m_stateInfoEnabled) {
        collectStateData();
        m_stateInfoDisplay->update(m_memoryMapping);
    }
    
    if (m_inputHistoryEnabled) {
        m_inputHistoryDisplay->update(m_inputHistory);
    }
    
    // Process hotkeys
    ProcessHotkeys();
}

void TrainingOverlay::render() {
    if (!m_renderer) {
        return;
    }
    
    // Render enabled components
    if (m_hitboxEnabled) {
        m_hitboxDisplay->render(m_hitboxOpacity, m_hitboxColors);
    }
    
    if (m_frameDataEnabled) {
        m_frameDataDisplay->render(m_fontSize, m_componentPositions["frameData"]);
    }
    
    if (m_inputHistoryEnabled) {
        m_inputHistoryDisplay->render(m_fontSize, m_componentPositions["inputHistory"]);
    }
    
    if (m_stateInfoEnabled) {
        m_stateInfoDisplay->render(m_fontSize, m_componentPositions["stateInfo"]);
    }
}

void TrainingOverlay::addInputEvent(int playerIndex, uint32_t inputBits) {
    if (playerIndex < 0 || playerIndex >= 2) {
        return;
    }
    
    InputEvent event;
    event.playerIndex = playerIndex;
    event.inputBits = inputBits;
    event.frameNumber = m_currentFrame;
    event.timestamp = m_currentTimestamp;
    
    m_inputHistory[playerIndex].push_front(event);
    
    // Trim to configured length
    while (m_inputHistory[playerIndex].size() > m_inputHistoryLength) {
        m_inputHistory[playerIndex].pop_back();
    }
}

void TrainingOverlay::setHitboxDisplay(bool enabled) {
    m_hitboxEnabled = enabled;
}

void TrainingOverlay::setFrameDataDisplay(bool enabled) {
    m_frameDataEnabled = enabled;
}

void TrainingOverlay::setInputHistoryDisplay(bool enabled) {
    m_inputHistoryEnabled = enabled;
}

void TrainingOverlay::setStateInfoDisplay(bool enabled) {
    m_stateInfoEnabled = enabled;
}

void TrainingOverlay::setHitboxOpacity(float opacity) {
    m_hitboxOpacity = std::clamp(opacity, 0.0f, 1.0f);
}

void TrainingOverlay::setHitboxColor(HitboxType type, float r, float g, float b) {
    m_hitboxColors[type] = Color(r, g, b, m_hitboxOpacity);
}

void TrainingOverlay::setFontSize(float fontSize) {
    m_fontSize = fontSize;
}

void TrainingOverlay::setInputHistoryLength(int length) {
    m_inputHistoryLength = length;
    
    // Trim existing history if needed
    for (int i = 0; i < 2; i++) {
        while (m_inputHistory[i].size() > m_inputHistoryLength) {
            m_inputHistory[i].pop_back();
        }
    }
}

void TrainingOverlay::setComponentPosition(const std::string& componentName, float x, float y) {
    m_componentPositions[componentName] = {x, y};
}

bool TrainingOverlay::saveConfiguration(const std::string& filename) {
    try {
        // Create JSON object
        json config;
        
        config["hitboxEnabled"] = m_hitboxEnabled;
        config["frameDataEnabled"] = m_frameDataEnabled;
        config["inputHistoryEnabled"] = m_inputHistoryEnabled;
        config["stateInfoEnabled"] = m_stateInfoEnabled;
        config["hitboxOpacity"] = m_hitboxOpacity;
        config["fontSize"] = m_fontSize;
        config["inputHistoryLength"] = m_inputHistoryLength;
        
        // Save hitbox colors
        json colors;
        for (const auto& [type, color] : m_hitboxColors) {
            colors[std::to_string(static_cast<int>(type))] = {
                {"r", color.r},
                {"g", color.g},
                {"b", color.b},
                {"a", color.a}
            };
        }
        config["hitboxColors"] = colors;
        
        // Save component positions
        json positions;
        for (const auto& [name, pos] : m_componentPositions) {
            positions[name] = {{"x", pos[0]}, {"y", pos[1]}};
        }
        config["componentPositions"] = positions;
        
        // Write to file
        std::ofstream file(filename);
        if (!file) {
            return false;
        }
        
        file << config.dump(4); // Pretty print with 4-space indent
        return true;
    } catch (...) {
        return false;
    }
}

bool TrainingOverlay::loadConfiguration(const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (!file) {
            return false;
        }
        
        json config;
        file >> config;
        
        // Load basic settings
        m_hitboxEnabled = config.value("hitboxEnabled", m_hitboxEnabled);
        m_frameDataEnabled = config.value("frameDataEnabled", m_frameDataEnabled);
        m_inputHistoryEnabled = config.value("inputHistoryEnabled", m_inputHistoryEnabled);
        m_stateInfoEnabled = config.value("stateInfoEnabled", m_stateInfoEnabled);
        m_hitboxOpacity = config.value("hitboxOpacity", m_hitboxOpacity);
        m_fontSize = config.value("fontSize", m_fontSize);
        m_inputHistoryLength = config.value("inputHistoryLength", m_inputHistoryLength);
        
        // Load hitbox colors
        if (config.contains("hitboxColors")) {
            for (const auto& [typeStr, colorData] : config["hitboxColors"].items()) {
                int typeInt = std::stoi(typeStr);
                HitboxType type = static_cast<HitboxType>(typeInt);
                
                m_hitboxColors[type] = Color(
                    colorData.value("r", 1.0f),
                    colorData.value("g", 1.0f),
                    colorData.value("b", 1.0f),
                    colorData.value("a", 1.0f)
                );
            }
        }
        
        // Load component positions
        if (config.contains("componentPositions")) {
            for (const auto& [name, posData] : config["componentPositions"].items()) {
                m_componentPositions[name] = {
                    posData.value("x", 0.0f),
                    posData.value("y", 0.0f)
                };
            }
        }
        
        return true;
    } catch (...) {
        return false;
    }
}

void TrainingOverlay::ToggleAllOverlays() {
    // Toggle all overlays on/off
    bool anyEnabled = m_hitboxEnabled || m_frameDataEnabled || 
                     m_inputHistoryEnabled || m_stateInfoEnabled;
    
    if (anyEnabled) {
        // Turn all off
        m_hitboxEnabled = false;
        m_frameDataEnabled = false;
        m_inputHistoryEnabled = false;
        m_stateInfoEnabled = false;
    } else {
        // Turn all on
        m_hitboxEnabled = true;
        m_frameDataEnabled = true;
        m_inputHistoryEnabled = true;
        m_stateInfoEnabled = true;
    }
}

void TrainingOverlay::AddHitbox(int id, const Hitbox& hitbox) {
    // Find existing hitbox with same id or add new one
    for (auto& hb : m_hitboxes) {
        if (hb.rect.x == id) { // Using x as ID field since there's no ID in the struct
            hb = hitbox;
            return;
        }
    }
    
    // Add new hitbox
    m_hitboxes.push_back(hitbox);
}

void TrainingOverlay::RemoveHitbox(int id) {
    // Remove hitbox with matching id
    m_hitboxes.erase(
        std::remove_if(m_hitboxes.begin(), m_hitboxes.end(),
            [id](const Hitbox& hb) { return hb.rect.x == id; }),
        m_hitboxes.end()
    );
}

void TrainingOverlay::ClearHitboxes() {
    m_hitboxes.clear();
}

void TrainingOverlay::UpdateFrameData(int playerIndex, const FrameData& frameData) {
    if (playerIndex < 0 || playerIndex >= 2) {
        return;
    }
    
    m_frameData[playerIndex] = frameData;
}

void TrainingOverlay::UpdateInputDisplay(int playerIndex, const InputDisplay& inputs) {
    if (playerIndex < 0 || playerIndex >= 2) {
        return;
    }
    
    // Convert InputDisplay to InputEvent
    uint32_t inputBits = 0;
    if (inputs.up) inputBits |= 0x01;
    if (inputs.down) inputBits |= 0x02;
    if (inputs.left) inputBits |= 0x04;
    if (inputs.right) inputBits |= 0x08;
    if (inputs.punch) inputBits |= 0x10;
    if (inputs.kick) inputBits |= 0x20;
    if (inputs.slash) inputBits |= 0x40;
    if (inputs.heavy) inputBits |= 0x80;
    if (inputs.special1) inputBits |= 0x100;
    if (inputs.special2) inputBits |= 0x200;
    if (inputs.start) inputBits |= 0x400;
    if (inputs.select) inputBits |= 0x800;
    
    addInputEvent(playerIndex, inputBits);
}

void TrainingOverlay::RegisterHotkeyCallback(const std::string& hotkeyName, HotkeyCallback callback) {
    // Store callback in the implementation
    if (pImpl) {
        // pImpl->hotkeyCallbacks[hotkeyName] = callback;
        // Note: We would need to implement this in the Impl class
    }
}

void TrainingOverlay::updateFrameCounter() {
    m_currentFrame++;
    m_currentTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

void TrainingOverlay::collectHitboxData() {
    // Implementation depends on game-specific memory mapping
    // This would read the hitbox data from game memory
    if (m_memoryMapping) {
        // Clear existing hitboxes
        m_hitboxes.clear();
        
        // Example: Read hitbox data based on memory mapping
        // Code would vary based on the game's memory layout
        
        // This is a simplified example and would need game-specific implementation
    }
}

void TrainingOverlay::collectFrameData() {
    // Implementation depends on game-specific memory mapping
    if (m_memoryMapping) {
        // Example: Read frame data from memory
        // Code would vary based on the game's memory layout
    }
}

void TrainingOverlay::collectStateData() {
    // Implementation depends on game-specific memory mapping
    if (m_memoryMapping) {
        // Example: Read game state data
        // Code would vary based on the game's memory layout
    }
}

void TrainingOverlay::calculateFrameAdvantage() {
    // Calculate frame advantage between players
    // This would involve timing the recovery of both players
    
    // Implementation depends on game-specific mechanics
}

void TrainingOverlay::ProcessHotkeys() {
    // Check for hotkey combinations
    // Implementation depends on input system and platform-specific code
}

// Implementation of HitboxDisplay class

HitboxDisplay::HitboxDisplay(MetalRenderer* renderer)
    : m_renderer(renderer)
{
}

HitboxDisplay::~HitboxDisplay() {
}

void HitboxDisplay::update(AIMemoryMapping* memoryMapping, std::vector<Hitbox>& hitboxes) {
    if (!memoryMapping) {
        return;
    }
    
    // Detect hitboxes from memory and add to hitboxes vector
    detectHitboxes(memoryMapping, hitboxes);
    
    // Transform hitbox coordinates to screen space
    for (auto& hitbox : hitboxes) {
        transformCoordinates(hitbox);
    }
}

void HitboxDisplay::render(float opacity, const std::unordered_map<HitboxType, Color>& colors) {
    if (!m_renderer) {
        return;
    }
    
    // Implementation would depend on the renderer's capabilities
    // This would use the MetalRenderer to draw hitboxes
    
    // Example (pseudocode):
    // for (const auto& hitbox : hitboxes) {
    //     Color color = colors.at(hitbox.type);
    //     color.a = opacity;
    //     m_renderer->drawRect(hitbox.rect.x, hitbox.rect.y, 
    //                         hitbox.rect.width, hitbox.rect.height, 
    //                         color.r, color.g, color.b, color.a);
    // }
}

void HitboxDisplay::detectHitboxes(AIMemoryMapping* memoryMapping, std::vector<Hitbox>& hitboxes) {
    // Game-specific implementation to detect hitboxes from memory
    // This would read memory locations that contain hitbox data
    
    // Implementation depends heavily on the game
}

void HitboxDisplay::transformCoordinates(Hitbox& hitbox) {
    // Transform hitbox coordinates from game space to screen space
    // This would depend on the game's coordinate system and screen resolution
}

// Implementation of other display classes would follow a similar pattern
// FrameDataDisplay, InputHistoryDisplay, StateInfoDisplay
// Each would have specialized logic for their respective functionality 