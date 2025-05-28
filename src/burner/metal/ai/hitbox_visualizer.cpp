#include "hitbox_visualizer.h"
#include "overlay_renderer.h"
#include "ai_memory_mapping.h"
#include <algorithm>
#include <sstream>
#include <cmath>
#include <iostream>

// Constructor
HitboxVisualizer::HitboxVisualizer()
    : m_renderer(nullptr)
    , m_memoryMapping(nullptr)
    , m_opacity(0.7f)
    , m_showDamageValues(true)
    , m_showFrameInfo(false)
    , m_scaleX(1.0f)
    , m_scaleY(1.0f)
    , m_offsetX(0.0f)
    , m_offsetY(0.0f)
    , m_initialized(false)
{
    // Set default colors for hitbox types
    m_hitboxColors[HitboxType::Attack] = {1.0f, 0.2f, 0.2f};     // Red
    m_hitboxColors[HitboxType::Hurt] = {0.2f, 0.2f, 1.0f};       // Blue
    m_hitboxColors[HitboxType::Throw] = {1.0f, 0.0f, 1.0f};      // Magenta
    m_hitboxColors[HitboxType::Pushbox] = {0.2f, 1.0f, 0.2f};    // Green
    m_hitboxColors[HitboxType::Proximity] = {1.0f, 1.0f, 0.2f};  // Yellow
    m_hitboxColors[HitboxType::Custom] = {1.0f, 0.5f, 0.0f};     // Orange

    // Enable all hitbox types by default
    m_enabledTypes[HitboxType::Attack] = true;
    m_enabledTypes[HitboxType::Hurt] = true;
    m_enabledTypes[HitboxType::Throw] = true;
    m_enabledTypes[HitboxType::Pushbox] = true;
    m_enabledTypes[HitboxType::Proximity] = true;
    m_enabledTypes[HitboxType::Custom] = true;
}

// Destructor
HitboxVisualizer::~HitboxVisualizer()
{
    // Nothing to clean up
}

// Initialize with dependencies
bool HitboxVisualizer::initialize(OverlayRenderer* renderer, AIMemoryMapping* memoryMapping)
{
    if (!renderer || !memoryMapping) {
        std::cerr << "HitboxVisualizer: Invalid renderer or memory mapping" << std::endl;
        return false;
    }

    m_renderer = renderer;
    m_memoryMapping = memoryMapping;

    // Check if memory mapping is loaded
    if (!m_memoryMapping->isLoaded()) {
        std::cerr << "HitboxVisualizer: Memory mapping not loaded" << std::endl;
        return false;
    }

    // Get game architecture from memory mapping for coordinate system setup
    std::string architecture = m_memoryMapping->getArchitecture();
    
    // Set coordinate transformation based on architecture
    if (architecture == "CPS1" || architecture == "CPS2") {
        m_scaleX = 1.0f;
        m_scaleY = 1.0f;
        m_offsetX = 0.0f;
        m_offsetY = 0.0f;
    } else if (architecture == "NEOGEO") {
        m_scaleX = 1.0f;
        m_scaleY = 1.0f;
        m_offsetX = 0.0f;
        m_offsetY = 0.0f;
    } else {
        // Default values for unknown architectures
        m_scaleX = 1.0f;
        m_scaleY = 1.0f;
        m_offsetX = 0.0f;
        m_offsetY = 0.0f;
    }

    m_initialized = true;
    return true;
}

// Update hitbox data from game memory
void HitboxVisualizer::update()
{
    if (!m_initialized) {
        return;
    }

    // Clear previous frame's hitbox data
    m_hitboxes.clear();

    // Extract hitbox data from game memory
    extractHitboxes();
}

// Render all visible hitboxes
void HitboxVisualizer::render()
{
    if (!m_initialized || !m_renderer) {
        return;
    }

    // Render each hitbox
    for (const auto& hitbox : m_hitboxes) {
        // Skip if this hitbox type is disabled
        if (!m_enabledTypes[hitbox.type]) {
            continue;
        }

        // Get color for this hitbox type
        const auto& color = m_hitboxColors[hitbox.type];

        // Skip inactive hitboxes
        if (!hitbox.active) {
            continue;
        }

        // Transform game coordinates to screen coordinates
        float screenX, screenY;
        transformCoordinates(hitbox.x, hitbox.y, screenX, screenY);

        // Scale width and height
        float screenWidth = hitbox.width * m_scaleX;
        float screenHeight = hitbox.height * m_scaleY;

        // Draw rectangle with appropriate color and opacity
        m_renderer->drawRect(
            screenX - screenWidth / 2.0f,  // Left
            screenY - screenHeight / 2.0f, // Top
            screenWidth,                  // Width
            screenHeight,                 // Height
            color[0], color[1], color[2], m_opacity);

        // Draw hitbox data (damage, priority) if applicable
        if (hitbox.type == HitboxType::Attack && hitbox.damage > 0) {
            char damageText[16];
            snprintf(damageText, sizeof(damageText), "%d", hitbox.damage);
            m_renderer->drawText(
                screenX, 
                screenY, 
                damageText, 
                1.0f, 1.0f, 1.0f, m_opacity);
        }
    }
}

