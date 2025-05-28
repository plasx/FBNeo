#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>

// Include our fixes for C/C++ compatibility
#include "fixes/c_cpp_compatibility.h"
#include "metal_declarations.h"
#include "metal_rom_loader.h"
#include "metal_zip_extract.h"
#include "rom_loading_debug.h"

// Define log file path
#define ROM_LOADING_LOG_PATH "rom_loading_debug.log"
#define MAX_DEBUG_LOG_SIZE (1024 * 1024 * 10) // Limit to 10MB

// Enable or disable console debugging (on by default for now)
#define ENABLE_CONSOLE_DEBUG 1

// Global debug file
static FILE* g_debugLogFile = NULL;

// Current debug log level - can be adjusted at runtime
static int g_currentLogLevel = LOG_DETAIL;

// Initialize debug log file
void ROMLoader_InitDebugLog() {
    // Open the log file if it's not already open
    if (!g_debugLogFile) {
        g_debugLogFile = fopen(ROM_LOADING_LOG_PATH, "w");
        if (g_debugLogFile) {
            fprintf(g_debugLogFile, "===== ROM Loading Debug Log Started =====\n");
            fprintf(g_debugLogFile, "FBNeo Metal Implementation Debug Log\n");
            fprintf(g_debugLogFile, "=======================================================\n\n");
            fflush(g_debugLogFile);
        }
    }
}

// Close debug log file
void ROMLoader_CloseDebugLog() {
    if (g_debugLogFile) {
        fprintf(g_debugLogFile, "\n===== ROM Loading Debug Log Closed =====\n");
        fclose(g_debugLogFile);
        g_debugLogFile = NULL;
    }
}

// Write to debug log with level
void ROMLoader_DebugLog(int level, const char* format, ...) {
    if (level > g_currentLogLevel) {
        return; // Skip if level is higher than current setting
    }
    
    va_list args;
    va_start(args, format);
    
    // Get current log level prefix
    const char* levelPrefix = "";
    switch (level) {
        case LOG_ERROR:   levelPrefix = "[ERROR] "; break;
        case LOG_WARNING: levelPrefix = "[WARNING] "; break;
        case LOG_INFO:    levelPrefix = "[INFO] "; break;
        case LOG_DETAIL:  levelPrefix = "[DETAIL] "; break;
        case LOG_VERBOSE: levelPrefix = "[VERBOSE] "; break;
        default:          levelPrefix = "[UNKNOWN] "; break;
    }
    
    // Log to console if enabled
    if (ENABLE_CONSOLE_DEBUG) {
        // Print prefix
        fprintf(stderr, "%s", levelPrefix);
        
        // Print actual message
        vfprintf(stderr, format, args);
        
        // Add newline if not present
        if (format[strlen(format) - 1] != '\n') {
            fprintf(stderr, "\n");
        }
    }
    
    // Reset args for file logging
    va_end(args);
    va_start(args, format);
    
    // Log to file if open
    if (g_debugLogFile) {
        // Print prefix
        fprintf(g_debugLogFile, "%s", levelPrefix);
        
        // Print actual message
        vfprintf(g_debugLogFile, format, args);
        
        // Add newline if not present
        if (format[strlen(format) - 1] != '\n') {
            fprintf(g_debugLogFile, "\n");
        }
        
        // Make sure it's written immediately
        fflush(g_debugLogFile);
        
        // Check file size and rotate if needed
        long fileSize = ftell(g_debugLogFile);
        if (fileSize > MAX_DEBUG_LOG_SIZE) {
            // Close and reopen to truncate
            fclose(g_debugLogFile);
            g_debugLogFile = fopen(ROM_LOADING_LOG_PATH, "w");
            if (g_debugLogFile) {
                fprintf(g_debugLogFile, "===== Log rotated due to size limit =====\n\n");
                fflush(g_debugLogFile);
            }
        }
    }
    
    va_end(args);
}

// Set debug log level
void ROMLoader_SetDebugLevel(int level) {
    if (level >= LOG_ERROR && level <= LOG_VERBOSE) {
        g_currentLogLevel = level;
        ROMLoader_DebugLog(LOG_INFO, "Debug log level set to %d", level);
    }
}

