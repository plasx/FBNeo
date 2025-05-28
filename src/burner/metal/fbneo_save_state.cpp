#include "burner.h"
#include "burnint.h"
#include "metal_declarations.h"
#include "metal_bridge.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <algorithm>
#include <map>
#include <time.h>

// Enhanced save state management for Metal implementation

// Define save state structure
typedef struct {
    char gameId[32];
    char description[256];
    char timestamp[32];
    char version[32];
    int dataSize;
    int thumbnailWidth;
    int thumbnailHeight;
    int thumbnailSize;
} SaveStateHeader;

// Save state container
typedef struct {
    SaveStateHeader header;
    void* data;
    void* thumbnail;
    bool isValid;
} SaveState;

// Global variables
static std::string g_saveStatePath;
static std::vector<SaveState> g_saveStates;
static bool g_saveStateInitialized = false;
static bool g_autoStateEnabled = true;
static int g_maxSaveSlots = 10;
static int g_currentSaveSlot = 0;
static int g_autoSaveInterval = 300; // 5 minutes in seconds
static time_t g_lastAutoSaveTime = 0;

// Forward declarations
static bool InitializeSaveStatePath();
static bool LoadSaveStateList();
static std::string GenerateSaveStateFilename(int slot);
static std::string FormatTimestamp();
static bool CreateSaveStateThumbnail(void* buffer, int width, int height, int* outWidth, int* outHeight, void** outData, int* outSize);

// Initialize the save state system
bool FBNeo_SaveState_Initialize() {
    if (g_saveStateInitialized) {
        return true;
    }
    
    // Initialize save state path
    if (!InitializeSaveStatePath()) {
        printf("Failed to initialize save state path\n");
        return false;
    }
    
    // Load save state list
    if (!LoadSaveStateList()) {
        printf("Failed to load save state list\n");
        return false;
    }
    
    g_saveStateInitialized = true;
    g_lastAutoSaveTime = time(nullptr);
    
    printf("Save state system initialized (path: %s)\n", g_saveStatePath.c_str());
    
    return true;
}

// Initialize save state directory
static bool InitializeSaveStatePath() {
    // Get home directory
    const char* homeDir = getenv("HOME");
    if (!homeDir) {
        printf("Could not get home directory\n");
        return false;
    }
    
    // Create save state directory path
    g_saveStatePath = std::string(homeDir) + "/Library/Application Support/FBNeo/savestates";
    
    // Create directory if it doesn't exist
    struct stat st;
    if (stat(g_saveStatePath.c_str(), &st) != 0) {
        printf("Creating save state directory: %s\n", g_saveStatePath.c_str());
        
        std::string command = "mkdir -p \"" + g_saveStatePath + "\"";
        if (system(command.c_str()) != 0) {
            printf("Failed to create save state directory\n");
            return false;
        }
    }
    
    return true;
}

// Load the list of available save states
static bool LoadSaveStateList() {
    // Clear existing save states
    g_saveStates.clear();
    
    // Open save state directory
    DIR* dir = opendir(g_saveStatePath.c_str());
    if (!dir) {
        printf("Could not open save state directory: %s\n", g_saveStatePath.c_str());
        return false;
    }
    
    // Read directory entries
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Check if file is a save state
        std::string filename = entry->d_name;
        if (filename.find(".fs") == std::string::npos) {
            continue;
        }
        
        // Open save state file
        std::string filepath = g_saveStatePath + "/" + filename;
        FILE* file = fopen(filepath.c_str(), "rb");
        if (!file) {
            printf("Could not open save state file: %s\n", filepath.c_str());
            continue;
        }
        
        // Read header
        SaveStateHeader header;
        if (fread(&header, sizeof(header), 1, file) != 1) {
            printf("Could not read save state header: %s\n", filepath.c_str());
            fclose(file);
            continue;
        }
        
        // Create save state
        SaveState state;
        state.header = header;
        state.data = nullptr;
        state.thumbnail = nullptr;
        state.isValid = true;
        
        // Add to list
        g_saveStates.push_back(state);
        
        fclose(file);
    }
    
    closedir(dir);
    
    printf("Loaded %zu save states\n", g_saveStates.size());
    
    return true;
}

