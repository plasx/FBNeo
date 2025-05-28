#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>

// FBNeo core includes
#include "burner.h"
#include "burnint.h"

// Metal-specific includes
#include "../metal_declarations.h"
#include "../metal_bridge.h"
#include "../metal_ai.h"

// Memory system namespace
namespace FBNeoMemory {
    // Memory hook types
    enum HookType {
        HOOK_READ8,
        HOOK_READ16,
        HOOK_READ32,
        HOOK_WRITE8,
        HOOK_WRITE16,
        HOOK_WRITE32
    };
    
    // Memory hook function signature
    typedef void (*MemoryHookFn)(uint32_t address, uint32_t value, void* userData);
    
    // Memory hook structure
    struct MemoryHook {
        uint32_t address;        // Memory address
        uint32_t mask;           // Address mask (for ranges)
        HookType type;           // Hook type
        MemoryHookFn callback;   // Callback function
        void* userData;          // User data passed to callback
        bool enabled;            // Whether hook is enabled
    };
    
    // Game memory map structure
    struct MemoryMap {
        std::string gameId;                  // Game ID
        std::vector<MemoryHook> hooks;       // Memory hooks
        std::unordered_map<std::string, uint32_t> namedAddresses;  // Named memory locations
    };
    
    // Memory system state
    static bool initialized = false;
    static std::mutex hookMutex;
    static MemoryMap currentMap;
    
    // Forward declarations
    bool LoadMemoryMap(const std::string& gameId);
    void ResetMemoryMap();
}

// Reset memory map
void FBNeoMemory::ResetMemoryMap() {
    std::lock_guard<std::mutex> lock(hookMutex);
    
    currentMap.hooks.clear();
    currentMap.namedAddresses.clear();
    currentMap.gameId = "";
}

// Load memory map for a specific game
bool FBNeoMemory::LoadMemoryMap(const std::string& gameId) {
    std::lock_guard<std::mutex> lock(hookMutex);
    
    // Reset current map
    ResetMemoryMap();
    
    // Set new game ID
    currentMap.gameId = gameId;
    
    // Marvel vs. Capcom specific memory map
    if (gameId == "mvsc") {
        // Player 1 health
        currentMap.namedAddresses["p1_health"] = 0xFF8451;
        
        // Player 2 health
        currentMap.namedAddresses["p2_health"] = 0xFF8851;
        
        // Round timer
        currentMap.namedAddresses["timer"] = 0xFF8109;
        
        // P1 combo counter
        currentMap.namedAddresses["p1_combo"] = 0xFF84D0;
        
        // P2 combo counter
        currentMap.namedAddresses["p2_combo"] = 0xFF88D0;
        
        // Game state (0=attract, 1=title, 2=character select, 3=fighting, etc.)
        currentMap.namedAddresses["game_state"] = 0xFF810A;
        
        printf("Loaded memory map for Marvel vs. Capcom\n");
        return true;
    }
    
    // Street Fighter Alpha 3 specific memory map
    else if (gameId == "sfa3") {
        // Player 1 health
        currentMap.namedAddresses["p1_health"] = 0xFF8400;
        
        // Player 2 health
        currentMap.namedAddresses["p2_health"] = 0xFF8800;
        
        // Round timer
        currentMap.namedAddresses["timer"] = 0xFF8120;
        
        printf("Loaded memory map for Street Fighter Alpha 3\n");
        return true;
    }
    
    // X-Men vs. Street Fighter specific memory map
    else if (gameId == "xmvsf") {
        // Player 1 health
        currentMap.namedAddresses["p1_health"] = 0xFF8438;
        
        // Player 2 health
        currentMap.namedAddresses["p2_health"] = 0xFF8838;
        
        // Round timer
        currentMap.namedAddresses["timer"] = 0xFF8100;
        
        printf("Loaded memory map for X-Men vs. Street Fighter\n");
        return true;
    }
    
    // Super Street Fighter II Turbo specific memory map
    else if (gameId == "ssf2t") {
        // Player 1 health
        currentMap.namedAddresses["p1_health"] = 0xFF83FE;
        
        // Player 2 health
        currentMap.namedAddresses["p2_health"] = 0xFF87FE;
        
        // Round timer
        currentMap.namedAddresses["timer"] = 0xFF8802;
        
        printf("Loaded memory map for Super Street Fighter II Turbo\n");
        return true;
    }
    
    // Vampire Savior specific memory map
    else if (gameId == "vsav") {
        // Player 1 health
        currentMap.namedAddresses["p1_health"] = 0xFF8454;
        
        // Player 2 health
        currentMap.namedAddresses["p2_health"] = 0xFF8854;
        
        // Round timer
        currentMap.namedAddresses["timer"] = 0xFF8103;
        
        printf("Loaded memory map for Vampire Savior\n");
        return true;
    }
    
    // No specific map found
    printf("No specific memory map found for %s, using generic map\n", gameId.c_str());
    return false;
}

