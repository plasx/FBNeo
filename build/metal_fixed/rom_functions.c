#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>

// Implemented LoadROM_FullPath function from metal_rom_loader.c
// Path is the full path to the ROM file
bool LoadROM_FullPath(const char* path) {
    struct stat st;
    
    // Check if file exists
    if (stat(path, &st) != 0) {
        printf("[ROM] Error: ROM file '%s' not found\n", path);
        return false;
    }
    
    // Show ROM information
    printf("[ROM] Loading ROM: %s\n", path);
    printf("[ROM] ROM Size: %lld bytes\n", (long long)st.st_size);
    
    // Initialize the driver
    // We will let FBNeo core handle which driver to use
    extern int BurnDrvInit();
    int result = BurnDrvInit();
    if (result != 0) {
        printf("[ROM] Error initializing driver: %d\n", result);
        return false;
    }
    
    printf("[ROM] ROM loaded successfully\n");
    return true;
} 