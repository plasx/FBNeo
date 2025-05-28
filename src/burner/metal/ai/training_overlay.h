#pragma once

#include <memory>
#include <string>
#include "../../ai/ai_memory_mapping.h"

// Forward declarations
class HitboxVisualizer;
class InputDisplay;
class OverlayRenderer;
class FrameDataDisplay;
class GameStateDisplay;
class MetalContext;

/**
 * Class that manages the training mode overlay for fighting games
 * 
 * This class coordinates the various components of the training overlay,
 * including hitbox visualization, input display, frame data, and game state display.
 */
class TrainingOverlay {
public:
    /**
     * Constructor
     */
    TrainingOverlay();
    
    /**
     * Destructor
     */
    ~TrainingOverlay();
    
    /**
     * Initialize the training overlay
     * @param memoryMapping Pointer to the memory mapping
     * @param metalContext Pointer to the Metal context
     * @return True if initialization was successful
     */
    bool initialize(AIMemoryMapping* memoryMapping, MetalContext* metalContext);
    
    /**
     * Update the training overlay
     * This should be called once per frame before rendering
     * @param deltaTime Time elapsed since the last update
     */
    void update(float deltaTime);
    
    /**
     * Render the training overlay
     * This should be called during the render phase
     * @param width Screen width
     * @param height Screen height
     */
    void render(int width, int height);
    
    /**
     * Enable or disable hitbox visualization
     * @param enabled Whether hitboxes should be displayed
     */
    void setHitboxesEnabled(bool enabled);
    
    /**
     * Check if hitbox visualization is enabled
     * @return True if hitboxes are enabled
     */
    bool isHitboxesEnabled() const;
    
    /**
     * Enable or disable frame data display
     * @param enabled Whether frame data should be displayed
     */
    void setFrameDataEnabled(bool enabled);
    
    /**
     * Check if frame data display is enabled
     * @return True if frame data is enabled
     */
    bool isFrameDataEnabled() const;
    
    /**
     * Enable or disable input display
     * @param enabled Whether input display should be shown
     */
    void setInputDisplayEnabled(bool enabled);
    
    /**
     * Check if input display is enabled
     * @return True if input display is enabled
     */
    bool isInputDisplayEnabled() const;
    
    /**
     * Enable or disable game state display
     * @param enabled Whether game state should be displayed
     */
    void setGameStateEnabled(bool enabled);
    
    /**
     * Check if game state display is enabled
     * @return True if game state display is enabled
     */
    bool isGameStateEnabled() const;
    
    /**
     * Set the opacity for all overlay elements
     * @param opacity The opacity value (0.0-1.0)
     */
    void setOpacity(float opacity);
    
    /**
     * Get the current opacity value
     * @return The current opacity value (0.0-1.0)
     */
    float getOpacity() const;
    
    /**
     * Save the current overlay settings to a file
     * @param filename The file to save to
     * @return True if settings were saved successfully
     */
    bool saveSettings(const std::string& filename = "training_overlay.json");
    
    /**
     * Load overlay settings from a file
     * @param filename The file to load from
     * @return True if settings were loaded successfully
     */
    bool loadSettings(const std::string& filename = "training_overlay.json");

private:
    // Components
    std::unique_ptr<OverlayRenderer> m_renderer;
    std::unique_ptr<HitboxVisualizer> m_hitboxVisualizer;
    std::unique_ptr<InputDisplay> m_inputDisplay;
    std::unique_ptr<FrameDataDisplay> m_frameDataDisplay;
    std::unique_ptr<GameStateDisplay> m_gameStateDisplay;
    
    // Memory mapping reference
    AIMemoryMapping* m_memoryMapping;
    MetalContext* m_metalContext;
    
    // Feature flags
    bool m_hitboxesEnabled;
    bool m_frameDataEnabled;
    bool m_inputDisplayEnabled;
    bool m_gameStateEnabled;
    
    // Display settings
    float m_opacity;
    
    // Initialization state
    bool m_initialized;
    
    /**
     * Render the game state display
     * Shows current game state like round timer, health, etc.
     */
    void renderGameState();
    
    /**
     * Render the frame data display
     * Shows attack frame data and frame advantage
     */
    void renderFrameData();
}; 