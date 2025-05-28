#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../metal_declarations.h"

// Define the AIActions struct since we're referencing it
struct AIActions {
    int player;
    unsigned int buttons;
    float joystickX;
    float joystickY;
    float confidence;
};

// BurnDrv functions
int BurnDrvExit() {
    fprintf(stderr, "[EMULATOR] BurnDrvExit called\n");
    return 0;
}

int BurnDrvFrame() {
    static int frame_count = 0;
    if (frame_count % 60 == 0) {
        fprintf(stderr, "[EMULATOR] BurnDrvFrame frame %d\n", frame_count);
    }
    frame_count++;
    return 0;
}

int BurnDrvGetIndex(const char* name) {
    fprintf(stderr, "[ROM CHECK] BurnDrvGetIndex called for '%s'\n", name ? name : "NULL");
    // Always return 0 for testing
    return 0;
}

const char* BurnDrvGetTextA(unsigned int iIndex) {
    switch (iIndex) {
        case DRV_NAME:
            return "mvsc";
        case DRV_FULLNAME:
            return "Marvel vs. Capcom: Clash of Super Heroes (USA 980123)";
        case DRV_TITLE:
            return "Marvel vs. Capcom";
        case DRV_DATE:
            return "1998";
        case DRV_MANUFACTURER:
            return "Capcom";
        case DRV_SYSTEM:
            return "CPS2";
        default:
            return "";
    }
}

int BurnDrvGetVisibleSize(int* pnWidth, int* pnHeight) {
    if (pnWidth) *pnWidth = 384;  // Standard CPS2 width
    if (pnHeight) *pnHeight = 224; // Standard CPS2 height
    return 0;
}

int BurnDrvInit() {
    fprintf(stderr, "[HW INIT] BurnDrvInit called\n");
    return 0;
}

int BurnDrvSelect(int nDrvNum) {
    fprintf(stderr, "[ROM CHECK] BurnDrvSelect called with index %d\n", nDrvNum);
    return 0;
}

int BurnSoundRender(short* pSoundBuf, int nSegmentLength) {
    // Just a stub - no actual sound rendering
    if (!pSoundBuf) return 0;
    
    // Fill with silence
    memset(pSoundBuf, 0, nSegmentLength * sizeof(short) * 2); // *2 for stereo
    
    return 0;
}

// Metal AI functions (stubs)
void Metal_AI_Initialize() {
    fprintf(stderr, "[AI] Metal_AI_Initialize called\n");
}

void Metal_AI_ProcessFrame() {
    // Nothing to do
}

void Metal_AI_RenderOverlay() {
    // Nothing to do
}

void Metal_AI_Shutdown() {
    fprintf(stderr, "[AI] Metal_AI_Shutdown called\n");
}

// AI stub functions
bool AI_ApplyActions(const struct AIActions* actions) {
    // Nothing to do
    return true;
}

// Frame buffer access
unsigned char* Metal_GetFrameBuffer(int* width, int* height, int* pitch) {
    if (width) *width = 384;
    if (height) *height = 224;
    if (pitch) *pitch = 384 * 4; // 4 bytes per pixel (RGBA)
    
    // Allocate a static buffer if we don't have one
    static unsigned char* buffer = NULL;
    if (!buffer) {
        buffer = (unsigned char*)malloc(384 * 224 * 4);
        // Fill with a pattern
        if (buffer) {
            memset(buffer, 0, 384 * 224 * 4);
        }
    }
    
    return buffer;
}

// ZIP utility functions
int Metal_GetZipFileInfo(const char* zipPath, const char* internalPath, unsigned int* size, unsigned int* crc) {
    // Just return some dummy values
    if (size) *size = 1024 * 1024; // 1MB
    if (crc) *crc = 0xDEADBEEF;
    return 0;
}

int Metal_ListZipContents(const char* zipPath, char** fileList, int maxFiles, int* numFiles) {
    if (!zipPath || !fileList || !numFiles) return -1;
    
    // Return a fake list with two entries
    if (maxFiles >= 2) {
        strcpy(fileList[0], "mvsc.key");
        strcpy(fileList[1], "mvsc.rom");
        *numFiles = 2;
    } else {
        *numFiles = 0;
    }
    
    return 0;
} 