#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct {
    int code;
    char message[256];
} ErrorInfo;

// REMOVED: Using the implementation from error_functions.c
// static ErrorInfo g_lastError = {0};

void Metal_SetError(int code, const char* message) {
    g_lastError.code = code;
    if (message) {
        strncpy(g_lastError.message, message, sizeof(g_lastError.message) - 1);
    }
    printf("ERROR: %s (code %d)\n", message, code);
}

bool Metal_HasError(void) {
    return g_lastError.code != 0;
}

const char* Metal_GetLastErrorMessage(void) {
    return g_lastError.message;
}

void Metal_SetDebugMode(bool enable) {
    printf("Debug mode: %s\n", enable ? "ON" : "OFF");
}

int Metal_EnableFallbackAudio(void) {
    return 0;
}