// Initialize memory hooks system
bool Memory_InitHooks() {
    if (FBNeoMemory::initialized) {
        printf("Memory_InitHooks: Memory hooks already initialized\n");
        return true;
    }
    
    printf("Memory_InitHooks: Initializing memory hooks system\n");
    
    // Reset the memory map
    FBNeoMemory::ResetMemoryMap();
    
    // Mark as initialized
    FBNeoMemory::initialized = true;
    printf("Memory_InitHooks: Memory hooks initialized successfully\n");
    
    return true;
}

// Shutdown memory hooks system
void Memory_ShutdownHooks() {
    if (!FBNeoMemory::initialized) {
        return;
    }
    
    printf("Memory_ShutdownHooks: Shutting down memory hooks system\n");
    
    // Reset memory map
    FBNeoMemory::ResetMemoryMap();
    
    // Mark as uninitialized
    FBNeoMemory::initialized = false;
    
    printf("Memory_ShutdownHooks: Memory hooks shut down\n");
}

// Load memory map for a game
bool Memory_LoadMap(const char* gameId) {
    if (!FBNeoMemory::initialized || !gameId) {
        return false;
    }
    
    return FBNeoMemory::LoadMemoryMap(gameId);
}

// Add a memory hook
int Memory_AddHook(uint32_t address, uint32_t mask, int type, void (*callback)(uint32_t, uint32_t, void*), void* userData) {
    if (!FBNeoMemory::initialized || !callback) {
        return -1;
    }
    
    std::lock_guard<std::mutex> lock(FBNeoMemory::hookMutex);
    
    // Create a new hook
    FBNeoMemory::MemoryHook hook;
    hook.address = address;
    hook.mask = mask;
    hook.type = static_cast<FBNeoMemory::HookType>(type);
    hook.callback = callback;
    hook.userData = userData;
    hook.enabled = true;
    
    // Add to current map
    FBNeoMemory::currentMap.hooks.push_back(hook);
    
    // Return hook index
    return FBNeoMemory::currentMap.hooks.size() - 1;
}

// Remove a memory hook
bool Memory_RemoveHook(int hookId) {
    if (!FBNeoMemory::initialized) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(FBNeoMemory::hookMutex);
    
    if (hookId < 0 || hookId >= static_cast<int>(FBNeoMemory::currentMap.hooks.size())) {
        return false;
    }
    
    // Disable hook
    FBNeoMemory::currentMap.hooks[hookId].enabled = false;
    
    return true;
}

// Get address by name
uint32_t Memory_GetAddressByName(const char* name) {
    if (!FBNeoMemory::initialized || !name) {
        return 0;
    }
    
    std::lock_guard<std::mutex> lock(FBNeoMemory::hookMutex);
    
    auto it = FBNeoMemory::currentMap.namedAddresses.find(name);
    if (it != FBNeoMemory::currentMap.namedAddresses.end()) {
        return it->second;
    }
    
    return 0;
}

// Read memory (8-bit)
uint8_t Memory_ReadByte(uint32_t address) {
    // Access FBNeo memory space directly
    if (address < 0x1000000) { // 16MB address space
        return ZetReadByte(address);
    }
    return 0;
}

// Read memory (16-bit)
uint16_t Memory_ReadWord(uint32_t address) {
    // Access FBNeo memory space directly
    if (address < 0x1000000) { // 16MB address space
        return ZetReadWord(address);
    }
    return 0;
}

// Read memory (32-bit)
uint32_t Memory_ReadLong(uint32_t address) {
    // Access FBNeo memory space directly
    if (address < 0x1000000) { // 16MB address space
        return ZetReadLong(address);
    }
    return 0;
}

// Write memory (8-bit)
void Memory_WriteByte(uint32_t address, uint8_t value) {
    // Access FBNeo memory space directly
    if (address < 0x1000000) { // 16MB address space
        ZetWriteByte(address, value);
    }
}

// Write memory (16-bit)
void Memory_WriteWord(uint32_t address, uint16_t value) {
    // Access FBNeo memory space directly
    if (address < 0x1000000) { // 16MB address space
        ZetWriteWord(address, value);
    }
}

// Write memory (32-bit)
void Memory_WriteLong(uint32_t address, uint32_t value) {
    // Access FBNeo memory space directly
    if (address < 0x1000000) { // 16MB address space
        ZetWriteLong(address, value);
    }
} 