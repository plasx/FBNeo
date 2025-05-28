#include "game_state_display.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <unordered_set>

// Forward declarations for any Metal-specific types needed
class MetalContext;

/**
 * @class GameStateDisplayPrivate
 * @brief Private implementation of GameStateDisplay
 */
class GameStateDisplayPrivate {
public:
    GameStateDisplayPrivate(AIMemoryMapping* memoryMapping)
        : m_memoryMapping(memoryMapping)
        , m_metalContext(nullptr)
        , m_initialized(false)
        , m_lastUpdateTime(0.0f)
        , m_stateHistoryMaxSize(60)
        , m_stateTransitionThreshold(0.1f)
    {
        // Initialize state history for transitions
        m_stateHistory.reserve(m_stateHistoryMaxSize);
    }
    
    ~GameStateDisplayPrivate() {
        // Clean up any resources
    }
    
    bool initialize(MetalContext* metalContext) {
        m_metalContext = metalContext;
        
        // Initialize any rendering resources needed
        
        m_initialized = true;
        return m_initialized;
    }
    
    void update(float deltaTime) {
        if (!m_initialized || !m_memoryMapping) {
            return;
        }
        
        m_lastUpdateTime += deltaTime;
        
        // Only update a few times per second to avoid overwhelming the system
        if (m_lastUpdateTime >= 0.1f) {
            m_lastUpdateTime = 0.0f;
            
            // Get changed mappings
            auto changedMappings = m_memoryMapping->getChangedMappings();
            
            // Update state history for transitions
            if (!changedMappings.empty()) {
                // Store current state snapshot
                StateSnapshot snapshot;
                snapshot.timestamp = std::time(nullptr);
                
                for (const auto& mapping : changedMappings) {
                    snapshot.states[mapping.first] = mapping.second;
                }
                
                // Check for significant state transitions
                detectStateTransitions(snapshot);
                
                // Add to history, removing oldest if at capacity
                m_stateHistory.push_back(std::move(snapshot));
                if (m_stateHistory.size() > m_stateHistoryMaxSize) {
                    m_stateHistory.erase(m_stateHistory.begin());
                }
            }
        }
    }
    
    void render(int width, int height, float opacity) {
        if (!m_initialized || !m_memoryMapping) {
            return;
        }
        
        // Prepare mappings to display
        std::vector<std::pair<std::string, std::string>> displayMappings;
        
        // Get all mappings or just the key ones based on settings
        std::vector<std::string> mappingNames;
        if (m_showAllStates) {
            mappingNames = m_memoryMapping->getAllMappingNames();
        } else {
            // Just get key states (this could be configurable)
            static const std::unordered_set<std::string> keyStates = {
                "p1_health", "p2_health", "p1_meter", "p2_meter",
                "timer", "round_state", "match_state", "game_state"
            };
            
            auto allNames = m_memoryMapping->getAllMappingNames();
            for (const auto& name : allNames) {
                if (keyStates.find(name) != keyStates.end() || 
                    name.find("state") != std::string::npos ||
                    name.find("_active") != std::string::npos) {
                    mappingNames.push_back(name);
                }
            }
        }
        
        // Process mappings
        for (const auto& name : mappingNames) {
            std::string value = m_memoryMapping->getStringValue(name);
            std::string label = getDisplayLabel(name);
            
            // Format special values
            if (name.find("health") != std::string::npos || 
                name.find("meter") != std::string::npos) {
                try {
                    int intValue = std::stoi(value);
                    std::stringstream ss;
                    ss << intValue;
                    value = ss.str();
                } catch (...) {
                    // Use the original value if conversion fails
                }
            }
            
            displayMappings.push_back({label, value});
        }
        
        // Sort mappings by name if not grouped
        if (!m_groupedDisplay) {
            std::sort(displayMappings.begin(), displayMappings.end());
        } else {
            // Group by categories
            // This would need to be implemented based on mapping categories
            // For now, we'll just group by common prefixes
            auto groupCompare = [](const std::pair<std::string, std::string>& a, 
                                 const std::pair<std::string, std::string>& b) {
                // Extract prefix (e.g., "p1_", "p2_", "game_")
                auto getPrefix = [](const std::string& s) {
                    size_t pos = s.find('_');
                    if (pos != std::string::npos) {
                        return s.substr(0, pos + 1);
                    }
                    return s;
                };
                
                std::string prefixA = getPrefix(a.first);
                std::string prefixB = getPrefix(b.first);
                
                if (prefixA == prefixB) {
                    return a.first < b.first;
                }
                return prefixA < prefixB;
            };
            
            std::sort(displayMappings.begin(), displayMappings.end(), groupCompare);
        }
        
        // Draw state information
        renderStateInfo(displayMappings, width, height, opacity);
        
        // Draw transitions if enabled
        if (m_showStateTransitions && !m_recentTransitions.empty()) {
            renderStateTransitions(width, height, opacity);
        }
        
        // Draw AI decisions if enabled
        if (m_showAIDecisions) {
            renderAIDecisions(width, height, opacity);
        }
    }
    
