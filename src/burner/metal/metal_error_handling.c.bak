#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#include "metal_declarations.h"
#include "metal_error_handling.h"

// Global error state
// REMOVED: Using the implementation from error_functions.c
// MetalErrorInfo g_lastError = {0};

// Error constants
const MetalErrorCode METAL_SUCCESS = 0;
const MetalErrorCode METAL_ERROR_GENERAL = -1;
const MetalErrorCode METAL_ERROR_INVALID_PARAM = -2;
const MetalErrorCode METAL_ERROR_FILE_NOT_FOUND = -3;
const MetalErrorCode METAL_ERROR_MEMORY_ALLOC = -4;
const MetalErrorCode METAL_ERROR_ROM_LOAD = -5;
const MetalErrorCode METAL_ERROR_RENDERER_INIT = -6;
const MetalErrorCode METAL_ERROR_SHADER_COMPILE = -7;
const MetalErrorCode METAL_ERROR_AUDIO_INIT = -8;
const MetalErrorCode METAL_ERROR_INPUT_INIT = -9;
const MetalErrorCode METAL_ERROR_UNSUPPORTED = -10;

// Current log level (default to warnings and errors)
MetalLogLevel g_logLevel = LOG_LEVEL_WARNING;

// Log file handle
static FILE* g_logFile = NULL;
static const char* LOG_FILE_PATH = "metal_fbneo.log";
static bool g_loggingInitialized = false;

// Initialize logging
static void InitializeLogging() {
    if (g_loggingInitialized) return;
    
    // Open log file
    g_logFile = fopen(LOG_FILE_PATH, "w");
    if (g_logFile) {
        time_t now = time(NULL);
        fprintf(g_logFile, "FBNeo Metal Log - Started %s", ctime(&now));
        fprintf(g_logFile, "---------------------------------------------------\n");
        fflush(g_logFile);
    }
    
    g_loggingInitialized = true;
}

// Close logging
static void ShutdownLogging() {
    if (g_logFile) {
        time_t now = time(NULL);
        fprintf(g_logFile, "\nFBNeo Metal Log - Ended %s", ctime(&now));
        fprintf(g_logFile, "---------------------------------------------------\n");
        fclose(g_logFile);
        g_logFile = NULL;
    }
    
    g_loggingInitialized = false;
}

// Convert log level to string
static const char* LogLevelToString(MetalLogLevel level) {
    switch (level) {
        case LOG_LEVEL_ERROR:   return "ERROR";
        case LOG_LEVEL_WARNING: return "WARNING";
        case LOG_LEVEL_INFO:    return "INFO";
        case LOG_LEVEL_DEBUG:   return "DEBUG";
        case LOG_LEVEL_VERBOSE: return "VERBOSE";
        default:                return "UNKNOWN";
    }
}

// Set log level
void Metal_SetLogLevel(MetalLogLevel level) {
    g_logLevel = level;
    
    // Initialize logging if needed
    if (!g_loggingInitialized) {
        InitializeLogging();
    }
    
    Metal_LogMessage(LOG_LEVEL_INFO, "Log level set to %s", LogLevelToString(level));
}

// Log a message with a given level
void Metal_LogMessage(MetalLogLevel level, const char* format, ...) {
    // Skip if log level is higher than current setting
    if (level > g_logLevel) return;
    
    // Initialize logging if needed
    if (!g_loggingInitialized) {
        InitializeLogging();
    }
    
    va_list args;
    char buffer[1024];
    
    // Format timestamp
    time_t now = time(NULL);
    struct tm* timeInfo = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%H:%M:%S", timeInfo);
    
    // Format log level
    const char* levelStr = LogLevelToString(level);
    
    // Format the message
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer)-1, format, args);
    va_end(args);
    buffer[sizeof(buffer)-1] = '\0';
    
    // Print to console for errors and warnings
    if (level <= LOG_LEVEL_WARNING) {
        printf("[%s] %s: %s\n", timestamp, levelStr, buffer);
    }
    
    // Write to log file
    if (g_logFile) {
        fprintf(g_logFile, "[%s] %s: %s\n", timestamp, levelStr, buffer);
        fflush(g_logFile);
    }
}

// Dump binary data in hex format
void Metal_LogHexDump(const void* data, size_t size, const char* description) {
    if (!data || size == 0 || g_logLevel < LOG_LEVEL_VERBOSE) return;
    
    // Initialize logging if needed
    if (!g_loggingInitialized) {
        InitializeLogging();
    }
    
    const unsigned char* bytes = (const unsigned char*)data;
    Metal_LogMessage(LOG_LEVEL_VERBOSE, "Hex dump of %s (%zu bytes):", 
                    description ? description : "data", size);
    
    // Limit size to avoid huge dumps
    const size_t maxBytes = (size > 256) ? 256 : size;
    
    char line[100];
    char* ptr;
    
    for (size_t offset = 0; offset < maxBytes; offset += 16) {
        // Format address
        snprintf(line, sizeof(line), "%04zX: ", offset);
        
        // Append hex values
        ptr = line + strlen(line);
        for (size_t i = 0; i < 16; i++) {
            if (offset + i < maxBytes) {
                snprintf(ptr, sizeof(line) - (ptr - line), "%02X ", bytes[offset + i]);
            } else {
                snprintf(ptr, sizeof(line) - (ptr - line), "   ");
            }
            ptr += 3;
            
            // Extra space after 8 bytes
            if (i == 7) {
                snprintf(ptr, sizeof(line) - (ptr - line), " ");
                ptr += 1;
            }
        }
        
        // Append ASCII values
        snprintf(ptr, sizeof(line) - (ptr - line), "| ");
        ptr += 2;
        for (size_t i = 0; i < 16; i++) {
            if (offset + i < maxBytes) {
                unsigned char c = bytes[offset + i];
                if (c >= 32 && c <= 126) {
                    snprintf(ptr, sizeof(line) - (ptr - line), "%c", c);
                } else {
                    snprintf(ptr, sizeof(line) - (ptr - line), ".");
                }
                ptr += 1;
            }
        }
        
        // Log the complete line
        Metal_LogMessage(LOG_LEVEL_VERBOSE, "%s", line);
    }
    
    if (size > maxBytes) {
        Metal_LogMessage(LOG_LEVEL_VERBOSE, "... truncated (%zu bytes total)", size);
    }
}

