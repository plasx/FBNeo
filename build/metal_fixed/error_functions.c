#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>

// Error info structure
typedef struct {
    int code;
    char message[256];
    char function[64];
    char file[128];
    int line;
} MetalErrorInfo;

// Global error state - this is the ONLY global instance
MetalErrorInfo g_lastError = {0};

// Error handling functions
void Metal_ClearLastError(void) {
    g_lastError.code = 0;
    memset(g_lastError.message, 0, sizeof(g_lastError.message));
    memset(g_lastError.function, 0, sizeof(g_lastError.function));
    memset(g_lastError.file, 0, sizeof(g_lastError.file));
    g_lastError.line = 0;
}

bool Metal_HasError(void) {
    return g_lastError.code != 0;
}

const char* Metal_GetLastErrorMessage(void) {
    return g_lastError.message;
}

void Metal_SetError(int code, const char* message) {
    g_lastError.code = code;
    if (message) {
        strncpy(g_lastError.message, message, sizeof(g_lastError.message) - 1);
        g_lastError.message[sizeof(g_lastError.message) - 1] = '\0';
    } else {
        g_lastError.message[0] = '\0';
    }
    
    // Print the error to console
    printf("ERROR: %s (code %d)\n", message ? message : "Unknown error", code);
}

// Log functions
int g_logLevel = 2; // Default to warning level

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
    
    // Print to console
    printf("[Metal] %s\n", buffer);
}

void Metal_SetLogLevel(int level) {
    g_logLevel = level;
}

int Metal_EnableFallbackAudio(void) {
    printf("Enabling fallback audio\n");
    return 0;
}

// Debug mode tracking
static bool g_debugMode = false;

void Metal_SetDebugMode(bool enabled) {
    g_debugMode = enabled;
    printf("Debug mode: %s\n", enabled ? "ON" : "OFF");
}

bool Metal_IsDebugMode(void) {
    return g_debugMode;
}
