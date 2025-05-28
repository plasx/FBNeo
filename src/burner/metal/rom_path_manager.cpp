#include "rom_path_manager.h"
#include "rom_verify.h"
#include "burner_metal.h"
#include "metal_declarations.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <ctime>

// Reference existing declarations from elsewhere
extern char szAppRomPaths[DIRS_MAX][MAX_PATH];

// Global variables for ROM path management
static std::string currentROMPath;
static std::vector<std::string> romPaths;
static std::vector<std::string> favoriteROMs;
static std::vector<std::string> recentROMs;
static std::unordered_map<std::string, ROMPathManager::ROMInfo> romCache;
static bool initialized = false;

// Helper function to check if a directory exists
static bool DirectoryExists(const char* path) {
    struct stat sb;
    return (stat(path, &sb) == 0 && S_ISDIR(sb.st_mode));
}

// Helper function to check if a file exists
static bool FileExists(const char* path) {
    struct stat sb;
    return (stat(path, &sb) == 0 && S_ISREG(sb.st_mode));
}

// Helper function to get file size
static size_t GetFileSize(const char* path) {
    struct stat sb;
    if (stat(path, &sb) == 0) {
        return sb.st_size;
    }
    return 0;
}

// Init function to ensure data is loaded
static void Initialize() {
    if (initialized) return;
    
    // Load data from configuration
    LoadROMPaths("config/rom_paths.cfg");
    
    // Import existing paths from FBNeo's configuration
    for (int i = 0; i < DIRS_MAX; i++) {
        if (szAppRomPaths[i][0] != '\0') {
            std::string path = szAppRomPaths[i];
            if (std::find(romPaths.begin(), romPaths.end(), path) == romPaths.end()) {
                romPaths.push_back(path);
            }
        }
    }
    
    initialized = true;
}

// Implementation of ROM path management functions
namespace ROMPathManager {

    // Add a new ROM path
    bool AddROMPath(const char* path) {
        Initialize();
        
        if (!path || !path[0]) return false;
        
        // Check if the directory exists
        if (!DirectoryExists(path)) return false;
        
        // Make sure it's not already in our list
        std::string pathStr = path;
        if (std::find(romPaths.begin(), romPaths.end(), pathStr) == romPaths.end()) {
            romPaths.push_back(pathStr);
            
            // Also update FBNeo's internal paths
            for (int i = 0; i < DIRS_MAX; i++) {
                if (szAppRomPaths[i][0] == '\0') {
                    strncpy(szAppRomPaths[i], path, MAX_PATH-1);
                    szAppRomPaths[i][MAX_PATH-1] = '\0';
                    break;
                }
            }
            
            // Save the configuration
            SaveROMPaths("config/rom_paths.cfg");
            return true;
        }
        
        return false;
    }
    
    // Remove a ROM path
    bool RemoveROMPath(const char* path) {
        Initialize();
        
        if (!path || !path[0]) return false;
        
        std::string pathStr = path;
        auto it = std::find(romPaths.begin(), romPaths.end(), pathStr);
        if (it != romPaths.end()) {
            romPaths.erase(it);
            
            // Update FBNeo's internal paths
            bool found = false;
            for (int i = 0; i < DIRS_MAX; i++) {
                if (strcmp(szAppRomPaths[i], path) == 0) {
                    found = true;
                }
                
                // Shift remaining paths up
                if (found && i < DIRS_MAX - 1) {
                    strcpy(szAppRomPaths[i], szAppRomPaths[i+1]);
                }
            }
            
            // Clear the last slot
            if (found) {
                szAppRomPaths[DIRS_MAX-1][0] = '\0';
            }
            
            // Save the configuration
            SaveROMPaths("config/rom_paths.cfg");
            return true;
        }
        
        return false;
    }
    
    // Set the current ROM path
    bool SetCurrentROMPath(const char* path) {
        if (!path) return false;
        
        currentROMPath = path;
        
        // Update the core's ROM path pointer
        extern int SetCurrentROMPath(const char* szPath);
        ::SetCurrentROMPath(path);
        
        return true;
    }
    
    // Get the current ROM path
    const char* GetCurrentROMPath() {
        return currentROMPath.c_str();
    }
    
    // Get all configured ROM paths
    std::vector<std::string> GetAllROMPaths() {
        Initialize();
        return romPaths;
    }
    
    // Save ROM paths to a configuration file
    bool SaveROMPaths(const char* configFile) {
        if (!configFile) return false;
        
        // Create config directory if it doesn't exist
        std::string configDir = "config";
        if (!DirectoryExists(configDir.c_str())) {
            #ifdef _WIN32
            mkdir(configDir.c_str());
            #else
            mkdir(configDir.c_str(), 0755);
            #endif
        }
        
        std::ofstream file(configFile);
        if (!file.is_open()) return false;
        
        // Write ROM paths
        file << "# FBNeo ROM Path Configuration\n";
        file << "# Generated: " << std::time(nullptr) << "\n\n";
        
        file << "[rom_paths]\n";
        for (const auto& path : romPaths) {
            file << path << "\n";
        }
        
        file << "\n[favorites]\n";
        for (const auto& rom : favoriteROMs) {
            file << rom << "\n";
        }
        
        file << "\n[recent]\n";
        for (const auto& rom : recentROMs) {
            file << rom << "\n";
        }
        
        return true;
    }
    
