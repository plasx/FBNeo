#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// Define basic types
typedef int32_t INT32;
typedef uint8_t UINT8;
typedef uint16_t UINT16;
typedef uint32_t UINT32;

// Global state
static bool g_bFBNeoInitialized = false;
static UINT8* g_pFrameBuffer = NULL;
static INT32 g_nFrameWidth = 384;
static INT32 g_nFrameHeight = 224;
static size_t g_nFrameSize = 384 * 224 * 4; // RGBA

// External variables
extern UINT32 nBurnDrvCount;
extern int nBurnDrvActive;

extern "C" {
    // Bridge function for BurnLibInit
    INT32 BurnLibInit_Metal() {
        printf("[BurnLibInit_Metal] === STARTING FBNEO LIBRARY INITIALIZATION ===\n");
        
        if (g_bFBNeoInitialized) {
            printf("[BurnLibInit_Metal] Already initialized, skipping\n");
            return 0;
        }
        
        printf("[BurnLibInit_Metal] Step 1: Allocating frame buffer (%dx%d)\n", g_nFrameWidth, g_nFrameHeight);
        g_pFrameBuffer = (UINT8*)malloc(g_nFrameSize);
        if (!g_pFrameBuffer) {
            printf("[BurnLibInit_Metal] ERROR: Failed to allocate frame buffer\n");
            return 1;
        }
        memset(g_pFrameBuffer, 0, g_nFrameSize);
        
        printf("[BurnLibInit_Metal] Step 2: Setting initialization flag\n");
        g_bFBNeoInitialized = true;
        
        printf("[BurnLibInit_Metal] === FBNEO LIBRARY INITIALIZATION COMPLETE ===\n");
        return 0;
    }
    
    // Bridge function for BurnDrvInit
    INT32 BurnDrvInit_Metal(INT32 nDrvNum) {
        printf("[BurnDrvInit_Metal] === STARTING DRIVER INITIALIZATION ===\n");
        printf("[BurnDrvInit_Metal] Called for driver #%d\n", nDrvNum);
        
        // Validate driver number
        if (nDrvNum < 0 || nDrvNum >= (INT32)nBurnDrvCount) {
            printf("[BurnDrvInit_Metal] ERROR: Invalid driver number %d (max: %d)\n", nDrvNum, nBurnDrvCount - 1);
            return 1;
        }
        
        printf("[BurnDrvInit_Metal] Step 1: Driver number validation passed\n");
        
        // Ensure FBNeo library is initialized
        if (!g_bFBNeoInitialized) {
            printf("[BurnDrvInit_Metal] ERROR: FBNeo library not initialized!\n");
            return 1;
        }
        
        printf("[BurnDrvInit_Metal] Step 2: Setting active driver\n");
        nBurnDrvActive = nDrvNum;
        
        printf("[BurnDrvInit_Metal] === DRIVER INITIALIZATION COMPLETE ===\n");
        return 0;
    }
    
    // Find a driver by short name
    INT32 BurnDrvFind(const char* szName) {
        printf("[BurnDrvFind] Searching for driver: '%s'\n", szName ? szName : "NULL");
        
        if (!szName) {
            printf("[BurnDrvFind] ERROR: NULL driver name\n");
            return -1;
        }
        
        // For our test, we only have one driver "mvsc"
        if (strcmp(szName, "mvsc") == 0) {
            printf("[BurnDrvFind] Found driver 'mvsc' at index 0\n");
            return 0;
        }
        
        printf("[BurnDrvFind] Driver '%s' not found\n", szName);
        return -1;
    }
    
    // Run a single frame of emulation
    void Metal_RunFrame(int bDraw) {
        static int frameCount = 0;
        frameCount++;
        
        printf("[Metal_RunFrame] === FRAME %d START (bDraw=%d) ===\n", frameCount, bDraw);
        
        if (!g_bFBNeoInitialized) {
            printf("[Metal_RunFrame] ERROR: FBNeo is not initialized\n");
            return;
        }
        
        printf("[Metal_RunFrame] Step 1: FBNeo initialization check passed\n");
        
        // Check if frame buffer is initialized
        if (!g_pFrameBuffer) {
            printf("[Metal_RunFrame] WARNING: Frame buffer is NULL\n");
            return;
        }
        
        printf("[Metal_RunFrame] Step 2: Frame buffer check passed\n");
        
        // Simulate some emulation work
        if (bDraw) {
            printf("[Metal_RunFrame] Step 3: Rendering frame to buffer\n");
            // Fill buffer with a test pattern (alternating colors)
            for (int y = 0; y < g_nFrameHeight; y++) {
                for (int x = 0; x < g_nFrameWidth; x++) {
                    UINT32* pixel = (UINT32*)(g_pFrameBuffer + (y * g_nFrameWidth + x) * 4);
                    *pixel = ((x + y + frameCount) % 2) ? 0xFF0000FF : 0xFF00FF00; // Red or Green
                }
            }
        }
        
        printf("[Metal_RunFrame] === FRAME %d COMPLETE ===\n", frameCount);
    }
} 