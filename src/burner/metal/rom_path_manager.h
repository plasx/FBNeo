#pragma once

#include "burner.h"
#include <vector>
#include <string>
#include <unordered_map>

// Enhanced ROM path management functionality
namespace ROMPathManager {

    // Structure to hold information about an available ROM
    struct ROMInfo {
        std::string filename;       // Filename (with extension)
        std::string gameName;       // Game name extracted from filename
        std::string fullPath;       // Full path to the ROM
        std::string type;           // ROM type (CPS1, CPS2, NeoGeo, etc.)
        bool isValid;               // Whether the ROM appears valid
        size_t fileSize;            // ROM file size
        std::string checksum;       // CRC32 checksum
    };

    // Functions to manage ROM paths
    bool AddROMPath(const char* path);
    bool RemoveROMPath(const char* path);
    bool SetCurrentROMPath(const char* path);
    const char* GetCurrentROMPath();
    std::vector<std::string> GetAllROMPaths();
    bool SaveROMPaths(const char* configFile);
    bool LoadROMPaths(const char* configFile);
    int DetectROMPaths();
    
    // Functions to scan for available ROMs
    std::vector<ROMInfo> ScanDirectory(const char* directory);
    std::vector<ROMInfo> GetAllAvailableROMs();
    ROMInfo GetROMInfo(const char* romPath);
    
    // Functions for favorite/recent ROMs
    bool AddToFavorites(const char* romPath);
    bool RemoveFromFavorites(const char* romPath);
    std::vector<std::string> GetFavoriteROMs();
    bool AddToRecentROMs(const char* romPath);
    std::vector<std::string> GetRecentROMs();
    
    // ROM filtering
    std::vector<ROMInfo> FilterROMs(const std::vector<ROMInfo>& roms, const char* type);
    std::vector<ROMInfo> SearchROMs(const std::vector<ROMInfo>& roms, const char* searchTerm);
} 