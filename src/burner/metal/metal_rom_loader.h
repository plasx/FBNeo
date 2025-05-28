#pragma once

#include "metal_declarations.h"

#ifdef __cplusplus
extern "C" {
#endif

// Initialize ROM loading paths
int Metal_InitROMPaths();

// Add a ROM path
int Metal_AddROMPath(const char* path);

// Find a ROM file in the configured paths
const char* Metal_FindROMFile(const char* fileName);

// Internal ROM loading function (not to be called directly)
int Metal_LoadROM_Internal(const char* romPath);

// Load a ROM
int Metal_LoadROM_Enhanced(const char* romPath);

// Load a ROM data file by name from a ZIP archive
// Returns 0 on success, non-zero on failure
// If successful, bytesRead will contain the number of bytes read
int Metal_LoadROMData(const char* zipPath, const char* fileName, 
                     unsigned char* dest, int maxSize, int* bytesRead);

// Exported BurnExtLoadRom function for FBNeo ROM loading
int BurnExtLoadRom(unsigned char* dest, int* pnWrote, int nNum);

#ifdef __cplusplus
}
#endif 