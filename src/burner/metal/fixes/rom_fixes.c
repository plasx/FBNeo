#include "burnint.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <zlib.h>

// ROM loading path storage
static char g_szROMPath[MAX_PATH] = {0};

// Get the current ROM path
const char* GetROMPathString() {
    return g_szROMPath;
}

// Set the current ROM path
int SetCurrentROMPath(const char* szPath) {
    if (!szPath) {
        return 1;
    }
    
    strncpy(g_szROMPath, szPath, MAX_PATH - 1);
    g_szROMPath[MAX_PATH - 1] = '\0';
    
    printf("[ROM_FIX] Set ROM path to: %s\n", g_szROMPath);
    return 0;
}

// Function to verify if a ROM file exists and matches expected size
int VerifyROMFile(const char* szName, unsigned int nLen) {
    char szFullPath[MAX_PATH];
    
    if (!szName || !g_szROMPath[0]) {
        return 0;
    }
    
    // Build full path
    snprintf(szFullPath, MAX_PATH, "%s/%s", g_szROMPath, szName);
    
    // Check if file exists
    struct stat st;
    if (stat(szFullPath, &st) != 0) {
        printf("[ROM_FIX] ERROR: ROM file not found: %s\n", szFullPath);
        return 0;
    }
    
    // Check file size
    if ((unsigned int)st.st_size < nLen) {
        printf("[ROM_FIX] ERROR: ROM file size mismatch for %s\n", szName);
        printf("[ROM_FIX] Expected: %u bytes, Found: %lld bytes\n", 
               nLen, (long long)st.st_size);
        return 0;
    }
    
    return 1;
}

// Function to calculate CRC32 of a file
unsigned int CalculateFileCRC32(const char* szFullPath) {
    FILE* f = fopen(szFullPath, "rb");
    if (!f) {
        return 0;
    }
    
    unsigned int crc = 0;
    unsigned char buffer[4096];
    size_t bytesRead;
    
    crc = crc32(0L, Z_NULL, 0);
    
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), f)) > 0) {
        crc = crc32(crc, buffer, bytesRead);
    }
    
    fclose(f);
    return crc;
}

// Function to load a ROM file into memory
// Returns 1 on success, 0 on failure
int LoadROMFile(const char* szName, unsigned char* pDest, unsigned int nLen) {
    char szFullPath[MAX_PATH];
    
    if (!szName || !pDest || !g_szROMPath[0]) {
        return 0;
    }
    
    // Build full path
    snprintf(szFullPath, MAX_PATH, "%s/%s", g_szROMPath, szName);
    
    // Open file
    FILE* f = fopen(szFullPath, "rb");
    if (!f) {
        printf("[ROM_FIX] ERROR: Failed to open ROM file: %s\n", szFullPath);
        return 0;
    }
    
    // Read data
    size_t bytesRead = fread(pDest, 1, nLen, f);
    fclose(f);
    
    if (bytesRead < nLen) {
        printf("[ROM_FIX] ERROR: Failed to read full ROM data from %s\n", szName);
        printf("[ROM_FIX] Expected: %u bytes, Read: %zu bytes\n", nLen, bytesRead);
        return 0;
    }
    
    return 1;
}

// Function to scan the ROM directory for valid ROM files
int ScanROMDirectory() {
    DIR* dir;
    struct dirent* ent;
    int fileCount = 0;
    
    if (!g_szROMPath[0]) {
        printf("[ROM_FIX] ERROR: ROM path not set\n");
        return 0;
    }
    
    dir = opendir(g_szROMPath);
    if (!dir) {
        printf("[ROM_FIX] ERROR: Failed to open ROM directory: %s\n", g_szROMPath);
        return 0;
    }
    
    printf("[ROM_FIX] Scanning ROM directory: %s\n", g_szROMPath);
    
    while ((ent = readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
            continue;
        }
        
        // Skip directories
        char fullPath[MAX_PATH];
        snprintf(fullPath, MAX_PATH, "%s/%s", g_szROMPath, ent->d_name);
        
        struct stat st;
        if (stat(fullPath, &st) == 0 && S_ISREG(st.st_mode)) {
            printf("[ROM_FIX] Found ROM file: %s (%lld bytes)\n", 
                   ent->d_name, (long long)st.st_size);
            fileCount++;
        }
    }
    
    closedir(dir);
    
    printf("[ROM_FIX] ROM directory scan complete. Found %d files.\n", fileCount);
    return fileCount;
}

// Function to extract files from a ZIP archive to the ROM directory
int ExtractZIPFile(const char* szZipPath) {
    printf("[ROM_FIX] ZIP extraction not implemented yet\n");
    printf("[ROM_FIX] Please extract %s manually to %s\n", szZipPath, g_szROMPath);
    return 0;
} 