// Extract hitbox data from memory
void HitboxVisualizer::extractHitboxes()
{
    // Check if memory mapping is available
    if (!m_memoryMapping || !m_memoryMapping->isLoaded()) {
        return;
    }

    // Get game architecture to determine how to extract hitboxes
    std::string architecture = m_memoryMapping->getArchitecture();
    
    if (architecture == "CPS1" || architecture == "CPS2") {
        processCPSHitboxes();
    } else if (architecture == "NEOGEO") {
        processNeoGeoHitboxes();
    } else {
        // For other architectures, use generic approach
        processGenericHitboxes();
    }
}

// Extract hitboxes for CPS architecture games (Street Fighter series, Marvel vs Capcom, etc.)
void HitboxVisualizer::processCPSHitboxes()
{
    // Implementation notes:
    // CPS1/CPS2 games typically store hitbox information in a specific format
    // The format varies by game, but usually involves:
    //  - Arrays of hitbox data in memory
    //  - Each hitbox has position relative to the character
    //  - Need to add character position to get world coordinates
    
    // Get player positions
    int p1X = 0, p1Y = 0, p2X = 0, p2Y = 0;
    
    // Try to get player positions from memory mapping
    if (m_memoryMapping->hasMapping("p1_pos_x")) {
        p1X = m_memoryMapping->readInt("p1_pos_x");
    }
    if (m_memoryMapping->hasMapping("p1_pos_y")) {
        p1Y = m_memoryMapping->readInt("p1_pos_y");
    }
    if (m_memoryMapping->hasMapping("p2_pos_x")) {
        p2X = m_memoryMapping->readInt("p2_pos_x");
    }
    if (m_memoryMapping->hasMapping("p2_pos_y")) {
        p2Y = m_memoryMapping->readInt("p2_pos_y");
    }
    
    // Read CPS-specific hitbox data from memory if available
    // This is game-specific and would need to be implemented based on
    // the specific game's memory layout
    
    // For now, we'll just add some placeholder generic hitboxes
    // In a real implementation, this would read from game-specific memory
    
    // P1 pushbox (typically around the character's position)
    Hitbox p1Pushbox;
    p1Pushbox.type = HitboxType::Pushbox;
    p1Pushbox.x = p1X;
    p1Pushbox.y = p1Y;
    p1Pushbox.width = 40;
    p1Pushbox.height = 80;
    p1Pushbox.playerIndex = 0;
    p1Pushbox.priority = 0;
    p1Pushbox.damage = 0;
    p1Pushbox.active = true;
    m_hitboxes.push_back(p1Pushbox);
    
    // P2 pushbox
    Hitbox p2Pushbox;
    p2Pushbox.type = HitboxType::Pushbox;
    p2Pushbox.x = p2X;
    p2Pushbox.y = p2Y;
    p2Pushbox.width = 40;
    p2Pushbox.height = 80;
    p2Pushbox.playerIndex = 1;
    p2Pushbox.priority = 0;
    p2Pushbox.damage = 0;
    p2Pushbox.active = true;
    m_hitboxes.push_back(p2Pushbox);
    
    // Note: In a complete implementation, we would parse memory for
    // active attack hitboxes, hurt hitboxes, throw boxes, etc.
}

// Extract hitboxes for Neo Geo architecture games (King of Fighters, Samurai Shodown, etc.)
void HitboxVisualizer::processNeoGeoHitboxes()
{
    // Implementation notes:
    // Neo Geo games use a different format for hitbox data
    // The process would be similar to CPS but with different memory layouts
    
    // Get player positions
    int p1X = 0, p1Y = 0, p2X = 0, p2Y = 0;
    
    // Try to get player positions from memory mapping
    if (m_memoryMapping->hasMapping("p1_pos_x")) {
        p1X = m_memoryMapping->readInt("p1_pos_x");
    }
    if (m_memoryMapping->hasMapping("p1_pos_y")) {
        p1Y = m_memoryMapping->readInt("p1_pos_y");
    }
    if (m_memoryMapping->hasMapping("p2_pos_x")) {
        p2X = m_memoryMapping->readInt("p2_pos_x");
    }
    if (m_memoryMapping->hasMapping("p2_pos_y")) {
        p2Y = m_memoryMapping->readInt("p2_pos_y");
    }
    
    // For Neo Geo games, we'd need to implement game-specific
    // hitbox extraction here
    
    // For now, add placeholder hitboxes
    // P1 pushbox
    Hitbox p1Pushbox;
    p1Pushbox.type = HitboxType::Pushbox;
    p1Pushbox.x = p1X;
    p1Pushbox.y = p1Y;
    p1Pushbox.width = 40;
    p1Pushbox.height = 80;
    p1Pushbox.playerIndex = 0;
    p1Pushbox.priority = 0;
    p1Pushbox.damage = 0;
    p1Pushbox.active = true;
    m_hitboxes.push_back(p1Pushbox);
    
    // P2 pushbox
    Hitbox p2Pushbox;
    p2Pushbox.type = HitboxType::Pushbox;
    p2Pushbox.x = p2X;
    p2Pushbox.y = p2Y;
    p2Pushbox.width = 40;
    p2Pushbox.height = 80;
    p2Pushbox.playerIndex = 1;
    p2Pushbox.priority = 0;
    p2Pushbox.damage = 0;
    p2Pushbox.active = true;
    m_hitboxes.push_back(p2Pushbox);
}

