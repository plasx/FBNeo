//
// metal_integration.cpp
//
// Implementation of the Metal integration with FBNeo
// This connects the Metal renderer to the FBNeo core, replacing test patterns
// with actual game rendering
//

// First include metal_declarations.h so we define our types first
#include "metal_declarations.h"

// Include other headers
#include "metal_bridge.h"
#include "metal_exports.h"

// System includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// External declarations from burn.h
#define DRV_NAME 0
#define DRV_FULLNAME 4

// External declarations
extern UINT8* pBurnDraw;
extern INT32 nBurnPitch;
extern INT32 nBurnBpp;
extern struct BurnDrvMeta BurnDrvInfo;

// Global variables for Metal integration
static UINT8* g_frameBuffer = NULL;      // Frame buffer for rendering
static size_t g_frameBufferSize = 0;     // Size of the frame buffer
static UINT8* g_bgraBuffer = NULL;       // Buffer for BGRA conversion
static size_t g_bgraBufferSize = 0;      // Size of the BGRA buffer
static bool g_initialized = false;       // Whether the Metal integration is initialized

// Game dimensions
static int g_gameWidth = 320;
static int g_gameHeight = 240;

// Forward declarations
int BurnLibInit_Metal();
int BurnLibExit_Metal();
int BurnDrvInit_Metal(INT32 nDrvNum);
int BurnDrvExit_Metal();

//
// Buffer Management
//

// Ensure the frame buffer is large enough
static int EnsureFrameBuffer(int width, int height, int bpp) {
    size_t needed_size = width * height * (bpp / 8);
    if (g_frameBufferSize < needed_size) {
        if (g_frameBuffer) {
            free(g_frameBuffer);
        }
        g_frameBuffer = (UINT8*)malloc(needed_size);
        if (!g_frameBuffer) {
            printf("Failed to allocate frame buffer of size %zu\n", needed_size);
            g_frameBufferSize = 0;
            return 0;
        }
        g_frameBufferSize = needed_size;
        
        // Clear the buffer
        memset(g_frameBuffer, 0, needed_size);
        
        printf("Allocated frame buffer: %dx%d, %d BPP (%zu bytes)\n", 
               width, height, bpp, needed_size);
    }
    return 1;
}

// Ensure the BGRA buffer is large enough
static int EnsureBGRABuffer(int width, int height) {
    size_t needed_size = width * height * 4; // 4 bytes per pixel (BGRA)
    if (g_bgraBufferSize < needed_size) {
        if (g_bgraBuffer) {
            free(g_bgraBuffer);
        }
        g_bgraBuffer = (UINT8*)malloc(needed_size);
        if (!g_bgraBuffer) {
            printf("Failed to allocate BGRA buffer of size %zu\n", needed_size);
            g_bgraBufferSize = 0;
            return 0;
        }
        g_bgraBufferSize = needed_size;
        
        // Clear the buffer
        memset(g_bgraBuffer, 0, needed_size);
        
        printf("Allocated BGRA buffer: %dx%d (%zu bytes)\n", 
               width, height, needed_size);
    }
    return 1;
}

//
// FBNeo Core Integration Functions
//

// Initialize the FBNeo library for Metal
int BurnLibInit_Metal() {
    printf("BurnLibInit_Metal() called\n");
    
    // Initialize the FBNeo library
    int result = BurnLibInit();
    if (result != 0) {
        printf("BurnLibInit() failed: %d\n", result);
        return result;
    }
    
    // Initialize color conversion using our Metal versions that match the signature
    BurnHighCol = BurnHighCol32;
    BurnHighColReduce = BurnHighCol32;
    BurnHighColReduceNew = BurnHighCol32;
    BurnRecalcPal();
    
    // Set up global pointers
    pBurnDraw = g_frameBuffer;
    nBurnPitch = g_gameWidth * (nBurnBpp / 8);
    
    printf("BurnLibInit_Metal() succeeded\n");
    return 0;
}

// Exit the FBNeo library for Metal
int BurnLibExit_Metal() {
    printf("BurnLibExit_Metal() called\n");
    
    // Exit the FBNeo library
    int result = BurnLibExit();
    
    // Free the frame buffer
    if (g_frameBuffer) {
        free(g_frameBuffer);
        g_frameBuffer = NULL;
        g_frameBufferSize = 0;
    }
    
    // Free the BGRA buffer
    if (g_bgraBuffer) {
        free(g_bgraBuffer);
        g_bgraBuffer = NULL;
        g_bgraBufferSize = 0;
    }
    
    // Reset global pointers
    pBurnDraw = NULL;
    nBurnPitch = 0;
    
    g_initialized = false;
    
    printf("BurnLibExit_Metal() returned: %d\n", result);
    return result;
}

