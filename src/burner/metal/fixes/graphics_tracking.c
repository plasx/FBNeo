#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Graphics tracking for Metal implementation

// Track sprite rendering
void GraphicsTracker_TrackSprites(int totalSprites, int renderedSprites) {
    fprintf(stderr, "[GRAPHICS INIT] Sprites: %d total, %d rendered\n", 
          totalSprites, renderedSprites);
}

// Track texture usage
void GraphicsTracker_TrackTexture(const char* textureName, int width, int height, int format) {
    fprintf(stderr, "[GRAPHICS INIT] Texture: %s, %dx%d, format: %d\n", 
          textureName ? textureName : "unknown", width, height, format);
}

// Track frame rendering
void GraphicsTracker_TrackFrame(int frameNumber, float frameTime) {
    if (frameNumber % 60 == 0) {  // Log only every 60 frames to reduce spam
        fprintf(stderr, "[RENDERER LOOP] Frame: %d, time: %.2f ms\n", 
              frameNumber, frameTime);
    }
}

// Initialize graphics tracking
void GraphicsTracker_Init() {
    fprintf(stderr, "[GRAPHICS INIT] Graphics tracking initialized\n");
    
    // Report basic graphics info for CPS2
    fprintf(stderr, "[GRAPHICS INIT] System: CPS2\n");
    fprintf(stderr, "[GRAPHICS INIT] Resolution: 384x224\n");
    fprintf(stderr, "[GRAPHICS INIT] Color depth: 16-bit\n");
    fprintf(stderr, "[GRAPHICS INIT] Sprites: Hardware accelerated\n");
}

// Shutdown graphics tracking
void GraphicsTracker_Shutdown() {
    fprintf(stderr, "[GRAPHICS INIT] Graphics tracking shutdown\n");
} 