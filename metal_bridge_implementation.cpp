//
// metal_bridge_implementation.cpp
// 
// Implementation of the Metal bridge for FBNeo
// This connects the Metal renderer to the FBNeo core
//

#include "metal_bridge.h"
#include "metal_exports.h"
#include "metal_wrappers.h"
#include "burner_metal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h> // For tolower()
#include <sys/stat.h> // For stat()
#include <unistd.h> // For sleep()

// Global variables
bool bRunPause = false;
bool g_gameInitialized = false;
int g_frameWidth = 384;
int g_frameHeight = 224;
void* g_frameBuffer = NULL;

// Core driver variables
struct BurnDrvMeta BurnDrvInfo;
UINT32 nBurnDrvCount = 0;

// Application path
char g_szAppPath[MAX_PATH] = "/Users/plasx/Documents/FBNeo";

// Frame buffer variables
UINT8* pBurnDraw_Metal = NULL;
INT32 nBurnPitch_Metal = 0;
INT32 nBurnBpp_Metal = 0;

// BGRA conversion buffer
static UINT8* g_bgra_buffer = NULL;
static size_t g_bgra_buffer_size = 0;

// ROM paths
char szAppRomPaths[DIRS_MAX][MAX_PATH] = { { "/Users/plasx/ROMs/arcade" }, { "roms" } };
char szAppDirPath[MAX_PATH] = "/Users/plasx/Documents/FBNeo";
char g_szCurrentROMPath[MAX_PATH] = "";

// Audio state
static bool g_audioEnabled = true;
static int g_audioVolume = 100;
static int g_audioSampleRate = 44100;
static int g_audioBufferSize = 2048;
static short* g_audioBuffer = NULL;

// AI state
static int g_aiEnabled = 0;
static int g_aiDifficulty = 3;
static int g_aiPlayer = 2;
static char g_aiModelPath[MAX_PATH] = "";
static int g_aiTrainingMode = 0;
static int g_aiDebugOverlay = 0;

// External references to core FBNeo variables
extern UINT8* pBurnDraw;
extern INT32 nBurnPitch;
extern INT32 nBurnBpp;

//
// ROM Path Management
//

// Get current ROM path
int GetCurrentROMPath(char* szPath, size_t len) {
    if (!szPath || len == 0)
        return 0;
    
    if (g_szCurrentROMPath[0] == '\0') {
        // Default to the roms directory in the app path
        snprintf(szPath, len, "%s/roms", g_szAppPath);
    } else {
        snprintf(szPath, len, "%s", g_szCurrentROMPath);
    }
    
    return 1;
}

// Return current ROM path as a string 
const char* GetROMPathString() {
    static char buf[MAX_PATH];
    GetCurrentROMPath(buf, sizeof(buf));
    return buf;
}

// Set current ROM path
int SetCurrentROMPath(const char* szPath) {
    if (!szPath)
        return 0;
    
    strncpy(g_szCurrentROMPath, szPath, MAX_PATH-1);
    g_szCurrentROMPath[MAX_PATH-1] = '\0';
    
    printf("ROM path set to: %s\n", g_szCurrentROMPath);
    return 1;
}

// Validate a ROM path
int ValidateROMPath(const char* path) {
    printf("Validating ROM path: %s\n", path);
    
    // Check if the path exists
    if (!path || strlen(path) < 1) {
        printf("Invalid ROM path (null or empty)\n");
        return 0;
    }
    
    // Try to stat the path to see if it exists
    struct stat buffer;
    if (stat(path, &buffer) != 0) {
        printf("ROM path does not exist: %s\n", path);
        return 0;
    }
    
    printf("ROM path is valid: %s\n", path);
    return 1;
}

// Convert game name to driver index
INT32 BurnDrvGetIndexByName(const char* szName) {
    // Convert const char* to char* (needed for BurnDrvGetIndex)
    char nameBuffer[100];
    strncpy(nameBuffer, szName, sizeof(nameBuffer) - 1);
    nameBuffer[sizeof(nameBuffer) - 1] = '\0';
    
    // Call the actual core function
    extern INT32 BurnDrvGetIndex(char* szName);
    return BurnDrvGetIndex(nameBuffer);
}

