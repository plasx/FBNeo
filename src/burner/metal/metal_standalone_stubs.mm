#include "metal_declarations.h"
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

// Emulation core variables
UINT8* pBurnDraw_Metal = NULL;
INT32 nBurnPitch_Metal = 0;
INT32 nBurnBpp_Metal = 0;

// Input state
static unsigned char inputState[256] = {0};

// Add missing global variables expected by metal_bridge.cpp
UINT8* pBurnDraw = NULL;
INT32 nBurnPitch = 0;
INT32 nBurnBpp = 32;  // Default to 32bpp for testing

// ROM paths
char szAppRomPaths[DIRS_MAX][MAX_PATH];
char szAppDirPath[MAX_PATH];
struct BurnDrvMeta BurnDrvInfo;

// Currently selected ROM
static NSString *currentRom = nil;

// Test frame buffer for rendering when no ROM is loaded
static uint32_t *testFrameBuffer = NULL;
static int testFrameWidth = 320;
static int testFrameHeight = 240;

// Metal view cleanup extension
@interface MTKView (Cleanup)
- (void)cleanup;
@end

@implementation MTKView (Cleanup)
- (void)cleanup {
    // This is a stub implementation for the cleanup selector
    NSLog(@"MTKView cleanup called");
}
@end

// BurnDrvGetVisibleSize implementation
void BurnDrvGetLocalVisibleSize(int *pnWidth, int *pnHeight) {
    if (pnWidth) *pnWidth = testFrameWidth;
    if (pnHeight) *pnHeight = testFrameHeight;
}

// Metal_GetFrameWidth implementation
int Metal_GetFrameWidth() {
    return testFrameWidth;
}

// Metal_GetFrameHeight implementation
int Metal_GetFrameHeight() {
    return testFrameHeight;
}

// Metal_RunFrame implementation
int Metal_RunFrame(int bDraw) {
    // For testing, just return 0 (success)
    return 0;
}

// GetROMPathString implementation
const char* GetROMPathString() {
    return currentRom ? [currentRom UTF8String] : "";
}

// SetCurrentROMPath implementation
int SetCurrentROMPath(const char* szPath) {
    if (szPath && szPath[0]) {
        currentRom = [NSString stringWithUTF8String:szPath];
        return 0;
    }
    return 1;
}

// Initialize the test frame buffer
void InitializeTestFrameBuffer() {
    if (testFrameBuffer) {
        free(testFrameBuffer);
    }
    
    testFrameBuffer = (uint32_t*)malloc(testFrameWidth * testFrameHeight * sizeof(uint32_t));
    if (!testFrameBuffer) {
        NSLog(@"Failed to allocate test frame buffer");
        return;
    }
    
    // Fill with a test pattern
    for (int y = 0; y < testFrameHeight; y++) {
        for (int x = 0; x < testFrameWidth; x++) {
            uint8_t r = (x * 255) / testFrameWidth;
            uint8_t g = (y * 255) / testFrameHeight;
            uint8_t b = ((x + y) * 127) / (testFrameWidth + testFrameHeight);
            
            testFrameBuffer[y * testFrameWidth + x] = (0xFF << 24) | (r << 16) | (g << 8) | b;
        }
    }
}

// Clean up the test frame buffer
void CleanupTestFrameBuffer() {
    if (testFrameBuffer) {
        free(testFrameBuffer);
        testFrameBuffer = NULL;
    }
}

// Get the test frame buffer
void* GetTestFrameBuffer() {
    if (!testFrameBuffer) {
        InitializeTestFrameBuffer();
    }
    return testFrameBuffer;
}

// Initialize controller handling
void Metal_ShowInputConfig(const char* gameName) {
    NSLog(@"Input config requested for game: %s", gameName ? gameName : "(global)");
}

// Initialize controller handling with tab name
void Metal_ShowInputConfigWithTab(const char* tabName) {
    NSLog(@"Input config requested with tab: %s", tabName ? tabName : "(default)");
}

// Metal_HandleKeyDown implementation
int Metal_HandleKeyDown(int keyCode) {
    // For testing, just return 0 (success)
    return 0;
}

// Metal_HandleKeyUp implementation
int Metal_HandleKeyUp(int keyCode) {
    // For testing, just return 0 (success)
    return 0;
}

