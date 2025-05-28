#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <deque>
#include <array>
#include <functional>

// Forward declarations
class MetalRenderer;
class AIMemoryMapping;

// Input event structure
struct InputEvent {
    int playerIndex;
    uint32_t inputBits;
    uint32_t frameNumber;
    uint64_t timestamp;
};

// Rectangle structure for hitboxes
struct Rect {
    float x, y, width, height;
    Rect(float _x = 0, float _y = 0, float _w = 0, float _h = 0) 
        : x(_x), y(_y), width(_w), height(_h) {}
};

// Color structure for visualization
struct Color {
    float r, g, b, a;
    Color(float _r = 1.0f, float _g = 1.0f, float _b = 1.0f, float _a = 1.0f) 
        : r(_r), g(_g), b(_b), a(_a) {}
    
    // Predefined colors
    static Color Red() { return Color(1.0f, 0.0f, 0.0f, 0.8f); }
    static Color Green() { return Color(0.0f, 1.0f, 0.0f, 0.8f); }
    static Color Blue() { return Color(0.0f, 0.0f, 1.0f, 0.8f); }
    static Color Yellow() { return Color(1.0f, 1.0f, 0.0f, 0.8f); }
    static Color Purple() { return Color(0.8f, 0.0f, 0.8f, 0.8f); }
    static Color Orange() { return Color(1.0f, 0.5f, 0.0f, 0.8f); }
    static Color White() { return Color(1.0f, 1.0f, 1.0f, 0.8f); }
    static Color Black() { return Color(0.0f, 0.0f, 0.0f, 0.8f); }
    static Color Transparent() { return Color(0.0f, 0.0f, 0.0f, 0.0f); }
};

// Hitbox types
enum class HitboxType {
    Attack,      // Attack hitbox (dangerous to opponent)
    Hurtbox,     // Hurtbox (vulnerable to attacks)
    Pushbox,     // Pushbox (character collision)
    Throwbox,    // Throw range
    Projectile,  // Projectile hitbox
    Special,     // Special move hitbox
    Counter      // Counter hit area
};

// Hitbox structure
struct Hitbox {
    Rect rect;
    HitboxType type;
    int damage;
    int priority;
    int frameStart;
    int frameDuration;
    bool active;
    Color color;
    
    Hitbox() : type(HitboxType::Attack), damage(0), priority(0),
               frameStart(0), frameDuration(0), active(false) {}
};

// Frame data structure
struct FrameData {
    int startupFrames;     // Frames before hitbox becomes active
    int activeFrames;      // Frames hitbox is active
    int recoveryFrames;    // Recovery frames after active frames
    int totalFrames;       // Total frames of the move
    int damage;            // Damage of the move
    int hitAdvantage;      // Frame advantage on hit
    int blockAdvantage;    // Frame advantage on block
    bool isProjectile;     // Is this a projectile move
    bool isInvincible;     // Does the move have invincibility frames
    bool isArmored;        // Does the move have armor
    bool isThrow;          // Is this a throw
    
    FrameData() : startupFrames(0), activeFrames(0), recoveryFrames(0),
                 totalFrames(0), damage(0), hitAdvantage(0), blockAdvantage(0),
                 isProjectile(false), isInvincible(false), isArmored(false),
                 isThrow(false) {}
};

// Input display structure
struct InputDisplay {
    bool up, down, left, right;
    bool punch, kick, slash, heavy;
    bool special1, special2;
    bool start, select;
    
    InputDisplay() : up(false), down(false), left(false), right(false),
                    punch(false), kick(false), slash(false), heavy(false),
                    special1(false), special2(false), start(false), select(false) {}
};

// Forward declarations for display components
class HitboxDisplay;
class FrameDataDisplay;
class InputHistoryDisplay;
class StateInfoDisplay;

/**
 * @class TrainingOverlay
 * @brief Manages the rendering of training mode overlays including hitboxes, frame data, etc.
 */
class TrainingOverlay {
public:
    /**
     * @brief Get the singleton instance of TrainingOverlay
     * @return Pointer to the singleton instance
     */
    static TrainingOverlay* getInstance();
    
