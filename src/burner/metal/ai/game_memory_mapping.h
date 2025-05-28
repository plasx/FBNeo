#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace fbneo {
namespace ai {

// Game genre types for memory mapping
enum GameType {
    GAME_UNKNOWN,
    GAME_FIGHTING,
    GAME_PLATFORMER,
    GAME_PUZZLE,
    GAME_SHOOTER,
    GAME_RACING,
    GAME_SPORTS
};

// State change event types
enum StateChangeType {
    CHANGE_PLAYER_HEALTH,   // Player health changed significantly
    CHANGE_PLAYER_POSITION, // Player position changed significantly
    CHANGE_PLAYER_STATE,    // Player state changed (animation state)
    CHANGE_ENEMY_HEALTH,    // Enemy health changed
    CHANGE_ENEMY_SPAWN,     // New enemy spawned
    CHANGE_ENEMY_DEFEAT,    // Enemy defeated
    CHANGE_SCORE,           // Score changed significantly
    CHANGE_LEVEL,           // Level/stage changed
    CHANGE_GAME_STATE,      // Game state changed (menu, playing, paused)
    CHANGE_GAME_OVER,       // Game over condition
    CHANGE_POWERUP,         // Power-up collected
    CHANGE_ROUND,           // Round changed (fighting games)
    CHANGE_TIME,            // Time changed significantly
    CHANGE_ENVIRONMENT,     // Environment changed
    CHANGE_CUSTOM           // Custom/game-specific change
};

// State change event data
struct StateChangeEvent {
    StateChangeType type;       // Type of change
    std::string regionName;     // Memory region that changed
    float oldValue;             // Previous value
    float newValue;             // New value
    std::string description;    // Human-readable description
    void* userData;             // Optional user data
};

// State change notification callback type
using StateChangeCallback = std::function<void(const StateChangeEvent&)>;

// Maximum values for arrays
#define MAX_BUTTONS 8
#define MAX_ENEMIES 16
#define MAX_PROJECTILES 32
#define MAX_POWERUPS 8
#define MAX_BOARD_WIDTH 16
#define MAX_BOARD_HEIGHT 32

// Fighting game memory structure
struct FightingGameState {
    // Player info
    int32_t playerHealth;
    int32_t playerX;
    int32_t playerY;
    uint8_t playerState;      // Current animation state
    uint8_t isBlocking;       // Whether player is blocking
    uint8_t wasHit;           // Whether player was hit
    uint8_t specialMoveExecuted; // Whether a special move was executed
    uint8_t comboCounter;     // Current combo counter
    
    // Opponent info
    int32_t opponentHealth;
    int32_t opponentX;
    int32_t opponentY;
    uint8_t opponentState;
    
    // Match info
    uint8_t roundNumber;
    uint8_t timeRemaining;
    uint8_t roundWon;
    uint8_t roundLost;
    uint8_t matchWon;
    uint8_t matchLost;
    
    // Projectiles
    uint8_t projectileCount;
    struct {
        int32_t x;
        int32_t y;
        uint8_t type;
        uint8_t isPlayerOwned;
    } projectiles[MAX_PROJECTILES];
};

// Platformer game memory structure
struct PlatformerGameState {
    // Player info
    int32_t playerX;
    int32_t playerY;
    int32_t velocityX;
    int32_t velocityY;
    uint8_t isJumping;
    uint8_t isOnGround;
    uint8_t isHurt;
    uint8_t lives;
    uint8_t powerUpState;     // Current power-up level
    
    // Level info
    uint8_t level;
    int32_t score;
    uint16_t coinsCollected;
    uint16_t enemiesDefeated;
    
    // Enemy info
    uint8_t enemyCount;
    struct {
        int32_t x;
        int32_t y;
        uint8_t type;
        uint8_t state;
    } enemies[MAX_ENEMIES];
    
    // Platform/obstacle info
    uint8_t nearestPlatformX;
    uint8_t nearestPlatformY;
    uint8_t nearestHazardX;
    uint8_t nearestHazardY;
};

// Puzzle game memory structure
struct PuzzleGameState {
    // Board state
    uint8_t boardWidth;
    uint8_t boardHeight;
    uint8_t board[MAX_BOARD_HEIGHT][MAX_BOARD_WIDTH];
    
    // Current piece info
    uint8_t currentPieceType;
    uint8_t currentPieceRotation;
    int8_t currentPieceX;
    int8_t currentPieceY;
    
    // Next piece info
    uint8_t nextPieceType;
    
    // Game stats
    int32_t score;
    uint16_t linesCleared;
    uint8_t level;
    uint8_t comboCounter;
    uint8_t stackHeight;  // Height of tallest stack
    uint8_t gameOver;
};

// Shooter game memory structure
struct ShooterGameState {
    // Player info
    int32_t playerX;
    int32_t playerY;
    int32_t playerHealth;
    int32_t lives;
    uint8_t powerUpLevel;
    
    // Game state
    int32_t score;
    uint16_t enemiesDestroyed;
    uint8_t level;
    uint8_t gameOver;
    
    // Enemy info
    uint8_t enemyCount;
    struct {
        int32_t x;
        int32_t y;
        uint8_t type;
        int16_t health;
    } enemies[MAX_ENEMIES];
    
    // Projectile info
    uint8_t projectileCount;
    struct {
        int32_t x;
        int32_t y;
        int8_t velocityX;
        int8_t velocityY;
        uint8_t isPlayerOwned;
    } projectiles[MAX_PROJECTILES];
    
    // Boss info
    uint8_t bossActive;
    int32_t bossHealth;
    int32_t bossX;
    int32_t bossY;
    
