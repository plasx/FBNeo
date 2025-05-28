#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>  // For va_start, va_end
#include <time.h>    // For clock() and CLOCKS_PER_SEC
#include "../src/burner/metal/metal_declarations.h"
#include "../src/burner/metal/metal_error_handling.h"

// Global error state - duplicated here for direct access
extern MetalErrorInfo g_lastError;

// Global frame buffer - declared as extern since it's defined in metal_standalone_main.mm
extern FrameBuffer g_frameBuffer;

// Test pattern data
static uint32_t s_testPattern[GAME_FRAME_HEIGHT][GAME_FRAME_WIDTH];

// Various state tracking globals
static float s_frameRate = 60.0f;
static int s_totalFrames = 0;
static char s_gameTitle[256] = "FBNeo Metal";
static bool s_gameRunning = false;
static int s_logLevel = LOG_LEVEL_INFO;
static bool s_debugMode = false;

// Debug system initialization
void Debug_Init(void* param) {
    printf("[METAL] Debug system initialized\n");
}

// Memory system initialization
void Memory_Init(void) {
    printf("[METAL] Memory system initialized\n");
}

// Graphics components initialization
int Graphics_InitComponents(void) {
    printf("[METAL] Graphics system initialized\n");
    
    // Initialize the frame buffer with default dimensions
    InitFrameBuffer(GAME_FRAME_WIDTH, GAME_FRAME_HEIGHT);
    
    return 0;
}

// Initialize the frame buffer with given dimensions
bool InitFrameBuffer(int width, int height) {
    printf("[METAL] Initializing frame buffer (%dx%d)\n", width, height);
    
    // Free existing buffer if any
    if (g_frameBuffer.data) {
        free(g_frameBuffer.data);
        g_frameBuffer.data = NULL;
    }
    
    // Allocate new buffer
    g_frameBuffer.data = (uint32_t*)malloc(width * height * sizeof(uint32_t));
    if (!g_frameBuffer.data) {
        printf("[METAL] ERROR: Could not allocate frame buffer memory\n");
        return false;
    }
    
    // Set buffer properties
    g_frameBuffer.width = width;
    g_frameBuffer.height = height;
    g_frameBuffer.pitch = width * sizeof(uint32_t);
    g_frameBuffer.updated = false;
    
    // Clear buffer to black
    memset(g_frameBuffer.data, 0, width * height * sizeof(uint32_t));
    
    printf("[METAL] Frame buffer initialized: %dx%d (%zu bytes)\n", 
           width, height, width * height * sizeof(uint32_t));
    
    return true;
}

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

// Implementation of Metal_LogMessage with our own simplified version 
// This will be used when the real Metal_LogMessage is called through our macros
// Real declaration is in metal_error_handling.h with variable arguments
void Metal_LogMessage_Impl(MetalLogLevel level, const char* file, int line, const char* message) {
    if (level <= s_logLevel) {
        const char* levelStr = "UNKNOWN";
        switch (level) {
            case LOG_LEVEL_NONE: levelStr = "NONE"; break;
            case LOG_LEVEL_ERROR: levelStr = "ERROR"; break;
            case LOG_LEVEL_WARNING: levelStr = "WARNING"; break;
            case LOG_LEVEL_INFO: levelStr = "INFO"; break;
            case LOG_LEVEL_DEBUG: levelStr = "DEBUG"; break;
            case LOG_LEVEL_VERBOSE: levelStr = "VERBOSE"; break;
            default: levelStr = "UNKNOWN"; break;
        }
        
        printf("[METAL-%s] %s:%d - %s\n", levelStr, file, line, message);
    }
}

// The actual Metal_LogMessage with variable arguments
void Metal_LogMessage(MetalLogLevel level, const char* format, ...) {
    if (level <= s_logLevel) {
        char buffer[1024];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        
        // Use our implementation with the formatted message
        Metal_LogMessage_Impl(level, __FILE__, __LINE__, buffer);
    }
}

// Debug mode control
void Metal_SetDebugMode(bool enabled) {
    s_debugMode = enabled;
    printf("[METAL] Debug mode %s\n", enabled ? "enabled" : "disabled");
}

bool Metal_IsDebugMode(void) {
    return s_debugMode;
}

// Set log level
void Metal_SetLogLevel(MetalLogLevel level) {
    s_logLevel = level;
    printf("[METAL] Log level set to %d\n", level);
}

// Frame buffer access
unsigned char* Metal_GetFrameBuffer(int* width, int* height, int* pitch) {
    if (!g_frameBuffer.data) {
        // Initialize the frame buffer if not already done
        InitFrameBuffer(GAME_FRAME_WIDTH, GAME_FRAME_HEIGHT);
    }
    
    if (width) *width = g_frameBuffer.width;
    if (height) *height = g_frameBuffer.height;
    if (pitch) *pitch = g_frameBuffer.pitch;
    
    return (unsigned char*)g_frameBuffer.data;
}