    /**
     * @brief Initialize the training overlay with renderer and memory mapping
     * @param renderer Metal renderer to use for drawing
     * @param memoryMapping Memory mapping to access game state
     * @return True if initialization was successful
     */
    bool initialize(MetalRenderer* renderer, AIMemoryMapping* memoryMapping);
    
    /**
     * @brief Shutdown and clean up the training overlay
     */
    void shutdown();
    
    /**
     * @brief Update the overlay data (call every frame)
     */
    void update();
    
    /**
     * @brief Render the overlay (call after game rendering)
     */
    void render();
    
    /**
     * @brief Add input event to history
     * @param playerIndex Player index (0-based)
     * @param inputBits Bit flags for pressed buttons
     */
    void addInputEvent(int playerIndex, uint32_t inputBits);
    
    /**
     * @brief Enable/disable hitbox display
     * @param enabled True to enable, false to disable
     */
    void setHitboxDisplay(bool enabled);
    
    /**
     * @brief Enable/disable frame data display
     * @param enabled True to enable, false to disable
     */
    void setFrameDataDisplay(bool enabled);
    
    /**
     * @brief Enable/disable input history display
     * @param enabled True to enable, false to disable
     */
    void setInputHistoryDisplay(bool enabled);
    
    /**
     * @brief Enable/disable state info display
     * @param enabled True to enable, false to disable
     */
    void setStateInfoDisplay(bool enabled);
    
    /**
     * @brief Set the opacity for hitbox rendering
     * @param opacity Value between 0.0 (transparent) and 1.0 (opaque)
     */
    void setHitboxOpacity(float opacity);
    
    /**
     * @brief Set the color for a specific hitbox type
     * @param type Hitbox type
     * @param r Red component (0.0-1.0)
     * @param g Green component (0.0-1.0)
     * @param b Blue component (0.0-1.0)
     */
    void setHitboxColor(HitboxType type, float r, float g, float b);
    
    /**
     * @brief Set the font size for text displays
     * @param fontSize Font size in points
     */
    void setFontSize(float fontSize);
    
    /**
     * @brief Set the input history length
     * @param length Number of inputs to track per player
     */
    void setInputHistoryLength(int length);
    
    /**
     * @brief Set position for a specific overlay component
     * @param componentName Component identifier (e.g., "frameData", "inputHistory")
     * @param x X coordinate
     * @param y Y coordinate
     */
    void setComponentPosition(const std::string& componentName, float x, float y);
    
    /**
     * @brief Save current configuration to file
     * @param filename Path to save configuration
     * @return True if successful
     */
    bool saveConfiguration(const std::string& filename);
    
    /**
     * @brief Load configuration from file
     * @param filename Path to load configuration
     * @return True if successful
     */
    bool loadConfiguration(const std::string& filename);

    // Toggle all overlay elements
    void ToggleAllOverlays();
    
    // Add/update a hitbox
    void AddHitbox(int id, const Hitbox& hitbox);
    
    // Remove a hitbox
    void RemoveHitbox(int id);
    
    // Clear all hitboxes
    void ClearHitboxes();
    
    // Update frame data
    void UpdateFrameData(int playerIndex, const FrameData& frameData);
    
    // Update input display
    void UpdateInputDisplay(int playerIndex, const InputDisplay& inputs);
    
    // Register callback for hotkey presses
    using HotkeyCallback = std::function<void()>;
    void RegisterHotkeyCallback(const std::string& hotkeyName, HotkeyCallback callback);

private:
    // Private constructor for singleton
    TrainingOverlay();
    ~TrainingOverlay();
    
    // Disallow copy/move
    TrainingOverlay(const TrainingOverlay&) = delete;
    TrainingOverlay& operator=(const TrainingOverlay&) = delete;
    TrainingOverlay(TrainingOverlay&&) = delete;
    TrainingOverlay& operator=(TrainingOverlay&&) = delete;
    
    // Singleton instance
    static TrainingOverlay* s_instance;
    
    // Component references
    MetalRenderer* m_renderer;
    AIMemoryMapping* m_memoryMapping;
    
    // Display components
    std::unique_ptr<HitboxDisplay> m_hitboxDisplay;
    std::unique_ptr<FrameDataDisplay> m_frameDataDisplay;
    std::unique_ptr<InputHistoryDisplay> m_inputHistoryDisplay;
    std::unique_ptr<StateInfoDisplay> m_stateInfoDisplay;
    
