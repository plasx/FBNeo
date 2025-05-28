#include "burner.h"
#include "burnint.h"
#include "../metal_declarations.h"
#include "../metal_bridge.h"
#include "../rom_verify.h"
#include "../rom_path_manager.h"
#include "../cps2_rom_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <string>
#include <iostream>

// Test program to verify CPS2 ROM functionality
int main(int argc, char* argv[]) {
    printf("CPS2 ROM Loading Test Program\n");
    printf("============================\n\n");
    
    // Initialize the CPS2 ROM loader
    if (!CPS2_InitROMLoader()) {
        printf("Failed to initialize CPS2 ROM loader\n");
        return 1;
    }
    
    // Get the list of supported games
    std::vector<CPS2GameInfo> games = CPS2_GetSupportedGames();
    
    printf("Supported CPS2 Games (%zu games):\n", games.size());
    printf("-----------------------\n");
    
    // Display all supported games with ROM availability
    for (size_t i = 0; i < games.size(); i++) {
        const auto& game = games[i];
        printf("%d. %s (%s) - %s\n", 
               (int)i + 1, 
               game.name.c_str(), 
               game.id.c_str(),
               game.romAvailable ? "ROM AVAILABLE" : "ROM NOT FOUND");
    }
    
    // If a game ID was specified on the command line, try to load it
    if (argc > 1) {
        const char* gameId = argv[1];
        printf("\nAttempting to load ROM set for %s\n", gameId);
        printf("----------------------------------\n");
        
        if (CPS2_LoadROMSet(gameId)) {
            printf("Successfully loaded ROM set for %s\n", gameId);
            
            // Get ROM info
            const CPS2ROMInfo* romInfo = CPS2_GetROMInfo();
            if (romInfo) {
                printf("\nROM Set Information:\n");
                printf("  Game: %s\n", romInfo->name.c_str());
                printf("  ID: %s\n", romInfo->id.c_str());
                printf("  Hardware Type: %d\n", romInfo->hardwareType);
                printf("  Display: %dx%d\n", romInfo->width, romInfo->height);
                printf("  ROM Files: %zu\n", romInfo->files.size());
                
                // Display loaded ROM files
                printf("\nLoaded ROM Files:\n");
                for (const auto& file : romInfo->files) {
                    const CPS2ROMFile* loadedFile = CPS2_GetROMFile(file.name.c_str());
                    if (loadedFile) {
                        printf("  %s (%zu bytes, CRC32: %s)\n", 
                               loadedFile->name.c_str(), 
                               loadedFile->size,
                               loadedFile->checksum.c_str());
                    }
                }
                
                // Try running the ROM if requested
                if (argc > 2 && strcmp(argv[2], "run") == 0) {
                    printf("\nAttempting to run the ROM...\n");
                    
                    if (CPS2_RunROM()) {
                        printf("ROM is running! Press Ctrl+C to stop.\n");
                        
                        // In a real implementation, we would enter a game loop here
                        // For this test, we'll just sleep for a bit to simulate running
                        printf("Simulating game running for 5 seconds...\n");
                        for (int i = 5; i > 0; i--) {
                            printf("%d...", i);
                            fflush(stdout);
                            sleep(1);
                        }
                        printf("\nExiting game\n");
                    } else {
                        printf("Failed to run the ROM\n");
                    }
                }
            }
            
            // Clean up
            CPS2_CleanupROMFiles();
        } else {
            printf("Failed to load ROM set for %s\n", gameId);
        }
    } else {
        printf("\nUsage: %s <gameId> [run]\n", argv[0]);
        printf("Example: %s mvsc run\n", argv[0]);
        printf("\nAvailable game IDs: ");
        for (size_t i = 0; i < games.size(); i++) {
            printf("%s%s", games[i].id.c_str(), (i < games.size() - 1) ? ", " : "");
        }
        printf("\n");
    }
    
    // Shutdown the CPS2 ROM loader
    CPS2_ShutdownROMLoader();
    
    return 0;
} 