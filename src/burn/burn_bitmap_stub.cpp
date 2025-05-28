#include "burnint.h"

// Stub implementations for missing bitmap functions

void BurnBitmapExit() {
    // No-op for Metal build
}

void BurnBitmapAllocate(INT32 num, INT32 width, INT32 height, bool allocPrio) {
    printf("[BurnBitmapAllocate] Allocate bitmap (Metal stub) - num: %d, size: %dx%d\n", num, width, height);
}

UINT16* BurnBitmapGetBitmap(INT32 num) {
    printf("[BurnBitmapGetBitmap] Get bitmap (Metal stub) - num: %d\n", num);
    return NULL;
}

UINT8* BurnBitmapGetPriomap(INT32 num) {
    printf("[BurnBitmapGetPriomap] Get priomap (Metal stub) - num: %d\n", num);
    return NULL;
}

void BurnTransferSetDimensions(INT32 width, INT32 height) {
    // No-op for Metal build
}

void BurnTransferRealloc() {
    // No-op for Metal build
}

void PutPix(void* pDst, UINT32 c) {
    // No-op for Metal build
}

// Stub for tilemap functions
void GenericTilemapExit() {
    // No-op for Metal build
}

void GenericTilemapSetGfx(INT32 nGfx, UINT8 *pGfx, INT32 nDepth, INT32 nTileWidth, INT32 nTileHeight, INT32 nGfxLen, UINT32 nColorOffset, UINT32 nColorMask) {
    // No-op for Metal build
} 