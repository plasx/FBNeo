#pragma once

#include "ai_controller.h"
#include <vector>
#include <string>

namespace fbneo {
namespace ai {

// Hitbox type
enum class HitboxType {
    ATTACK,    // Attack hitbox (red)
    VULNERABLE,// Vulnerable/hurtbox (blue)
    COLLISION, // Collision box (green)
    THROW,     // Throw box (yellow)
    CUSTOM     // Custom hitbox
};

// Hitbox representation
struct Hitbox {
    HitboxType type;
    int x, y, width, height;
    int frame_active;   // How many frames the hitbox is active
    int damage;         // Damage value if applicable
    
    Hitbox() : type(HitboxType::ATTACK), x(0), y(0), width(0), height(0),
               frame_active(0), damage(0) {}
};

// Frame data information
struct FrameData {
    int startup;        // Startup frames
    int active;         // Active frames
    int recovery;       // Recovery frames 
    int advantage;      // Frame advantage (+ on hit/block, - on whiff)
    int damage;         // Damage
    bool isProjectile;  // Is this a projectile move
    
    FrameData() : startup(0), active(0), recovery(0), advantage(0),
                  damage(0), isProjectile(false) {}
};

// Training mode manager
class TrainingMode {
private:
    bool m_enabled;
    TrainingModeOptions m_options;
    std::vector<Hitbox> m_currentHitboxes;
    FrameData m_lastMoveData;
    std::vector<std::string> m_inputHistory;
    
public:
    TrainingMode();
    ~TrainingMode();
    
    // Initialize training mode
    bool initialize();
    
    // Enable/disable training mode
    void setEnabled(bool enabled);
    bool isEnabled() const;
    
    // Set training mode options
    void setOptions(const TrainingModeOptions& options);
    const TrainingModeOptions& getOptions() const;
    
    // Update hitboxes for current frame (game-specific implementation)
    void updateHitboxes(const uint8_t* gameMemory, int gameType);
    
    // Get current hitboxes
    const std::vector<Hitbox>& getHitboxes() const;
    
    // Update frame data for current move (game-specific implementation)
    void updateFrameData(const uint8_t* gameMemory, int gameType);
    
    // Get current frame data
    const FrameData& getFrameData() const;
    
    // Add input to history display
    void addInputToHistory(const std::string& input);
    
    // Get input history for display
    const std::vector<std::string>& getInputHistory() const;
    
    // Render training mode elements
    void render(unsigned char* screenBuffer, int width, int height, int pitch);
};

// Game-specific training mode data providers
// These would be implemented for each supported game
class GameSpecificTrainingData {
public:
    virtual ~GameSpecificTrainingData() {}
    
    // Get hitboxes for current frame
    virtual std::vector<Hitbox> getHitboxes(const uint8_t* gameMemory) = 0;
    
    // Get frame data for current move
    virtual FrameData getFrameData(const uint8_t* gameMemory) = 0;
    
    // Get player health addresses
    virtual std::vector<int> getHealthAddresses() = 0;
    
    // Get timer address
    virtual int getTimerAddress() = 0;
    
    // Apply infinite health
    virtual void applyInfiniteHealth(uint8_t* gameMemory) = 0;
    
    // Apply infinite time
    virtual void applyInfiniteTime(uint8_t* gameMemory) = 0;
};

// Factory to create game-specific training data providers
GameSpecificTrainingData* createTrainingDataProvider(const char* gameName);

} // namespace ai
} // namespace fbneo 