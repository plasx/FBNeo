#import <Cocoa/Cocoa.h>
#import <MetalKit/MetalKit.h>
#include "burnint.h"
#include "app/metal_app.h"
#include "metal_bridge.h"
#include "metal_renderer.h"
#include "metal_exports.h"

// Game renderer implementation for Metal
// This connects the FBNeo core's rendering to our Metal implementation

// Function declarations from external modules
extern unsigned char* VidGetFrameBuffer();
extern void UpdateMetalFrameTexture(const void *frameData, unsigned int width, unsigned int height);
extern int nBurnBpp;
extern bool bRunPause;

// Frame counter for animation
static int frameCount = 0;

// Game renderer state
typedef struct {
    unsigned int width;
    unsigned int height;
    unsigned int pitch;
    unsigned int bpp;
    bool gameRunning;
    bool useCoreRendering;
} GameRenderState;

static GameRenderState renderState = {
    .width = 320,
    .height = 240,
    .pitch = 320 * 4,
    .bpp = 32,
    .gameRunning = false,
    .useCoreRendering = false
};

// Implement extern "C" functions from game_renderer.h
extern "C" {

// Initialize the game renderer
int GameRenderer_Init(int width, int height, int bpp) {
    printf("GameRenderer_Init(%d, %d, %d)\n", width, height, bpp);
    
    renderState.width = width > 0 ? width : 320;
    renderState.height = height > 0 ? height : 240;
    renderState.bpp = bpp > 0 ? bpp : 32;
    renderState.pitch = renderState.width * (renderState.bpp / 8);
    
    // Initialize the video subsystem with our dimensions
    int result = VidInit();
    if (result != 0) {
        printf("Error: Failed to initialize video subsystem\n");
        return result;
    }
    
    // Set the frame buffer size
    result = VidSetSize(renderState.width, renderState.height, renderState.bpp);
    if (result != 0) {
        printf("Error: Failed to set frame buffer size\n");
        return result;
    }
    
    printf("Game renderer initialized: %dx%d, %d bpp\n", 
           renderState.width, renderState.height, renderState.bpp);
    
    return 0;
}

// Shutdown the game renderer
int GameRenderer_Exit() {
    printf("GameRenderer_Exit()\n");
    
    // Cleanup the video subsystem
    int result = VidExit();
    
    renderState.gameRunning = false;
    
    return result;
}

// Render a frame of the game
int GameRenderer_RenderFrame() {
    printf("GameRenderer_RenderFrame called\n");
    
    // Get the frame buffer from the video subsystem
    unsigned char* frameBuffer = VidGetFrameBuffer();
    if (!frameBuffer) {
        printf("Error: Frame buffer is null\n");
        return 1;
    }
    
    // Print debug information
    printf("GameRenderer: frameBuffer=%p, renderState: %dx%d, BPP: %d\n", 
           frameBuffer, renderState.width, renderState.height, renderState.bpp);
    
    // Print first few bytes of the frame buffer
    printf("First 16 bytes: ");
    for (int i = 0; i < 16; i++) {
        printf("%02X ", frameBuffer[i]);
    }
    printf("\n");
    
    if (renderState.useCoreRendering) {
        // Use the core's rendering
        printf("Using core rendering\n");
        
        // This will update the frame buffer via BurnDrvFrame
        BurnDrvFrame();
        
        // Don't redeclare these variables - use the ones already declared in metal_renderer.mm
        printf("FBNeo core: pBurnDraw=%p, nBurnPitch=%d, nBurnBpp=%d\n", 
               pBurnDraw, nBurnPitch, nBurnBpp);
        
        // Draw a very thin colored border around the frame to verify rendering is active
        // This will help us see if the frame is being drawn at all, without being too intrusive
        if (pBurnDraw) {
            int width = renderState.width;
            int height = renderState.height;
            int bytesPerPixel = nBurnBpp / 8;
            if (bytesPerPixel < 1) bytesPerPixel = 1;
            
            // Use a less intrusive color for the border - green is more visible for debugging
            UINT32 borderColor = 0xFF00FF00; // Green in BGRA
            
            // Only draw at the very edges (1 pixel border)
            for (int x = 0; x < width; x++) {
                // Top and bottom edges only - just 1 pixel
                if (x % 8 == 0) { // Draw dotted line instead of solid for less intrusion
                    if (bytesPerPixel == 2) {
                        ((UINT16*)pBurnDraw)[x] = 0x07E0; // Green in RGB565
                        ((UINT16*)pBurnDraw)[(height-1)*width + x] = 0x07E0;
                    } else if (bytesPerPixel == 4) {
                        ((UINT32*)pBurnDraw)[x] = borderColor;
                        ((UINT32*)pBurnDraw)[(height-1)*width + x] = borderColor;
                    }
                }
            }
            
            for (int y = 0; y < height; y++) {
                // Left and right edges only - just 1 pixel
                if (y % 8 == 0) { // Draw dotted line instead of solid for less intrusion
                    if (bytesPerPixel == 2) {
                        ((UINT16*)pBurnDraw)[y*width] = 0x07E0; // Green in RGB565
                        ((UINT16*)pBurnDraw)[y*width + width-1] = 0x07E0;
                    } else if (bytesPerPixel == 4) {
                        ((UINT32*)pBurnDraw)[y*width] = borderColor;
                        ((UINT32*)pBurnDraw)[y*width + width-1] = borderColor;
                    }
                }
            }
        }
        
        // Let Metal_RenderFrame function handle the conversion and texture update
        if (pBurnDraw) {
            printf("Calling Metal_RenderFrame with pBurnDraw\n");
            Metal_RenderFrame(pBurnDraw, renderState.width, renderState.height);
        } else {
            // If pBurnDraw is null, use our frameBuffer as a fallback
            printf("WARNING: Calling Metal_RenderFrame with frameBuffer fallback\n");
            Metal_RenderFrame(frameBuffer, renderState.width, renderState.height);
        }
    } else {
        // For debugging: Draw a moving checkerboard pattern
        frameCount++;
        
        if (frameCount % 60 == 0) {
            printf("Rendering frame %d (test pattern)\n", frameCount);
        }
        
        int tileSize = 32;
        int offset = (frameCount / 2) % tileSize; // Animate the pattern
        
        // Fill the buffer with a test pattern
        for (int y = 0; y < renderState.height; y++) {
            for (int x = 0; x < renderState.width; x++) {
                int tileX = (x + offset) / tileSize;
                int tileY = (y + offset) / tileSize;
                bool isEvenTile = (tileX + tileY) % 2 == 0;
                
                uint32_t color;
                if (isEvenTile) {
                    // Calculate a gradient color based on position
                    uint8_t r = (x * 255) / renderState.width;
                    uint8_t g = (y * 255) / renderState.height;
                    uint8_t b = ((x + y) * 127) / (renderState.width + renderState.height);
                    color = (255 << 24) | (r << 16) | (g << 8) | b; // RGBA
                } else {
                    // Black color for odd tiles
                    color = 0xFF000000; // Black with full alpha
                }
                
                // Write the pixel to the frame buffer - note BGRA format
                uint32_t* pixel = (uint32_t*)(frameBuffer + y * renderState.pitch + x * 4);
                *pixel = color;
            }
        }
        
        // Draw a message that we're in test mode
        const char* testText = "TEST PATTERN - GAME LOADING";
        uint32_t textColor = 0xFFFFFFFF; // White
        int textX = 10;
        int textY = renderState.height / 2 - 4;
        
        for (int i = 0; testText[i] != '\0'; i++) {
            int charX = textX + i * 8;
            for (int y = 0; y < 8; y++) {
                for (int x = 0; x < 8; x++) {
                    if (x < 6 && y < 6) { // Simple rectangular characters
                        int bufferPos = (textY + y) * renderState.pitch + (charX + x) * 4;
                        *((uint32_t*)(frameBuffer + bufferPos)) = textColor;
                    }
                }
            }
        }
        
        // Draw FPS counter
        char fpsText[16];
        snprintf(fpsText, sizeof(fpsText), "FPS: %d", 60); // Hardcoded for now
        
        textX = 10;
        textY = 10;
        
        for (int i = 0; fpsText[i] != '\0'; i++) {
            int charX = textX + i * 8;
            for (int y = 0; y < 8; y++) {
                for (int x = 0; x < 8; x++) {
                    if (x < 6 && y < 6) { // Simple rectangular characters
                        int bufferPos = (textY + y) * renderState.pitch + (charX + x) * 4;
                        *((uint32_t*)(frameBuffer + bufferPos)) = textColor;
                    }
                }
            }
        }
        
        // Update Metal texture with the test pattern frame buffer
        Metal_RenderFrame(frameBuffer, renderState.width, renderState.height);
    }
    
    return 0;
}

// Set whether to use the core's rendering or our test pattern
void GameRenderer_SetUseCoreRendering(bool useCoreRendering) {
    renderState.useCoreRendering = useCoreRendering;
}

// Resize the game rendering surface
int GameRenderer_Resize(int width, int height) {
    printf("GameRenderer_Resize(%d, %d)\n", width, height);
    
    if (width <= 0 || height <= 0) {
        printf("Error: Invalid dimensions\n");
        return 1;
    }
    
    renderState.width = width;
    renderState.height = height;
    renderState.pitch = renderState.width * (renderState.bpp / 8);
    
    // Resize the frame buffer
    int result = VidSetSize(renderState.width, renderState.height, renderState.bpp);
    
    return result;
}

// Start the game rendering
int GameRenderer_Start() {
    printf("GameRenderer_Start()\n");
    renderState.gameRunning = true;
    return 0;
}

// Stop the game rendering
int GameRenderer_Stop() {
    printf("GameRenderer_Stop()\n");
    renderState.gameRunning = false;
    return 0;
}

// Is the game currently rendering
bool GameRenderer_IsRunning() {
    return renderState.gameRunning && !bRunPause;
}

} // extern "C" 