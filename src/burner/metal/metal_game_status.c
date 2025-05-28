#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "metal_declarations.h"

// Game status tracking
static char g_gameTitle[256] = "No Game Loaded";
static bool g_gameRunning = false;
static int g_totalFrames = 0;
static double g_frameRate = 60.0;

// Timing
static struct timespec g_lastFrameTime;
static double g_frameTimes[60] = {0};  // Keep track of last 60 frames
static int g_frameTimeIndex = 0;

// Set game title
void Metal_SetGameTitle(const char* title) {
    if (title && title[0]) {
        strncpy(g_gameTitle, title, sizeof(g_gameTitle)-1);
        g_gameTitle[sizeof(g_gameTitle)-1] = '\0';
    } else {
        strcpy(g_gameTitle, "Unknown Game");
    }
}

// Get game title
const char* Metal_GetGameTitle() {
    return g_gameTitle;
}

// Set game running state
void Metal_SetGameRunning(bool running) {
    g_gameRunning = running;
    
    // Reset frame counter when starting a new game
    if (running) {
        g_totalFrames = 0;
        
        // Initialize frame timing
        clock_gettime(CLOCK_MONOTONIC, &g_lastFrameTime);
        
        // Clear frame times
        for (int i = 0; i < 60; i++) {
            g_frameTimes[i] = 1.0 / 60.0;  // Default to 60fps (16.67ms)
        }
        g_frameTimeIndex = 0;
    }
}

// Get game running state
bool Metal_IsGameRunning() {
    return g_gameRunning;
}

// Track a new rendered frame
void Metal_TrackFrame() {
    if (!g_gameRunning) return;
    
    // Increment frame counter
    g_totalFrames++;
    
    // Update timing
    struct timespec currentTime;
    clock_gettime(CLOCK_MONOTONIC, &currentTime);
    
    // Calculate frame time in seconds
    double frameTime = (currentTime.tv_sec - g_lastFrameTime.tv_sec) + 
                      (currentTime.tv_nsec - g_lastFrameTime.tv_nsec) / 1000000000.0;
    
    // Store frame time
    g_frameTimes[g_frameTimeIndex] = frameTime;
    g_frameTimeIndex = (g_frameTimeIndex + 1) % 60;
    
    // Set current time as last frame time
    g_lastFrameTime = currentTime;
    
    // Calculate average frame rate over last 60 frames
    if (g_totalFrames >= 60) {
        double totalTime = 0.0;
        for (int i = 0; i < 60; i++) {
            totalTime += g_frameTimes[i];
        }
        
        // Avoid division by zero
        if (totalTime > 0.0) {
            g_frameRate = 60.0 / totalTime;
        }
    }
    
    // Debugging - periodically log frame rate
    if (Metal_IsDebugMode() && g_totalFrames % 60 == 0) {
        Metal_LogMessage(LOG_LEVEL_DEBUG, "Frame %d - FPS: %.2f", g_totalFrames, g_frameRate);
    }
}

// Get current frame rate
float Metal_GetFrameRate() {
    return (float)g_frameRate;
}

// Get total frames rendered
int Metal_GetTotalFrames() {
    return g_totalFrames;
}

// Compute frame timing information for UI display
void Metal_GetFrameTiming(double* averageFrameTime, double* minFrameTime, double* maxFrameTime) {
    if (!averageFrameTime || !minFrameTime || !maxFrameTime) {
        return;
    }
    
    // Default values
    *averageFrameTime = 1.0 / 60.0;  // 16.67ms
    *minFrameTime = 1.0 / 60.0;
    *maxFrameTime = 1.0 / 60.0;
    
    if (g_totalFrames < 60) {
        return;
    }
    
    // Calculate statistics
    double totalTime = 0.0;
    double min = 1.0;  // Start with a high value
    double max = 0.0;  // Start with a low value
    
    for (int i = 0; i < 60; i++) {
        totalTime += g_frameTimes[i];
        if (g_frameTimes[i] < min) min = g_frameTimes[i];
        if (g_frameTimes[i] > max) max = g_frameTimes[i];
    }
    
    *averageFrameTime = totalTime / 60.0;
    *minFrameTime = min;
    *maxFrameTime = max;
}

// Return an estimated percentage of target frame rate (60fps)
int Metal_GetPerformancePercentage() {
    if (g_frameRate <= 0.0) return 0;
    
    int percentage = (int)((g_frameRate / 60.0) * 100.0);
    return (percentage > 100) ? 100 : percentage;
} 