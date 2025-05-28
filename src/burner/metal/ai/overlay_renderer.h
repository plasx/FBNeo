#pragma once

#include <string>
#include <memory>
#include <vector>
#include <functional>

/**
 * Manages rendering of overlay elements for the training mode
 * 
 * This class provides an abstraction for rendering 2D elements on top
 * of the game viewport, including text, shapes, and UI elements.
 */
class OverlayRenderer {
public:
    /**
     * Constructor
     */
    OverlayRenderer();
    
    /**
     * Destructor
     */
    ~OverlayRenderer();
    
    /**
     * Initialize the renderer
     * @return True if initialization was successful
     */
    bool initialize();
    
    /**
     * Begin a new frame for rendering
     * Called once per frame before any drawing operations
     */
    void beginFrame();
    
    /**
     * End the current frame and present it
     * Called once per frame after all drawing operations
     */
    void endFrame();
    
    /**
     * Draw a rectangle with specified position, size, and color
     * @param x Left position
     * @param y Top position
     * @param width Width of the rectangle
     * @param height Height of the rectangle
     * @param r Red component (0.0-1.0)
     * @param g Green component (0.0-1.0)
     * @param b Blue component (0.0-1.0)
     * @param a Alpha/opacity (0.0-1.0)
     */
    void drawRect(float x, float y, float width, float height, 
                 float r, float g, float b, float a);
    
    /**
     * Draw an outline of a rectangle
     * @param x Left position
     * @param y Top position
     * @param width Width of the rectangle
     * @param height Height of the rectangle
     * @param r Red component (0.0-1.0)
     * @param g Green component (0.0-1.0)
     * @param b Blue component (0.0-1.0)
     * @param a Alpha/opacity (0.0-1.0)
     * @param thickness Line thickness in pixels
     */
    void drawRectOutline(float x, float y, float width, float height,
                        float r, float g, float b, float a, float thickness = 1.0f);
    
    /**
     * Draw a line between two points
     * @param x1 Start X position
     * @param y1 Start Y position
     * @param x2 End X position
     * @param y2 End Y position
     * @param r Red component (0.0-1.0)
     * @param g Green component (0.0-1.0)
     * @param b Blue component (0.0-1.0)
     * @param a Alpha/opacity (0.0-1.0)
     * @param thickness Line thickness in pixels
     */
    void drawLine(float x1, float y1, float x2, float y2,
                 float r, float g, float b, float a, float thickness = 1.0f);
    
    /**
     * Draw text at the specified position
     * @param x Text X position
     * @param y Text Y position
     * @param text Text to display
     * @param r Red component (0.0-1.0)
     * @param g Green component (0.0-1.0)
     * @param b Blue component (0.0-1.0)
     * @param a Alpha/opacity (0.0-1.0)
     * @param fontSize Font size in points (default: 14.0)
     */
    void drawText(float x, float y, const char* text,
                 float r, float g, float b, float a, float fontSize = 14.0f);
    
    /**
     * Draw text with shadow for better visibility
     * @param x Text X position
     * @param y Text Y position
     * @param text Text to display
     * @param r Red component (0.0-1.0)
     * @param g Green component (0.0-1.0)
     * @param b Blue component (0.0-1.0)
     * @param a Alpha/opacity (0.0-1.0)
     * @param fontSize Font size in points (default: 14.0)
     */
    void drawTextWithShadow(float x, float y, const char* text,
                           float r, float g, float b, float a, float fontSize = 14.0f);
    
    /**
     * Get the current viewport width
     * @return Width in pixels
     */
    int getViewportWidth() const;
    
    /**
     * Get the current viewport height
     * @return Height in pixels
     */
    int getViewportHeight() const;
    
    /**
     * Set the viewport dimensions
     * @param width Width in pixels
     * @param height Height in pixels
     */
    void setViewportSize(int width, int height);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
}; 