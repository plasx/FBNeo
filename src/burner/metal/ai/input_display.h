#pragma once

#include <memory>
#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include "overlay_renderer.h"
#include "../../ai/ai_memory_mapping.h"

/**
 * Class for displaying real-time and historical inputs for fighting games
 * 
 * This class visualizes the current inputs for both players, as well as keeping
 * a history of previous inputs to display combos, move executions, etc.
 */
class InputDisplay {
public:
    /**
     * Constructor
     */
    InputDisplay();
    
    /**
     * Destructor
     */
    ~InputDisplay();
    
    /**
     * Initialize the input display
     * @param renderer Pointer to the overlay renderer
     * @param memoryMapping Pointer to the memory mapping
     * @return True if initialization was successful
     */
    bool initialize(OverlayRenderer* renderer, AIMemoryMapping* memoryMapping);
    
    /**
     * Update the input display with the latest input state
     * This should be called once per frame before rendering
     */
    void update();
    
    /**
     * Render the input display
     * This should be called during the render phase
     */
    void render();
    
    /**
     * Set the position for Player 1's input display
     * @param x X position
     * @param y Y position
     */
    void setP1Position(float x, float y);
    
    /**
     * Set the position for Player 2's input display
     * @param x X position
     * @param y Y position
     */
    void setP2Position(float x, float y);
    
    /**
     * Set the input history size (number of frames to record)
     * @param frames Number of frames to keep in history
     */
    void setHistorySize(int frames);
    
    /**
     * Enable or disable the display of input history
     * @param enabled Whether to show input history
     */
    void setHistoryEnabled(bool enabled);
    
    /**
     * Enable or disable the display of input icons
     * @param enabled Whether to show input icons
     */
    void setIconsEnabled(bool enabled);
    
    /**
     * Enable or disable the display of frame advantage information
     * @param enabled Whether to show frame advantage
     */
    void setFrameAdvantageEnabled(bool enabled);
    
    /**
     * Set the opacity (alpha) for the input display
     * @param opacity The opacity value (0.0-1.0)
     */
    void setOpacity(float opacity);
    
    /**
     * Get the current opacity value
     * @return The current opacity value (0.0-1.0)
     */
    float getOpacity() const;

private:
    // Structure representing a single input state
    struct InputState {
        bool up;            // Up direction
        bool down;          // Down direction
        bool left;          // Left direction
        bool right;         // Right direction
        bool attack1;       // Light punch/attack
        bool attack2;       // Medium punch/attack
        bool attack3;       // Heavy punch/attack
        bool attack4;       // Light kick/attack
        bool attack5;       // Medium kick/attack
        bool attack6;       // Heavy kick/attack
        bool start;         // Start button
        bool select;        // Select/coin button
        int frame;          // Frame number for this input state
        
        // Check if this input state is the same as another
        bool operator==(const InputState& other) const;
        
        // Check if this input state is different from another
        bool operator!=(const InputState& other) const;
        
        // Check if any button is pressed
        bool anyButton() const;
        
        // Check if any direction is pressed
        bool anyDirection() const;
        
        // Convert to string notation (e.g., "↓↘→+P")
        std::string toNotation() const;
    };
    
    // Structure for motion detection (special moves)
    struct Motion {
        std::string name;                  // Name of the motion (e.g., "Hadouken")
        std::vector<std::string> sequence; // Sequence of inputs that make up the motion
        int windowFrames;                  // Frame window to complete the motion
    };
    
    /**
     * Read the current input state from memory
     * @param playerIndex Player index (0 for P1, 1 for P2)
     * @return InputState object with the current inputs
     */
    InputState readInputState(int playerIndex);
    
    /**
     * Render the current input state
     * @param state The input state to render
     * @param x X position
     * @param y Y position
     * @param playerIndex Player index (0 for P1, 1 for P2)
     */
    void renderInputState(const InputState& state, float x, float y, int playerIndex);
    
    /**
     * Render the input history
     * @param history The input history to render
     * @param x X position
     * @param y Y position
     * @param playerIndex Player index (0 for P1, 1 for P2)
     */
    void renderInputHistory(const std::deque<InputState>& history, float x, float y, int playerIndex);
    
    /**
     * Detect and render frame advantage
     * @param p1History Player 1's input history
     * @param p2History Player 2's input history
     */
    void renderFrameAdvantage(const std::deque<InputState>& p1History, const std::deque<InputState>& p2History);
    
    /**
     * Detect special move motions in the input history
     * @param history The input history to analyze
     * @return List of detected motions
     */
    std::vector<std::string> detectMotions(const std::deque<InputState>& history);
    
    // References to external components
    OverlayRenderer* m_renderer;
    AIMemoryMapping* m_memoryMapping;
    
    // Input state history for both players
    std::deque<InputState> m_p1History;
    std::deque<InputState> m_p2History;
    
    // Current input states
    InputState m_p1CurrentState;
    InputState m_p2CurrentState;
    
    // Display settings
    float m_p1X, m_p1Y;     // Position for P1 display
    float m_p2X, m_p2Y;     // Position for P2 display
    int m_historySize;      // Number of frames to keep in history
    bool m_historyEnabled;  // Whether to display input history
    bool m_iconsEnabled;    // Whether to display input icons
    bool m_frameAdvantageEnabled; // Whether to show frame advantage
    float m_opacity;        // Opacity for rendering
    
    // List of motion patterns to detect
    std::vector<Motion> m_motionPatterns;
    
    // Current frame counter
    int m_currentFrame;
    
    // Whether the display is fully initialized
    bool m_initialized;
}; 