// Initialize a driver for Metal
int BurnDrvInit_Metal(INT32 nDrvNum) {
    printf("BurnDrvInit_Metal(%d) called\n", nDrvNum);
    
    // Select the driver
    int result = BurnDrvSelect(nDrvNum);
    if (result != 0) {
        printf("BurnDrvSelect(%d) failed: %d\n", nDrvNum, result);
        return result;
    }
    
    // Copy driver metadata
    BurnDrvInfo.szShortName = BurnDrvGetTextA(DRV_NAME);
    BurnDrvInfo.szFullNameA = BurnDrvGetTextA(DRV_FULLNAME);
    
    // Get the visible size
    INT32 width, height;
    BurnDrvGetVisibleSize(&width, &height);
    BurnDrvInfo.nWidth = width;
    BurnDrvInfo.nHeight = height;
    
    // Get the aspect ratio
    INT32 xAspect, yAspect;
    BurnDrvGetAspect(&xAspect, &yAspect);
    BurnDrvInfo.nAspectX = xAspect;
    BurnDrvInfo.nAspectY = yAspect;
    
    // Get the width and height
    g_gameWidth = BurnDrvInfo.nWidth;
    g_gameHeight = BurnDrvInfo.nHeight;
    
    // Allocate frame buffer
    if (!EnsureFrameBuffer(g_gameWidth, g_gameHeight, 32)) {
        printf("Failed to allocate frame buffer\n");
        return 1;
    }
    
    // Allocate BGRA buffer for conversion
    if (!EnsureBGRABuffer(g_gameWidth, g_gameHeight)) {
        printf("Failed to allocate BGRA buffer\n");
        return 1;
    }
    
    // Set up global pointers
    pBurnDraw = g_frameBuffer;
    nBurnPitch = g_gameWidth * 4; // 32-bit color
    nBurnBpp = 32;
    
    // Initialize the driver
    result = BurnDrvInit();
    if (result != 0) {
        printf("BurnDrvInit() failed: %d\n", result);
        return result;
    }
    
    g_initialized = true;
    
    printf("BurnDrvInit_Metal() succeeded: %s (%dx%d)\n", 
           BurnDrvInfo.szFullNameA, g_gameWidth, g_gameHeight);
    return 0;
}

// Exit the current driver for Metal
int BurnDrvExit_Metal() {
    printf("BurnDrvExit_Metal() called\n");
    
    // Exit the driver
    int result = BurnDrvExit();
    
    g_initialized = false;
    
    printf("BurnDrvExit_Metal() returned: %d\n", result);
    return result;
}

// Reset the current driver for Metal
int BurnDrvReset_Metal() {
    printf("BurnDrvReset_Metal() called\n");
    
    // Reset the driver
    int result = BurnDrvReset();
    
    printf("BurnDrvReset_Metal() returned: %d\n", result);
    return result;
}

//
// Metal Bridge Functions
//