    // Load ROM paths from a configuration file
    bool LoadROMPaths(const char* configFile) {
        if (!configFile) return false;
        
        if (!FileExists(configFile)) {
            // File doesn't exist, but that's not an error
            return true;
        }
        
        std::ifstream file(configFile);
        if (!file.is_open()) return false;
        
        romPaths.clear();
        favoriteROMs.clear();
        recentROMs.clear();
        
        std::string line;
        std::string section;
        
        while (std::getline(file, line)) {
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') continue;
            
            // Check for section headers
            if (line[0] == '[' && line.back() == ']') {
                section = line.substr(1, line.length() - 2);
                continue;
            }
            
            // Process lines based on section
            if (section == "rom_paths") {
                romPaths.push_back(line);
            } else if (section == "favorites") {
                favoriteROMs.push_back(line);
            } else if (section == "recent") {
                recentROMs.push_back(line);
            }
        }
        
        return true;
    }
    
    // Auto-detect ROM paths on the system
    int DetectROMPaths() {
        Initialize();
        
        // Clear existing paths
        romPaths.clear();
        
        // Common ROM locations
        std::vector<std::string> commonPaths = {
            "./roms",
            "~/ROMs",
            "~/roms",
            "~/Documents/ROMs",
            "~/Documents/roms",
            "~/Applications/FBNeo/roms",
            "~/Downloads/ROMs",
            "~/Downloads/roms"
        };
        
        // Expand user home directory
        const char* homeDir = getenv("HOME");
        if (homeDir) {
            for (auto& path : commonPaths) {
                if (path.substr(0, 2) == "~/") {
                    path.replace(0, 1, homeDir);
                }
            }
        }
        
        // Check each path
        for (const auto& path : commonPaths) {
            DIR* dir = opendir(path.c_str());
            if (dir) {
                closedir(dir);
                romPaths.push_back(path);
                
                // Update FBNeo's internal paths
                for (int i = 0; i < DIRS_MAX && i < romPaths.size(); i++) {
                    strncpy(szAppRomPaths[i], romPaths[i].c_str(), MAX_PATH-1);
                    szAppRomPaths[i][MAX_PATH-1] = '\0';
                }
            }
        }
        
        // Save configuration
        SaveROMPaths("config/rom_paths.cfg");
        
        return romPaths.size();
    }
    
    // Scan a directory for ROMs
    std::vector<ROMInfo> ScanDirectory(const char* directory) {
        std::vector<ROMInfo> results;
        
        if (!directory || !DirectoryExists(directory)) {
            return results;
        }
        
        DIR* dir = opendir(directory);
        if (!dir) {
            return results;
        }
        
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            // Skip . and ..
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            
            // Check for ZIP and 7Z files
            std::string filename = entry->d_name;
            std::string ext = filename.substr(filename.find_last_of(".") + 1);
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            
            if (ext == "zip" || ext == "7z") {
                std::string fullPath = std::string(directory) + "/" + filename;
                
                // Check if we already have this ROM in our cache
                if (romCache.find(fullPath) != romCache.end()) {
                    results.push_back(romCache[fullPath]);
                    continue;
                }
                
                // Create ROM info
                ROMInfo romInfo;
                romInfo.filename = filename;
                romInfo.fullPath = fullPath;
                romInfo.fileSize = GetFileSize(fullPath.c_str());
                
                // Extract game name (remove extension)
                romInfo.gameName = filename.substr(0, filename.find_last_of("."));
                
                // Determine ROM type
                if (ROMVerify::IsCPS2ROM(fullPath.c_str(), false)) {
                    romInfo.type = "CPS2";
                } else {
                    // TODO: Add detection for other ROM types
                    romInfo.type = "Unknown";
                }
                
                // Calculate checksum (only CRC for performance)
                std::string crc, md5, sha1;
                if (ROMVerify::CalculateROMChecksum(fullPath.c_str(), crc, md5, sha1)) {
                    romInfo.checksum = crc;
                }
                
                // Verify ROM validity
                romInfo.isValid = !romInfo.checksum.empty();
                
                // Add to results and cache
                results.push_back(romInfo);
                romCache[fullPath] = romInfo;
            }
        }
        
