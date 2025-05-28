#include "../metal_declarations.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ROM paths for FBNeo core
char szAppRomPaths[DIRS_MAX][MAX_PATH] = {{0}};

// Import the BurnExtLoadRom function from metal_rom_loader.c
extern INT32 BurnExtLoadRom(UINT8* Dest, INT32* pnWrote, INT32 i);

// Stub for missing BurnDrv functions - use extern "C" for C++ compatibility
#ifdef __cplusplus
extern "C" {
#endif

int BurnDrvGetIndexByName(const char* szName) {
    printf("BurnDrvGetIndexByName called for %s\n", szName ? szName : "NULL");
    
    // Return 0 for mvsc and -1 for others
    if (szName && strcmp(szName, "mvsc") == 0) {
        return 0;
    }
    
    return -1;
}

int BurnDrvGetRomInfo(void* pri, int i) {
    printf("BurnDrvGetRomInfo called for index %d\n", i);
    
    if (!pri) return 1;
    
    // Create a dummy structure similar to what's used in metal_rom_loader.cpp
    struct BurnRomInfo {
        int nLen;
        int nCrc;
        int nType;
        int nState;
    };
    
    struct BurnRomInfo* romInfo = (struct BurnRomInfo*)pri;
    
    // Set up dummy ROM info
    romInfo->nLen = 1024 * 1024; // 1MB
    romInfo->nCrc = 0x12345678;
    romInfo->nType = 0;
    romInfo->nState = 0;
    
    return 0;
}

int BurnDrvGetRomName(char* szName, int i, int j) {
    printf("BurnDrvGetRomName called for indices %d, %d\n", i, j);
    
    if (!szName) return 1;
    
    // Return a dummy ROM name
    sprintf(szName, "game_rom_%d_%d.bin", i, j);
    
    return 0;
}

#ifdef __cplusplus
}
#endif 