//
// BGRA Buffer Management
//

// Ensure the BGRA buffer is large enough
static int EnsureBGRABuffer(int width, int height) {
    size_t needed_size = width * height * 4;
    if (g_bgra_buffer_size < needed_size) {
        if (g_bgra_buffer) {
            free(g_bgra_buffer);
        }
        g_bgra_buffer = (UINT8*)malloc(needed_size);
        if (!g_bgra_buffer) {
            printf("Failed to allocate BGRA buffer of size %zu\n", needed_size);
            g_bgra_buffer_size = 0;
            return 0;
        }
        g_bgra_buffer_size = needed_size;
    }
    return 1;
}

//
// Metal Core Integration
//

// Initialize Metal interface
int Metal_Init() {
    printf("Metal_Init() called\n");
    
    // Set default frame buffer dimensions
    g_frameWidth = 384;
    g_frameHeight = 224;
    
    // Initialize FBNeo core
    extern INT32 BurnLibInit_Metal();
    INT32 result = BurnLibInit_Metal();
    
    if (result != 0) {
        printf("Failed to initialize FBNeo core: %d\n", result);
        return result;
    }
    
    // Allocate frame buffer with proper alignment
    size_t bufferSize = 800 * 600 * 4; // Large enough for most games
    pBurnDraw_Metal = (UINT8*)malloc(bufferSize);
    if (!pBurnDraw_Metal) {
        printf("Failed to allocate frame buffer\n");
        return 1;
    }
    
    // Clear the buffer
    memset(pBurnDraw_Metal, 0, bufferSize);
    
    nBurnPitch_Metal = 800 * 4; // Pitch in bytes
    nBurnBpp_Metal = 32; // 32-bit color (BGRA)
    
    // Set global pointers for FBNeo core
    pBurnDraw = pBurnDraw_Metal;
    nBurnPitch = nBurnPitch_Metal;
    nBurnBpp = nBurnBpp_Metal;
    
    // Initialize frame buffer conversion
    extern INT32 SetBurnHighCol(INT32 nDepth);
    SetBurnHighCol(32);
    
    // Initialize audio system
    Metal_InitAudio(44100);
    
    // Initialize CoreAudio system
    extern int Metal_InitAudioSystem(int sampleRate);
    Metal_InitAudioSystem(44100);
    
    // Initialize ROM paths
    extern void FixRomPaths();
    FixRomPaths();
    
    // Connect to CPS2 linkage
    extern void Cps2_SetupMetalLinkage();
    Cps2_SetupMetalLinkage();
    
    printf("Metal initialization complete\n");
    return 0;
}

// Clean up Metal interface
int Metal_Exit() {
    printf("Metal_Exit() called\n");
    
    // Shut down any active game
    if (g_gameInitialized) {
        printf("Shutting down active game\n");
        extern INT32 BurnDrvExit_Metal();
        BurnDrvExit_Metal();
        g_gameInitialized = false;
    }
    
    // Free frame buffer
    if (pBurnDraw_Metal) {
        free(pBurnDraw_Metal);
        pBurnDraw_Metal = NULL;
    }
    
    // Free BGRA buffer
    if (g_bgra_buffer) {
        free(g_bgra_buffer);
        g_bgra_buffer = NULL;
        g_bgra_buffer_size = 0;
    }
    
    // Free audio buffer
    if (g_audioBuffer) {
        free(g_audioBuffer);
        g_audioBuffer = NULL;
    }
    
    // Exit FBNeo core
    extern INT32 BurnLibExit_Metal();
    BurnLibExit_Metal();
    
    printf("Metal exit complete\n");
    return 0;
}

