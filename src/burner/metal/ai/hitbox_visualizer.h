#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include "overlay_renderer.h"
#include "../../ai/ai_memory_mapping.h"

// Forward declarations
class OverlayRenderer;
class AIMemoryMapping;

/**
 * Enum for different types of hitboxes
 */
enum class HitboxType {
    Attack,     // Attack hitbox (usually red)
    Hurt,       // Hurt/vulnerable hitbox (usually blue)
    Throw,      // Throw hitbox (usually purple/magenta)
    Pushbox,    // Collision/push hitbox (usually green)
    Proximity,  // Proximity box for move activation (usually yellow)
    Custom      // Custom hitbox for game-specific features
};

/**
 * Structure representing a hitbox in game coordinates
 */
struct Hitbox {
    HitboxType type;
    int x, y;           // Center position (game coordinates)
    int width, height;  // Dimensions
    int playerIndex;    // Which player the hitbox belongs to (0 or 1)
    int priority;       // Priority level (higher = stronger, game dependent)
    int damage;         // Damage value (if applicable)
    bool active;        // Whether the hitbox is currently active
};

/**
 * Class for visualizing hitboxes from fighting games
 * 
 * This class extracts hitbox data from game memory and renders it
 * as colored rectangles on the screen. It supports different types of
 * hitboxes with customizable colors and can be toggled on/off.
 */
class HitboxVisualizer {
public:
    HitboxVisualizer();
    ~HitboxVisualizer();

    /**
     * Initialize the hitbox visualizer
     * @param renderer Pointer to the overlay renderer
     * @param memoryMapping Pointer to the memory mapping
     * @return True if initialization was successful
     */
    bool initialize(OverlayRenderer* renderer, AIMemoryMapping* memoryMapping);

    /**
     * Update hitbox data from game memory
     * This should be called once per frame before rendering
     */
    void update();

    /**
     * Render hitboxes to the screen
     * This should be called during the render phase
     */
    void render();

    /**
     * Enable or disable a specific type of hitbox
     * @param type The type of hitbox to toggle
     * @param enabled Whether to enable or disable the hitbox type
     */
    void setHitboxTypeEnabled(HitboxType type, bool enabled);

    /**
     * Check if a specific hitbox type is enabled
     * @param type The hitbox type to check
     * @return True if the hitbox type is enabled
     */
    bool isHitboxTypeEnabled(HitboxType type) const;

    /**
     * Set the color for a specific hitbox type
     * @param type The hitbox type to set the color for
     * @param r Red component (0.0-1.0)
     * @param g Green component (0.0-1.0)
     * @param b Blue component (0.0-1.0)
     */
    void setHitboxColor(HitboxType type, float r, float g, float b);

    /**
     * Set the opacity (alpha) for all hitbox rendering
     * @param opacity The opacity value (0.0-1.0)
     */
    void setOpacity(float opacity);

    /**
     * Get the current opacity value
     * @return The current opacity value (0.0-1.0)
     */
    float getOpacity() const;

private:
    /**
     * Extract hitbox data from game memory
     * This is game-specific and depends on the memory mapping
     */
    void extractHitboxes();

    /**
     * Process CPS1/CPS2 style hitboxes
     */
    void processCPSHitboxes();

    /**
     * Process Neo Geo style hitboxes
     */
    void processNeoGeoHitboxes();

    /**
     * Process generic hitboxes from memory mapping
     */
    void processGenericHitboxes();

    /**
     * Transform game coordinates to screen coordinates
     * @param gameX X coordinate in game space
     * @param gameY Y coordinate in game space
     * @param screenX Output X coordinate in screen space
     * @param screenY Output Y coordinate in screen space
     */
    void transformCoordinates(int gameX, int gameY, float& screenX, float& screenY);

    // Reference to the overlay renderer
    OverlayRenderer* m_renderer;

    // Reference to the memory mapping
    AIMemoryMapping* m_memoryMapping;

    // Hitbox collection
    std::vector<Hitbox> m_hitboxes;

    // Enabled state for each hitbox type
    std::unordered_map<HitboxType, bool> m_enabledTypes;

    // Color for each hitbox type (RGB)
    std::unordered_map<HitboxType, std::array<float, 3>> m_hitboxColors;

    // Global opacity for hitbox rendering
    float m_opacity;

    // Game-specific scale and offset for coordinate transformation
    float m_scaleX, m_scaleY;
    float m_offsetX, m_offsetY;

    // Whether the visualizer is fully initialized
    bool m_initialized;
}; 