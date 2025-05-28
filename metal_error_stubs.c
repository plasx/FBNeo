#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>

// Error info structure
typedef struct {
    int code;
    char message[256];
    char function[64];
    char file[128];
    int line;
} MetalErrorInfo;

// Global error info (need this here for standalone_main.mm)
MetalErrorInfo g_lastError = {0};

// Stub implementations for error handling and logging

// Clear the last error
void Metal_ClearLastError(void) {
    // Nothing to do in stub
}

// Get the last error message
const char* Metal_GetLastErrorMessage(void) {
    return "No error";
}

// Check if there's an error
bool Metal_HasError(void) {
    return false;
}

// Set debug mode
void Metal_SetDebugMode(bool enable) {
    printf("Metal_SetDebugMode: %s\n", enable ? "enabled" : "disabled");
}

// Set log level
typedef enum {
    LOG_LEVEL_NONE = 0,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_VERBOSE
} MetalLogLevel;

void Metal_SetLogLevel(MetalLogLevel level) {
    printf("Metal_SetLogLevel: %d\n", level);
}

// Log a message
void Metal_LogMessage(MetalLogLevel level, const char* format, ...) {
    if (format) {
        va_list args;
        va_start(args, format);
        printf("[Metal Log] ");
        vprintf(format, args);
        printf("\n");
        va_end(args);
    }
}

// Enable fallback audio
void Metal_EnableFallbackAudio(bool enable) {
    printf("Metal_EnableFallbackAudio: %s\n", enable ? "enabled" : "disabled");
}

// Check if debug mode is enabled
bool Metal_IsDebugMode(void) {
    return false;
} 