// Load a ROM for emulation
int Metal_LoadROM(const char* romPath) {
    printf("Metal_LoadROM(%s) called\n", romPath);
    
    // Exit previous game if one was running
    if (g_gameInitialized) {
        printf("Exiting previous game...\n");
        extern INT32 BurnDrvExit_Metal();
        BurnDrvExit_Metal();
        g_gameInitialized = false;
    }
    
    // Validate ROM path
    if (!ValidateROMPath(romPath)) {
        printf("Invalid ROM path: %s\n", romPath);
        return 1;
    }
    
    // Extract game short name from path
    char szShortName[32];
    const char* pszBasename = strrchr(romPath, '/');
    if (pszBasename) {
        pszBasename++; // Skip the '/'
    } else {
        pszBasename = romPath;
    }
    
    // Extract the game short name (without extension)
    strncpy(szShortName, pszBasename, sizeof(szShortName) - 1);
    szShortName[sizeof(szShortName) - 1] = '\0';
    
    // Remove extension if present
    char* pszDot = strrchr(szShortName, '.');
    if (pszDot) {
        *pszDot = '\0';
    }
    
    // Find game index by name
    INT32 nDrvNum = BurnDrvGetIndexByName(szShortName);
    
    // Default to Marvel vs Capcom if specified game not found
    if (nDrvNum < 0) {
        printf("ROM not found, trying 'mvsc' as default\n");
        nDrvNum = BurnDrvGetIndexByName("mvsc");
        
        if (nDrvNum < 0) {
            printf("Default ROM not found either\n");
            return 1;
        }
    }
    
    // Update the ROM path
    SetCurrentROMPath(romPath);
    
    // Initialize the driver
    extern INT32 BurnDrvInit_Metal(INT32 nDrvNum);
    INT32 nRet = BurnDrvInit_Metal(nDrvNum);
    
    if (nRet == 0) {
        // Successfully initialized
        g_gameInitialized = true;
        
        // Get game dimensions
        g_frameWidth = BurnDrvInfo.nWidth;
        g_frameHeight = BurnDrvInfo.nHeight;
        
        // Allocate BGRA buffer for frame conversion
        if (!EnsureBGRABuffer(g_frameWidth, g_frameHeight)) {
            printf("Failed to allocate BGRA conversion buffer\n");
            BurnDrvExit_Metal();
            g_gameInitialized = false;
            return 1;
        }
        
        printf("Game initialized: %s (%dx%d)\n", 
               BurnDrvInfo.szFullNameA, g_frameWidth, g_frameHeight);
        
        // Run a frame to start things up
        Metal_RunFrame(true);
        
        return 0;
    } else {
        printf("Failed to initialize driver: %d\n", nRet);
        return nRet;
    }
}

// Run a frame of the emulation
int Metal_RunFrame(bool bDraw) {
    // Don't process if paused
    if (bRunPause) {
        return 0;
    }
    
    // Process AI if enabled
    if (g_aiEnabled && g_gameInitialized) {
        extern int AI_ProcessFrame(const void* frameBuffer, int width, int height);
        AI_ProcessFrame(g_frameBuffer, g_frameWidth, g_frameHeight);
    }
    
    // Connect frame buffer to core
    pBurnDraw = pBurnDraw_Metal;
    nBurnPitch = nBurnPitch_Metal;
    nBurnBpp = nBurnBpp_Metal;
    
    // Run one frame of emulation
    extern INT32 BurnDrvFrame();
    INT32 nRet = BurnDrvFrame();
    if (nRet != 0) {
        printf("Error in BurnDrvFrame(): %d\n", nRet);
        return nRet;
    }
    
    // Render the frame if requested
    if (bDraw) {
        // Store frame buffer for AI access
        g_frameBuffer = pBurnDraw;
        
        // Check if we have valid dimensions
        if (pBurnDraw && BurnDrvInfo.nWidth > 0 && BurnDrvInfo.nHeight > 0) {
            g_frameWidth = BurnDrvInfo.nWidth;
            g_frameHeight = BurnDrvInfo.nHeight;
            
            // Render to Metal
            int renderResult = Metal_RenderFrame(pBurnDraw, g_frameWidth, g_frameHeight);
            if (renderResult != 0) {
                printf("Error in Metal_RenderFrame(): %d\n", renderResult);
                return renderResult;
            }
        } else {
            printf("Invalid dimensions or null frame buffer\n");
            return 1;
        }
    }
    
    return 0;
}

