//
// rom_path_utils.cpp
//
// ROM path utilities for FBNeo Metal implementation
//

#include "metal_declarations.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <dirent.h>

// External references
extern char szAppRomPaths[DIRS_MAX][MAX_PATH];
extern char szAppDirPath[MAX_PATH];

// Common ROM path locations to check
static const char* kCommonRomPaths[] = {
    "roms",                                 // Current directory/roms
    "ROMs",                                 // Current directory/ROMs
    "../roms",                              // Parent directory/roms
    "~/ROMs",                               // Home directory/ROMs
    "~/roms",                               // Home directory/roms
    "~/Documents/ROMs",                     // Home directory/Documents/ROMs
    "~/Documents/roms",                     // Home directory/Documents/roms
    "~/Documents/FBNeo/roms",               // Home directory/Documents/FBNeo/roms
    "/Applications/FBNeo.app/Contents/roms" // Application bundle/roms
};

//
// Path Utilities
//

// Expand the path (e.g. ~/roms -> /Users/username/roms)
static void ExpandPath(const char* path, char* result, size_t resultSize) {
    if (!path || !result || resultSize == 0) {
        return;
    }
    
    // Initialize the result
    result[0] = '\0';
    
    // Check for home directory prefix
    if (path[0] == '~' && (path[1] == '/' || path[1] == '\0')) {
        // Get the home directory
        const char* home = getenv("HOME");
        
        // If HOME is not defined, use passwd
        if (!home) {
            struct passwd* pwd = getpwuid(getuid());
            if (pwd) {
                home = pwd->pw_dir;
            }
        }
        
        // Combine home directory and the rest of the path
        if (home) {
            snprintf(result, resultSize, "%s%s", home, path + 1);
            return;
        }
    }
    
    // If not home directory or can't expand, just copy the path
    snprintf(result, resultSize, "%s", path);
}

// Check if a directory exists
static bool DirectoryExists(const char* path) {
    // Expand the path first
    char expandedPath[MAX_PATH];
    ExpandPath(path, expandedPath, sizeof(expandedPath));
    
    // Check if the directory exists
    struct stat sb;
    if (stat(expandedPath, &sb) == 0 && S_ISDIR(sb.st_mode)) {
        return true;
    }
    
    return false;
}

// Check if a file exists
static bool FileExists(const char* path) {
    // Expand the path first
    char expandedPath[MAX_PATH];
    ExpandPath(path, expandedPath, sizeof(expandedPath));
    
    // Check if the file exists
    struct stat sb;
    if (stat(expandedPath, &sb) == 0 && S_ISREG(sb.st_mode)) {
        return true;
    }
    
    return false;
}

// Check if a file exists with a specific extension (case insensitive)
static bool FileExistsWithExtension(const char* directory, const char* filename, const char* extension) {
    // Expand the directory path
    char expandedDir[MAX_PATH];
    ExpandPath(directory, expandedDir, sizeof(expandedDir));
    
    // Build the full path
    char fullPath[MAX_PATH];
    snprintf(fullPath, sizeof(fullPath), "%s/%s%s", expandedDir, filename, extension);
    
    // Check if the file exists
    return FileExists(fullPath);
}

// Check if a ROM file exists in the given directory
static bool RomFileExists(const char* directory, const char* romName) {
    // Check with .zip extension
    if (FileExistsWithExtension(directory, romName, ".zip")) {
        return true;
    }
    
    // Check with .7z extension
    if (FileExistsWithExtension(directory, romName, ".7z")) {
        return true;
    }
    
    // Check with no extension
    char expandedDir[MAX_PATH];
    ExpandPath(directory, expandedDir, sizeof(expandedDir));
    
    char fullPath[MAX_PATH];
    snprintf(fullPath, sizeof(fullPath), "%s/%s", expandedDir, romName);
    
    return FileExists(fullPath);
}

//
// ROM Path Detection
//

