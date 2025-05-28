#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <unordered_map>
#include <functional>
#include <variant>
#include <optional>
#include <deque>

namespace AI {

// Forward declarations
struct MemoryMappingEntry;

/**
 * @brief The supported data types for memory mapping.
 */
enum class MemoryType {
    BYTE = 0,    // 8-bit value
    WORD = 1,    // 16-bit value
    DWORD = 2,   // 32-bit value
    FLOAT = 3,   // 32-bit floating point
    BIT = 4,     // Single bit within a byte
    RAM,         // Main CPU RAM
    VRAM,        // Video RAM
    ROM,         // ROM data
    PALETTE,     // Palette RAM
    NVRAM,       // Non-volatile RAM
    EEPROM,      // EEPROM/FRAM
    PORT,        // I/O port
    REGISTER    // CPU register
};

/**
 * @brief Represents the endianness of multi-byte values
 */
enum class Endianness {
    LITTLE,
    BIG
};

/**
 * @brief Represents a single memory mapping entry
 */
struct MemoryMappingEntry {
    std::string name;               // Unique identifier
    std::string address;            // Memory address (hex string)
    std::string description;        // Human-readable description
    std::string category;           // Optional category
    MemoryType type;                // Data type
    int playerIndex;                // Player index (0-based) or -1 if not player-specific
    double scale;                   // Scale factor
    double offset;                  // Offset to add
    std::optional<double> minValue; // Minimum valid value (for normalization)
    std::optional<double> maxValue; // Maximum valid value (for normalization)
    std::optional<int> bitPosition; // Bit position (0-31) for bit type
    std::optional<std::string> mask; // Bit mask (hex string)
    Endianness endianness;         // Byte order
    std::string relativeTo;         // Name of reference mapping
    double changeThreshold;         // Threshold for considering a value changed

    // Constructor with default values
    MemoryMappingEntry() : 
        type(MemoryType::BYTE),
        playerIndex(-1),
        scale(1.0),
        offset(0.0),
        endianness(Endianness::LITTLE),
        changeThreshold(0.0) {}
};

/**
 * @class AIMemoryMapping
 * @brief Class for accessing game memory based on predefined mappings (simplified for test)
 */
class AIMemoryMapping {
public:
    using ValueType = std::variant<uint8_t, uint16_t, uint32_t, float, bool>;

    // Constructor and destructor
    AIMemoryMapping() {}
    ~AIMemoryMapping() {}

    // Initialize with a game ID (stub for testing)
    bool initialize(const std::string& gameDriverName) { return true; }

    // Load a memory mapping from a file (stub for testing)
    bool loadFromFile(const std::string& filePath) { return true; }

    // Check if a mapping is loaded (stub)
    bool isLoaded() const { return true; }

    // Get the game name (stub)
    std::string getGameName() const { return "test_game"; }

    // Get the architecture (stub)
    std::string getArchitecture() const { return "test_arch"; }

    // Refresh all cached values (stub)
    void refreshValues(int currentFrame = -1) {}

    // Get mappings that changed in the last refresh (stub)
    const std::vector<std::string>& getChangedMappings() const { 
        static std::vector<std::string> empty;
        return empty; 
    }
};

} // namespace AI 