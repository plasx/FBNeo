#include "burner_metal.h"
#include "metal_exports.h"
#include "metal_declarations.h"  // Include this for MAX_PATH and DIRS_MAX constants
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/stat.h>
#include <errno.h>

// Reference existing declarations from elsewhere - not redefining them here
extern char szAppRomPaths[DIRS_MAX][MAX_PATH];
extern char szAppDirPath[MAX_PATH];

// Current ROM path for the last loaded ROM
static char currentRomPath[MAX_PATH] = {0};

// Helper function to check if a directory exists
bool DirectoryExists(const char* path) {
    struct stat sb;
    return (stat(path, &sb) == 0 && S_ISDIR(sb.st_mode));
}

// Helper function to create a directory if it doesn't exist
bool CreateDirectoryIfNeeded(const char* path) {
    if (DirectoryExists(path)) {
        return true;
    }
    
    return mkdir(path, 0755) == 0;
}

// Check if a ROM path is valid - returning int to match header declaration
int ValidateROMPath(const char* path) {
    if (!path || !path[0]) {
        printf("ValidateROMPath: Path is NULL or empty\n");
        return 0; // Return 0 for false
    }
    
    printf("Validating ROM path: %s\n", path);
    
    // Check file extension for ZIP
    const char* ext = strrchr(path, '.');
    if (!ext || strcasecmp(ext, ".zip") != 0) {
        printf("ValidateROMPath: Not a ZIP file (needs .zip extension)\n");
        // For testing purposes, we'll allow non-ZIP files
        // return 0;
    }
    
    struct stat sb;
    if (stat(path, &sb) != 0) {
        printf("ValidateROMPath: File doesn't exist (stat error: %s)\n", strerror(errno));
        return 0; // Return 0 for false
    }
    
    // Make sure it's a file
    if (!S_ISREG(sb.st_mode)) {
        printf("ValidateROMPath: Not a regular file\n");
        return 0; // Return 0 for false
    }
    
    // Check if file is readable
    FILE* f = fopen(path, "rb");
    if (!f) {
        printf("ValidateROMPath: Cannot open file for reading: %s\n", strerror(errno));
        return 0; // Return 0 for false
    }
    
    // Check if file has content
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fclose(f);
    
    if (size <= 0) {
        printf("ValidateROMPath: File is empty (size: %ld)\n", size);
        return 0; // Return 0 for false
    }
    
    printf("ValidateROMPath: File is valid (size: %ld bytes)\n", size);
    return 1; // Return 1 for true
}

// Fix the ROM paths - ensure they are valid and contain ROMs
void FixRomPaths() {
    printf("FixRomPaths called\n");
    
    // By default, check ROM paths in this order:
    // 1. User's home ROMs directory
    // 2. Current working directory's "roms" folder
    // 3. Application bundle resources "roms" folder
    // 4. User's Desktop
    // 5. User's Downloads folder
    
    // Clear all existing paths first to avoid duplicates
    memset(szAppRomPaths, 0, sizeof(szAppRomPaths));
    
    // Get user's home directory 
    const char* homeDir = getenv("HOME");
    if (!homeDir) {
        struct passwd* pwd = getpwuid(getuid());
        if (pwd) {
            homeDir = pwd->pw_dir;
        }
    }
    
    // Initialize potential ROM paths
    char potentialPaths[DIRS_MAX][MAX_PATH] = {0};
    int pathIndex = 0;
    
    // Add common ROM locations to check
    if (homeDir) {
        // ~/ROMs
        snprintf(potentialPaths[pathIndex++], MAX_PATH, "%s/ROMs", homeDir);
        
        // ~/roms 
        snprintf(potentialPaths[pathIndex++], MAX_PATH, "%s/roms", homeDir);
        
        // ~/Downloads
        snprintf(potentialPaths[pathIndex++], MAX_PATH, "%s/Downloads", homeDir);
        
        // ~/Desktop
        snprintf(potentialPaths[pathIndex++], MAX_PATH, "%s/Desktop", homeDir);
        
        // ~/Documents/ROMs
        snprintf(potentialPaths[pathIndex++], MAX_PATH, "%s/Documents/ROMs", homeDir);
        
        // ~/Documents/roms
        snprintf(potentialPaths[pathIndex++], MAX_PATH, "%s/Documents/roms", homeDir);
    }
    
    // Add current working directory and its "roms" subfolder
    char cwd[MAX_PATH] = {0};
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        // Current directory itself
        strncpy(potentialPaths[pathIndex++], cwd, MAX_PATH-1);
        
        // roms subdirectory of current directory
        snprintf(potentialPaths[pathIndex++], MAX_PATH, "%s/roms", cwd);
    }
    
    // Now check all potential paths and add valid ones to szAppRomPaths
    int validPathIndex = 0;
    
    for (int i = 0; i < pathIndex && validPathIndex < DIRS_MAX; i++) {
        // Check if directory exists and is readable
        if (DirectoryExists(potentialPaths[i])) {
            printf("Found ROM directory: %s\n", potentialPaths[i]);
            
            // Special check: verify there are actually ROM files in this directory
            char testPath[MAX_PATH];
            struct stat sb;
            bool hasRoms = false;
            
            // Test for common CPS2 ROMs
            const char* testRoms[] = {"mvsc.zip", "mvscu.zip", "sf2ce.zip", "sfz3.zip", "sfa3.zip", NULL};
            for (int j = 0; testRoms[j] != NULL; j++) {
                snprintf(testPath, MAX_PATH, "%s/%s", potentialPaths[i], testRoms[j]);
                if (stat(testPath, &sb) == 0 && S_ISREG(sb.st_mode)) {
                    printf("  Contains ROM: %s (size: %lld bytes)\n", testPath, (long long)sb.st_size);
                    hasRoms = true;
                    break;
                }
            }
            
            // If this directory contains ROM files, add it to our list
            if (hasRoms) {
                strncpy(szAppRomPaths[validPathIndex++], potentialPaths[i], MAX_PATH-1);
                // Found at least one valid path with ROMs
                break;
            }
        }
    }
    
    // If no valid ROM paths were found, use a default
    if (validPathIndex == 0) {
        printf("No valid ROM paths found, using defaults\n");
        
        // Use the current working directory as a fallback
        if (getcwd(szAppRomPaths[0], MAX_PATH) == NULL) {
            // If even that fails, use a hardcoded path
            strncpy(szAppRomPaths[0], "/Users/plasx/dev/ROMs", MAX_PATH-1);
        }
        validPathIndex = 1;
    }
    
    // Print the configured ROM paths
    printf("Configured ROM paths:\n");
    for (int i = 0; i < validPathIndex; i++) {
        printf("  Path %d: %s\n", i, szAppRomPaths[i]);
    }
}

// Helper to get the current ROM path as a string
const char* GetROMPathString() {
    // Get the current ROM path using the proper signature
    GetCurrentROMPath(currentRomPath, sizeof(currentRomPath));
    return currentRomPath;
}

// Return the game to load (default: mvsc)
const char* GetGameToLoad() {
    // Check if a ROM was specified via command line or UI
    const char* romPath = GetROMPathString();
    if (romPath && romPath[0]) {
        return romPath;
    }
    
    // Default to Marvel vs. Capcom
    return "mvsc";
} 