// Generic hitbox extraction for other architectures
void HitboxVisualizer::processGenericHitboxes()
{
    // For games without specific hitbox extraction implementations,
    // we can try to use generic mappings if they exist
    
    // Try to get player positions
    int p1X = 0, p1Y = 0, p2X = 0, p2Y = 0;
    
    if (m_memoryMapping->hasMapping("p1_pos_x")) {
        p1X = m_memoryMapping->readInt("p1_pos_x");
    }
    if (m_memoryMapping->hasMapping("p1_pos_y")) {
        p1Y = m_memoryMapping->readInt("p1_pos_y");
    }
    if (m_memoryMapping->hasMapping("p2_pos_x")) {
        p2X = m_memoryMapping->readInt("p2_pos_x");
    }
    if (m_memoryMapping->hasMapping("p2_pos_y")) {
        p2Y = m_memoryMapping->readInt("p2_pos_y");
    }
    
    // Check for direct hitbox mappings
    // If the game's memory mapping contains direct hitbox info, use it
    
    // Check if we have any hitbox width/height mappings
    if (m_memoryMapping->hasMapping("p1_hitbox_width") && 
        m_memoryMapping->hasMapping("p1_hitbox_height")) {
        
        int width = m_memoryMapping->readInt("p1_hitbox_width");
        int height = m_memoryMapping->readInt("p1_hitbox_height");
        
        // Create a generic hurt hitbox based on these dimensions
        Hitbox p1HurtBox;
        p1HurtBox.type = HitboxType::Hurt;
        p1HurtBox.x = p1X;
        p1HurtBox.y = p1Y;
        p1HurtBox.width = width;
        p1HurtBox.height = height;
        p1HurtBox.playerIndex = 0;
        p1HurtBox.priority = 0;
        p1HurtBox.damage = 0;
        p1HurtBox.active = true;
        m_hitboxes.push_back(p1HurtBox);
    }
    
    // Same for p2
    if (m_memoryMapping->hasMapping("p2_hitbox_width") && 
        m_memoryMapping->hasMapping("p2_hitbox_height")) {
        
        int width = m_memoryMapping->readInt("p2_hitbox_width");
        int height = m_memoryMapping->readInt("p2_hitbox_height");
        
        Hitbox p2HurtBox;
        p2HurtBox.type = HitboxType::Hurt;
        p2HurtBox.x = p2X;
        p2HurtBox.y = p2Y;
        p2HurtBox.width = width;
        p2HurtBox.height = height;
        p2HurtBox.playerIndex = 1;
        p2HurtBox.priority = 0;
        p2HurtBox.damage = 0;
        p2HurtBox.active = true;
        m_hitboxes.push_back(p2HurtBox);
    }
    
    // If we have attack hitbox info
    if (m_memoryMapping->hasMapping("p1_attack_box_x") && 
        m_memoryMapping->hasMapping("p1_attack_box_y") &&
        m_memoryMapping->hasMapping("p1_attack_box_width") && 
        m_memoryMapping->hasMapping("p1_attack_box_height")) {
        
        int attackX = m_memoryMapping->readInt("p1_attack_box_x");
        int attackY = m_memoryMapping->readInt("p1_attack_box_y");
        int width = m_memoryMapping->readInt("p1_attack_box_width");
        int height = m_memoryMapping->readInt("p1_attack_box_height");
        
        // Check if attack is active
        bool active = true;
        if (m_memoryMapping->hasMapping("p1_attack_active")) {
            active = m_memoryMapping->readInt("p1_attack_active") != 0;
        }
        
        // Only create active attack hitboxes
        if (active && width > 0 && height > 0) {
            Hitbox p1AttackBox;
            p1AttackBox.type = HitboxType::Attack;
            p1AttackBox.x = p1X + attackX;  // Attack box is relative to player position
            p1AttackBox.y = p1Y + attackY;
            p1AttackBox.width = width;
            p1AttackBox.height = height;
            p1AttackBox.playerIndex = 0;
            
            // Read damage if available
            if (m_memoryMapping->hasMapping("p1_attack_damage")) {
                p1AttackBox.damage = m_memoryMapping->readInt("p1_attack_damage");
            } else {
                p1AttackBox.damage = 0;
            }
            
            // Read priority if available
            if (m_memoryMapping->hasMapping("p1_attack_priority")) {
                p1AttackBox.priority = m_memoryMapping->readInt("p1_attack_priority");
            } else {
                p1AttackBox.priority = 1;
            }
            
            p1AttackBox.active = true;
            m_hitboxes.push_back(p1AttackBox);
        }
    }
    
    // Same for p2
    // (Similar code for P2 attack hitboxes would go here)
    
    // If there's no specific hitbox info, create default pushboxes
    if (m_hitboxes.empty()) {
        // Default P1 pushbox
        Hitbox p1Pushbox;
        p1Pushbox.type = HitboxType::Pushbox;
        p1Pushbox.x = p1X;
        p1Pushbox.y = p1Y;
        p1Pushbox.width = 40;  // Default size
        p1Pushbox.height = 80;
        p1Pushbox.playerIndex = 0;
        p1Pushbox.priority = 0;
        p1Pushbox.damage = 0;
        p1Pushbox.active = true;
        m_hitboxes.push_back(p1Pushbox);
        
        // Default P2 pushbox
        Hitbox p2Pushbox;
        p2Pushbox.type = HitboxType::Pushbox;
        p2Pushbox.x = p2X;
        p2Pushbox.y = p2Y;
        p2Pushbox.width = 40;  // Default size
        p2Pushbox.height = 80;
        p2Pushbox.playerIndex = 1;
        p2Pushbox.priority = 0;
        p2Pushbox.damage = 0;
        p2Pushbox.active = true;
        m_hitboxes.push_back(p2Pushbox);
    }
}

