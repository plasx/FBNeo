#include "burner.h"
#include "burnint.h"
#include "../metal_declarations.h"
#include "../metal_bridge.h"
#include "../rom_path_manager.h"
#include "../rom_verify.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <string>
#include <iostream>

// Test program to verify ROM path management functionality
int main(int argc, char* argv[]) {
    printf("ROM Path Management Test Program\n");
    printf("===============================\n\n");
    
    // 1. Basic path management
    printf("ROM Path Management:\n");
    printf("-----------------\n");
    
    // Detect available ROM paths
    int numPaths = ROMPathManager::DetectROMPaths();
    printf("Detected %d ROM paths\n", numPaths);
    
    // Get all configured paths
    std::vector<std::string> paths = ROMPathManager::GetAllROMPaths();
    printf("Configured ROM paths:\n");
    for (const auto& path : paths) {
        printf("  - %s\n", path.c_str());
    }
    
    // 2. Scan for ROMs
    printf("\nROM Scanning:\n");
    printf("-----------\n");
    
    // Scan all configured paths
    std::vector<ROMPathManager::ROMInfo> allROMs = ROMPathManager::GetAllAvailableROMs();
    printf("Found %lu ROM files\n", allROMs.size());
    
    // Show some sample ROMs
    printf("\nSample ROMs:\n");
    int showCount = std::min(5, (int)allROMs.size());
    for (int i = 0; i < showCount; i++) {
        printf("ROM #%d:\n", i+1);
        printf("  Name: %s\n", allROMs[i].gameName.c_str());
        printf("  File: %s\n", allROMs[i].filename.c_str());
        printf("  Path: %s\n", allROMs[i].fullPath.c_str());
        printf("  Type: %s\n", allROMs[i].type.c_str());
        printf("  Size: %lu bytes\n", allROMs[i].fileSize);
        printf("  CRC32: %s\n", allROMs[i].checksum.c_str());
        printf("  Valid: %s\n", allROMs[i].isValid ? "Yes" : "No");
        printf("\n");
    }
    
    // 3. Filter ROMs
    printf("\nROM Filtering:\n");
    printf("-------------\n");
    
    std::vector<ROMPathManager::ROMInfo> cps2ROMs = ROMPathManager::FilterROMs(allROMs, "CPS2");
    printf("Found %lu CPS2 ROMs\n", cps2ROMs.size());
    
    // 4. Test search functionality
    printf("\nROM Search:\n");
    printf("----------\n");
    
    const char* searchTerms[] = {"street", "marvel", "fighter", "vs", nullptr};
    for (int i = 0; searchTerms[i] != nullptr; i++) {
        std::vector<ROMPathManager::ROMInfo> searchResults = ROMPathManager::SearchROMs(allROMs, searchTerms[i]);
        printf("Search for '%s': %lu results\n", searchTerms[i], searchResults.size());
        
        // Display up to 3 matches
        int resultCount = std::min(3, (int)searchResults.size());
        for (int j = 0; j < resultCount; j++) {
            printf("  - %s (%s)\n", searchResults[j].gameName.c_str(), searchResults[j].type.c_str());
        }
        printf("\n");
    }
    
    // 5. Test favorites functionality
    printf("\nFavorites Management:\n");
    printf("-------------------\n");
    
    // Add some ROMs to favorites
    for (int i = 0; i < std::min(3, (int)allROMs.size()); i++) {
        ROMPathManager::AddToFavorites(allROMs[i].fullPath.c_str());
        printf("Added to favorites: %s\n", allROMs[i].gameName.c_str());
    }
    
    // List favorites
    std::vector<std::string> favs = ROMPathManager::GetFavoriteROMs();
    printf("\nFavorite ROMs (%lu):\n", favs.size());
    for (const auto& fav : favs) {
        printf("  - %s\n", fav.c_str());
    }
    
    // 6. Test recents functionality
    printf("\nRecent ROMs Management:\n");
    printf("---------------------\n");
    
    // Add some ROMs to recents
    for (int i = 0; i < std::min(5, (int)allROMs.size()); i++) {
        ROMPathManager::AddToRecentROMs(allROMs[i].fullPath.c_str());
        printf("Added to recent ROMs: %s\n", allROMs[i].gameName.c_str());
    }
    
    // List recents
    std::vector<std::string> recents = ROMPathManager::GetRecentROMs();
    printf("\nRecent ROMs (%lu):\n", recents.size());
    for (const auto& recent : recents) {
        printf("  - %s\n", recent.c_str());
    }
    
    return 0;
} 