    std::string getDisplayLabel(const std::string& mappingName) {
        // Check for custom label first
        auto it = m_customLabels.find(mappingName);
        if (it != m_customLabels.end()) {
            return it->second;
        }
        
        // Format the default label
        std::string label = mappingName;
        
        // Replace underscores with spaces
        std::replace(label.begin(), label.end(), '_', ' ');
        
        // Capitalize words
        bool cap = true;
        for (auto& c : label) {
            if (cap && std::isalpha(c)) {
                c = std::toupper(c);
                cap = false;
            } else if (c == ' ') {
                cap = true;
            }
        }
        
        return label;
    }
    
    void detectStateTransitions(const StateSnapshot& snapshot) {
        if (m_stateHistory.empty()) {
            return;
        }
        
        // Compare with the previous state
        const auto& prevSnapshot = m_stateHistory.back();
        
        for (const auto& [name, value] : snapshot.states) {
            auto prevIt = prevSnapshot.states.find(name);
            if (prevIt != prevSnapshot.states.end() && prevIt->second != value) {
                // State transition detected
                StateTransition transition;
                transition.mappingName = name;
                transition.fromValue = prevIt->second;
                transition.toValue = value;
                transition.timestamp = snapshot.timestamp;
                
                // Add to recent transitions
                m_recentTransitions.push_back(transition);
                
                // Limit the number of recent transitions
                if (m_recentTransitions.size() > 5) {
                    m_recentTransitions.erase(m_recentTransitions.begin());
                }
            }
        }
    }
    
