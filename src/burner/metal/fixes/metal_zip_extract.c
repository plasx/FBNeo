#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include "metal_tchar.h"
#include "../metal_declarations.h"

// Define the RomInfo struct 
typedef struct {
    char name[256];
    unsigned int size;
    unsigned int crc32;
    unsigned int status;  // 0 = not found, 1 = found, 2 = loaded
} RomInfo;

// Debug logging for ROM extraction
static void log_extraction(const char* message, const char* filename) {
    fprintf(stderr, "[ROM CHECK] %s: %s\n", message, filename ? filename : "unknown");
}

// Basic ZIP extraction function to extract ROM files
int ExtractZipFile(const char* zipPath, const char* fileName, void* buffer, int bufferSize) {
    log_extraction("Attempting to extract file from ZIP", fileName);
    
    // Implementation will call into zlib
    fprintf(stderr, "[ROM CHECK] Extracting %s from %s\n", fileName, zipPath);
    
    // Placeholder: In a real implementation, this would use zlib/libzip to actually
    // extract the file from the ZIP archive
    
    // Return bytes read (0 = failure for now)
    return 0;
}

// Get ROM info from a ZIP file
int GetRomInfoFromZip(const char* zipPath, RomInfo* info) {
    if (!zipPath || !info) {
        log_extraction("Invalid parameters for GetRomInfoFromZip", zipPath);
        return 0;
    }
    
    log_extraction("Getting ROM info from ZIP", zipPath);
    
    // In a real implementation, this would scan the ZIP contents
    // and fill the RomInfo structure with details about ROM files
    
    // For now, return failure
    return 0;
}

// Main entry point for ZIP extraction called by the emulator
bool Metal_ExtractZipFile(const char* zipPath, const char* internalPath, void* buffer, int bufferSize) {
    fprintf(stderr, "[ROM CHECK] Metal_ExtractZipFile: %s -> %s\n", zipPath, internalPath);
    
    int bytesRead = ExtractZipFile(zipPath, internalPath, buffer, bufferSize);
    
    return (bytesRead > 0);
}

// Get information about a ZIP file
int Metal_GetZipFileInfo(const char* zipPath, char* infoBuffer, int bufferSize) {
    if (!zipPath || !infoBuffer || bufferSize <= 0) {
        return 0;
    }
    
    fprintf(stderr, "[ROM CHECK] Getting ZIP info: %s\n", zipPath);
    
    // Generate some basic info
    snprintf(infoBuffer, bufferSize, 
           "File: %s\nSize: Unknown\nEntries: Unknown\nCRC32: Unknown", 
           zipPath);
    
    return 1;
}

// List contents of a ZIP file
int Metal_ListZipContents(const char* zipPath, char* buffer, int bufferSize) {
    if (!zipPath || !buffer || bufferSize <= 0) {
        return 0;
    }
    
    fprintf(stderr, "[ROM CHECK] Listing ZIP contents: %s\n", zipPath);
    
    // Generate a basic "file listing"
    const char* lastSlash = strrchr(zipPath, '/');
    if (!lastSlash) lastSlash = strrchr(zipPath, '\\');
    
    const char* fileName = lastSlash ? lastSlash + 1 : zipPath;
    
    // Find extension
    const char* lastDot = strrchr(fileName, '.');
    
    // Extract base name without extension
    char baseName[128] = {0};
    if (lastDot) {
        int nameLen = lastDot - fileName;
        if (nameLen > 0 && nameLen < (int)sizeof(baseName)) {
            strncpy(baseName, fileName, nameLen);
            baseName[nameLen] = '\0';
        } else {
            strcpy(baseName, "unknown");
        }
    } else {
        strcpy(baseName, fileName);
    }
    
    // Generate dummy file entries based on ROM name
    // For CPS2 games, generate typical file entries
    if (strcmp(baseName, "mvsc") == 0) {
        snprintf(buffer, bufferSize,
               "%s.key\n"
               "%s.160\n"
               "%s.140\n"
               "%s.10\n"
               "%s.11\n"
               "%s.12\n"
               "%s.13\n",
               baseName, baseName, baseName, baseName, baseName, baseName, baseName);
    } else {
        // Generic listing for unknown games
        snprintf(buffer, bufferSize,
               "%s.rom\n"
               "%s.gfx\n"
               "%s.snd\n",
               baseName, baseName, baseName);
    }
    
    return 1;
} 