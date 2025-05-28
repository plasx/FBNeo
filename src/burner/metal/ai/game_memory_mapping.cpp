#include "game_memory_mapping.h"
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <functional>
#include <algorithm>
#include <cmath>

// External C functions from emulator core for memory access
extern "C" {
    // Function to read a byte from emulator memory at a specific address
    extern uint8_t ReadByte(uint32_t address);
    
    // Function to read a word from emulator memory at a specific address
    extern uint16_t ReadWord(uint32_t address);
    
    // Function to read a long from emulator memory at a specific address
    extern uint32_t ReadLong(uint32_t address);
    
    // Get current driver name
    extern char* BurnDrvGetTextA(uint32_t i);
}

namespace fbneo {
namespace ai {

// Define state change event types
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

// Structure to hold memory watch configuration
struct MemoryWatchConfig {
    std::string name;           // Name of the watched region
    StateChangeType changeType; // Type of change to monitor
    float threshold;            // Threshold for significant change
    bool usePercentage;         // Whether threshold is a percentage
    float lastValue;            // Last observed value
    bool initialized;           // Whether the watch has been initialized
};

// Structure to hold memory region information
struct MemoryRegion {
    std::string name;
    uint32_t address;
    uint32_t size;
    int dataType;  // 0 = byte, 1 = word, 2 = long
    
    // Watch configuration (optional)
    bool isWatched;
    MemoryWatchConfig watchConfig;
};

// Structure to hold game memory mapping information
struct GameMemoryMap {
    std::string gameId;
    GameType gameType;
    std::vector<MemoryRegion> regions;
    
    // State notification setup
    bool notificationsEnabled;
    std::vector<StateChangeCallback> callbacks;
};

// Database of game memory mappings
static std::unordered_map<std::string, GameMemoryMap> g_memoryMaps;

// Vector of supported game IDs for iteration
static std::vector<std::string> g_supportedGames;

// Vector of string pointers for C API
static std::vector<const char*> g_supportedGameIDs;

// Currently active game ID
static std::string g_activeGameId;

// Memory snapshot for change detection
static std::vector<uint8_t> g_previousMemorySnapshot;

// Global notification settings
static bool g_notificationsEnabled = true;
static std::vector<StateChangeCallback> g_globalCallbacks;

// Additional structures and classes for game state change notifications
class GameStateChangeNotifier;

// Global GameStateChangeNotifier instance
static GameStateChangeNotifier* g_stateNotifier = nullptr;

// Game State Change Notifier - handles state change notifications across the codebase
class GameStateChangeNotifier {
public:
    GameStateChangeNotifier() {
        // Initialize state cache
        m_gameStateCache.clear();
        
        // Register for global callbacks
        GameMemoryMapper::registerStateChangeCallback(
            [this](const StateChangeEvent& event) {
                this->handleStateChange(event);
            }
        );
    }
    
    ~GameStateChangeNotifier() {
        clearListeners();
    }
    
    // Register a callback for any state change
    bool registerStateChangeListener(const std::function<void(const std::string&, const std::vector<StateChangeEvent>&)>& listener) {
        if (!listener) {
            return false;
        }
        
        m_stateChangeListeners.push_back(listener);
        return true;
    }
    
    // Register a callback for specific game and change type
    bool registerSpecificChangeListener(
        const std::string& gameId, 
        StateChangeType changeType,
        const std::function<void(const StateChangeEvent&)>& listener) {
        
        if (!listener) {
            return false;
        }
        
        SpecificChangeListener specificListener;
        specificListener.gameId = gameId;
        specificListener.changeType = changeType;
        specificListener.listener = listener;
        
        m_specificChangeListeners.push_back(specificListener);
        return true;
    }
    
    // Handle a state change event
    void handleStateChange(const StateChangeEvent& event) {
        // Add to pending events for the appropriate game
        std::string gameId = g_activeGameId; // Use the active game ID
        m_pendingEvents[gameId].push_back(event);
        
        // Update cache
        m_gameStateCache[gameId][event.regionName] = event.newValue;
        
        // Notify specific listeners immediately
        for (const auto& listener : m_specificChangeListeners) {
            if ((listener.gameId.empty() || listener.gameId == gameId) && 
                (listener.changeType == event.type || listener.changeType == CHANGE_CUSTOM)) {
                listener.listener(event);
            }
        }
    }
    