// Set error with message only
void Metal_SetError(MetalErrorCode code, const char* message) {
    g_lastError.code = code;
    if (message) {
        strncpy(g_lastError.message, message, sizeof(g_lastError.message) - 1);
        g_lastError.message[sizeof(g_lastError.message) - 1] = '\0';
    } else {
        g_lastError.message[0] = '\0';
    }
    
    // Clear other fields
    g_lastError.function[0] = '\0';
    g_lastError.file[0] = '\0';
    g_lastError.line = 0;
}

// Set error with detailed info
void Metal_SetErrorEx(MetalErrorCode code, const char* message, const char* function, const char* file, int line) {
    g_lastError.code = code;
    
    if (message) {
        strncpy(g_lastError.message, message, sizeof(g_lastError.message) - 1);
        g_lastError.message[sizeof(g_lastError.message) - 1] = '\0';
    } else {
        g_lastError.message[0] = '\0';
    }
    
    if (function) {
        strncpy(g_lastError.function, function, sizeof(g_lastError.function) - 1);
        g_lastError.function[sizeof(g_lastError.function) - 1] = '\0';
    } else {
        g_lastError.function[0] = '\0';
    }
    
    if (file) {
        strncpy(g_lastError.file, file, sizeof(g_lastError.file) - 1);
        g_lastError.file[sizeof(g_lastError.file) - 1] = '\0';
    } else {
        g_lastError.file[0] = '\0';
    }
    
    g_lastError.line = line;
}

// Debug fallback functions
int Metal_EnableFallbackRenderer() {
    Metal_LogMessage(LOG_LEVEL_WARNING, "Enabling fallback renderer");
    return 0;
}

int Metal_EnableFallbackAudio() {
    Metal_LogMessage(LOG_LEVEL_WARNING, "Enabling fallback audio");
    return 0;
}

int Metal_EnableFallbackInput() {
    Metal_LogMessage(LOG_LEVEL_WARNING, "Enabling fallback input");
    return 0;
}

// Debug mode tracking
static bool g_debugMode = false;
static int g_frameLogInterval = 60;  // Log every 60 frames by default

bool Metal_IsDebugMode() {
    return g_debugMode;
}

void Metal_SetDebugMode(bool enabled) {
    g_debugMode = enabled;
    Metal_LogMessage(LOG_LEVEL_INFO, "Debug mode %s", enabled ? "enabled" : "disabled");
    
    // Also set verbose logging if debug mode is enabled
    if (enabled) {
        Metal_SetLogLevel(LOG_LEVEL_DEBUG);
    }
}

void Metal_SetFrameLogInterval(int frames) {
    g_frameLogInterval = frames;
    Metal_LogMessage(LOG_LEVEL_INFO, "Frame log interval set to %d", frames);
}

// Game state dumping - for debugging
void Metal_DumpGameState() {
    Metal_LogMessage(LOG_LEVEL_DEBUG, "==== GAME STATE DUMP ====");
    
    // Log frame buffer info if available
    if (g_frameBuffer.data) {
        Metal_LogMessage(LOG_LEVEL_DEBUG, "Frame buffer: %dx%d, pitch: %d", 
                        g_frameBuffer.width, g_frameBuffer.height, g_frameBuffer.pitch);
        
        // Dump a small portion of the frame buffer
        if (g_logLevel >= LOG_LEVEL_VERBOSE) {
            // Just dump first few pixels
            size_t sampleSize = 16 * sizeof(uint32_t);  // 16 pixels
            Metal_LogHexDump(g_frameBuffer.data, sampleSize, "Frame buffer sample");
        }
    } else {
        Metal_LogMessage(LOG_LEVEL_DEBUG, "Frame buffer: NULL");
    }
    
    // Driver information if available
    const char* drvName = BurnDrvGetTextA(0);  // DRV_NAME
    const char* drvFullName = BurnDrvGetTextA(2);  // DRV_FULLNAME
    
    if (drvName && drvFullName) {
        Metal_LogMessage(LOG_LEVEL_DEBUG, "Driver: %s - %s", drvName, drvFullName);
    }
    
    Metal_LogMessage(LOG_LEVEL_DEBUG, "========================");
}

// Cleanup when program exits
__attribute__((destructor))
static void CleanupErrorHandling() {
    ShutdownLogging();
} 