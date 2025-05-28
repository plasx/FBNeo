#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>  // For memset

// Forward declarations
bool InitFrameBuffer(int width, int height);

// Game state variables
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

// BurnDrv related functions
int BurnDrvExit(void) {
    printf("BurnDrvExit called\n");
    return 0;
}

const char* BurnDrvGetTextA(unsigned int iIndex) {
    static char gameName[] = "Metal CPS2 Game";
    return gameName;
}

int BurnDrvGetVisibleSize(int* pnWidth, int* pnHeight) {
    if (pnWidth) *pnWidth = 384;  // Standard CPS2 resolution
    if (pnHeight) *pnHeight = 224;
    return 0;
}

// Sound functions
int BurnSoundRender(int16_t* pDest, int nLen) {
    // Just fill with silence
    if (pDest && nLen > 0) {
        memset(pDest, 0, nLen * 2 * sizeof(int16_t));
    }
    return 0;
}

// Memory functions
int Memory_Init(void) {
    printf("Memory_Init called\n");
    return 0;
}

// Frame buffer functions
typedef struct {
    uint32_t* data;
    int width;
    int height;
    int pitch;
    bool updated;
} FrameBuffer;

// The frame buffer is defined in metal_standalone_main.mm
extern FrameBuffer g_frameBuffer;

FrameBuffer* Metal_GetFrameBuffer(void) {
    return &g_frameBuffer;
}

// Frame rate tracking
float Metal_GetFrameRate(void) {
    return 60.0f;
}

int Metal_GetTotalFrames(void) {
    return 0;
}

// ROM loading functions
bool Metal_LoadAndInitROM(const char* path) {
    printf("Metal_LoadAndInitROM: %s\n", path);
    return true;
}

int Metal_SetRomPath(const char* path) {
    printf("Metal_SetRomPath: %s\n", path);
    return 0;
}

// Frame processing
int Metal_ProcessFrame(void) {
    return 0;
}

// Frame tracking
void Metal_TrackFrame(void) {
    // Nothing to do
}

// Additional functions from metal_game_control.c
void Metal_ProcessKeyDown(int keyCode) {
    printf("Metal_ProcessKeyDown: %d\n", keyCode);
}

void Metal_ProcessKeyUp(int keyCode) {
    printf("Metal_ProcessKeyUp: %d\n", keyCode);
}

void Metal_UpdateInputState(void) {
    // Nothing to do
}

int Metal_ProcessAudio(void) {
    // Process audio for current frame (stub implementation)
    return 0;
}

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
            int checker = ((x / 16) + (y / 16)) % 2;
            uint32_t color = checker ? 0xFFFFFFFF : 0xFF000000;
            buffer[y * width + x] = color;
        }
    }
    
    // Mark frame buffer as updated
    g_frameBuffer.updated = true;
    
    return 0;
}

int Graphics_InitComponents(void) {
    printf("Graphics_InitComponents: Initializing graphics components\n");
    
    // For now, just initialize the frame buffer
    if (!g_frameBuffer.data) {
        // Create a default frame buffer
        InitFrameBuffer(384, 224);  // Default CPS2 resolution
    }
    
    return 0;
}

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

void Metal_UnloadROM(void) {
    printf("Metal_UnloadROM called\n");
    // Clean up driver if active
    BurnDrvExit();
} 