// Generate filename for a save state
static std::string GenerateSaveStateFilename(int slot) {
    char filename[256];
    
    // Get game ID
    const char* gameId = BurnDrvGetTextA(DRV_NAME);
    
    // Create filename
    if (slot >= 0) {
        snprintf(filename, sizeof(filename), "%s_slot%d.fs", gameId, slot);
    } else {
        // Auto save
        snprintf(filename, sizeof(filename), "%s_auto.fs", gameId);
    }
    
    return g_saveStatePath + "/" + filename;
}

// Format current timestamp
static std::string FormatTimestamp() {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    return std::string(buffer);
}

// Create a thumbnail from the current screen
static bool CreateSaveStateThumbnail(void* buffer, int width, int height, int* outWidth, int* outHeight, void** outData, int* outSize) {
    if (!buffer || !outWidth || !outHeight || !outData || !outSize) {
        return false;
    }
    
    // Simple thumbnail creation - just downsample to 160x120
    int thumbWidth = 160;
    int thumbHeight = 120;
    
    // Adjust to maintain aspect ratio
    float aspectRatio = (float)width / (float)height;
    if (aspectRatio > 4.0f / 3.0f) {
        thumbHeight = (int)(thumbWidth / aspectRatio);
    } else {
        thumbWidth = (int)(thumbHeight * aspectRatio);
    }
    
    // Allocate thumbnail buffer
    int thumbSize = thumbWidth * thumbHeight * 4;  // RGBA
    void* thumbData = malloc(thumbSize);
    if (!thumbData) {
        printf("Failed to allocate thumbnail buffer\n");
        return false;
    }
    
    // Downsample the image (simple nearest neighbor algorithm)
    uint8_t* src = (uint8_t*)buffer;
    uint8_t* dst = (uint8_t*)thumbData;
    
    float xRatio = width / (float)thumbWidth;
    float yRatio = height / (float)thumbHeight;
    
    for (int y = 0; y < thumbHeight; y++) {
        for (int x = 0; x < thumbWidth; x++) {
            int srcX = (int)(x * xRatio);
            int srcY = (int)(y * yRatio);
            int srcOffset = (srcY * width + srcX) * 4;
            int dstOffset = (y * thumbWidth + x) * 4;
            
            // Copy RGBA values
            dst[dstOffset] = src[srcOffset];
            dst[dstOffset + 1] = src[srcOffset + 1];
            dst[dstOffset + 2] = src[srcOffset + 2];
            dst[dstOffset + 3] = src[srcOffset + 3];
        }
    }
    
    // Set output values
    *outWidth = thumbWidth;
    *outHeight = thumbHeight;
    *outData = thumbData;
    *outSize = thumbSize;
    
    return true;
}