// Frame rendering function
// This converts the FBNeo frame buffer to the format expected by Metal
int Metal_RenderFrame(void* frameData, int width, int height) {
    if (!g_initialized) {
        printf("Error: Cannot render frame - not initialized\n");
        return 1;
    }
    
    if (!frameData) {
        printf("Error: Null frame data provided to Metal_RenderFrame\n");
        return 1;
    }
    
    if (width <= 0 || height <= 0) {
        printf("Error: Invalid dimensions: %dx%d\n", width, height);
        return 1;
    }
    
    // Debug: Print details periodically
    static int frameCount = 0;
    bool debugFrame = (++frameCount % 60) == 0;
    
    if (debugFrame) {
        printf("RenderFrame: %dx%d, bpp=%d, pitch=%d, buffer=%p\n", 
               width, height, nBurnBpp, nBurnPitch, frameData);
    }
    
    // Ensure BGRA buffer is large enough
    if (!EnsureBGRABuffer(width, height)) {
        printf("Error: Failed to allocate BGRA buffer\n");
        return 1;
    }
    
    // Check buffer size requirements
    size_t srcBufferSize = height * nBurnPitch;
    size_t dstBufferSize = height * width * 4; // BGRA (4 bytes per pixel)
    
    if (debugFrame) {
        printf("Buffer sizes: src=%zu bytes, dst=%zu bytes\n", srcBufferSize, dstBufferSize);
    }
    
    // Make sure nBurnPitch is valid
    if (nBurnPitch <= 0) {
        nBurnPitch = width * (nBurnBpp / 8);
        printf("Warning: Invalid nBurnPitch, setting to width * (bpp/8) = %d\n", nBurnPitch);
    }
    
    // Convert frame buffer based on bit depth
    if (nBurnBpp == 16) {
        // Convert RGB565 to BGRA
        UINT16* pSrc = (UINT16*)frameData;
        UINT32* pDst = (UINT32*)g_bgraBuffer;
        
        // Calculate pitch in pixels
        int srcPitchInPixels = nBurnPitch / 2;
        if (srcPitchInPixels <= 0) srcPitchInPixels = width;
        
        // Log the first few pixels for debugging
        if (debugFrame) {
            printf("First 4 pixels (RGB565): ");
            for (int i = 0; i < 4 && i < width; i++) {
                UINT16 pixel = pSrc[i];
                printf("0x%04X ", pixel);
            }
            printf("\n");
        }
        
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
        UpdateMetalFrameTexture(g_bgraBuffer, width, height);
        
    } else if (nBurnBpp == 24) {
        // Convert RGB888 to BGRA8888
        UINT8* pSrc = (UINT8*)frameData;
        UINT32* pDst = (UINT32*)g_bgraBuffer;
        
        // Calculate pitch in bytes
        int srcPitch = nBurnPitch;
        if (srcPitch <= 0) srcPitch = width * 3;
        
        // Log the first few pixels for debugging
        if (debugFrame) {
            printf("First 4 pixels (RGB888): ");
            for (int i = 0; i < 4 && i < width; i++) {
                UINT8 r = pSrc[i*3 + 0];
                UINT8 g = pSrc[i*3 + 1];
                UINT8 b = pSrc[i*3 + 2];
                printf("(%d,%d,%d) ", r, g, b);
            }
            printf("\n");
        }
        
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
        UpdateMetalFrameTexture(g_bgraBuffer, width, height);
        
    } else if (nBurnBpp == 32) {
        // Convert RGBA8888 to BGRA8888 if needed
        UINT32* pSrc = (UINT32*)frameData;
        UINT32* pDst = (UINT32*)g_bgraBuffer;
        
        // Calculate pitch in 32-bit pixels
        int srcPitchInPixels = nBurnPitch / 4;
        if (srcPitchInPixels <= 0) srcPitchInPixels = width;
        
        // Log the first few pixels for debugging
        if (debugFrame) {
            printf("First 4 pixels (RGBA8888): ");
            for (int i = 0; i < 4 && i < width; i++) {
                UINT32 pixel = pSrc[i];
                UINT8 a = (pixel >> 24) & 0xFF;
                UINT8 r = (pixel >> 16) & 0xFF;
                UINT8 g = (pixel >> 8) & 0xFF;
                UINT8 b = pixel & 0xFF;
                printf("(%d,%d,%d,%d) ", r, g, b, a);
            }
            printf("\n");
        }
        
        // For now, always convert - this makes it more reliable
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
        UpdateMetalFrameTexture(g_bgraBuffer, width, height);
    } else {
        // Fallback for unsupported bit depths
        printf("Unsupported bit depth: %d - Using simple coloring\n", nBurnBpp);
        
        // Create a simple colored pattern as fallback
        UINT32* pDst = (UINT32*)g_bgraBuffer;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                // Create a visual pattern to show something is happening
                UINT8 r = (x * 255) / width;
                UINT8 g = (y * 255) / height;
                UINT8 b = ((x ^ y) * 255) / (width ^ height);
                
                // Store as BGRA8888 (Metal format)
                pDst[y * width + x] = (b) | (g << 8) | (r << 16) | (0xFF << 24);
            }
        }
        
        // Update Metal texture with fallback pattern
        UpdateMetalFrameTexture(g_bgraBuffer, width, height);
        return 0;
    }
    
    // Debug: Log BGRA buffer checksum periodically
    if (debugFrame) {
        UINT32 checksum = 0;
        for (int i = 0; i < 16 && i < width * height * 4; i++) {
            checksum += ((UINT8*)g_bgraBuffer)[i];
        }
        printf("BGRA buffer checksum (first 16 bytes): 0x%08X\n", checksum);
    }
    
    return 0;
}

// Run a single frame of emulation
int Metal_RunFrame(int bDraw) {
    if (!g_initialized) {
        printf("Metal_RunFrame: Not initialized\n");
        return 1;
    }
    
    // Set up global pointers
    pBurnDraw = bDraw ? g_frameBuffer : NULL;
    
    // Print debug info every 60 frames
    static int frameCounter = 0;
    if (++frameCounter % 60 == 0) {
        printf("Running frame %d, bDraw=%d, buffer=%p, width=%d, height=%d\n", 
               frameCounter, bDraw, pBurnDraw, BurnDrvInfo.nWidth, BurnDrvInfo.nHeight);
    }
    
    // Run one frame of emulation
    INT32 nRet = BurnDrvFrame();
    if (nRet != 0) {
        printf("Error in BurnDrvFrame(): %d\n", nRet);
        return nRet;
    }
    
    // Render the frame if requested
    if (bDraw && pBurnDraw) {
        // Get dimensions from the driver
        int width = BurnDrvInfo.nWidth;
        int height = BurnDrvInfo.nHeight;
        
        // Make sure we have valid dimensions
        if (width <= 0 || height <= 0) {
            printf("Warning: Invalid game dimensions: %dx%d\n", width, height);
            width = 320;  // Default width
            height = 240; // Default height
        }
        
        // Render the frame
        int renderResult = Metal_RenderFrame(pBurnDraw, width, height);
        if (renderResult != 0) {
            printf("Error in Metal_RenderFrame(): %d\n", renderResult);
            return renderResult;
        }
        
        // Debug: Print a checksum of the first few bytes to verify content
        if (frameCounter % 60 == 0 && pBurnDraw) {
            uint32_t checksum = 0;
            for (int i = 0; i < 16 && i < width * height * (nBurnBpp / 8); i++) {
                checksum += ((uint8_t*)pBurnDraw)[i];
            }
            printf("Frame data checksum (first 16 bytes): 0x%08X\n", checksum);
        }
    }
    
    return 0;
} 