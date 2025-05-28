#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace AI {

// Forward declarations
class AIMemoryMapping;

// Character state enumeration
enum class CharacterState {
    STANDING = 0,
    CROUCHING,
    JUMPING,
    ATTACKING,
    BLOCKING,
    HITSTUN,
    KNOCKDOWN,
    GETTING_UP,
    SPECIAL_MOVE,
    SUPER_MOVE,
    DIZZY,
    DEFEATED,
    INACTIVE,
    UNKNOWN
};

/**
 * @brief Data structure containing normalized game state observations
 * 
 * This class encapsulates all the game state information needed by the AI,
 * with normalization to ensure values are in appropriate ranges for the model.
 */
class AIInputFrame {
public:
    /**
     * @brief Default constructor, initializes to default values
     */
    AIInputFrame();

    /**
     * Constructor with frame number.
     * @param frameNumber The frame number this state represents
     */
    AIInputFrame(int frameNumber);

    /**
     * Destructor
     */
    ~AIInputFrame();

    /**
     * Set the frame number.
     * @param frameNumber The frame number
     */
    void setFrameNumber(int frameNumber);

    /**
     * Get the frame number.
     * @return The frame number
     */
    int getFrameNumber() const;

    /**
     * Set the game ID.
     * @param gameId The game ID
     */
    void setGameId(const std::string& gameId);

    /**
     * Get the game ID.
     * @return The game ID
     */
    const std::string& getGameId() const;

    /**
     * Set the frame hash.
     * @param hash The frame hash value
     */
    void setHash(const std::string& hash);

    /**
     * Get the frame hash.
     * @return The frame hash value
     */
    const std::string& getHash() const;

    /**
     * Generate a hash for this frame based on its contents.
     * This will update the internal hash value.
     */
    void generateHash();

    /**
     * Add a value for a specific player.
     * @param playerIndex The player index (0 for P1, 1 for P2)
     * @param name The name of the value
     * @param value The actual value
     */
    void addPlayerValue(int playerIndex, const std::string& name, float value);

    /**
     * Get a value for a specific player.
     * @param playerIndex The player index (0 for P1, 1 for P2)
     * @param name The name of the value
     * @return The value, or 0.0f if not found
     */
    float getPlayerValue(int playerIndex, const std::string& name) const;

    /**
     * Get all value names for a specific player.
     * @param playerIndex The player index
     * @return Vector of value names
     */
    std::vector<std::string> getPlayerValueNames(int playerIndex) const;

    /**
     * Get all player indices in this frame.
     * @return Vector of player indices
     */
    std::vector<int> getPlayerIndices() const;

    /**
     * Add a feature value.
     * @param name The name of the feature
     * @param value The actual value
     */
    void addFeatureValue(const std::string& name, float value);

    /**
     * Get a feature value.
     * @param name The name of the feature
     * @return The value, or 0.0f if not found
     */
    float getFeatureValue(const std::string& name) const;

    /**
     * Get all feature value names.
     * @return Vector of feature names
     */
    std::vector<std::string> getFeatureValueNames() const;

    /**
     * Convert to JSON string.
     * @return JSON representation of the frame
     */
    std::string toJson() const;

    /**
     * Load from JSON string.
     * @param jsonString The JSON string to load from
     * @return True if loaded successfully, false otherwise
     */
    bool fromJson(const std::string& jsonString);

    /**
     * Check if this frame is equal to another frame.
     * @param other The other frame to compare with
     * @return True if equal, false otherwise
     */
    bool equals(const AIInputFrame& other) const;

    /**
     * Find differences between this frame and another frame.
     * @param other The other frame to compare with
     * @param differences Output vector to store the differences
     * @return True if differences were found, false if frames are identical
     */
    bool findDifferences(const AIInputFrame& other, std::vector<std::string>& differences) const;

    /**
     * @brief Get a string representation for debugging
     * @return String with frame values
     */
    std::string toString() const;

    // Match state
    float time_remaining;    // [0.0-1.0] Normalized time remaining
    float round;             // [0.0-1.0] Current round normalized
    