    // Process all pending notifications
    void processPendingNotifications() {
        for (auto& [gameId, events] : m_pendingEvents) {
            if (!events.empty()) {
                // Notify general listeners
                for (const auto& listener : m_stateChangeListeners) {
                    listener(gameId, events);
                }
                
                // Clear processed events
                events.clear();
            }
        }
    }
    
    // Clear all listeners
    void clearListeners() {
        m_stateChangeListeners.clear();
        m_specificChangeListeners.clear();
    }
    
    // Get last known value for a game state variable
    float getLastKnownValue(const std::string& gameId, const std::string& variableName) {
        auto gameIt = m_gameStateCache.find(gameId);
        if (gameIt != m_gameStateCache.end()) {
            auto varIt = gameIt->second.find(variableName);
            if (varIt != gameIt->second.end()) {
                return varIt->second;
            }
        }
        return 0.0f;
    }
    
    // Get a map of all current values for a game
    std::unordered_map<std::string, float> getGameState(const std::string& gameId) {
        auto it = m_gameStateCache.find(gameId);
        if (it != m_gameStateCache.end()) {
            return it->second;
        }
        return {};
    }
    
    // Check if a specific game state condition is met
    bool checkGameStateCondition(
        const std::string& gameId,
        const std::string& variableName,
        float targetValue,
        bool approximate = false) {
        
        float value = getLastKnownValue(gameId, variableName);
        
        if (approximate) {
            // Allow for small differences (within 5%)
            return std::abs(value - targetValue) <= 0.05f * std::abs(targetValue);
        } else {
            return value == targetValue;
        }
    }
    
    // Static methods to access the global instance
    static GameStateChangeNotifier* getInstance() {
        if (!g_stateNotifier) {
            g_stateNotifier = new GameStateChangeNotifier();
        }
        return g_stateNotifier;
    }
    
    static void destroyInstance() {
        if (g_stateNotifier) {
            delete g_stateNotifier;
            g_stateNotifier = nullptr;
        }
    }

private:
    // Structure for specific listeners
    struct SpecificChangeListener {
        std::string gameId;
        StateChangeType changeType;
        std::function<void(const StateChangeEvent&)> listener;
    };
    
    // List of listeners for any state change
    std::vector<std::function<void(const std::string&, const std::vector<StateChangeEvent>&)>> m_stateChangeListeners;
    
    // List of listeners for specific state changes
    std::vector<SpecificChangeListener> m_specificChangeListeners;
    
    // Cache of current game state values
    std::unordered_map<std::string, std::unordered_map<std::string, float>> m_gameStateCache;
    