// Find ROM directory from common locations
void DetectRomPaths() {
    bool foundPath = false;
    
    // Check environment variable first
    const char* envPath = getenv("FBNEO_ROM_PATH");
    if (envPath && DirectoryExists(envPath)) {
        char expandedPath[MAX_PATH];
        ExpandPath(envPath, expandedPath, sizeof(expandedPath));
        
        printf("Found ROM path from environment: %s\n", expandedPath);
        
        strncpy(szAppRomPaths[0], expandedPath, MAX_PATH - 1);
        szAppRomPaths[0][MAX_PATH - 1] = '\0';
        
        foundPath = true;
    }
    
    // Check common paths if environment variable not set or not valid
    if (!foundPath) {
        for (size_t i = 0; i < sizeof(kCommonRomPaths) / sizeof(kCommonRomPaths[0]); i++) {
            char expandedPath[MAX_PATH];
            ExpandPath(kCommonRomPaths[i], expandedPath, sizeof(expandedPath));
            
            if (DirectoryExists(expandedPath)) {
                printf("Found ROM path: %s\n", expandedPath);
                
                strncpy(szAppRomPaths[0], expandedPath, MAX_PATH - 1);
                szAppRomPaths[0][MAX_PATH - 1] = '\0';
                
                foundPath = true;
                break;
            }
        }
    }
    
    // If still not found, use current directory
    if (!foundPath) {
        char cwd[MAX_PATH];
        if (getcwd(cwd, sizeof(cwd))) {
            printf("No ROM path found, using current directory: %s\n", cwd);
            
            strncpy(szAppRomPaths[0], cwd, MAX_PATH - 1);
            szAppRomPaths[0][MAX_PATH - 1] = '\0';
        } else {
            printf("No ROM path found and couldn't get current directory\n");
            
            // Use a fallback path
            strncpy(szAppRomPaths[0], ".", MAX_PATH - 1);
            szAppRomPaths[0][MAX_PATH - 1] = '\0';
        }
    }
    
    // Print the final ROM path
    printf("ROM path set to: %s\n", szAppRomPaths[0]);
}

// Get a specific ROM file path
bool GetRomFilePath(const char* romName, char* romPath, size_t pathSize) {
    if (!romName || !romPath || pathSize == 0) {
        return false;
    }
    
    // Initialize the result
    romPath[0] = '\0';
    
    // Try each ROM directory
    for (int i = 0; i < DIRS_MAX && szAppRomPaths[i][0] != '\0'; i++) {
        // Check with .zip extension
        if (FileExistsWithExtension(szAppRomPaths[i], romName, ".zip")) {
            snprintf(romPath, pathSize, "%s/%s.zip", szAppRomPaths[i], romName);
            return true;
        }
        
        // Check with .7z extension
        if (FileExistsWithExtension(szAppRomPaths[i], romName, ".7z")) {
            snprintf(romPath, pathSize, "%s/%s.7z", szAppRomPaths[i], romName);
            return true;
        }
        
        // Check with no extension
        char fullPath[MAX_PATH];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", szAppRomPaths[i], romName);
        
        if (FileExists(fullPath)) {
            snprintf(romPath, pathSize, "%s", fullPath);
            return true;
        }
    }
    
    // ROM not found
    return false;
}

// Find a specific ROM in the ROM directories
bool FindRomByName(const char* romName) {
    char romPath[MAX_PATH];
    return GetRomFilePath(romName, romPath, sizeof(romPath));
}

// Initialize ROM paths
void InitRomPaths() {
    // Detect ROM paths
    DetectRomPaths();
    
    // Check for specific ROMs
    const char* testRoms[] = {"mvsc", "sf2ce", "sfa3", "kof98", "mslug", NULL};
    
    printf("Checking for common ROMs...\n");
    
    for (int i = 0; testRoms[i] != NULL; i++) {
        char romPath[MAX_PATH];
        if (GetRomFilePath(testRoms[i], romPath, sizeof(romPath))) {
            printf("Found ROM: %s at %s\n", testRoms[i], romPath);
        } else {
            printf("ROM not found: %s\n", testRoms[i]);
        }
    }
}

// Set ROM directories for FBNeo
void FixRomPaths() {
    // Initialize ROM paths
    InitRomPaths();
    
    // Additional setup can be done here if needed
} 