// Minimal CPS2 driver for Metal build
// This provides the essential CPS2 functions needed by the Metal frontend

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Basic types
typedef int32_t INT32;
typedef uint8_t UINT8;
typedef uint16_t UINT16;
typedef uint32_t UINT32;
typedef int16_t INT16;

// Global variables needed by CPS2
INT32 Cps = 2;  // CPS version
UINT8* CpsGfx = NULL;
UINT8* CpsRom = NULL;
UINT8* CpsZRom = NULL;
UINT8* CpsQSam = NULL;
UINT32 nCpsGfxLen = 0;
UINT32 nCpsRomLen = 0;
UINT32 nCpsZRomLen = 0;
UINT32 nCpsQSamLen = 0;

// Frame buffer - defined in metal_minimal_core.cpp
// extern UINT8* pBurnDraw;
// extern INT32 nBurnPitch;
// extern INT32 nBurnBpp;

// Input variables
UINT8 CpsReset = 0;
UINT8 CpsInp000[8] = {0};
UINT8 CpsInp001[8] = {0};
UINT8 CpsInp010[8] = {0};
UINT8 CpsInp011[8] = {0};
UINT8 CpsInp018[8] = {0};
UINT8 CpsInp020[8] = {0};
UINT8 CpsInp021[8] = {0};
UINT8 CpsInp119[8] = {0};  // Player 3 controls for some games

// Audio variables
INT16* pBurnSoundOut = NULL;
INT32 nBurnSoundLen = 0;
INT32 nBurnSoundRate = 44100;

// CPS2 initialization
extern "C" INT32 Cps2Init() {
    printf("[Cps2Init] Initializing CPS2 emulation\n");
    
    // Allocate memory for CPS2 components
    CpsGfx = (UINT8*)calloc(1, nCpsGfxLen);
    CpsRom = (UINT8*)calloc(1, nCpsRomLen);
    CpsZRom = (UINT8*)calloc(1, nCpsZRomLen);
    CpsQSam = (UINT8*)calloc(1, nCpsQSamLen);
    
    if (!CpsGfx || !CpsRom || !CpsZRom || !CpsQSam) {
        printf("[Cps2Init] Failed to allocate memory\n");
        return 1;
    }
    
    // Frame buffer is managed by Metal renderer, not here
    // pBurnDraw will be set by Metal_RunFrame
    
    printf("[Cps2Init] CPS2 initialized successfully\n");
    return 0;
}

// CPS2 exit
extern "C" INT32 CpsExit() {
    printf("[CpsExit] Exiting CPS2 emulation\n");
    
    // Free allocated memory
    if (CpsGfx) { free(CpsGfx); CpsGfx = NULL; }
    if (CpsRom) { free(CpsRom); CpsRom = NULL; }
    if (CpsZRom) { free(CpsZRom); CpsZRom = NULL; }
    if (CpsQSam) { free(CpsQSam); CpsQSam = NULL; }
    
    // Frame buffer is managed by Metal renderer
    
    return 0;
}