    // Pending events to be processed
    std::unordered_map<std::string, std::vector<StateChangeEvent>> m_pendingEvents;
};

// Initialize memory maps
static void initializeMemoryMaps() {
    static bool initialized = false;
    if (initialized) return;
    
    // Clear existing data
    g_memoryMaps.clear();
    g_supportedGames.clear();
    g_supportedGameIDs.clear();
    
    // CPS2 Fighting Games
    
    // Street Fighter Alpha 3 (sfa3)
    GameMemoryMap sfa3;
    sfa3.gameId = "sfa3";
    sfa3.gameType = GAME_FIGHTING;
    sfa3.notificationsEnabled = true;
    sfa3.regions = {
        {"p1_health", 0x5E1, 1, 0},
        {"p2_health", 0x6E1, 1, 0},
        {"p1_x", 0x5E8, 2, 1},
        {"p1_y", 0x5EC, 2, 1},
        {"p2_x", 0x6E8, 2, 1},
        {"p2_y", 0x6EC, 2, 1},
        {"round", 0x5C4C, 1, 0},
        {"timer", 0x5C40, 2, 1},
        {"p1_state", 0x5F0, 1, 0},
        {"p2_state", 0x6F0, 1, 0},
        {"p1_combo", 0x5C50, 1, 0},
        {"p2_combo", 0x6C50, 1, 0}
    };
    
    // Set up watch configurations for SFA3
    for (auto& region : sfa3.regions) {
        region.isWatched = true;
        
        if (region.name == "p1_health" || region.name == "p2_health") {
            region.watchConfig = {
                region.name, 
                CHANGE_PLAYER_HEALTH, 
                5.0f,       // Notify on 5% health change
                true,       // Use percentage
                0.0f,       // Last value (will be initialized)
                false       // Not initialized yet
            };
        } else if (region.name == "p1_state" || region.name == "p2_state") {
            region.watchConfig = {
                region.name,
                CHANGE_PLAYER_STATE,
                0.0f,       // Any change
                false,      // Not percentage
                0.0f,       // Last value
                false       // Not initialized
            };
        } else if (region.name == "round") {
            region.watchConfig = {
                region.name,
                CHANGE_ROUND,
                0.0f,       // Any change
                false,      // Not percentage
                0.0f,       // Last value
                false       // Not initialized
            };
        } else if (region.name == "timer") {
            region.watchConfig = {
                region.name,
                CHANGE_TIME,
                5.0f,       // Notify every 5 seconds
                false,      // Not percentage
                0.0f,       // Last value
                false       // Not initialized
            };
        } else if (region.name.find("_x") != std::string::npos || 
                  region.name.find("_y") != std::string::npos) {
            region.watchConfig = {
                region.name,
                CHANGE_PLAYER_POSITION,
                10.0f,      // Notify on 10 pixel change
                false,      // Not percentage
                0.0f,       // Last value
                false       // Not initialized
            };
        } else if (region.name.find("combo") != std::string::npos) {
            region.watchConfig = {
                region.name,
                CHANGE_CUSTOM,
                0.0f,       // Any change
                false,      // Not percentage
                0.0f,       // Last value
                false       // Not initialized
            };
        }
    }

    // Street Fighter Alpha 2 (sfa2)
    GameMemoryMap sfa2;
    sfa2.gameId = "sfa2";
    sfa2.gameType = GAME_FIGHTING;
    sfa2.notificationsEnabled = true;
    sfa2.regions = {
        {"p1_health", 0x5E9, 1, 0},
        {"p2_health", 0x6E9, 1, 0},
        {"p1_x", 0x5F0, 2, 1},
        {"p1_y", 0x5F4, 2, 1},
        {"p2_x", 0x6F0, 2, 1},
        {"p2_y", 0x6F4, 2, 1},
        {"round", 0x5C50, 1, 0},
        {"timer", 0x5C44, 2, 1},
        {"p1_state", 0x5F8, 1, 0},
        {"p2_state", 0x6F8, 1, 0}
    };
    
    // Configure watches similar to SFA3
    for (auto& region : sfa2.regions) {
        region.isWatched = true;
        
        if (region.name == "p1_health" || region.name == "p2_health") {
            region.watchConfig = {region.name, CHANGE_PLAYER_HEALTH, 5.0f, true, 0.0f, false};
        } else if (region.name == "p1_state" || region.name == "p2_state") {
            region.watchConfig = {region.name, CHANGE_PLAYER_STATE, 0.0f, false, 0.0f, false};
        } else if (region.name == "round") {
            region.watchConfig = {region.name, CHANGE_ROUND, 0.0f, false, 0.0f, false};
        } else if (region.name == "timer") {
            region.watchConfig = {region.name, CHANGE_TIME, 5.0f, false, 0.0f, false};
        } else if (region.name.find("_x") != std::string::npos || 
                  region.name.find("_y") != std::string::npos) {
            region.watchConfig = {region.name, CHANGE_PLAYER_POSITION, 10.0f, false, 0.0f, false};
        }
    }

    // Add all game maps to the database
    g_memoryMaps["sfa3"] = sfa3;
    g_memoryMaps["sfa2"] = sfa2;
    
    // Continue with other game definitions...
    
    // Build list of supported games
    for (const auto& pair : g_memoryMaps) {
        g_supportedGames.push_back(pair.first);
    }
    
    // Build C-style array of game IDs
    g_supportedGameIDs.reserve(g_supportedGames.size() + 1);
    for (const auto& game : g_supportedGames) {
        g_supportedGameIDs.push_back(game.c_str());
    }
    g_supportedGameIDs.push_back(nullptr); // NULL terminator
    
    initialized = true;
}

// Helper functions for state change notification

// Get value for memory region as float
static float getMemoryValueAsFloat(const MemoryRegion& region) {
    float value = 0.0f;
    
    switch (region.dataType) {
        case 0: // byte
            value = static_cast<float>(ReadByte(region.address));
            break;
        case 1: // word
            value = static_cast<float>(ReadWord(region.address));
            break;
        case 2: // long
            value = static_cast<float>(ReadLong(region.address));
            break;
    }
    
    return value;
}

// Check if a change exceeds the threshold
static bool isSignificantChange(float oldValue, float newValue, const MemoryWatchConfig& config) {
    if (config.threshold == 0.0f) {
        // Any change is significant
        return oldValue != newValue;
    }
    
    float diff = std::abs(newValue - oldValue);
    
    if (config.usePercentage) {
        // Calculate percentage change (avoid division by zero)
        if (oldValue == 0.0f) {
            return newValue != 0.0f;
        }
        float percentChange = (diff / std::abs(oldValue)) * 100.0f;
        return percentChange >= config.threshold;
    } else {
        // Absolute change
        return diff >= config.threshold;
    }
}

// Generate description for a state change event
static std::string generateChangeDescription(const StateChangeEvent& event) {
    std::string description;
    
    switch (event.type) {
        case CHANGE_PLAYER_HEALTH:
            description = "Player health changed from " + std::to_string(static_cast<int>(event.oldValue)) + 
                         " to " + std::to_string(static_cast<int>(event.newValue));
            break;
        case CHANGE_PLAYER_POSITION:
            description = "Player position " + event.regionName + " changed from " + 
                         std::to_string(static_cast<int>(event.oldValue)) + 
                         " to " + std::to_string(static_cast<int>(event.newValue));
            break;
        case CHANGE_PLAYER_STATE:
            description = "Player state changed from " + std::to_string(static_cast<int>(event.oldValue)) + 
                         " to " + std::to_string(static_cast<int>(event.newValue));
            break;
        case CHANGE_ROUND:
            description = "Round changed from " + std::to_string(static_cast<int>(event.oldValue)) + 
                         " to " + std::to_string(static_cast<int>(event.newValue));
            break;
        case CHANGE_TIME:
            description = "Timer changed from " + std::to_string(static_cast<int>(event.oldValue)) + 
                         " to " + std::to_string(static_cast<int>(event.newValue));
            break;
        default:
            description = event.regionName + " changed from " + std::to_string(event.oldValue) + 
                         " to " + std::to_string(event.newValue);
            break;
    }
    
    return description;
}

// Check memory for changes and trigger notifications
static void checkForStateChanges(const std::string& gameId) {
    auto it = g_memoryMaps.find(gameId);
    if (it == g_memoryMaps.end() || !it->second.notificationsEnabled) {
        return;
    }
    
    GameMemoryMap& gameMap = it->second;
    bool gameStateChanged = false;
    std::vector<StateChangeEvent> events;
    
    for (auto& region : gameMap.regions) {
        if (!region.isWatched) {
            continue;
        }
        
        MemoryWatchConfig& watchConfig = region.watchConfig;
        float currentValue = getMemoryValueAsFloat(region);
        
        if (!watchConfig.initialized) {
            // Initialize on first check
            watchConfig.lastValue = currentValue;
            watchConfig.initialized = true;
            continue;
        }
        
        // Check if the change is significant
        if (isSignificantChange(watchConfig.lastValue, currentValue, watchConfig)) {
            // Create state change event
            StateChangeEvent event;
            event.type = watchConfig.changeType;
            event.regionName = region.name;
            event.oldValue = watchConfig.lastValue;
            event.newValue = currentValue;
            event.description = generateChangeDescription(event);
            event.userData = nullptr;
            
            // Log the event
            std::cout << "State change detected: " << event.description << std::endl;
            
            // Add to events list for batch processing
            events.push_back(event);
            
            // Mark game state as changed
            gameStateChanged = true;
            
            // Call game-specific callbacks
            for (const auto& callback : gameMap.callbacks) {
                callback(event);
            }
            
            // Call global callbacks
            if (g_notificationsEnabled) {
                for (const auto& callback : g_globalCallbacks) {
                    callback(event);
                }
            }
        }
        
        // Update last value
        watchConfig.lastValue = currentValue;
    }
    
    // If game state changed, notify GameStateChangeNotifier
    if (gameStateChanged) {
        GameStateChangeNotifier::getInstance()->processPendingNotifications();
    }
}

// Enhanced GameMemoryMapping class to support real-time notifications
class GameMemoryMapping {
private:
    std::string m_currentGameId;
    bool m_notificationsEnabled;
    GameStateChangeNotifier* m_notifier;
    
public:
    GameMemoryMapping() : m_notificationsEnabled(true) {
        m_notifier = GameStateChangeNotifier::getInstance();
    }
    
