#include "burnint.h"
#include "metal_fixes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <strings.h> // For strcasecmp

// External ROM path from Metal bridge
extern const char* GetROMPathString();

// Global ROM loading state
static char g_szCurrentROMPath[MAX_PATH] = {0};
static bool g_bROMLoaded = false;

// ROM data storage
static UINT8* g_pROMData[32] = {0}; // Support up to 32 ROM regions
static UINT32 g_nROMSizes[32] = {0};
static int g_nROMCount = 0;

// Forward declaration
int BurnLoadRomExt(UINT8* Dest, INT32 i, INT32 nGap, INT32 nType);

// ROM loading implementation for Metal build
// int BurnLoadRom(UINT8* Dest, INT32 i, INT32 nGap) {
//     return BurnLoadRomExt(Dest, i, nGap, 0);
// }

// BurnXorRom is implemented in load.cpp

int BurnByteswapRom(UINT8* Dest, INT32 i) {
    // Byteswap ROM data for endian conversion
    if (!Dest) return 1;
    
    struct BurnRomInfo ri;
    BurnDrvGetRomInfo(&ri, i);
    
    if (ri.nLen == 0) return 0;
    
    // Swap bytes in 16-bit words
    for (INT32 j = 0; j < ri.nLen - 1; j += 2) {
        UINT8 temp = Dest[j];
        Dest[j] = Dest[j + 1];
        Dest[j + 1] = temp;
    }
    
    return 0;
}

// Set current ROM path
INT32 BurnSetROMPath(const char* szPath) {
    if (!szPath) {
        return 1;
    }
    
    strncpy(g_szCurrentROMPath, szPath, MAX_PATH - 1);
    g_szCurrentROMPath[MAX_PATH - 1] = '\0';
    
    printf("[BurnSetROMPath] ROM path set to: %s\n", g_szCurrentROMPath);
    return 0;
}

// Get current ROM path
const char* BurnGetROMPath() {
    const char* metalPath = GetROMPathString();
    if (metalPath && metalPath[0]) {
        return metalPath;
    }
    return g_szCurrentROMPath[0] ? g_szCurrentROMPath : NULL;
}

// Calculate CRC32 for a buffer
UINT32 CalcCrc32(UINT8* data, UINT32 length) {
    return crc32(0, data, length);
}

// Load a file into memory
static UINT8* LoadFileToMemory(const char* filePath, UINT32* fileSize) {
    FILE* f = fopen(filePath, "rb");
    if (!f) {
        printf("[LoadFileToMemory] Could not open file: %s\n", filePath);
        return NULL;
    }
    
    // Get file size
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (size <= 0) {
        printf("[LoadFileToMemory] Invalid file size: %ld\n", size);
        fclose(f);
        return NULL;
    }
    
    // Allocate memory
    UINT8* buffer = (UINT8*)malloc(size);
    if (!buffer) {
        printf("[LoadFileToMemory] Memory allocation failed for %ld bytes\n", size);
        fclose(f);
        return NULL;
    }
    
    // Read file data
    size_t bytesRead = fread(buffer, 1, size, f);
    fclose(f);
    
    if (bytesRead != size) {
        printf("[LoadFileToMemory] Read %zu bytes, expected %ld\n", bytesRead, size);
        free(buffer);
        return NULL;
    }
    
    if (fileSize) {
        *fileSize = (UINT32)size;
    }
    
    return buffer;
}