    void renderStateInfo(const std::vector<std::pair<std::string, std::string>>& mappings, 
                        int width, int height, float opacity) {
        // Actual implementation using Metal to render the state information
        if (!m_metalContext || mappings.empty()) {
            return;
        }
        
        // Get Metal renderer functions
        extern void MetalRenderer_BeginOverlay();
        extern void MetalRenderer_EndOverlay();
        extern void MetalRenderer_DrawRect(int x, int y, int width, int height, float r, float g, float b, float a);
        extern void MetalRenderer_DrawText(int x, int y, const char* text, float r, float g, float b, float a, float scale);
        
        // Start overlay rendering
        MetalRenderer_BeginOverlay();
        
        // Panel settings
        float x = m_posX;
        float y = m_posY;
        float lineHeight = 20.0f * m_scale;
        float panelWidth = 300.0f * m_scale;
        float panelHeight = lineHeight * mappings.size() + 40.0f;
        float textScale = m_scale * 0.9f;
        
        // Draw background panel
        MetalRenderer_DrawRect(x, y, panelWidth, panelHeight, 0.0f, 0.0f, 0.0f, 0.7f * opacity);
        
        // Draw title
        MetalRenderer_DrawText(x + 10.0f, y + 10.0f, "GAME STATE", 1.0f, 1.0f, 0.0f, opacity, textScale * 1.2f);
        
        // Draw separator line
        MetalRenderer_DrawRect(x + 5.0f, y + 30.0f, panelWidth - 10.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.8f * opacity);
        
        // Draw each mapping
        y += 40.0f;
        
        // Track current group for grouped display
        std::string currentGroup = "";
        
        for (const auto& [label, value] : mappings) {
            // For grouped display, check if we need a group header
            if (m_groupedDisplay) {
                // Extract group prefix
                size_t pos = label.find(' ');
                std::string group = (pos != std::string::npos) ? label.substr(0, pos) : label;
                
                // If this is a new group, add a header
                if (group != currentGroup) {
                    // Add extra space between groups (except for first)
                    if (!currentGroup.empty()) {
                        y += lineHeight * 0.5f;
                    }
                    
                    currentGroup = group;
                    
                    // Draw group header with special color
                    MetalRenderer_DrawText(x + 10.0f, y, group.c_str(), 0.0f, 1.0f, 1.0f, opacity, textScale);
                    y += lineHeight;
                    
                    // Draw group separator line
                    MetalRenderer_DrawRect(x + 15.0f, y - 5.0f, panelWidth - 30.0f, 1.0f, 0.0f, 0.7f, 0.7f, 0.5f * opacity);
                }
            }
            
            // Determine text color based on value change
            float r = 1.0f, g = 1.0f, b = 1.0f;
            bool valueChanged = false;
            
            // Check state history to see if value changed recently
            if (!m_stateHistory.empty()) {
                const auto& prevSnapshot = m_stateHistory.back();
                auto labelWithoutSpaces = label;
                std::replace(labelWithoutSpaces.begin(), labelWithoutSpaces.end(), ' ', '_');
                std::transform(labelWithoutSpaces.begin(), labelWithoutSpaces.end(), labelWithoutSpaces.begin(), ::tolower);
                
                auto it = prevSnapshot.states.find(labelWithoutSpaces);
                if (it != prevSnapshot.states.end() && it->second != value) {
                    // Value changed - highlight it
                    r = 1.0f; g = 0.8f; b = 0.2f;
                    valueChanged = true;
                }
            }
            
            // Apply special colors for certain types of values
            if (!valueChanged) {
                if (label.find("Health") != std::string::npos) {
                    // Health values in green
                    r = 0.2f; g = 1.0f; b = 0.2f;
                } else if (label.find("Meter") != std::string::npos || label.find("Super") != std::string::npos) {
                    // Meter values in blue
                    r = 0.2f; g = 0.7f; b = 1.0f;
                } else if (label.find("State") != std::string::npos || label.find("Mode") != std::string::npos) {
                    // State/mode values in purple
                    r = 0.8f; g = 0.4f; b = 1.0f;
                }
            }
            
            // Draw label
            MetalRenderer_DrawText(x + 20.0f, y, label.c_str(), 1.0f, 1.0f, 1.0f, opacity, textScale);
            
            // Draw value (with appropriate color)
            MetalRenderer_DrawText(x + 170.0f, y, value.c_str(), r, g, b, opacity, textScale);
            
            // Move to next line
            y += lineHeight;
        }
        
        // End overlay rendering
        MetalRenderer_EndOverlay();
    }
    
