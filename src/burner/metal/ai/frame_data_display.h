#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "../../ai/ai_memory_mapping.h"

class OverlayRenderer;
class FrameDataDisplayPrivate;

/**
 * @class FrameDataDisplay
 * @brief Visualizes frame data information for fighting games
 * 
 * This class provides visualization for frame data information such as
 * startup frames, active frames, recovery frames, and frame advantage.
 */
class FrameDataDisplay {
public:
    /**
     * @brief Constructor
     */
    FrameDataDisplay();
    
    /**
     * @brief Destructor
     */
    ~FrameDataDisplay();
    
    /**
     * @brief Initialize the frame data display
     * @param renderer Pointer to the overlay renderer
     * @param memoryMapping Pointer to the memory mapping
     * @return True if initialization was successful, false otherwise
     */
    bool initialize(OverlayRenderer* renderer, AIMemoryMapping* memoryMapping);
    
    /**
     * @brief Update the frame data display
     * @param deltaTime Time elapsed since the last update
     */
    void update(float deltaTime);
    
    /**
     * @brief Render the frame data display
     * @param width Screen width
     * @param height Screen height
     * @param opacity Opacity of the display (0.0f - 1.0f)
     */
    void render(int width, int height, float opacity = 1.0f);
    
    /**
     * @brief Set whether the frame data display is enabled
     * @param enabled Whether the display is enabled
     */
    void setEnabled(bool enabled);
    
    /**
     * @brief Check if the frame data display is enabled
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
     * @brief Set the opacity of the display
     * @param opacity Opacity value (0.0f - 1.0f)
     */
    void setOpacity(float opacity);
    
    /**
     * @brief Get the opacity of the display
     * @return The opacity value
     */
    float getOpacity() const;
    
    /**
     * @brief Set whether to show detailed frame data
     * @param detailed Whether to show detailed information
     */
    void setDetailedView(bool detailed);
    
    /**
     * @brief Check if detailed view is enabled
     * @return True if detailed view is enabled
     */
    bool isDetailedView() const;
    
    /**
     * @brief Save settings to a file
     * @param filename The file to save to
     * @return True if settings were saved successfully
     */
    bool saveSettings(const std::string& filename = "frame_data_display.json");
    
    /**
     * @brief Load settings from a file
     * @param filename The file to load from
     * @return True if settings were loaded successfully
     */
    bool loadSettings(const std::string& filename = "frame_data_display.json");

private:
    OverlayRenderer* m_renderer;
    AIMemoryMapping* m_memoryMapping;
    bool m_enabled;
    bool m_detailedView;
    float m_posX;
    float m_posY;
    float m_opacity;
    
    struct FrameData {
        int startup;
        int active;
        int recovery;
        int advantage;
        bool isAttacking;
        int currentFrame;
        std::string moveName;
    };
    
    FrameData m_p1FrameData;
    FrameData m_p2FrameData;
    
    /**
     * @brief Get frame data for a player
     * @param playerIndex Player index (1 or 2)
     * @return The frame data for the player
     */
    FrameData getFrameData(int playerIndex);
    
    /**
     * @brief Calculate frame advantage
     * @param attacker The attacker's frame data
     * @param defender The defender's frame data
     * @return The frame advantage
     */
    int calculateFrameAdvantage(const FrameData& attacker, const FrameData& defender);
    
    /**
     * @brief Draw frame data for a player
     * @param x X coordinate
     * @param y Y coordinate
     * @param playerIndex Player index (1 or 2)
     * @param data The frame data to draw
     * @param opacity Opacity value
     */
    void drawFrameData(float x, float y, int playerIndex, const FrameData& data, float opacity);
}; 