    // Player 1 state
    float p1_x;              // [0.0-1.0] X position normalized
    float p1_y;              // [0.0-1.0] Y position normalized
    float p1_health;         // [0.0-1.0] Health normalized
    float p1_meter;          // [0.0-1.0] Special meter normalized
    float p1_state;          // Enumerated state (standing, crouching, jumping, etc.)
    float p1_facing;         // -1.0 or 1.0 (facing left or right)
    float p1_attacking;      // Boolean: 0.0 or 1.0
    float p1_blocking;       // Boolean: 0.0 or 1.0
    
    // Player 2 state
    float p2_x;              // [0.0-1.0] X position normalized
    float p2_y;              // [0.0-1.0] Y position normalized
    float p2_health;         // [0.0-1.0] Health normalized
    float p2_meter;          // [0.0-1.0] Special meter normalized
    float p2_state;          // Enumerated state (standing, crouching, jumping, etc.)
    float p2_facing;         // -1.0 or 1.0 (facing left or right)
    float p2_attacking;      // Boolean: 0.0 or 1.0
    float p2_blocking;       // Boolean: 0.0 or 1.0
    
    // Distance metrics
    float x_distance;        // [0.0-1.0] X distance between players 
    float y_distance;        // [0.0-1.0] Y distance between players

    // Frame metadata
    uint32_t frame_number;   // Current frame number
    uint32_t inputs;         // Current input bitmask
    uint32_t rng_seed;       // Current RNG seed for determinism checking
    std::string state_hash;  // Hash of complete game state for determinism validation

    // Methods
    static AIInputFrame extractFromMemory(const AIMemoryMapping& mapping);
    void normalize();
    std::vector<float> toVector() const;
    
    // Serialization
    std::string toJSON() const;
    static AIInputFrame fromJSON(const std::string& jsonStr);
    
    // State hash calculation
    void computeStateHash();
    
    // Comparing frames for determinism validation
    bool operator==(const AIInputFrame& other) const;
    bool operator!=(const AIInputFrame& other) const;

    /**
     * @brief Get the input dimension size
     * @return Number of input features
     */
    static size_t getInputDimension();

    /**
     * Create a new input frame
     * 
     * @param player_idx The player index (0 = common, 1 = P1, 2 = P2, etc.)
     */
    AIInputFrame(int player_idx = 1);
    
    /**
     * Initialize the frame with the current game state
     * 
     * @param memory_mapping Optional pointer to memory mapping for game-specific values
     */
    void Init(AIMemoryMapping* memory_mapping = nullptr);
    
    /**
     * Set the memory mapping to use for game-specific values
     * 
     * @param memory_mapping Pointer to the memory mapping
     */
    void SetMemoryMapping(AIMemoryMapping* memory_mapping);
    
    /**
     * Update the frame with the current game state
     */
    void Update();
    
    /**
     * Get the player index
     * 
     * @return The player index (0 = common, 1 = P1, 2 = P2, etc.)
     */
    int GetPlayerIndex() const;
    
    /**
     * Set the player index
     * 
     * @param idx The player index (0 = common, 1 = P1, 2 = P2, etc.)
     */
    void SetPlayerIndex(int idx);
    
    /**
     * Get the frame counter
     * 
     * @return The frame counter
     */
    int GetFrameCounter() const;
    
    /**
     * Set the frame counter
     * 
     * @param counter The frame counter
     */
    void SetFrameCounter(int counter);
    
    /**
     * Get the normalized health value
     * 
     * @return Normalized health value (0-1)
     */
    float GetHealth() const;
    
    /**
     * Set the normalized health value
     * 
     * @param health Normalized health value (0-1)
     */
    void SetHealth(float health);
    
    /**
     * Get the opponent's normalized health value
     * 
     * @return Normalized opponent health value (0-1)
     */
    float GetOpponentHealth() const;
    
    /**
     * Set the opponent's normalized health value
     * 
     * @param health Normalized opponent health value (0-1)
     */
    void SetOpponentHealth(float health);
    