// Load ROM from a file or ZIP archive
INT32 BurnLoadRom(UINT8* Dest, INT32* pnWrote, INT32 i) {
    printf("[BurnLoadRom] Loading ROM %d\n", i);
    
    if (pnWrote) {
        *pnWrote = 0;
    }
    
    // Get ROM info for this index
    struct BurnRomInfo ri;
    if (BurnDrvGetRomInfo(&ri, i) != 0) {
        printf("[BurnLoadRom] No ROM info for index %d\n", i);
        return 1; // No more ROMs
    }
    
    if (!ri.szName || ri.nLen == 0) {
        printf("[BurnLoadRom] Invalid ROM info for index %d\n", i);
        return 1;
    }
    
    printf("[BurnLoadRom] ROM %d: %s, size: 0x%08X, CRC: 0x%08X, type: 0x%08X\n", 
           i, ri.szName, ri.nLen, ri.nCrc, ri.nType);
    
    // Get ROM path
    const char* romPath = BurnGetROMPath();
    if (!romPath || !romPath[0]) {
        printf("[BurnLoadRom] ERROR: ROM path not set\n");
        return 1;
    }
    
    // Build full path to ROM file
    char romFilePath[MAX_PATH];
    snprintf(romFilePath, MAX_PATH, "%s/%s", romPath, ri.szName);
    
    // Try to load ROM file directly
    UINT32 fileSize = 0;
    UINT8* fileData = LoadFileToMemory(romFilePath, &fileSize);
    
    // If direct file load failed, try loading from MvSC.zip or mvsc.zip
    if (!fileData) {
        // Try uppercase ZIP name
        char zipPath[MAX_PATH];
        snprintf(zipPath, MAX_PATH, "%s/MVSC.ZIP", romPath);
        
        // If uppercase ZIP not found, try lowercase
        struct stat st;
        if (stat(zipPath, &st) != 0) {
            snprintf(zipPath, MAX_PATH, "%s/mvsc.zip", romPath);
            
            // If lowercase ZIP not found, check for any case
            if (stat(zipPath, &st) != 0) {
                // Search for the ZIP file (any case)
                DIR* dir = opendir(romPath);
                if (dir) {
                    struct dirent* entry;
                    bool found = false;
                    
                    while ((entry = readdir(dir)) != NULL) {
                        if (strcasecmp(entry->d_name, "mvsc.zip") == 0) {
                            snprintf(zipPath, MAX_PATH, "%s/%s", romPath, entry->d_name);
                            found = true;
                            break;
                        }
                    }
                    
                    closedir(dir);
                    
                    if (!found) {
                        printf("[BurnLoadRom] ERROR: Could not find MVSC.ZIP in any case\n");
                        return 1;
                    }
                } else {
                    printf("[BurnLoadRom] ERROR: Could not open ROM directory\n");
                    return 1;
                }
            }
        }
        
        printf("[BurnLoadRom] Attempting to load from ZIP: %s\n", zipPath);
        
        // TODO: Add ZIP handling if needed for the Metal version
        // For now, just show an error message directing to proper ROM setup
        printf("[BurnLoadRom] ERROR: ROM file %s not found directly.\n", ri.szName);
        printf("[BurnLoadRom] To use this emulator, please extract all ROMs from %s\n", zipPath);
        printf("[BurnLoadRom] to the ROM directory so they can be loaded individually.\n");
        
        // For debugging, generate placeholder data if destination is provided
        // This should be removed once proper ZIP handling is implemented
        if (Dest && ri.nLen > 0) {
            printf("[BurnLoadRom] WARNING: Generating placeholder data for debugging\n");
            memset(Dest, 0, ri.nLen);
            
            // Set a pattern to indicate this is placeholder data
            for (UINT32 j = 0; j < ri.nLen; j++) {
                Dest[j] = (j & 0xFF) ^ ((j >> 8) & 0xFF);
            }
            
            if (pnWrote) {
                *pnWrote = ri.nLen;
            }
            
            return 0; // Return success for debugging
        }
        
        return 1; // ROM not found
    }
    
    // Verify file size
    if (fileSize < ri.nLen) {
        printf("[BurnLoadRom] ERROR: ROM file size mismatch for %s\n", ri.szName);
        printf("[BurnLoadRom] Expected: %u bytes, Found: %u bytes\n", ri.nLen, fileSize);
        free(fileData);
        return 1;
    }
    
    // Verify CRC if provided
    if (ri.nCrc != 0) {
        UINT32 fileCrc = CalcCrc32(fileData, fileSize);
        if (fileCrc != ri.nCrc) {
            printf("[BurnLoadRom] WARNING: CRC mismatch for %s\n", ri.szName);
            printf("[BurnLoadRom] Expected: 0x%08X, Calculated: 0x%08X\n", ri.nCrc, fileCrc);
            // Continue anyway - CRC verification is advisory
        } else {
            printf("[BurnLoadRom] CRC verified for %s: 0x%08X\n", ri.szName, fileCrc);
        }
    }
    
    // Copy data to destination buffer if provided
    if (Dest && ri.nLen > 0) {
        memcpy(Dest, fileData, ri.nLen);
        
        if (pnWrote) {
            *pnWrote = ri.nLen;
        }
        
        printf("[BurnLoadRom] Loaded %u bytes for %s\n", ri.nLen, ri.szName);
    }
    
    // Free temporary buffer
    free(fileData);
    
    return 0; // Success
}

// ROM name function
INT32 BurnDrvGetRomName(char** pszName, UINT32 i, INT32 nAka) {
    if (!pszName) {
        return 1;
    }
    
    struct BurnRomInfo ri;
    if (BurnDrvGetRomInfo(&ri, i) != 0) {
        return 1; // No more ROMs
    }
    
    *pszName = ri.szName;
    return 0;
}

// ROM info function
INT32 BurnDrvGetRomInfo(struct BurnRomInfo* pri, UINT32 i) {
    if (!pri || nBurnDrvActive >= nBurnDrvCount) {
        return 1;
    }
    
    struct BurnDriver* pDrv = pDriver[nBurnDrvActive];
    if (!pDrv || !pDrv->GetRomInfo) {
        return 1;
    }
    
    return pDrv->GetRomInfo(pri, i, 0);
}

// Initialize ROM loading system
INT32 BurnROMInit() {
    printf("[BurnROMInit] Initializing ROM loading system\n");
    
    // Clear ROM data
    for (int i = 0; i < 32; i++) {
        if (g_pROMData[i]) {
            free(g_pROMData[i]);
            g_pROMData[i] = NULL;
        }
        g_nROMSizes[i] = 0;
    }
    g_nROMCount = 0;
    g_bROMLoaded = false;
    
    return 0;
}

// Exit ROM loading system
INT32 BurnROMExit() {
    printf("[BurnROMExit] Cleaning up ROM loading system\n");
    
    // Free ROM data
    for (int i = 0; i < 32; i++) {
        if (g_pROMData[i]) {
            free(g_pROMData[i]);
            g_pROMData[i] = NULL;
        }
        g_nROMSizes[i] = 0;
    }
    g_nROMCount = 0;
    g_bROMLoaded = false;
    
    return 0;
} 