// Frame rendering
int Metal_RenderFrame(void* frameData, int width, int height) {
    if (!g_gameInitialized || !frameData) {
        printf("Error: Cannot render frame - game not initialized or no frame data\n");
        return 1;
    }
    
    if (width <= 0 || height <= 0) {
        printf("Error: Invalid dimensions: %dx%d\n", width, height);
        return 1;
    }
    
    // Ensure BGRA buffer is large enough
    if (!EnsureBGRABuffer(width, height)) {
        printf("Error: Failed to allocate BGRA buffer\n");
        return 1;
    }
    
    // Convert frame buffer based on bit depth
    if (nBurnBpp == 16) {
        // Convert RGB565 to BGRA
        UINT16* pSrc = (UINT16*)frameData;
        UINT32* pDst = (UINT32*)g_bgra_buffer;
        
        // Calculate pitch in pixels
        int srcPitchInPixels = nBurnPitch / 2;
        if (srcPitchInPixels <= 0) srcPitchInPixels = width;
        
        // Convert each pixel
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                UINT16 pixel = pSrc[x];
                
                // Extract RGB565 components
                UINT8 r = (pixel >> 11) & 0x1F;
                UINT8 g = (pixel >> 5) & 0x3F;
                UINT8 b = pixel & 0x1F;
                
                // Convert to RGB888 (expand bits)
                r = (r << 3) | (r >> 2);
                g = (g << 2) | (g >> 4);
                b = (b << 3) | (b >> 2);
                
                // Store as BGRA8888 (Metal format)
                pDst[y * width + x] = (b) | (g << 8) | (r << 16) | (0xFF << 24);
            }
            
            // Move to next row
            pSrc += srcPitchInPixels;
        }
        
        // Update Metal texture
        extern void UpdateMetalFrameTexture(const void *frameData, unsigned int width, unsigned int height);
        UpdateMetalFrameTexture(g_bgra_buffer, width, height);
        
    } else if (nBurnBpp == 24) {
        // Convert RGB888 to BGRA8888
        UINT8* pSrc = (UINT8*)frameData;
        UINT32* pDst = (UINT32*)g_bgra_buffer;
        
        // Calculate pitch in bytes
        int srcPitch = nBurnPitch;
        if (srcPitch <= 0) srcPitch = width * 3;
        
        // Convert each pixel
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                UINT8 r = pSrc[x*3 + 0];
                UINT8 g = pSrc[x*3 + 1];
                UINT8 b = pSrc[x*3 + 2];
                
                // Store as BGRA8888 (Metal format)
                pDst[y * width + x] = (b) | (g << 8) | (r << 16) | (0xFF << 24);
            }
            
            // Move to next row
            pSrc += srcPitch;
        }
        
        // Update Metal texture
        extern void UpdateMetalFrameTexture(const void *frameData, unsigned int width, unsigned int height);
        UpdateMetalFrameTexture(g_bgra_buffer, width, height);
        
    } else if (nBurnBpp == 32) {
        // Convert RGBA8888 to BGRA8888 if needed
        UINT32* pSrc = (UINT32*)frameData;
        UINT32* pDst = (UINT32*)g_bgra_buffer;
        
        // Calculate pitch in 32-bit pixels
        int srcPitchInPixels = nBurnPitch / 4;
        if (srcPitchInPixels <= 0) srcPitchInPixels = width;
        
        // Check if conversion is needed or can use direct data
        bool needsConversion = false;
        
        // Sample a few pixels to see if conversion is needed (look for red/blue swap)
        for (int i = 0; i < 10 && i < height * srcPitchInPixels; i += srcPitchInPixels) {
            UINT32 pixel = pSrc[i];
            UINT8 r = (pixel >> 16) & 0xFF;
            UINT8 b = pixel & 0xFF;
            
            // If red component is significantly different from blue, assume we need conversion
            if (abs(r - b) > 32) {
                needsConversion = true;
                break;
            }
        }
        
        if (needsConversion) {
            // Convert each pixel (swap R and B channels)
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    UINT32 pixel = pSrc[x];
                    
                    UINT8 a = (pixel >> 24) & 0xFF;
                    UINT8 r = (pixel >> 16) & 0xFF;
                    UINT8 g = (pixel >> 8) & 0xFF;
                    UINT8 b = pixel & 0xFF;
                    
                    // Store as BGRA8888 (Metal format)
                    pDst[y * width + x] = (b) | (g << 8) | (r << 16) | (a << 24);
                }
                
                // Move to next row
                pSrc += srcPitchInPixels;
            }
            
            // Update Metal texture with converted buffer
            extern void UpdateMetalFrameTexture(const void *frameData, unsigned int width, unsigned int height);
            UpdateMetalFrameTexture(g_bgra_buffer, width, height);
        } else {
            // Direct update - data is already in correct format
            extern void UpdateMetalFrameTexture(const void *frameData, unsigned int width, unsigned int height);
            UpdateMetalFrameTexture(frameData, width, height);
        }
    } else {
        printf("Unsupported bit depth: %d\n", nBurnBpp);
        return 1;
    }
    
    return 0;
}