    void renderStateTransitions(int width, int height, float opacity) {
        // Actual implementation using Metal to render state transitions
        if (!m_metalContext || m_recentTransitions.empty()) {
            return;
        }
        
        // Get Metal renderer functions
        extern void MetalRenderer_BeginOverlay();
        extern void MetalRenderer_EndOverlay();
        extern void MetalRenderer_DrawRect(int x, int y, int width, int height, float r, float g, float b, float a);
        extern void MetalRenderer_DrawText(int x, int y, const char* text, float r, float g, float b, float a, float scale);
        extern void MetalRenderer_DrawArrow(int x1, int y1, int x2, int y2, float r, float g, float b, float a, float thickness);
        
        // Start overlay rendering
        MetalRenderer_BeginOverlay();
        
        // Panel settings
        float x = m_posX;
        float y = m_posY + 350.0f;
        float lineHeight = 25.0f * m_scale;
        float panelWidth = 400.0f * m_scale;
        float panelHeight = lineHeight * m_recentTransitions.size() + 40.0f;
        float textScale = m_scale * 0.9f;
        
        // Draw background panel
        MetalRenderer_DrawRect(x, y, panelWidth, panelHeight, 0.0f, 0.0f, 0.0f, 0.7f * opacity);
        
        // Draw title
        MetalRenderer_DrawText(x + 10.0f, y + 10.0f, "RECENT STATE TRANSITIONS", 1.0f, 0.7f, 0.7f, opacity, textScale * 1.2f);
        
        // Draw separator line
        MetalRenderer_DrawRect(x + 5.0f, y + 30.0f, panelWidth - 10.0f, 1.0f, 1.0f, 0.7f, 0.7f, 0.8f * opacity);
        
        // Draw each transition
        y += 40.0f;
        
        // Get current time for age display
        std::time_t now = std::time(nullptr);
        
        for (const auto& transition : m_recentTransitions) {
            // Format the transition text
            std::string mappingLabel = getDisplayLabel(transition.mappingName);
            
            // Calculate age of transition for color fading
            int ageInSeconds = static_cast<int>(difftime(now, transition.timestamp));
            float ageFactor = std::max(0.0f, 1.0f - (ageInSeconds / 10.0f)); // Fade over 10 seconds
            
            // Transition text with arrow
            char transitionText[256];
            snprintf(transitionText, sizeof(transitionText), "%s:", mappingLabel.c_str());
            
            // Draw mapping name
            MetalRenderer_DrawText(x + 10.0f, y, transitionText, 1.0f, 1.0f, 1.0f, opacity * ageFactor, textScale);
            
            // Draw from value
            float fromX = x + 150.0f;
            MetalRenderer_DrawText(fromX, y, transition.fromValue.c_str(), 0.7f, 0.7f, 1.0f, opacity * ageFactor, textScale);
            
            // Draw arrow
            float arrowX1 = fromX + 80.0f;
            float arrowX2 = arrowX1 + 40.0f;
            MetalRenderer_DrawArrow(arrowX1, y + lineHeight/2, arrowX2, y + lineHeight/2, 
                                  1.0f, 0.7f, 0.7f, opacity * ageFactor, 2.0f * m_scale);
            
            // Draw to value
            MetalRenderer_DrawText(arrowX2 + 10.0f, y, transition.toValue.c_str(), 1.0f, 0.7f, 0.7f, opacity * ageFactor, textScale);
            
            // Move to next line
            y += lineHeight;
        }
        
        // End overlay rendering
        MetalRenderer_EndOverlay();
    }
    
