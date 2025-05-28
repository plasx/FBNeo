#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "../metal_intf.h"
#include "../../ai/ai_memory_mapping.h"

class GameStateDisplayPrivate;

/**
 * @class GameStateDisplay
 * @brief Visualizes game state information from memory mappings
 * 
 * This class provides visualization for game state information based on
 * memory mappings. It displays information such as player states, round time,
 * match state, and other game-specific data.
 */
class GameStateDisplay {
public:
    /**
     * @brief Constructor
     * @param memoryMapping Pointer to the AIMemoryMapping instance
     */
    GameStateDisplay(AIMemoryMapping* memoryMapping);
    
    /**
     * @brief Destructor
     */
    ~GameStateDisplay();
    
    /**
     * @brief Initialize the game state display
     * @param metalContext The Metal rendering context
     * @return True if initialization was successful, false otherwise
     */
    bool initialize(MetalContext* metalContext);
    
    /**
     * @brief Update the game state display
     * @param deltaTime Time elapsed since the last update
     */
    void update(float deltaTime);
    
    /**
     * @brief Render the game state display
     * @param width Screen width
     * @param height Screen height
     * @param opacity Opacity of the display (0.0f - 1.0f)
     */
    void render(int width, int height, float opacity = 1.0f);
    
    /**
     * @brief Set whether the game state display is enabled
     * @param enabled Whether the display is enabled
     */
    void setEnabled(bool enabled);
    
    /**
     * @brief Check if the game state display is enabled
     * @return True if enabled, false otherwise
     */
    bool isEnabled() const;
    
    /**
     * @brief Set the position of the display
     * @param x X coordinate
     * @param y Y coordinate
     */
    void setPosition(float x, float y);
    
    /**
     * @brief Get the position of the display
     * @param x Output parameter for X coordinate
     * @param y Output parameter for Y coordinate
     */
    void getPosition(float& x, float& y) const;
    
    /**
     * @brief Set the scale of the display
     * @param scale Scale factor
     */
    void setScale(float scale);
    
    /**
     * @brief Get the scale of the display
     * @return The scale factor
     */
    float getScale() const;
    
    /**
     * @brief Set whether to show all state information or just key states
     * @param showAll True to show all information, false for key states only
     */
    void setShowAllStates(bool showAll);
    
    /**
     * @brief Check if all state information is being shown
     * @return True if all states are shown, false otherwise
     */
    bool isShowingAllStates() const;
    
    /**
     * @brief Set whether to show AI decision information
     * @param showDecisions True to show AI decisions, false otherwise
     */
    void setShowAIDecisions(bool showDecisions);
    
    /**
     * @brief Check if AI decision information is being shown
     * @return True if AI decisions are shown, false otherwise
     */
    bool isShowingAIDecisions() const;
    
    /**
     * @brief Set whether to show state transitions
     * @param showTransitions True to show transitions, false otherwise
     */
    void setShowStateTransitions(bool showTransitions);
    
    /**
     * @brief Check if state transitions are being shown
     * @return True if transitions are shown, false otherwise
     */
    bool isShowingStateTransitions() const;
    
    /**
     * @brief Set a custom label for a memory mapping
     * @param mappingName Name of the mapping
     * @param label Custom label to display
     */
    void setCustomLabel(const std::string& mappingName, const std::string& label);
    
    /**
     * @brief Get a custom label for a memory mapping
     * @param mappingName Name of the mapping
     * @return The custom label, or empty string if not set
     */
    std::string getCustomLabel(const std::string& mappingName) const;
    
    /**
     * @brief Clear all custom labels
     */
    void clearCustomLabels();
    
    /**
     * @brief Set the grouped display mode
     * @param grouped True to group by categories, false for flat list
     */
    void setGroupedDisplay(bool grouped);
    
    /**
     * @brief Check if grouped display mode is enabled
     * @return True if grouped, false otherwise
     */
    bool isGroupedDisplay() const;
    
    /**
     * @brief Load settings from persistent storage
     */
    void loadSettings();
    
    /**
     * @brief Save settings to persistent storage
     */
    void saveSettings();

private:
    std::unique_ptr<GameStateDisplayPrivate> m_private;
    AIMemoryMapping* m_memoryMapping;
    bool m_enabled;
    float m_posX;
    float m_posY;
    float m_scale;
    bool m_showAllStates;
    bool m_showAIDecisions;
    bool m_showStateTransitions;
    bool m_groupedDisplay;
    std::unordered_map<std::string, std::string> m_customLabels;
    
    /**
     * @brief Get the current time as a formatted string
     * @return Formatted time string
     */
    std::string getCurrentTimeString() const;
}; 