    ~GameMemoryMapping() {
        // Nothing to do here, don't destroy the global instance
    }
    
    void configureForGame(const std::string& gameId, int gameType) {
        m_currentGameId = gameId;
        
        // Register game if not already registered
        GameMemoryMapper::registerGame(gameId.c_str(), static_cast<GameType>(gameType));
        
        // Set up default watch configurations based on game type
        setupDefaultWatchConfig(gameId, static_cast<GameType>(gameType));
    }
    
    bool processMemory(const void* memoryData, size_t size) {
        if (m_currentGameId.empty() || !memoryData || size == 0) {
            return false;
        }
        
        // Process state changes
        bool result = GameMemoryMapper::processStateChanges(m_currentGameId.c_str());
        
        // Process any pending notifications
        if (m_notifier) {
            m_notifier->processPendingNotifications();
        }
        
        return result;
    }
    
    void setNotificationsEnabled(bool enabled) {
        m_notificationsEnabled = enabled;
        GameMemoryMapper::setNotificationsEnabled(enabled);
    }
    
    bool registerStateChangeListener(std::function<void(const std::string&, const std::vector<StateChangeEvent>&)> listener) {
        if (!m_notifier) {
            return false;
        }
        return m_notifier->registerStateChangeListener(listener);
    }
    