    void renderAIDecisions(int width, int height, float opacity) {
        // Actual implementation using Metal to render AI decision information
        if (!m_metalContext) {
            return;
        }
        
        // Get Metal renderer functions
        extern void MetalRenderer_BeginOverlay();
        extern void MetalRenderer_EndOverlay();
        extern void MetalRenderer_DrawRect(int x, int y, int width, int height, float r, float g, float b, float a);
        extern void MetalRenderer_DrawText(int x, int y, const char* text, float r, float g, float b, float a, float scale);
        extern void MetalRenderer_DrawProgressBar(int x, int y, int width, int height, float value, 
                                               float r1, float g1, float b1, float r2, float g2, float b2, float a);
        
        // Get AI information from the global AI controller
        extern const char* AI_GetCurrentAction();
        extern float AI_GetCurrentActionConfidence();
        extern float AI_GetStateValue();
        extern int AI_GetTopActionCount();
        extern void AI_GetTopActionInfo(int index, char* actionName, float* confidence);
        
        // Start overlay rendering
        MetalRenderer_BeginOverlay();
        
        // Panel settings
        float x = width - 320.0f * m_scale;
        float y = m_posY;
        float lineHeight = 22.0f * m_scale;
        float panelWidth = 300.0f * m_scale;
        float panelHeight = 250.0f * m_scale;
        float textScale = m_scale * 0.9f;
        
        // Draw background panel
        MetalRenderer_DrawRect(x, y, panelWidth, panelHeight, 0.0f, 0.0f, 0.0f, 0.7f * opacity);
        
        // Draw title
        MetalRenderer_DrawText(x + 10.0f, y + 10.0f, "AI DECISIONS", 0.4f, 1.0f, 0.8f, opacity, textScale * 1.2f);
        
        // Draw separator line
        MetalRenderer_DrawRect(x + 5.0f, y + 30.0f, panelWidth - 10.0f, 1.0f, 0.4f, 1.0f, 0.8f, 0.8f * opacity);
        
        // Draw AI information
        y += 40.0f;
        
        // Current action
        const char* currentAction = AI_GetCurrentAction();
        if (!currentAction) currentAction = "None";
        MetalRenderer_DrawText(x + 10.0f, y, "Current Action:", 1.0f, 1.0f, 1.0f, opacity, textScale);
        MetalRenderer_DrawText(x + 130.0f, y, currentAction, 1.0f, 1.0f, 0.0f, opacity, textScale);
        
        // Action confidence
        y += lineHeight;
        float confidence = AI_GetCurrentActionConfidence();
        char confidenceText[32];
        snprintf(confidenceText, sizeof(confidenceText), "Confidence: %.2f", confidence);
        MetalRenderer_DrawText(x + 10.0f, y, confidenceText, 1.0f, 1.0f, 1.0f, opacity, textScale);
        
        // Draw confidence bar
        float barWidth = 120.0f * m_scale;
        float barHeight = 10.0f * m_scale;
        MetalRenderer_DrawProgressBar(x + 150.0f, y + 5.0f, barWidth, barHeight, confidence,
                                   0.2f, 0.8f, 0.2f, 0.8f, 0.2f, 0.2f, opacity);
        
        // State value
        y += lineHeight;
        float stateValue = AI_GetStateValue();
        char stateValueText[32];
        snprintf(stateValueText, sizeof(stateValueText), "State Value: %.2f", stateValue);
        MetalRenderer_DrawText(x + 10.0f, y, stateValueText, 1.0f, 1.0f, 1.0f, opacity, textScale);
        
        // Top actions
        y += lineHeight + 5.0f;
        MetalRenderer_DrawText(x + 10.0f, y, "Top Actions:", 1.0f, 1.0f, 1.0f, opacity, textScale);
        y += lineHeight;
        
        // Draw top actions with confidence bars
        int actionCount = AI_GetTopActionCount();
        for (int i = 0; i < actionCount && i < 5; i++) { // Show up to 5 top actions
            char actionName[64];
            float actionConfidence;
            AI_GetTopActionInfo(i, actionName, &actionConfidence);
            
            // Draw action name
            MetalRenderer_DrawText(x + 20.0f, y, actionName, 0.9f, 0.9f, 0.9f, opacity, textScale * 0.9f);
            
            // Draw confidence value
            char confText[16];
            snprintf(confText, sizeof(confText), "%.2f", actionConfidence);
            MetalRenderer_DrawText(x + 140.0f, y, confText, 0.9f, 0.9f, 0.9f, opacity, textScale * 0.9f);
            
            // Draw confidence bar
            float barWidth = 100.0f * m_scale;
            float barHeight = 8.0f * m_scale;
            
            // Color gradient based on position (best action is green, others fade to red)
            float r1 = i == 0 ? 0.2f : 0.8f;
            float g1 = i == 0 ? 0.8f : 0.2f + (0.6f / actionCount) * (actionCount - i);
            float b1 = 0.2f;
            
            MetalRenderer_DrawProgressBar(x + 180.0f, y + 5.0f, barWidth, barHeight, actionConfidence,
                                       r1, g1, b1, r1 * 0.5f, g1 * 0.5f, b1 * 0.5f, opacity);
            
            y += lineHeight;
        }
        
        // End overlay rendering
        MetalRenderer_EndOverlay();
    }
    
    // Data structures for state history and transitions
    struct StateSnapshot {
        std::time_t timestamp;
        std::unordered_map<std::string, std::string> states;
    };
    
    struct StateTransition {
        std::string mappingName;
        std::string fromValue;
        std::string toValue;
        std::time_t timestamp;
    };
    
    // Member variables
    AIMemoryMapping* m_memoryMapping;
    MetalContext* m_metalContext;
    bool m_initialized;
    float m_lastUpdateTime;
    
    // State tracking
    std::vector<StateSnapshot> m_stateHistory;
    std::vector<StateTransition> m_recentTransitions;
    size_t m_stateHistoryMaxSize;
    float m_stateTransitionThreshold;
    
    // Settings
    bool m_showAllStates;
    bool m_showAIDecisions;
    bool m_showStateTransitions;
    bool m_groupedDisplay;
    float m_posX;
    float m_posY;
    float m_scale;
    std::unordered_map<std::string, std::string> m_customLabels;
};