    // Component state
    bool m_hitboxEnabled;
    bool m_frameDataEnabled;
    bool m_inputHistoryEnabled;
    bool m_stateInfoEnabled;
    
    // Configuration
    float m_hitboxOpacity;
    std::unordered_map<HitboxType, Color> m_hitboxColors;
    float m_fontSize;
    int m_inputHistoryLength;
    std::unordered_map<std::string, std::array<float, 2>> m_componentPositions;
    
    // State data
    uint32_t m_currentFrame;
    uint64_t m_currentTimestamp;
    std::array<std::deque<InputEvent>, 2> m_inputHistory;
    std::vector<Hitbox> m_hitboxes;
    std::array<FrameData, 2> m_frameData;
    
    // Helper methods
    void collectHitboxData();
    void collectFrameData();
    void collectStateData();
    void updateFrameCounter();
    void calculateFrameAdvantage();
    
    // Private implementation (PIMPL idiom)
    class Impl;
    std::unique_ptr<Impl> pImpl;
    
    // Helper functions
    void DrawRect(const Rect& rect, const Color& color, bool filled = true);
    void DrawText(const std::string& text, float x, float y, const Color& color, float scale = 1.0f);
    void DrawInputIcon(const std::string& inputName, float x, float y, float scale = 1.0f);
    
    // Render specific overlay elements
    void RenderHitboxes();
    void RenderFrameData();
    void RenderStateDisplay();
    void RenderInputDisplay();
    
    // Input handling for hotkeys
    void ProcessHotkeys();
};

/**
 * @class HitboxDisplay
 * @brief Handles rendering of hitboxes
 */
class HitboxDisplay {
public:
    HitboxDisplay(MetalRenderer* renderer);
    ~HitboxDisplay();
    
    void update(AIMemoryMapping* memoryMapping, std::vector<Hitbox>& hitboxes);
    void render(float opacity, const std::unordered_map<HitboxType, Color>& colors);
    
private:
    MetalRenderer* m_renderer;
    
    // Helper methods
    void detectHitboxes(AIMemoryMapping* memoryMapping, std::vector<Hitbox>& hitboxes);
    void transformCoordinates(Hitbox& hitbox);
};

/**
 * @class FrameDataDisplay
 * @brief Handles rendering of frame data
 */
class FrameDataDisplay {
public:
    FrameDataDisplay(MetalRenderer* renderer);
    ~FrameDataDisplay();
    
    void update(AIMemoryMapping* memoryMapping, std::array<FrameData, 2>& frameData);
    void render(float fontSize, const std::array<float, 2>& position);
    
private:
    MetalRenderer* m_renderer;
    
    // Helper methods
    void detectMoves(AIMemoryMapping* memoryMapping, int playerIndex, FrameData& frameData);
    void calculateAdvantage(int playerIndex, FrameData& frameData);
};

/**
 * @class InputHistoryDisplay
 * @brief Handles rendering of input history
 */
class InputHistoryDisplay {
public:
    InputHistoryDisplay(MetalRenderer* renderer);
    ~InputHistoryDisplay();
    
    void update(const std::array<std::deque<InputEvent>, 2>& inputHistory);
    void render(float fontSize, const std::array<float, 2>& position);
    
private:
    MetalRenderer* m_renderer;
    
    // Helper methods
    void renderInputIcon(uint32_t inputBit, float x, float y, float size);
    void detectSpecialMoves(const std::deque<InputEvent>& history, std::vector<std::string>& moveNames);
};

/**
 * @class StateInfoDisplay
 * @brief Handles rendering of state information
 */
class StateInfoDisplay {
public:
    StateInfoDisplay(MetalRenderer* renderer);
    ~StateInfoDisplay();
    
    void update(AIMemoryMapping* memoryMapping);
    void render(float fontSize, const std::array<float, 2>& position);
    
private:
    MetalRenderer* m_renderer;
    
    // State data
    std::unordered_map<std::string, std::string> m_stateValues;
    
    // Helper methods
    void collectPlayerState(AIMemoryMapping* memoryMapping, int playerIndex);
    void collectGameState(AIMemoryMapping* memoryMapping);
    void collectCharacterSpecificState(AIMemoryMapping* memoryMapping, int playerIndex);
}; 