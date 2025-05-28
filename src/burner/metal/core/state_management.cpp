#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>

// FBNeo core includes
#include "burner.h"
#include "burnint.h"

// Metal-specific includes
#include "../metal_declarations.h"
#include "../metal_bridge.h"

// State management namespace
namespace FBNeoState {
    // Constants
    const int MAX_STATE_SIZE = 1024 * 1024 * 8;  // 8MB max state size
    
    // State system tracking
    static bool initialized = false;
    static std::string stateDir = "./states";
    static std::string currentGame = "";
    
    // State buffer
    static std::vector<uint8_t> stateBuffer;
    
    // Forward declarations
    bool EnsureStateDirectory();
    std::string GetStateFilename(int slot);
}

// Ensure the state directory exists
bool FBNeoState::EnsureStateDirectory() {
    struct stat st;
    
    // Check if directory exists
    if (stat(stateDir.c_str(), &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            return true;
        }
    }
    
    // Create directory
    int result = mkdir(stateDir.c_str(), 0755);
    if (result != 0) {
        printf("FBNeoState::EnsureStateDirectory: Failed to create state directory: %s\n", stateDir.c_str());
        return false;
    }
    
    printf("FBNeoState::EnsureStateDirectory: Created state directory: %s\n", stateDir.c_str());
    return true;
}

// Get state filename for a slot
std::string FBNeoState::GetStateFilename(int slot) {
    char filename[256];
    snprintf(filename, sizeof(filename), "%s/%s_%02d.fs", stateDir.c_str(), currentGame.c_str(), slot);
    return filename;
}

// Initialize state management system
bool State_Init() {
    if (FBNeoState::initialized) {
        printf("State_Init: State system already initialized\n");
        return true;
    }
    
    printf("State_Init: Initializing state management system\n");
    
    // Initialize state buffer
    FBNeoState::stateBuffer.resize(FBNeoState::MAX_STATE_SIZE);
    
    // Ensure state directory exists
    if (!FBNeoState::EnsureStateDirectory()) {
        printf("State_Init: Failed to initialize state directory\n");
        return false;
    }
    
    // Mark as initialized
    FBNeoState::initialized = true;
    printf("State_Init: State management system initialized successfully\n");
    
    return true;
}

// Shutdown state management system
void State_Shutdown() {
    if (!FBNeoState::initialized) {
        return;
    }
    
    printf("State_Shutdown: Shutting down state management system\n");
    
    // Clear state buffer
    FBNeoState::stateBuffer.clear();
    
    // Mark as uninitialized
    FBNeoState::initialized = false;
    
    printf("State_Shutdown: State management system shut down\n");
}

// Set current game for state management
void State_SetGame(const char* gameId) {
    if (!FBNeoState::initialized || !gameId) {
        return;
    }
    
    FBNeoState::currentGame = gameId;
    printf("State_SetGame: Set current game to %s\n", gameId);
}

// Save state to a slot
bool State_Save(int slot) {
    if (!FBNeoState::initialized || FBNeoState::currentGame.empty()) {
        return false;
    }
    
    printf("State_Save: Saving state to slot %d\n", slot);
    
    // Get state filename
    std::string filename = FBNeoState::GetStateFilename(slot);
    
    // Call FBNeo save state function
    int size = FBNeoState::MAX_STATE_SIZE;
    int result = BurnStateSave(FBNeoState::stateBuffer.data(), &size);
    
    if (result != 0 || size <= 0) {
        printf("State_Save: Failed to save state (error code: %d)\n", result);
        return false;
    }
    
    // Write state file
    std::ofstream file(filename.c_str(), std::ios::binary);
    if (!file.is_open()) {
        printf("State_Save: Failed to open state file for writing: %s\n", filename.c_str());
        return false;
    }
    
    file.write(reinterpret_cast<const char*>(FBNeoState::stateBuffer.data()), size);
    file.close();
    
    printf("State_Save: Successfully saved state to %s (%d bytes)\n", filename.c_str(), size);
    return true;
}

// Load state from a slot
bool State_Load(int slot) {
    if (!FBNeoState::initialized || FBNeoState::currentGame.empty()) {
        return false;
    }
    
    printf("State_Load: Loading state from slot %d\n", slot);
    
    // Get state filename
    std::string filename = FBNeoState::GetStateFilename(slot);
    
    // Check if file exists
    std::ifstream checkFile(filename.c_str(), std::ios::binary);
    if (!checkFile.is_open()) {
        printf("State_Load: State file not found: %s\n", filename.c_str());
        return false;
    }
    
    // Get file size
    checkFile.seekg(0, std::ios::end);
    int fileSize = static_cast<int>(checkFile.tellg());
    checkFile.seekg(0, std::ios::beg);
    
    if (fileSize <= 0 || fileSize > FBNeoState::MAX_STATE_SIZE) {
        printf("State_Load: Invalid state file size: %d\n", fileSize);
        return false;
    }
    
    // Read the file
    checkFile.read(reinterpret_cast<char*>(FBNeoState::stateBuffer.data()), fileSize);
    checkFile.close();
    
    // Call FBNeo load state function
    int size = fileSize;
    int result = BurnStateLoad(FBNeoState::stateBuffer.data(), size);
    
    if (result != 0) {
        printf("State_Load: Failed to load state (error code: %d)\n", result);
        return false;
    }
    
    printf("State_Load: Successfully loaded state from %s (%d bytes)\n", filename.c_str(), size);
    return true;
}

// Check if a state exists in a slot
bool State_Exists(int slot) {
    if (!FBNeoState::initialized || FBNeoState::currentGame.empty()) {
        return false;
    }
    
    // Get state filename
    std::string filename = FBNeoState::GetStateFilename(slot);
    
    // Check if file exists
    std::ifstream file(filename.c_str(), std::ios::binary);
    bool exists = file.is_open();
    file.close();
    
    return exists;
}

// Delete a state from a slot
bool State_Delete(int slot) {
    if (!FBNeoState::initialized || FBNeoState::currentGame.empty()) {
        return false;
    }
    
    printf("State_Delete: Deleting state from slot %d\n", slot);
    
    // Get state filename
    std::string filename = FBNeoState::GetStateFilename(slot);
    
    // Check if file exists
    if (!State_Exists(slot)) {
        printf("State_Delete: State file not found: %s\n", filename.c_str());
        return false;
    }
    
    // Delete the file
    if (unlink(filename.c_str()) != 0) {
        printf("State_Delete: Failed to delete state file: %s\n", filename.c_str());
        return false;
    }
    
    printf("State_Delete: Successfully deleted state file: %s\n", filename.c_str());
    return true;
} 