#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <time.h>

// Error info structure
typedef struct {
    int code;
    char message[256];
    char function[64];
    char file[128];
    int line;
} MetalErrorInfo;

// Global error state
// REMOVED: Using the implementation from error_functions.c
// MetalErrorInfo g_lastError = {0};

// Error constants
const int METAL_SUCCESS = 0;
const int METAL_ERROR_GENERAL = -1;
const int METAL_ERROR_INVALID_PARAM = -2;
const int METAL_ERROR_FILE_NOT_FOUND = -3;
const int METAL_ERROR_MEMORY_ALLOC = -4;
const int METAL_ERROR_ROM_LOAD = -5;
const int METAL_ERROR_RENDERER_INIT = -6;
const int METAL_ERROR_SHADER_COMPILE = -7;
const int METAL_ERROR_AUDIO_INIT = -8;
const int METAL_ERROR_INPUT_INIT = -9;
const int METAL_ERROR_UNSUPPORTED = -10;

// Current log level (default to warnings and errors)
int g_logLevel = 2; // LOG_LEVEL_WARNING

// Log message with variable arguments
void Metal_LogMessage(int level, const char* format, ...) {
    // Skip if log level is higher than current setting
    if (level > g_logLevel) return;
    
    va_list args;
    char buffer[1024];
    
    // Format the message
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer)-1, format, args);
    va_end(args);
    buffer[sizeof(buffer)-1] = '\0';
    
    // Print to console for all messages
    printf("[Metal] %s\n", buffer);
}

// Set error with message only
void Metal_SetError(int code, const char* message) {
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
    
    // Log the error
    Metal_LogMessage(1, "ERROR: %s (code %d)", message ? message : "Unknown error", code);
}

// Get the last error message
const char* Metal_GetLastErrorMessage() {
    return g_lastError.message;
}

// Check if there's an error
bool Metal_HasError() {
    return g_lastError.code != 0;
}

// Clear the last error
void Metal_ClearLastError() {
    g_lastError.code = 0;
    memset(g_lastError.message, 0, sizeof(g_lastError.message));
    memset(g_lastError.function, 0, sizeof(g_lastError.function));
    memset(g_lastError.file, 0, sizeof(g_lastError.file));
    g_lastError.line = 0;
}

// Set log level
void Metal_SetLogLevel(int level) {
    g_logLevel = level;
    Metal_LogMessage(2, "Log level set to %d", level);
}

// Debug mode tracking
static bool s_debugMode = false;

bool Metal_IsDebugMode() {
    return s_debugMode;
}

void Metal_SetDebugMode(bool enabled) {
    s_debugMode = enabled;
    Metal_LogMessage(2, "Debug mode %s", enabled ? "enabled" : "disabled");
}

// Fallback functions
int Metal_EnableFallbackAudio() {
    Metal_LogMessage(2, "Enabling fallback audio");
    return 0;
}

int Metal_EnableFallbackRenderer() {
    Metal_LogMessage(2, "Enabling fallback renderer");
    return 0;
}

int Metal_EnableFallbackInput() {
    Metal_LogMessage(2, "Enabling fallback input");
    return 0;
}