// Initialize FBNeo core
extern "C" INT32 BurnLibInit() {
    printf("BurnLibInit: Initializing FBNeo core (stub)\n");
    return 0; // Success
}

// Shut down FBNeo core
extern "C" INT32 BurnLibExit() {
    printf("BurnLibExit: Shutting down FBNeo core (stub)\n");
    return 0; // Success
}

// Get driver index - match the declaration in metal_declarations.h
extern "C" INT32 BurnDrvGetIndex(const char* name) {
    printf("BurnDrvGetIndex: Looking for ROM driver %s (stub)\n", name);
    // Return a dummy index for testing
    return 0;
}

// Select driver
extern "C" INT32 BurnDrvSelect(INT32 nDriver) {
    printf("BurnDrvSelect: Selecting driver %d (stub)\n", nDriver);
    
    // Set up minimal driver info
    BurnDrvInfo.szName = (char*)"demo";
    BurnDrvInfo.szFullNameA = (char*)"Demo ROM";
    BurnDrvInfo.nWidth = 320;
    BurnDrvInfo.nHeight = 240;
    
    return 0; // Success
}

// Get driver text - match the declaration in metal_declarations.h
extern "C" const char* BurnDrvGetTextA(INT32 i) {
    switch (i) {
        case DRV_NAME:
            return "Demo ROM";
        case DRV_FULLNAME:
            return "Demo ROM for FBNeo Metal";
        case DRV_DATE:
            return "2025";
        default:
            return "";
    }
}

// Initialize driver
extern "C" INT32 BurnDrvInit() {
    printf("BurnDrvInit: Initializing ROM driver (stub)\n");
    return 0; // Success
}

// Exit driver
extern "C" INT32 BurnDrvExit() {
    printf("BurnDrvExit: Shutting down ROM driver (stub)\n");
    return 0; // Success
}

// Stub implementations of Metal functions for standalone build

// Metal initialization
int Metal_Initialize() {
    NSLog(@"[STUB] Metal_Initialize called");
    return 0;
}

void Metal_Shutdown() {
    NSLog(@"[STUB] Metal_Shutdown called");
}

int Metal_Init(void* viewPtr, void* settings) {
    NSLog(@"[STUB] Metal_Init called");
    return 0;
}

int Metal_Exit() {
    NSLog(@"[STUB] Metal_Exit called");
    return 0;
}

// Metal frame handling
int Metal_RenderFrame(void* frameData, int width, int height) {
    // This would normally render a frame to the Metal view
    return 0;
}

// Stub implementations for standalone build
int MetalRenderer_GetWidth() {
    return 320;
}

int MetalRenderer_GetHeight() {
    return 240;
}

int MetalRenderer_UpdateFrame(void* frameBuffer, int width, int height) {
    return 0;
}

// Export C functions for C code to call
extern "C" {
    int Metal_GetWidth() {
        return 320;
    }
    
    int Metal_GetHeight() {
        return 240;
    }
    
    void Metal_RenderTestPattern() {
        // This would render a test pattern
    }
}

// Main entry point for test app
int main(int argc, char* argv[]) {
    printf("FBNeo Metal Frame Verification Tool\n");
    printf("-----------------------------------\n");
    
    // Initialize frame buffer
    pBurnDraw_Metal = (UINT8*)malloc(320 * 240 * 4);
    if (!pBurnDraw_Metal) {
        printf("ERROR: Failed to allocate frame buffer\n");
        return 1;
    }
    
    // Clear buffer
    memset(pBurnDraw_Metal, 0, 320 * 240 * 4);
    
    // Set up dimensions
    nBurnBpp_Metal = 32;
    nBurnPitch_Metal = 320 * 4;
    
    // Set core pointers to our buffer
    pBurnDraw = pBurnDraw_Metal;
    nBurnPitch = nBurnPitch_Metal;
    nBurnBpp = nBurnBpp_Metal;
    
    // Verify the frame pipeline
    int width = 320;
    int height = 240;
    
    if (argc > 2) {
        width = atoi(argv[1]);
        height = atoi(argv[2]);
    }
    
    // Call the verification function
    extern int Metal_VerifyFramePipeline(int width, int height);
    Metal_VerifyFramePipeline(width, height);
    
    // Clean up
    free(pBurnDraw_Metal);
    pBurnDraw_Metal = NULL;
    pBurnDraw = NULL;
    
    return 0;
} 