// Save current game state to a save state file
bool FBNeo_SaveState_Save(int slot, const char* description) {
    // Check if a game is running
    if (BurnDrvGetTextA(DRV_NAME) == nullptr) {
        printf("No game is running\n");
        return false;
    }
    
    // Make sure save state system is initialized
    if (!g_saveStateInitialized) {
        FBNeo_SaveState_Initialize();
    }
    
    // Use current slot if not specified
    if (slot < 0) {
        slot = g_currentSaveSlot;
    }
    
    // Update current slot
    g_currentSaveSlot = slot;
    
    // Generate filename
    std::string filename = GenerateSaveStateFilename(slot);
    
    // Open file for writing
    FILE* file = fopen(filename.c_str(), "wb");
    if (!file) {
        printf("Could not create save state file: %s\n", filename.c_str());
        return false;
    }
    
    // Create header
    SaveStateHeader header;
    memset(&header, 0, sizeof(header));
    
    // Fill header
    strncpy(header.gameId, BurnDrvGetTextA(DRV_NAME), sizeof(header.gameId) - 1);
    strncpy(header.description, description ? description : "Manual save", sizeof(header.description) - 1);
    strncpy(header.timestamp, FormatTimestamp().c_str(), sizeof(header.timestamp) - 1);
    strncpy(header.version, "1.0", sizeof(header.version) - 1);
    
    // Create thumbnail from current screen
    void* thumbnailData = nullptr;
    int thumbnailWidth = 0;
    int thumbnailHeight = 0;
    int thumbnailSize = 0;
    
    bool hasThumbnail = false;
    if (pBurnDraw) {
        hasThumbnail = CreateSaveStateThumbnail(
            pBurnDraw,
            BurnDrvInfo.nWidth,
            BurnDrvInfo.nHeight,
            &thumbnailWidth,
            &thumbnailHeight,
            &thumbnailData,
            &thumbnailSize
        );
    }
    
    // Update header with thumbnail info
    if (hasThumbnail) {
        header.thumbnailWidth = thumbnailWidth;
        header.thumbnailHeight = thumbnailHeight;
        header.thumbnailSize = thumbnailSize;
    } else {
        header.thumbnailWidth = 0;
        header.thumbnailHeight = 0;
        header.thumbnailSize = 0;
    }
    
    // Get state data size
    int stateSize = 0;
    BurnAcb.GetState(nullptr, &stateSize);
    
    if (stateSize <= 0) {
        printf("Failed to get state size\n");
        fclose(file);
        if (thumbnailData) free(thumbnailData);
        return false;
    }
    
    // Allocate memory for state data
    void* stateData = malloc(stateSize);
    if (!stateData) {
        printf("Failed to allocate memory for state data\n");
        fclose(file);
        if (thumbnailData) free(thumbnailData);
        return false;
    }
    
    // Get state data
    BurnAcb.GetState(stateData, &stateSize);
    
    // Update header with state data size
    header.dataSize = stateSize;
    
    // Write header
    if (fwrite(&header, sizeof(header), 1, file) != 1) {
        printf("Failed to write save state header\n");
        free(stateData);
        fclose(file);
        if (thumbnailData) free(thumbnailData);
        return false;
    }
    
    // Write state data
    if (fwrite(stateData, stateSize, 1, file) != 1) {
        printf("Failed to write save state data\n");
        free(stateData);
        fclose(file);
        if (thumbnailData) free(thumbnailData);
        return false;
    }
    
    // Write thumbnail if available
    if (hasThumbnail && thumbnailData) {
        if (fwrite(thumbnailData, thumbnailSize, 1, file) != 1) {
            printf("Failed to write thumbnail data\n");
            free(stateData);
            fclose(file);
            free(thumbnailData);
            return false;
        }
    }
    
    // Clean up
    free(stateData);
    if (thumbnailData) free(thumbnailData);
    fclose(file);
    
    printf("Saved state to slot %d: %s\n", slot, filename.c_str());
    
    // Update save state list
    LoadSaveStateList();
    
    return true;
}

// Load a save state from a file
bool FBNeo_SaveState_Load(int slot) {
    // Check if a game is running
    if (BurnDrvGetTextA(DRV_NAME) == nullptr) {
        printf("No game is running\n");
        return false;
    }
    
    // Make sure save state system is initialized
    if (!g_saveStateInitialized) {
        FBNeo_SaveState_Initialize();
    }
    
    // Use current slot if not specified
    if (slot < 0) {
        slot = g_currentSaveSlot;
    }
    
    // Update current slot
    g_currentSaveSlot = slot;
    
    // Generate filename
    std::string filename = GenerateSaveStateFilename(slot);
    
    // Open file for reading
    FILE* file = fopen(filename.c_str(), "rb");
    if (!file) {
        printf("Could not open save state file: %s\n", filename.c_str());
        return false;
    }
    
    // Read header
    SaveStateHeader header;
    if (fread(&header, sizeof(header), 1, file) != 1) {
        printf("Could not read save state header: %s\n", filename.c_str());
        fclose(file);
        return false;
    }
    
    // Check if this save state is for the current game
    if (strcmp(header.gameId, BurnDrvGetTextA(DRV_NAME)) != 0) {
        printf("Save state is for a different game: %s (current: %s)\n", 
               header.gameId, BurnDrvGetTextA(DRV_NAME));
        fclose(file);
        return false;
    }
    
    // Check if state data size is valid
    if (header.dataSize <= 0) {
        printf("Invalid state data size: %d\n", header.dataSize);
        fclose(file);
        return false;
    }
    
    // Allocate memory for state data
    void* stateData = malloc(header.dataSize);
    if (!stateData) {
        printf("Failed to allocate memory for state data\n");
        fclose(file);
        return false;
    }
    
    // Read state data
    if (fread(stateData, header.dataSize, 1, file) != 1) {
        printf("Failed to read save state data\n");
        free(stateData);
        fclose(file);
        return false;
    }
    
    // Apply state data
    int result = BurnAcb.SetState(stateData, header.dataSize);
    
    // Clean up
    free(stateData);
    fclose(file);
    
    if (result != 0) {
        printf("Failed to apply save state data (error code: %d)\n", result);
        return false;
    }
    
    printf("Loaded state from slot %d: %s\n", slot, filename.c_str());
    
    return true;
}