    // Power-up info
    uint8_t powerUpCount;
    struct {
        int32_t x;
        int32_t y;
        uint8_t type;
    } powerUps[MAX_POWERUPS];
};

// Racing game memory structure
struct RacingGameState {
    // Player car info
    int32_t x;
    int32_t y;
    int32_t speed;
    int32_t acceleration;
    int16_t angle;
    
    // Race info
    float trackProgress;  // 0.0-1.0 for a lap
    uint8_t lap;
    uint8_t totalLaps;
    uint8_t position;     // 1st, 2nd, etc.
    uint8_t totalRacers;
    
    // Track state
    uint8_t offTrack;
    uint8_t collisionFlag;
    
    // Race status
    uint16_t raceTimeSeconds;
    uint8_t raceComplete;
    
    // Opponent info
    struct {
        int32_t x;
        int32_t y;
        int32_t speed;
        float trackProgress;
        uint8_t lap;
    } opponents[8];
};

// Sports game memory structure
struct SportsGameState {
    // Team info
    int32_t playerScore;
    int32_t opponentScore;
    
    // Player positions
    struct {
        int32_t x;
        int32_t y;
        uint8_t state;
    } players[11];
    
    // Ball info
    int32_t ballX;
    int32_t ballY;
    int32_t ballZ;
    int8_t ballVelocityX;
    int8_t ballVelocityY;
    int8_t ballVelocityZ;
    
    // Game state
    uint16_t timeRemaining;
    uint8_t period;
    uint8_t possession;  // 0 = player, 1 = opponent
};

// Generic game state structure (used in AIInputFrame)
struct GameState {
    void* data;         // Pointer to one of the game-specific structures
    GameType gameType;  // Type of game for proper casting
    uint8_t gameOver;   // Whether the game is over
    
    // Memory validation
    uint32_t dataSize;  // Size of the data structure
    uint32_t checksum;  // Simple validation checksum
};

// Game memory mapping handlers
class GameMemoryMapper {
public:
    // Register game-specific memory regions for observation
    static bool registerGame(const char* gameId, GameType gameType);
    
    // Map game memory to appropriate state structure
    static GameState getGameState(const char* gameId);
    
    // Register a memory region for observation
    static bool registerMemoryRegion(const char* gameId, const char* name, 
                                    uint32_t address, uint32_t size, int dataType);
    
    // Get a value from memory
    static float getMemoryValue(const char* gameId, const char* name);
    
    // Get the offset of a named memory region
    static int getMemoryOffset(const char* gameId, const char* variableName);
    
    // Check if a game is supported by the memory mapper
    static bool isGameSupported(const char* gameId);
    
    // Get the number of supported games
    static int getSupportedGameCount();
    
    // Get a list of supported game IDs
    static const char** getSupportedGameIDs();
    
    // Get the type of a game
    static GameType getGameType(const char* gameId);
    
    // --- State Change Notification System ---
    
    // Register a global state change callback
    static bool registerStateChangeCallback(StateChangeCallback callback);
    
    // Register a game-specific state change callback
    static bool registerGameStateChangeCallback(const char* gameId, StateChangeCallback callback);
    
    // Enable or disable state change notifications
    static void setNotificationsEnabled(bool enabled);
    
    // Configure watch for a specific memory region
    static bool configureMemoryWatch(const char* gameId, const char* regionName, 
                                    StateChangeType changeType, float threshold, 
                                    bool usePercentage);
    
    // Process memory for state changes (should be called each frame)
    static bool processStateChanges(const char* gameId);
};

// C interface for memory mapping
extern "C" {
    // Opaque handle type for memory mapper
    typedef void* FBNEO_MemoryMapper;
    
    // Create a memory mapper
    FBNEO_MemoryMapper FBNEO_MemoryMapper_Create();
    
    // Destroy a memory mapper
    void FBNEO_MemoryMapper_Destroy(void* handle);
    
    // Initialize the memory mapper system
    int FBNEO_MemoryMapper_Initialize();
    
    // Register a game
    int FBNEO_MemoryMapper_RegisterGame(const char* gameId, int gameType);
    
    // Get the type of a game
    int FBNEO_MemoryMapper_GetGameType(const char* gameId);
    
    // Get a value from memory
    float FBNEO_MemoryMapper_GetMemoryValue(const char* gameId, const char* name);
    
    // Register a memory region
    int FBNEO_MemoryMapper_RegisterMemoryRegion(const char* gameId, const char* name, 
                                              uint32_t address, uint32_t size, int dataType);
    
    // State change notification API
    
    // Register a state change callback (returns a handle that can be used to unregister)
    void* FBNEO_MemoryMapper_RegisterStateChangeCallback(
        void (*callback)(const char* gameId, const char* regionName, 
                        int changeType, float oldValue, float newValue, 
                        const char* description));
    
    // Unregister a callback
    int FBNEO_MemoryMapper_UnregisterStateChangeCallback(void* callbackHandle);
    
    // Enable or disable notifications
    void FBNEO_MemoryMapper_SetNotificationsEnabled(int enabled);
    
    // Process state changes for current game
    int FBNEO_MemoryMapper_ProcessStateChanges(const char* gameId);
    
    // Configure memory watch
    int FBNEO_MemoryMapper_ConfigureMemoryWatch(const char* gameId, const char* regionName, 
                                             int changeType, float threshold, int usePercentage);
    
    // Metal integration
    int FBNEO_MemoryMapper_InitializeMetalBuffers(void* metalDevice, void* commandQueue);
    int FBNEO_MemoryMapper_UpdateMetalBuffers(const char* gameId);
    void FBNEO_MemoryMapper_CleanupMetalResources();
}

} // namespace ai
} // namespace fbneo 