// Dump memory to debug log - useful for inspecting ROM data
void ROMLoader_DumpMemory(const void* data, int size, const char* label) {
    if (g_currentLogLevel < LOG_VERBOSE || !data || size <= 0) {
        return;
    }
    
    const unsigned char* bytes = (const unsigned char*)data;
    ROMLoader_DebugLog(LOG_VERBOSE, "Memory dump of %s (%d bytes):", label, size);
    
    // Maximum bytes to show to avoid huge logs
    const int maxBytes = (size > 256) ? 256 : size;
    
    // Format nicely as hex with ASCII representation
    char line[128]; // Buffer for one line
    char* ptr;
    int len;
    
    for (int i = 0; i < maxBytes; i += 16) {
        ptr = line;
        // Use snprintf instead of sprintf
        len = snprintf(ptr, sizeof(line) - (ptr - line), "%04X: ", i);
        ptr += len;
        
        // Hex values
        for (int j = 0; j < 16; j++) {
            if (i + j < maxBytes) {
                len = snprintf(ptr, sizeof(line) - (ptr - line), "%02X ", bytes[i + j]);
                ptr += len;
            } else {
                len = snprintf(ptr, sizeof(line) - (ptr - line), "   ");
                ptr += len;
            }
            if (j == 7) {
                len = snprintf(ptr, sizeof(line) - (ptr - line), " ");
                ptr += len;
            }
        }
        
        // ASCII values
        len = snprintf(ptr, sizeof(line) - (ptr - line), " |");
        ptr += len;
        
        for (int j = 0; j < 16; j++) {
            if (i + j < maxBytes) {
                unsigned char c = bytes[i + j];
                if (c >= 32 && c <= 126) {
                    len = snprintf(ptr, sizeof(line) - (ptr - line), "%c", c);
                    ptr += len;
                } else {
                    len = snprintf(ptr, sizeof(line) - (ptr - line), ".");
                    ptr += len;
                }
            } else {
                len = snprintf(ptr, sizeof(line) - (ptr - line), " ");
                ptr += len;
            }
        }
        
        snprintf(ptr, sizeof(line) - (ptr - line), "|");
        
        // Log the line
        ROMLoader_DebugLog(LOG_VERBOSE, "%s", line);
    }
    
    if (size > maxBytes) {
        ROMLoader_DebugLog(LOG_VERBOSE, "... (truncated, %d more bytes)", size - maxBytes);
    }
}

// Detailed ROM file information
void ROMLoader_LogROMInfo(const char* romPath) {
    ROMLoader_DebugLog(LOG_INFO, "Examining ROM file: %s", romPath);
    
    // Check if file exists
    struct stat st;
    if (stat(romPath, &st) != 0) {
        ROMLoader_DebugLog(LOG_ERROR, "ROM file does not exist: %s", romPath);
        return;
    }
    
    // Basic file info
    ROMLoader_DebugLog(LOG_INFO, "File size: %lld bytes", (long long)st.st_size);
    ROMLoader_DebugLog(LOG_INFO, "Last modified: %s", ctime(&st.st_mtime));
    
    // List ZIP contents if it's a ZIP file
    if (strstr(romPath, ".zip") || strstr(romPath, ".ZIP")) {
        ROMLoader_DebugLog(LOG_INFO, "Analyzing ZIP contents:");
        
        // Allocate space for filenames
        const int MAX_FILES = 1000;
        char** filenames = (char**)malloc(MAX_FILES * sizeof(char*));
        if (!filenames) {
            ROMLoader_DebugLog(LOG_ERROR, "Failed to allocate memory for ZIP analysis");
            return;
        }
        
        // Allocate each filename buffer
        for (int i = 0; i < MAX_FILES; i++) {
            filenames[i] = (char*)malloc(256);
            if (!filenames[i]) {
                ROMLoader_DebugLog(LOG_ERROR, "Failed to allocate memory for filename %d", i);
                // Free previously allocated buffers
                for (int j = 0; j < i; j++) {
                    free(filenames[j]);
                }
                free(filenames);
                return;
            }
            filenames[i][0] = '\0';
        }
        
        // List the ZIP contents
        int numFiles = 0;
        if (Metal_ListZipContents(romPath, filenames, MAX_FILES, &numFiles) == 0) {
            ROMLoader_DebugLog(LOG_INFO, "ZIP contains %d files:", numFiles);
            
            for (int i = 0; i < numFiles; i++) {
                UINT32 size = 0;
                UINT32 crc = 0;
                Metal_GetZipFileInfo(romPath, filenames[i], &size, &crc);
                ROMLoader_DebugLog(LOG_INFO, "  [%d] %s (Size: %u bytes, CRC32: 0x%08X)", 
                                  i, filenames[i], size, crc);
            }
        } else {
            ROMLoader_DebugLog(LOG_ERROR, "Failed to list ZIP contents");
        }
        
        // Free allocated memory
        for (int i = 0; i < MAX_FILES; i++) {
            free(filenames[i]);
        }
        free(filenames);
    }
}