// Auto-save state management
void FBNeo_SaveState_AutoSave() {
    // Check if auto-save is enabled
    if (!g_autoStateEnabled) {
        return;
    }
    
    // Check if a game is running
    if (BurnDrvGetTextA(DRV_NAME) == nullptr) {
        return;
    }
    
    // Check if it's time for auto-save
    time_t now = time(nullptr);
    if (now - g_lastAutoSaveTime < g_autoSaveInterval) {
        return;
    }
    
    // Save state
    FBNeo_SaveState_Save(-1, "Auto save");
    
    // Update last auto-save time
    g_lastAutoSaveTime = now;
}

// Enable/disable auto-save
void FBNeo_SaveState_SetAutoSave(bool enable) {
    g_autoStateEnabled = enable;
    printf("Auto-save %s\n", enable ? "enabled" : "disabled");
}

// Set auto-save interval in seconds
void FBNeo_SaveState_SetAutoSaveInterval(int seconds) {
    if (seconds < 10) {
        seconds = 10;  // Minimum 10 seconds
    }
    
    g_autoSaveInterval = seconds;
    printf("Auto-save interval set to %d seconds\n", seconds);
}

// Get the list of save states for the current game
int FBNeo_SaveState_GetStateList(SaveStateHeader** headers, int maxCount) {
    // Make sure save state system is initialized
    if (!g_saveStateInitialized) {
        FBNeo_SaveState_Initialize();
    }
    
    // Check if a game is running
    if (BurnDrvGetTextA(DRV_NAME) == nullptr) {
        return 0;
    }
    
    // Get game ID
    const char* gameId = BurnDrvGetTextA(DRV_NAME);
    
    // Count matching save states
    int count = 0;
    for (const auto& state : g_saveStates) {
        if (strcmp(state.header.gameId, gameId) == 0) {
            count++;
        }
    }
    
    // Allocate memory for headers if requested
    if (headers && maxCount > 0) {
        int index = 0;
        for (const auto& state : g_saveStates) {
            if (strcmp(state.header.gameId, gameId) == 0 && index < maxCount) {
                headers[index] = new SaveStateHeader;
                *headers[index] = state.header;
                index++;
            }
        }
    }
    
    return count;
}

// Get the current save slot
int FBNeo_SaveState_GetCurrentSlot() {
    return g_currentSaveSlot;
}

// Set the current save slot
void FBNeo_SaveState_SetCurrentSlot(int slot) {
    if (slot >= 0 && slot < g_maxSaveSlots) {
        g_currentSaveSlot = slot;
        printf("Current save slot set to %d\n", slot);
    }
}

// Get maximum number of save slots
int FBNeo_SaveState_GetMaxSlots() {
    return g_maxSaveSlots;
}

// Set maximum number of save slots
void FBNeo_SaveState_SetMaxSlots(int maxSlots) {
    if (maxSlots > 0 && maxSlots <= 100) {
        g_maxSaveSlots = maxSlots;
        printf("Maximum save slots set to %d\n", maxSlots);
    }
}

// Helpers for Metal layer integration