    bool registerSpecificChangeListener(StateChangeType changeType, std::function<void(const StateChangeEvent&)> listener) {
        if (!m_notifier) {
            return false;
        }
        return m_notifier->registerSpecificChangeListener(m_currentGameId, changeType, listener);
    }
    
    float getGameStateValue(const std::string& variableName) {
        if (!m_notifier) {
            return 0.0f;
        }
        return m_notifier->getLastKnownValue(m_currentGameId, variableName);
    }
    
    std::unordered_map<std::string, float> getCurrentGameState() {
        if (!m_notifier) {
            return {};
        }
        return m_notifier->getGameState(m_currentGameId);
    }
    
    bool isStateConditionMet(const std::string& variableName, float targetValue, bool approximate = false) {
        if (!m_notifier) {
            return false;
        }
        return m_notifier->checkGameStateCondition(m_currentGameId, variableName, targetValue, approximate);
    }
    
private:
    void setupDefaultWatchConfig(const std::string& gameId, GameType gameType) {
        // Set up default watch configurations based on game type
        switch (gameType) {
            case GAME_FIGHTING:
                GameMemoryMapper::configureMemoryWatch(gameId.c_str(), "p1_health", CHANGE_PLAYER_HEALTH, 5.0f, true);
                GameMemoryMapper::configureMemoryWatch(gameId.c_str(), "p2_health", CHANGE_PLAYER_HEALTH, 5.0f, true);
                GameMemoryMapper::configureMemoryWatch(gameId.c_str(), "round", CHANGE_ROUND, 0.0f, false);
                GameMemoryMapper::configureMemoryWatch(gameId.c_str(), "timer", CHANGE_TIME, 5.0f, false);
                break;
                
            case GAME_PLATFORMER:
                GameMemoryMapper::configureMemoryWatch(gameId.c_str(), "player_x", CHANGE_PLAYER_POSITION, 20.0f, false);
                GameMemoryMapper::configureMemoryWatch(gameId.c_str(), "player_y", CHANGE_PLAYER_POSITION, 20.0f, false);
                GameMemoryMapper::configureMemoryWatch(gameId.c_str(), "lives", CHANGE_PLAYER_HEALTH, 0.0f, false);
                break;
                
            case GAME_PUZZLE:
                GameMemoryMapper::configureMemoryWatch(gameId.c_str(), "score", CHANGE_SCORE, 100.0f, false);
                GameMemoryMapper::configureMemoryWatch(gameId.c_str(), "level", CHANGE_LEVEL, 0.0f, false);
                break;
                
            case GAME_SHOOTER:
                GameMemoryMapper::configureMemoryWatch(gameId.c_str(), "lives", CHANGE_PLAYER_HEALTH, 0.0f, false);
                GameMemoryMapper::configureMemoryWatch(gameId.c_str(), "score", CHANGE_SCORE, 100.0f, false);
                break;
                
            case GAME_RACING:
                GameMemoryMapper::configureMemoryWatch(gameId.c_str(), "position", CHANGE_PLAYER_POSITION, 0.0f, false);
                GameMemoryMapper::configureMemoryWatch(gameId.c_str(), "lap", CHANGE_LEVEL, 0.0f, false);
                break;
                
            default:
                break;
        }
    }
};

// Extend C API for game state notifications
extern "C" {
    // Type definitions
    typedef void (*FBNEO_StateChangeBatchCallback)(const char* gameId, void* eventArray, int eventCount, void* userData);
    typedef void (*FBNEO_StateChangeEventCallback)(int changeType, const char* regionName, float oldValue, float newValue, const char* description, void* userData);
    
    typedef void* FBNEO_StateChangeListener;
    
    // Register for batch game state change notifications
    FBNEO_StateChangeListener FBNEO_MemoryMapper_RegisterStateChangeListener(
        FBNEO_StateChangeBatchCallback callback, void* userData) {
        
        if (!callback) {
            return nullptr;
        }
        
        GameStateChangeNotifier* notifier = GameStateChangeNotifier::getInstance();
        if (!notifier) {
            return nullptr;
        }
        
        // Create a wrapper function that adapts our C++ callback to the C callback
        auto listenerWrapper = [callback, userData](const std::string& gameId, const std::vector<StateChangeEvent>& events) {
            // Call the C callback directly with the event array
            callback(gameId.c_str(), const_cast<void*>(static_cast<const void*>(events.data())), events.size(), userData);
        };
        
        // Register the wrapper with our C++ notifier
        bool success = notifier->registerStateChangeListener(listenerWrapper);
        
        // Return a handle to the listener (just use the function pointer as a handle)
        return success ? reinterpret_cast<FBNEO_StateChangeListener>(callback) : nullptr;
    }
    
    // Register for specific state change notifications
    FBNEO_StateChangeListener FBNEO_MemoryMapper_RegisterSpecificChangeListener(
        const char* gameId, int changeType, FBNEO_StateChangeEventCallback callback, void* userData) {
        
        if (!callback) {
            return nullptr;
        }
        
        GameStateChangeNotifier* notifier = GameStateChangeNotifier::getInstance();
        if (!notifier) {
            return nullptr;
        }
        
        // Create a wrapper function that adapts our C++ callback to the C callback
        auto listenerWrapper = [callback, userData](const StateChangeEvent& event) {
            callback(
                static_cast<int>(event.type),
                event.regionName.c_str(),
                event.oldValue,
                event.newValue,
                event.description.c_str(),
                userData
            );
        };
        
        // Register the wrapper with our C++ notifier
        bool success = notifier->registerSpecificChangeListener(
            gameId ? gameId : "", 
            static_cast<StateChangeType>(changeType),
            listenerWrapper
        );
        
        // Return a handle to the listener (just use the function pointer as a handle)
        return success ? reinterpret_cast<FBNEO_StateChangeListener>(callback) : nullptr;
    }
    
    // Get the last known value for a game state variable
    float FBNEO_MemoryMapper_GetGameStateValue(const char* gameId, const char* variableName) {
        if (!gameId || !variableName) {
            return 0.0f;
        }
        
        GameStateChangeNotifier* notifier = GameStateChangeNotifier::getInstance();
        if (!notifier) {
            return 0.0f;
        }
        
        return notifier->getLastKnownValue(gameId, variableName);
    }
    
    // Check if a game state condition is met
    int FBNEO_MemoryMapper_CheckGameStateCondition(const char* gameId, const char* variableName, 
                                                float targetValue, int approximate) {
        if (!gameId || !variableName) {
            return 0;
        }
        
        GameStateChangeNotifier* notifier = GameStateChangeNotifier::getInstance();
        if (!notifier) {
            return 0;
        }
        
        return notifier->checkGameStateCondition(
            gameId, variableName, targetValue, approximate != 0) ? 1 : 0;
    }
    
    // Initialize the state change notifier
    int FBNEO_MemoryMapper_InitStateChangeNotifier() {
        GameStateChangeNotifier::getInstance();
        return 1;
    }
    
    // Clean up the state change notifier
    void FBNEO_MemoryMapper_CleanupStateChangeNotifier() {
        GameStateChangeNotifier::destroyInstance();
    }
}

} // namespace ai
} // namespace fbneo 