// Track ROM loading step - used for detailed status output
void ROMLoader_TrackLoadStep(const char* step, const char* format, ...) {
    char buffer[1024];
    va_list args;
    
    // Format the message
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    // Create the formatted message with prefix
    char formattedMessage[1100];
    snprintf(formattedMessage, sizeof(formattedMessage), "[%s] %s", step, buffer);
    
    // Log this step to the debug log
    ROMLoader_DebugLog(LOG_INFO, "%s", formattedMessage);
    
    // Also output directly to console for immediate visibility
    // Don't use buffered output to ensure messages are displayed
    // We'll use plain printf for standardized format
    printf("%s\n", formattedMessage);
    fflush(stdout); // Force output to be displayed immediately
}

// Special function to verify ROM data was loaded correctly
bool ROMLoader_VerifyROMData(const UINT8* data, INT32 size, const char* romName) {
    if (!data || size <= 0) {
        ROMLoader_DebugLog(LOG_ERROR, "Invalid ROM data pointer or size for %s", romName);
        return false;
    }
    
    // Calculate basic checksum for verification
    UINT32 checksum = 0;
    for (INT32 i = 0; i < size; i++) {
        checksum += data[i];
    }
    
    // Count non-zero bytes
    INT32 nonZeroBytes = 0;
    for (INT32 i = 0; i < size; i++) {
        if (data[i] != 0) nonZeroBytes++;
    }
    
    // Calculate percentage of non-zero bytes
    float nonZeroPercent = (nonZeroBytes * 100.0f) / size;
    
    // Log results
    ROMLoader_DebugLog(LOG_INFO, "ROM Data Verification for %s:", romName);
    ROMLoader_DebugLog(LOG_INFO, "  Size: %d bytes", size);
    ROMLoader_DebugLog(LOG_INFO, "  Checksum: 0x%08X", checksum);
    ROMLoader_DebugLog(LOG_INFO, "  Non-zero bytes: %d (%.2f%%)", 
                     nonZeroBytes, nonZeroPercent);
    
    // Dump first 64 bytes for inspection
    ROMLoader_DumpMemory(data, size > 64 ? 64 : size, romName);
    
    // Consider it valid if at least 25% of bytes are non-zero
    // This is a heuristic to detect if data is present and reasonable
    bool isValid = (nonZeroPercent >= 25.0f);
    
    // If data is mostly zero but has some non-zero content (5-25%), 
    // it might be a sparse ROM that's still valid
    if (!isValid && nonZeroPercent >= 5.0f) {
        ROMLoader_DebugLog(LOG_INFO, "  Sparse ROM detected (%.2f%% non-zero)", nonZeroPercent);
        
        // Check if there's a consistent pattern in the data
        bool hasPattern = false;
        if (size >= 16) {
            // Check if there's a repeating pattern in the first 16 bytes
            int patternLen = 0;
            for (patternLen = 1; patternLen <= 8; patternLen++) {
                bool matches = true;
                for (int i = patternLen; i < 16; i++) {
                    if (data[i % patternLen] != data[i]) {
                        matches = false;
                        break;
                    }
                }
                if (matches) {
                    ROMLoader_DebugLog(LOG_INFO, "  Detected pattern of length %d in ROM data", patternLen);
                    hasPattern = true;
                    break;
                }
            }
        }
        
        // If we found a pattern, it might be valid sparse data
        if (hasPattern) {
            isValid = true;
            ROMLoader_DebugLog(LOG_INFO, "  Accepting sparse ROM with pattern");
        }
    }
    
    ROMLoader_DebugLog(LOG_INFO, "  Validation result: %s", isValid ? "VALID" : "INVALID");
    
    return isValid;
}

// Initialize the debugging hooks
__attribute__((constructor))
static void initROMLoaderDebugHooks() {
    ROMLoader_InitDebugLog();
    ROMLoader_DebugLog(LOG_INFO, "ROM Loader Debug hooks initialized");
} 