// Get a thumbnail from a save state file
bool FBNeo_SaveState_GetThumbnail(int slot, void** thumbnailData, int* width, int* height, int* size) {
    if (!thumbnailData || !width || !height || !size) {
        return false;
    }
    
    // Make sure save state system is initialized
    if (!g_saveStateInitialized) {
        FBNeo_SaveState_Initialize();
    }
    
    // Check if a game is running
    if (BurnDrvGetTextA(DRV_NAME) == nullptr) {
        return false;
    }
    
    // Generate filename
    std::string filename = GenerateSaveStateFilename(slot);
    
    // Open file for reading
    FILE* file = fopen(filename.c_str(), "rb");
    if (!file) {
        printf("Could not open save state file: %s\n", filename.c_str());
        return false;
    }
    
    // Read header
    SaveStateHeader header;
    if (fread(&header, sizeof(header), 1, file) != 1) {
        printf("Could not read save state header: %s\n", filename.c_str());
        fclose(file);
        return false;
    }
    
    // Check if thumbnail is available
    if (header.thumbnailSize <= 0) {
        printf("No thumbnail available in save state\n");
        fclose(file);
        return false;
    }
    
    // Seek to thumbnail data
    if (fseek(file, sizeof(header) + header.dataSize, SEEK_SET) != 0) {
        printf("Failed to seek to thumbnail data\n");
        fclose(file);
        return false;
    }
    
    // Allocate memory for thumbnail
    void* data = malloc(header.thumbnailSize);
    if (!data) {
        printf("Failed to allocate memory for thumbnail\n");
        fclose(file);
        return false;
    }
    
    // Read thumbnail data
    if (fread(data, header.thumbnailSize, 1, file) != 1) {
        printf("Failed to read thumbnail data\n");
        free(data);
        fclose(file);
        return false;
    }
    
    // Close file
    fclose(file);
    
    // Set output values
    *thumbnailData = data;
    *width = header.thumbnailWidth;
    *height = header.thumbnailHeight;
    *size = header.thumbnailSize;
    
    return true;
}

// Delete a save state
bool FBNeo_SaveState_Delete(int slot) {
    // Make sure save state system is initialized
    if (!g_saveStateInitialized) {
        FBNeo_SaveState_Initialize();
    }
    
    // Check if a game is running
    if (BurnDrvGetTextA(DRV_NAME) == nullptr) {
        return false;
    }
    
    // Generate filename
    std::string filename = GenerateSaveStateFilename(slot);
    
    // Delete file
    if (remove(filename.c_str()) != 0) {
        printf("Failed to delete save state file: %s\n", filename.c_str());
        return false;
    }
    
    printf("Deleted save state in slot %d: %s\n", slot, filename.c_str());
    
    // Update save state list
    LoadSaveStateList();
    
    return true;
}

// Check if a save state exists in a slot
bool FBNeo_SaveState_Exists(int slot) {
    // Make sure save state system is initialized
    if (!g_saveStateInitialized) {
        FBNeo_SaveState_Initialize();
    }
    
    // Check if a game is running
    if (BurnDrvGetTextA(DRV_NAME) == nullptr) {
        return false;
    }
    
    // Generate filename
    std::string filename = GenerateSaveStateFilename(slot);
    
    // Check if file exists
    struct stat st;
    return (stat(filename.c_str(), &st) == 0);
}

// Reset and clean up save state system
void FBNeo_SaveState_Reset() {
    g_saveStates.clear();
    g_saveStateInitialized = false;
    g_lastAutoSaveTime = 0;
}

// Get save state info
bool FBNeo_SaveState_GetInfo(int slot, SaveStateHeader* header) {
    if (!header) {
        return false;
    }
    
    // Make sure save state system is initialized
    if (!g_saveStateInitialized) {
        FBNeo_SaveState_Initialize();
    }
    
    // Check if a game is running
    if (BurnDrvGetTextA(DRV_NAME) == nullptr) {
        return false;
    }
    
    // Generate filename
    std::string filename = GenerateSaveStateFilename(slot);
    
    // Open file for reading
    FILE* file = fopen(filename.c_str(), "rb");
    if (!file) {
        printf("Could not open save state file: %s\n", filename.c_str());
        return false;
    }
    
    // Read header
    if (fread(header, sizeof(SaveStateHeader), 1, file) != 1) {
        printf("Could not read save state header: %s\n", filename.c_str());
        fclose(file);
        return false;
    }
    
    // Close file
    fclose(file);
    
    return true;
} 