        closedir(dir);
        return results;
    }
    
    // Get all available ROMs across all configured paths
    std::vector<ROMInfo> GetAllAvailableROMs() {
        Initialize();
        
        std::vector<ROMInfo> allROMs;
        std::unordered_map<std::string, bool> addedFilenames; // To avoid duplicates
        
        for (const auto& path : romPaths) {
            std::vector<ROMInfo> dirROMs = ScanDirectory(path.c_str());
            
            for (const auto& rom : dirROMs) {
                // Only add if we haven't seen this filename yet
                if (addedFilenames.find(rom.filename) == addedFilenames.end()) {
                    allROMs.push_back(rom);
                    addedFilenames[rom.filename] = true;
                }
            }
        }
        
        return allROMs;
    }
    
    // Get information about a specific ROM
    ROMInfo GetROMInfo(const char* romPath) {
        ROMInfo emptyInfo;
        
        if (!romPath) return emptyInfo;
        
        std::string path = romPath;
        
        // Check cache first
        if (romCache.find(path) != romCache.end()) {
            return romCache[path];
        }
        
        // Verify this is a real file
        if (!FileExists(path.c_str())) {
            return emptyInfo;
        }
        
        // Extract filename
        std::string filename = path;
        size_t lastSlash = filename.find_last_of("/\\");
        if (lastSlash != std::string::npos) {
            filename = filename.substr(lastSlash + 1);
        }
        
        // Create ROM info
        ROMInfo romInfo;
        romInfo.filename = filename;
        romInfo.fullPath = path;
        romInfo.fileSize = GetFileSize(path.c_str());
        
        // Extract game name (remove extension)
        romInfo.gameName = filename.substr(0, filename.find_last_of("."));
        
        // Determine ROM type
        if (ROMVerify::IsCPS2ROM(path.c_str(), false)) {
            romInfo.type = "CPS2";
        } else {
            // TODO: Add detection for other ROM types
            romInfo.type = "Unknown";
        }
        
        // Calculate checksum
        std::string crc, md5, sha1;
        if (ROMVerify::CalculateROMChecksum(path.c_str(), crc, md5, sha1)) {
            romInfo.checksum = crc;
        }
        
        // Verify ROM validity
        romInfo.isValid = !romInfo.checksum.empty();
        
        // Add to cache
        romCache[path] = romInfo;
        
        return romInfo;
    }
    
    // Add a ROM to favorites
    bool AddToFavorites(const char* romPath) {
        Initialize();
        
        if (!romPath) return false;
        
        std::string path = romPath;
        
        // Check if it's already in favorites
        if (std::find(favoriteROMs.begin(), favoriteROMs.end(), path) != favoriteROMs.end()) {
            return true; // Already a favorite
        }
        
        // Add to favorites
        favoriteROMs.push_back(path);
        
        // Save configuration
        SaveROMPaths("config/rom_paths.cfg");
        
        return true;
    }
    
    // Remove a ROM from favorites
    bool RemoveFromFavorites(const char* romPath) {
        Initialize();
        
        if (!romPath) return false;
        
        std::string path = romPath;
        
        // Find and remove
        auto it = std::find(favoriteROMs.begin(), favoriteROMs.end(), path);
        if (it != favoriteROMs.end()) {
            favoriteROMs.erase(it);
            
            // Save configuration
            SaveROMPaths("config/rom_paths.cfg");
            return true;
        }
        
        return false;
    }
    
    // Get all favorite ROMs
    std::vector<std::string> GetFavoriteROMs() {
        Initialize();
        return favoriteROMs;
    }
    
    // Add a ROM to recently used list
    bool AddToRecentROMs(const char* romPath) {
        Initialize();
        
        if (!romPath) return false;
        
        std::string path = romPath;
        
        // Remove if already in the list (to move it to the top)
        auto it = std::find(recentROMs.begin(), recentROMs.end(), path);
        if (it != recentROMs.end()) {
            recentROMs.erase(it);
        }
        
        // Add to the front
        recentROMs.insert(recentROMs.begin(), path);
        
        // Limit to 10 recent ROMs
        if (recentROMs.size() > 10) {
            recentROMs.resize(10);
        }
        
        // Save configuration
        SaveROMPaths("config/rom_paths.cfg");
        
        return true;
    }
    
    // Get recently used ROMs
    std::vector<std::string> GetRecentROMs() {
        Initialize();
        return recentROMs;
    }
    
    // Filter ROMs by type
    std::vector<ROMInfo> FilterROMs(const std::vector<ROMInfo>& roms, const char* type) {
        if (!type) return roms;
        
        std::string typeStr = type;
        std::vector<ROMInfo> filtered;
        
        for (const auto& rom : roms) {
            if (rom.type == typeStr) {
                filtered.push_back(rom);
            }
        }
        
        return filtered;
    }
    
    // Search ROMs by name
    std::vector<ROMInfo> SearchROMs(const std::vector<ROMInfo>& roms, const char* searchTerm) {
        if (!searchTerm || !searchTerm[0]) return roms;
        
        std::string term = searchTerm;
        std::transform(term.begin(), term.end(), term.begin(), ::tolower);
        
        std::vector<ROMInfo> results;
        
        for (const auto& rom : roms) {
            // Convert name to lowercase for case-insensitive search
            std::string name = rom.gameName;
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            
            // Check if the search term is in the name
            if (name.find(term) != std::string::npos) {
                results.push_back(rom);
            }
        }
        
        return results;
    }
} 