// Transform coordinates from game to screen space
void HitboxVisualizer::transformCoordinates(int gameX, int gameY, float& screenX, float& screenY)
{
    // Transform from game coordinates to screen coordinates
    // This will depend on the renderer and the game's coordinate system
    
    // For now, apply simple scaling and offset
    screenX = gameX * m_scaleX + m_offsetX;
    screenY = gameY * m_scaleY + m_offsetY;
    
    // Note: In a real implementation, this would also take into account:
    // - The game's viewport coordinates
    // - Screen resolution and aspect ratio
    // - Any scaling or letterboxing applied by the renderer
}

// Enable/disable a hitbox type
void HitboxVisualizer::setHitboxTypeEnabled(HitboxType type, bool enabled)
{
    m_enabledTypes[type] = enabled;
}

// Check if a hitbox type is enabled
bool HitboxVisualizer::isHitboxTypeEnabled(HitboxType type) const
{
    auto it = m_enabledTypes.find(type);
    if (it != m_enabledTypes.end()) {
        return it->second;
    }
    return false;
}

// Set the opacity for all hitboxes
void HitboxVisualizer::setOpacity(float opacity)
{
    m_opacity = std::max(0.0f, std::min(1.0f, opacity));
}

// Get the current opacity
float HitboxVisualizer::getOpacity() const
{
    return m_opacity;
}

// Set the color for a specific hitbox type
void HitboxVisualizer::setHitboxColor(HitboxType type, float r, float g, float b)
{
    m_hitboxColors[type] = {r, g, b};
}

// Get the color for a specific hitbox type
HitboxVisualizer::Color HitboxVisualizer::getHitboxColor(HitboxType type) const
{
    auto it = m_hitboxColors.find(type);
    return (it != m_hitboxColors.end()) ? it->second : Color(1.0f, 1.0f, 1.0f);
}

// Enable/disable showing damage values
void HitboxVisualizer::setShowDamageValues(bool show)
{
    m_showDamageValues = show;
}

// Check if damage values are being shown
bool HitboxVisualizer::isShowingDamageValues() const
{
    return m_showDamageValues;
}

// Enable/disable showing frame data
void HitboxVisualizer::setShowFrameInfo(bool show)
{
    m_showFrameInfo = show;
}

// Check if frame data is being shown
bool HitboxVisualizer::isShowingFrameInfo() const
{
    return m_showFrameInfo;
}

// Set scaling factor for coordinate transformation
void HitboxVisualizer::setScaleFactor(float scale)
{
    m_scaleX = m_scaleY = scale;
}

// Set screen offset in pixels
void HitboxVisualizer::setScreenOffset(float x, float y)
{
    m_offsetX = x;
    m_offsetY = y;
}

// Get the number of hitboxes being tracked
int HitboxVisualizer::getHitboxCount() const
{
    return static_cast<int>(m_hitboxes.size());
} 