// Run one frame
extern "C" INT32 Cps2Frame() {
    static int frameCount = 0;
    frameCount++;
    
    // In a real implementation, this would:
    // 1. Run 68000 CPU
    // 2. Handle interrupts
    // 3. Update graphics
    // 4. Mix audio
    
    // For now, generate a simple animated pattern to show integration is working
    if (pBurnDraw) {
        UINT32* pixels = (UINT32*)pBurnDraw;
        int width = 384;
        int height = 224;
        
        // Create animated CPS2-style pattern
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int offset = y * width + x;
                
                // Create a CPS2-style grid pattern with animation
                bool gridX = (x % 32) < 2;
                bool gridY = (y % 32) < 2;
                bool isGrid = gridX || gridY;
                
                // Animate colors
                int phase = frameCount % 360;
                float hue = (phase + (x + y) * 0.5f) / 360.0f * 6.0f;
                
                UINT8 r = 0, g = 0, b = 0;
                
                if (isGrid) {
                    // Grid lines - animated color
                    int hi = (int)hue;
                    float f = hue - hi;
                    
                    switch (hi % 6) {
                        case 0: r = 255; g = (UINT8)(f * 255); b = 0; break;
                        case 1: r = (UINT8)((1-f) * 255); g = 255; b = 0; break;
                        case 2: r = 0; g = 255; b = (UINT8)(f * 255); break;
                        case 3: r = 0; g = (UINT8)((1-f) * 255); b = 255; break;
                        case 4: r = (UINT8)(f * 255); g = 0; b = 255; break;
                        case 5: r = 255; g = 0; b = (UINT8)((1-f) * 255); break;
                    }
                } else {
                    // Background - dark blue gradient
                    r = 0;
                    g = 0;
                    b = 32 + (y * 32 / height);
                }
                
                // Add "CPS2" text in the center
                int textX = x - (width/2 - 40);
                int textY = y - (height/2 - 10);
                bool inText = false;
                
                // Simple "CPS2" bitmap
                if (textY >= 0 && textY < 20 && textX >= 0 && textX < 80) {
                    const char* cps2Text[20] = {
                        "  CCCC  PPPP   SSSS  2222  ",
                        " CC  CC PP  PP SS  SS    22 ",
                        "CC      PP  PP SS        22 ",
                        "CC      PPPP    SSS     22  ",
                        "CC      PP        SS   22   ",
                        " CC  CC PP    SS  SS  22    ",
                        "  CCCC  PP     SSSS  222222 ",
                        "                            ",
                        "   METAL DRIVER ACTIVE      ",
                        "                            "
                    };
                    
                    if (textY < 10) {
                        int charIndex = textX / 3;
                        if (charIndex < 28 && cps2Text[textY][charIndex] != ' ') {
                            inText = true;
                        }
                    }
                }
                
                if (inText) {
                    // White text
                    r = 255;
                    g = 255;
                    b = 255;
                }
                
                pixels[offset] = (0xFF << 24) | (r << 16) | (g << 8) | b;
            }
        }
    }
    
    if (frameCount % 60 == 0) {
        printf("[Cps2Frame] Frame %d completed\n", frameCount);
    }
    
    return 0;
}

// Input processing
extern "C" void CpsRwGetInp() {
    // Process input state
    // This would read from the input arrays set by the frontend
}

// Graphics functions
extern "C" INT32 CpsDraw() {
    // Drawing is handled in Cps2Frame for now
    return 0;
}

extern "C" INT32 CpsObjGet() {
    // Object/sprite processing
    return 0;
}

// Run init/exit
extern "C" INT32 CpsRunInit() {
    printf("[CpsRunInit] Initializing CPS run system\n");
    return 0;
}

extern "C" INT32 CpsRunExit() {
    printf("[CpsRunExit] Exiting CPS run system\n");
    return 0;
}

// ROM loading functions
extern "C" INT32 CpsGetROMs(INT32 bLoad) {
    printf("[CpsGetROMs] Getting ROMs (load=%d)\n", bLoad);
    
    if (bLoad) {
        // Allocate memory for ROMs
        nCpsRomLen = 4 * 1024 * 1024;   // 4MB program
        nCpsGfxLen = 16 * 1024 * 1024;  // 16MB graphics
        nCpsZRomLen = 64 * 1024;         // 64KB Z80
        nCpsQSamLen = 4 * 1024 * 1024;  // 4MB QSound
        
        CpsRom = (UINT8*)calloc(1, nCpsRomLen);
        CpsGfx = (UINT8*)calloc(1, nCpsGfxLen);
        CpsZRom = (UINT8*)calloc(1, nCpsZRomLen);
        CpsQSam = (UINT8*)calloc(1, nCpsQSamLen);
        
        if (!CpsRom || !CpsGfx || !CpsZRom || !CpsQSam) {
            printf("[CpsGetROMs] Failed to allocate ROM memory\n");
            return 1;
        }
    }
    
    return 0;
}

// Driver functions
extern "C" INT32 BurnDrvFind(const char* name) {
    printf("[BurnDrvFind] Looking for driver: %s\n", name);
    if (strcmp(name, "mvsc") == 0) {
        return 0;  // Return index 0 for mvsc
    }
    return -1;
}

extern "C" INT32 BurnDrvSelect(INT32 i) {
    printf("[BurnDrvSelect] Selecting driver %d\n", i);
    return 0;
}

extern "C" UINT32 nBurnDrvCount = 1;
extern "C" INT32 nBurnDrvActive = 0; 