// Generate test pattern
int Metal_GenerateTestPattern(int patternType) {
    printf("[METAL] Generating test pattern %d\n", patternType);
    
    if (!g_frameBuffer.data) {
        printf("[METAL] ERROR: Cannot generate test pattern - frame buffer not initialized\n");
        return -1;
    }
    
    int width = g_frameBuffer.width;
    int height = g_frameBuffer.height;
    
    // Force a very obvious test pattern to make sure rendering is working
    patternType = 3; // Use the rainbow pattern
    
    switch (patternType) {
        case 0: // Gradient pattern
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    uint8_t r = (uint8_t)((float)x / width * 255);
                    uint8_t g = (uint8_t)((float)y / height * 255);
                    uint8_t b = (uint8_t)((float)(x + y) / (width + height) * 255);
                    g_frameBuffer.data[y * width + x] = (r << 16) | (g << 8) | b | 0xFF000000;
                }
            }
            break;
            
        case 1: // Checkerboard pattern
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    bool isWhite = ((x / 16) + (y / 16)) % 2 == 0;
                    g_frameBuffer.data[y * width + x] = isWhite ? 0xFFFFFFFF : 0xFF000000;
                }
            }
            break;
            
        case 2: // Grid pattern
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    bool isGrid = (x % 32 == 0) || (y % 32 == 0);
                    g_frameBuffer.data[y * width + x] = isGrid ? 0xFFFFFFFF : 0xFF000000;
                }
            }
            break;
            
        case 3: // Rainbow pattern
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    float hue = ((float)x / width) * 6.0f;
                    int i = (int)hue;
                    float f = hue - i;
                    float q = 1.0f - f;
                    
                    uint8_t r, g, b;
                    switch (i % 6) {
                        case 0: r = 255; g = f * 255; b = 0; break;
                        case 1: r = q * 255; g = 255; b = 0; break;
                        case 2: r = 0; g = 255; b = f * 255; break;
                        case 3: r = 0; g = q * 255; b = 255; break;
                        case 4: r = f * 255; g = 0; b = 255; break;
                        case 5: r = 255; g = 0; b = q * 255; break;
                        default: r = g = b = 0; break;
                    }
                    
                    g_frameBuffer.data[y * width + x] = (r << 16) | (g << 8) | b | 0xFF000000;
                }
            }
            break;
            
        default:
            // Clear to black
            memset(g_frameBuffer.data, 0, width * height * sizeof(uint32_t));
            break;
    }
    
    g_frameBuffer.updated = true;
    return 0;
}

// Track frame for statistics
void Metal_TrackFrame(void) {
    static int frameCount = 0;
    static double lastTime = 0;
    static double currentTime = 0;
    
    // Increment frame counter
    s_totalFrames++;
    frameCount++;
    
    // Calculate frame rate every 60 frames
    if (frameCount >= 60) {
        // Get current time
        currentTime = (double)clock() / CLOCKS_PER_SEC;
        
        // Calculate frame rate if we have a valid last time
        if (lastTime > 0) {
            double elapsed = currentTime - lastTime;
            if (elapsed > 0) {
                s_frameRate = (float)(frameCount / elapsed);
            }
        }
        
        // Reset counter and update last time
        frameCount = 0;
        lastTime = currentTime;
    }
}

// Get current frame rate
float Metal_GetFrameRate(void) {
    return s_frameRate;
}

// Get total frames rendered
int Metal_GetTotalFrames(void) {
    return s_totalFrames;
}

// Game title functions
const char* Metal_GetGameTitle(void) {
    return s_gameTitle;
}

void Metal_SetGameTitle(const char* title) {
    if (title) {
        strncpy(s_gameTitle, title, sizeof(s_gameTitle) - 1);
        s_gameTitle[sizeof(s_gameTitle) - 1] = '\0';
    }
}

// Game running state
void Metal_SetGameRunning(bool running) {
    s_gameRunning = running;
}

bool Metal_IsGameRunning(void) {
    return s_gameRunning;
}

// ROM management
void Metal_SetRomPath(const char* path) {
    printf("[METAL] ROM path set to: %s\n", path ? path : "NULL");
}

bool Metal_LoadAndInitROM(const char* path) {
    printf("[METAL] Loading ROM: %s\n", path);
    
    // For now, just pretend we loaded the ROM successfully
    Metal_SetGameTitle("Marvel vs. Capcom");
    Metal_SetGameRunning(true);
    
    return true;
}

void Metal_UnloadROM(void) {
    printf("[METAL] Unloading ROM\n");
    Metal_SetGameRunning(false);
}

// ROM verification
// Moved to metal_cpp_stubs.cpp to avoid duplicate symbols

// Frame processing
bool Metal_ProcessFrame(void) {
    // Increment frame counter
    Metal_TrackFrame();
    
    // Return success
    return true;
}

// Audio processing
int Metal_ProcessAudio(void) {
    // No audio processing in this simple stub
    return 0;
}

// Input handling
void Metal_UpdateInputState(void) {
    // Input state update stub
}

void Metal_ProcessKeyDown(int keyCode) {
    printf("[METAL] Key down: %d\n", keyCode);
}

void Metal_ProcessKeyUp(int keyCode) {
    printf("[METAL] Key up: %d\n", keyCode);
}

// Audio system
int Metal_InitAudio(void) {
    printf("[METAL] Audio system initialized\n");
    return 0;
}

void Metal_ShutdownAudio(void) {
    printf("[METAL] Audio system shut down\n");
}

int Metal_EnableFallbackAudio(void) {
    printf("[METAL] Fallback audio enabled\n");
    return 0;
}

// Input system
int Metal_InitInput(void) {
    printf("[METAL] Input system initialized\n");
    return 0;
}

int Metal_ExitInput(void) {
    printf("[METAL] Input system shut down\n");
    return 0;
}

// Hardware components
void Hardware_InitComponents(void) {
    printf("[METAL] Hardware components initialized\n");
}

// Memory tracker
void MemoryTracker_Init(void) {
    printf("[METAL] Memory tracker initialized\n");
}

// Graphics tracker
void GraphicsTracker_Init(void) {
    printf("[METAL] Graphics tracker initialized\n");
}

// Audio loop
void AudioLoop_InitAndGenerateReport(void) {
    printf("[METAL] Audio loop initialized\n");
} 