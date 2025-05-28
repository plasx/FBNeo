#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// Frame buffer structure
typedef struct {
    uint32_t* data;      // Pixel data
    int width;           // Width in pixels
    int height;          // Height in pixels
    int pitch;           // Bytes per row
    bool updated;        // Has been updated
} EmulatorFrameBuffer;

// Minimal set of variables to avoid duplication
EmulatorFrameBuffer g_frameBuffer = {0};
char g_gameTitle[256] = "FBNeo Metal";
bool g_gameRunning = false;

// Function to get the game title
const char* Metal_GetGameTitle() {
    return g_gameTitle;
}

// Set the game title
void Metal_SetGameTitle(const char* title) {
    if (title && title[0]) {
        strncpy(g_gameTitle, title, sizeof(g_gameTitle)-1);
        g_gameTitle[sizeof(g_gameTitle)-1] = '\0';
    } else {
        strcpy(g_gameTitle, "Unknown Game");
    }
}

// Set game running state
void Metal_SetGameRunning(bool running) {
    g_gameRunning = running;
}

// Error info structure
typedef struct {
    int code;
    char message[256];
    char function[64];
    char file[128];
    int line;
} MetalErrorInfo;

// Global error state
MetalErrorInfo g_lastError = {0};