    /**
     * Get the normalized X position
     * 
     * @return Normalized X position (0-1)
     */
    float GetPositionX() const;
    
    /**
     * Set the normalized X position
     * 
     * @param x Normalized X position (0-1)
     */
    void SetPositionX(float x);
    
    /**
     * Get the normalized Y position
     * 
     * @return Normalized Y position (0-1)
     */
    float GetPositionY() const;
    
    /**
     * Set the normalized Y position
     * 
     * @param y Normalized Y position (0-1)
     */
    void SetPositionY(float y);
    
    /**
     * Get the opponent's normalized X position
     * 
     * @return Normalized opponent X position (0-1)
     */
    float GetOpponentPositionX() const;
    
    /**
     * Set the opponent's normalized X position
     * 
     * @param x Normalized opponent X position (0-1)
     */
    void SetOpponentPositionX(float x);
    
    /**
     * Get the opponent's normalized Y position
     * 
     * @return Normalized opponent Y position (0-1)
     */
    float GetOpponentPositionY() const;
    
    /**
     * Set the opponent's normalized Y position
     * 
     * @param y Normalized opponent Y position (0-1)
     */
    void SetOpponentPositionY(float y);
    
    /**
     * Get the normalized distance to the opponent
     * 
     * @return Normalized distance (0-1)
     */
    float GetDistanceToOpponent() const;
    
    /**
     * Set a game-specific memory value
     * 
     * @param name The name of the value (should match a mapping name)
     * @param value The value to set
     */
    void SetMemoryValue(const std::string& name, float value);
    
    /**
     * Get a game-specific memory value
     * 
     * @param name The name of the value (should match a mapping name)
     * @return The value, or 0 if not found
     */
    float GetMemoryValue(const std::string& name) const;
    
    /**
     * Check if a game-specific memory value exists
     * 
     * @param name The name of the value
     * @return True if the value exists
     */
    bool HasMemoryValue(const std::string& name) const;
    
    /**
     * Get all game-specific memory values
     * 
     * @return Map of name -> value
     */
    const std::unordered_map<std::string, float>& GetMemoryValues() const;
    
    /**
     * Get a vector of all values (standard + game-specific)
     * 
     * @return Vector of all values
     */
    std::vector<float> GetFeatureVector() const;
    
    /**
     * Get the standard feature count (without game-specific values)
     * 
     * @return Standard feature count
     */
    size_t GetStandardFeatureCount() const;
    
    /**
     * Get the total feature count (standard + game-specific)
     * 
     * @return Total feature count
     */
    size_t GetTotalFeatureCount() const;
    
    /**
     * Convert the frame to JSON format
     * 
     * @return JSON string representation
     */
    std::string ToJson() const;
    
    /**
     * Load frame from JSON format
     * 
     * @param json_str JSON string representation
     * @return True if loading succeeded
     */
    bool FromJson(const std::string& json_str);

    /**
     * @brief Extract game state from memory with change detection
     * 
     * @param mapping The memory mapping to use
     * @param onlySignificantChanges Whether to only include values that changed significantly
     * @param changeThreshold The threshold for significant changes (0.0-1.0)
     * @return true if the frame state changed significantly, false otherwise
     */
    bool extractFromMemoryWithChangeDetection(const AIMemoryMapping& mapping, 
                                             bool onlySignificantChanges = false,
                                             double changeThreshold = 0.05);
    
    /**
     * @brief Get the list of values that changed in this update
     * 
     * @return Vector of mapping names that changed
     */
    std::vector<std::string> getChangedValues() const;
    
    /**
     * @brief Get the list of values that changed significantly in this update
     * 
     * @param threshold The significance threshold (0.0-1.0)
     * @return Vector of mapping names that changed significantly
     */
    std::vector<std::string> getSignificantChanges(double threshold = 0.05) const;

private:
    int m_frameNumber;
    std::string m_gameId;
    std::string m_hash;
    std::unordered_map<int, std::unordered_map<std::string, float>> m_playerValues;
    std::unordered_map<std::string, float> m_featureValues;
};

} // namespace AI 