// GameStateDisplay implementation

GameStateDisplay::GameStateDisplay(AIMemoryMapping* memoryMapping)
    : m_memoryMapping(memoryMapping)
    , m_enabled(true)
    , m_posX(20.0f)
    , m_posY(20.0f)
    , m_scale(1.0f)
    , m_showAllStates(false)
    , m_showAIDecisions(true)
    , m_showStateTransitions(true)
    , m_groupedDisplay(true)
{
    m_private = std::make_unique<GameStateDisplayPrivate>(memoryMapping);
}

GameStateDisplay::~GameStateDisplay() {
    // Private impl will be automatically deleted
}

bool GameStateDisplay::initialize(MetalContext* metalContext) {
    if (!m_private) {
        return false;
    }
    
    m_private->m_showAllStates = m_showAllStates;
    m_private->m_showAIDecisions = m_showAIDecisions;
    m_private->m_showStateTransitions = m_showStateTransitions;
    m_private->m_groupedDisplay = m_groupedDisplay;
    m_private->m_posX = m_posX;
    m_private->m_posY = m_posY;
    m_private->m_scale = m_scale;
    m_private->m_customLabels = m_customLabels;
    
    return m_private->initialize(metalContext);
}

void GameStateDisplay::update(float deltaTime) {
    if (!m_enabled || !m_private) {
        return;
    }
    
    m_private->update(deltaTime);
}

void GameStateDisplay::render(int width, int height, float opacity) {
    if (!m_enabled || !m_private) {
        return;
    }
    
    m_private->render(width, height, opacity);
}

void GameStateDisplay::setEnabled(bool enabled) {
    m_enabled = enabled;
}

bool GameStateDisplay::isEnabled() const {
    return m_enabled;
}

void GameStateDisplay::setPosition(float x, float y) {
    m_posX = x;
    m_posY = y;
    
    if (m_private) {
        m_private->m_posX = x;
        m_private->m_posY = y;
    }
}

void GameStateDisplay::getPosition(float& x, float& y) const {
    x = m_posX;
    y = m_posY;
}

void GameStateDisplay::setScale(float scale) {
    m_scale = scale;
    
    if (m_private) {
        m_private->m_scale = scale;
    }
}

float GameStateDisplay::getScale() const {
    return m_scale;
}

void GameStateDisplay::setShowAllStates(bool showAll) {
    m_showAllStates = showAll;
    
    if (m_private) {
        m_private->m_showAllStates = showAll;
    }
}

bool GameStateDisplay::isShowingAllStates() const {
    return m_showAllStates;
}

void GameStateDisplay::setShowAIDecisions(bool showDecisions) {
    m_showAIDecisions = showDecisions;
    
    if (m_private) {
        m_private->m_showAIDecisions = showDecisions;
    }
}

bool GameStateDisplay::isShowingAIDecisions() const {
    return m_showAIDecisions;
}

void GameStateDisplay::setShowStateTransitions(bool showTransitions) {
    m_showStateTransitions = showTransitions;
    
    if (m_private) {
        m_private->m_showStateTransitions = showTransitions;
    }
}

bool GameStateDisplay::isShowingStateTransitions() const {
    return m_showStateTransitions;
}

void GameStateDisplay::setCustomLabel(const std::string& mappingName, const std::string& label) {
    m_customLabels[mappingName] = label;
    
    if (m_private) {
        m_private->m_customLabels[mappingName] = label;
    }
}

std::string GameStateDisplay::getCustomLabel(const std::string& mappingName) const {
    auto it = m_customLabels.find(mappingName);
    if (it != m_customLabels.end()) {
        return it->second;
    }
    return "";
}

void GameStateDisplay::clearCustomLabels() {
    m_customLabels.clear();
    
    if (m_private) {
        m_private->m_customLabels.clear();
    }
}

void GameStateDisplay::setGroupedDisplay(bool grouped) {
    m_groupedDisplay = grouped;
    
    if (m_private) {
        m_private->m_groupedDisplay = grouped;
    }
}

bool GameStateDisplay::isGroupedDisplay() const {
    return m_groupedDisplay;
}

