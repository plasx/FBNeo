#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// Frame buffer structure
typedef struct {
    uint32_t* data;      // Pixel data
    int width;           // Width in pixels
    int height;          // Height in pixels
    int pitch;           // Bytes per row
    bool updated;        // Has been updated
} FrameBuffer;

// Global frame buffer instance
FrameBuffer g_frameBuffer = {0};

// Game state variables
char g_gameTitle[256] = "FBNeo Metal";
bool g_gameRunning = false;

// Frame buffer access
FrameBuffer* Metal_GetFrameBuffer() {
    return &g_frameBuffer;
}

// Initialize frame buffer
bool InitFrameBuffer(int width, int height) {
    if (width <= 0 || height <= 0) {
        printf("Invalid frame buffer dimensions: %dx%d\n", width, height);
        return false;
    }
    
    // Free existing buffer if any
    if (g_frameBuffer.data) {
        free(g_frameBuffer.data);
        g_frameBuffer.data = NULL;
    }
    
    // Allocate new buffer
    size_t bufferSize = width * height * sizeof(uint32_t);
    g_frameBuffer.data = (uint32_t*)malloc(bufferSize);
    
    if (!g_frameBuffer.data) {
        printf("Failed to allocate frame buffer memory (%zu bytes)\n", bufferSize);
        return false;
    }
    
    // Initialize buffer properties
    g_frameBuffer.width = width;
    g_frameBuffer.height = height;
    g_frameBuffer.pitch = width * sizeof(uint32_t);
    g_frameBuffer.updated = false;
    
    // Clear buffer to black
    memset(g_frameBuffer.data, 0, bufferSize);
    
    printf("Frame buffer initialized: %dx%d (%zu bytes)\n", width, height, bufferSize);
    return true;
}

// Generate test pattern
int Metal_GenerateTestPattern(int patternType) {
    // Only proceed if frame buffer exists
    if (!g_frameBuffer.data || g_frameBuffer.width <= 0 || g_frameBuffer.height <= 0) {
        return -1;
    }
    
    uint32_t* buffer = g_frameBuffer.data;
    int width = g_frameBuffer.width;
    int height = g_frameBuffer.height;
    
    // Fill with a simple pattern (checkerboard)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint32_t color;
            
            switch (patternType) {
                case 0: // Gradient
                    color = (((x * 255) / width) << 16) | 
                           (((y * 255) / height) << 8) | 
                           ((x + y) * 255) / (width + height);
                    break;
                    
                case 1: // Checkerboard
                    color = ((x / 16) + (y / 16)) % 2 ? 0xFFFFFFFF : 0xFF000000;
                    break;
                    
                case 2: // Grid
                    color = (x % 32 == 0 || y % 32 == 0) ? 0xFFFFFFFF : 0xFF000000;
                    break;
                    
                default:
                    // Red background for unknown pattern
                    color = 0xFFFF0000;
                    break;
            }
            
            buffer[y * width + x] = color | 0xFF000000; // Ensure alpha is set
        }
    }
    
    // Mark frame buffer as updated
    g_frameBuffer.updated = true;
    
    return 0;
}

// Game title functions
const char* Metal_GetGameTitle() {
    return g_gameTitle;
}

void Metal_SetGameTitle(const char* title) {
    if (title && title[0]) {
        strncpy(g_gameTitle, title, sizeof(g_gameTitle)-1);
        g_gameTitle[sizeof(g_gameTitle)-1] = '\0';
    } else {
        strcpy(g_gameTitle, "Unknown Game");
    }
}

// Game running state
bool Metal_IsGameRunning() {
    return g_gameRunning;
}

void Metal_SetGameRunning(bool running) {
    g_gameRunning = running;
}

// Frame rate tracking
static float s_frameRate = 60.0f;
static int s_totalFrames = 0;

float Metal_GetFrameRate() {
    return s_frameRate;
}

int Metal_GetTotalFrames() {
    return s_totalFrames;
}

void Metal_TrackFrame() {
    s_totalFrames++;
    // In a full implementation, we would calculate the actual frame rate here
}