//
// Audio Management
//

// Audio initialization
int Metal_InitAudio(int sampleRate) {
    printf("Metal_InitAudio(%d) called\n", sampleRate);
    
    // Set audio sample rate
    g_audioSampleRate = sampleRate > 0 ? sampleRate : 44100;
    
    // Calculate buffer size (1/60th second worth of samples)
    g_audioBufferSize = g_audioSampleRate / 60;
    
    // Allocate audio buffer
    if (g_audioBuffer) {
        free(g_audioBuffer);
    }
    g_audioBuffer = (short*)malloc(g_audioBufferSize * 2 * sizeof(short)); // Stereo
    
    if (!g_audioBuffer) {
        printf("Error: Failed to allocate audio buffer\n");
        return 1;
    }
    
    // Clear buffer
    memset(g_audioBuffer, 0, g_audioBufferSize * 2 * sizeof(short));
    
    printf("Audio initialized: %dHz, buffer size: %d samples\n", 
           g_audioSampleRate, g_audioBufferSize);
    
    return 0;
}

// Enable/disable audio
int Metal_SetAudioEnabled(int enabled) {
    printf("Metal_SetAudioEnabled(%d) called\n", enabled);
    g_audioEnabled = enabled ? true : false;
    return 0;
}

// Set audio volume
int Metal_SetVolume(int volume) {
    printf("Metal_SetVolume(%d) called\n", volume);
    
    // Clamp volume to 0-100
    g_audioVolume = volume;
    if (g_audioVolume < 0) g_audioVolume = 0;
    if (g_audioVolume > 100) g_audioVolume = 100;
    
    return 0;
}

// Get audio buffer
short* Metal_GetAudioBuffer() {
    return g_audioBuffer;
}

// Get audio buffer size
int Metal_GetAudioBufferSize() {
    return g_audioBufferSize;
}

// Is audio enabled?
bool Metal_IsAudioEnabled() {
    return g_audioEnabled;
}

// Get volume
int Metal_GetVolume() {
    return g_audioVolume;
}

//
// AI Integration
//

// AI Enable/disable
int Metal_SetAIEnabled(int enabled) {
    g_aiEnabled = enabled;
    return 0;
}

// Get AI enabled state
int Metal_IsAIEnabled() {
    return g_aiEnabled;
}

// Set AI difficulty
int Metal_SetAIDifficulty(int level) {
    g_aiDifficulty = level;
    return 0;
}

// Get AI difficulty
int Metal_GetAIDifficulty() {
    return g_aiDifficulty;
}

// Set AI controlled player
int Metal_SetAIControlledPlayer(int playerIndex) {
    g_aiPlayer = playerIndex;
    return 0;
}

// Get AI controlled player
int Metal_GetAIControlledPlayer() {
    return g_aiPlayer;
}

// BurnDrvReset_Metal implementation
extern "C" INT32 BurnDrvReset_Metal() {
    // Call the actual BurnDrvReset function
    extern INT32 BurnDrvReset();
    return BurnDrvReset();
} 