void GameStateDisplay::loadSettings() {
    // Load settings from a configuration file
    std::string configPath = "config/game_state_display.cfg";
    
    // Expand path if it starts with ~
    if (configPath.substr(0, 1) == "~") {
        char* home = getenv("HOME");
        if (home) {
            configPath.replace(0, 1, home);
        }
    }
    
    std::ifstream file(configPath);
    
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') {
                continue;
            }
            
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                
                // Trim whitespace
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
                
                // Parse settings
                if (key == "enabled") {
                    m_enabled = (value == "true" || value == "1");
                } else if (key == "posX") {
                    try { m_posX = std::stof(value); } catch (...) {}
                } else if (key == "posY") {
                    try { m_posY = std::stof(value); } catch (...) {}
                } else if (key == "scale") {
                    try { m_scale = std::stof(value); } catch (...) {}
                } else if (key == "showAllStates") {
                    m_showAllStates = (value == "true" || value == "1");
                } else if (key == "showAIDecisions") {
                    m_showAIDecisions = (value == "true" || value == "1");
                } else if (key == "showStateTransitions") {
                    m_showStateTransitions = (value == "true" || value == "1");
                } else if (key == "groupedDisplay") {
                    m_groupedDisplay = (value == "true" || value == "1");
                } else if (key.substr(0, 12) == "customLabel_") {
                    std::string mappingName = key.substr(12);
                    m_customLabels[mappingName] = value;
                }
            }
        }
        
        file.close();
        
        // Update private implementation
        if (m_private) {
            m_private->m_showAllStates = m_showAllStates;
            m_private->m_showAIDecisions = m_showAIDecisions;
            m_private->m_showStateTransitions = m_showStateTransitions;
            m_private->m_groupedDisplay = m_groupedDisplay;
            m_private->m_posX = m_posX;
            m_private->m_posY = m_posY;
            m_private->m_scale = m_scale;
            m_private->m_customLabels = m_customLabels;
        }
        
        std::cout << "Loaded game state display settings from " << configPath << std::endl;
    } else {
        // If config file doesn't exist, create it with default settings
        saveSettings();
    }
}

void GameStateDisplay::saveSettings() {
    // Save settings to a configuration file
    std::string configDir = "config";
    std::string configPath = configDir + "/game_state_display.cfg";
    
    // Expand path if it starts with ~
    if (configPath.substr(0, 1) == "~") {
        char* home = getenv("HOME");
        if (home) {
            configPath.replace(0, 1, home);
            configDir.replace(0, 1, home);
        }
    }
    
    // Create directory if it doesn't exist
    if (access(configDir.c_str(), F_OK) != 0) {
        #ifdef _WIN32
        mkdir(configDir.c_str());
        #else
        mkdir(configDir.c_str(), 0755);
        #endif
    }
    
    std::ofstream file(configPath);
    
    if (file.is_open()) {
        // Write header
        file << "# Game State Display Configuration\n";
        file << "# Generated on " << getCurrentTimeString() << "\n\n";
        
        // Write settings
        file << "enabled=" << (m_enabled ? "true" : "false") << std::endl;
        file << "posX=" << m_posX << std::endl;
        file << "posY=" << m_posY << std::endl;
        file << "scale=" << m_scale << std::endl;
        file << "showAllStates=" << (m_showAllStates ? "true" : "false") << std::endl;
        file << "showAIDecisions=" << (m_showAIDecisions ? "true" : "false") << std::endl;
        file << "showStateTransitions=" << (m_showStateTransitions ? "true" : "false") << std::endl;
        file << "groupedDisplay=" << (m_groupedDisplay ? "true" : "false") << std::endl;
        
        // Write custom labels
        file << "\n# Custom Labels\n";
        for (const auto& [mappingName, label] : m_customLabels) {
            file << "customLabel_" << mappingName << "=" << label << std::endl;
        }
        
        file.close();
        std::cout << "Saved game state display settings to " << configPath << std::endl;
    } else {
        std::cerr << "Error: Could not save game state display settings to " << configPath << std::endl;
    }
}

// Helper function to get current time as a string
std::string GameStateDisplay::getCurrentTimeString() const {
    std::time_t now = std::time(nullptr);
    char buffer[64];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    return std::string(buffer);
} 