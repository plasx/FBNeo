#include "burnint.h"
#include "tiles_generic.h"
#include "metal_fixes.h"

// Global variables for tile system
UINT16* pTransDraw = NULL;
PRIORITY_TYPE* pPrioDraw = NULL;

// Screen dimensions
INT32 nScreenWidth = 384;
INT32 nScreenHeight = 224;
INT32 nTransWidth = 384;
INT32 nTransHeight = 224;

// Clipping variables
static INT32 nClipX1 = 0, nClipX2 = 384;
static INT32 nClipY1 = 0, nClipY2 = 224;

// GFX storage
static UINT8* pGfxBase[MAX_GFX];
static INT32 nGfxDepth[MAX_GFX];
static INT32 nGfxWidth[MAX_GFX];
static INT32 nGfxHeight[MAX_GFX];
static INT32 nGfxLen[MAX_GFX];
static UINT32 nGfxColorOffset[MAX_GFX];
static UINT32 nGfxColorMask[MAX_GFX];

INT32 GenericTilesInit()
{
    Debug_GenericTilesInitted = 1;
    
    // Initialize clip region
    nClipX1 = 0; nClipX2 = nScreenWidth;
    nClipY1 = 0; nClipY2 = nScreenHeight;
    
    // Clear GFX arrays
    memset(pGfxBase, 0, sizeof(pGfxBase));
    memset(nGfxDepth, 0, sizeof(nGfxDepth));
    memset(nGfxWidth, 0, sizeof(nGfxWidth));
    memset(nGfxHeight, 0, sizeof(nGfxHeight));
    memset(nGfxLen, 0, sizeof(nGfxLen));
    memset(nGfxColorOffset, 0, sizeof(nGfxColorOffset));
    memset(nGfxColorMask, 0, sizeof(nGfxColorMask));
    
    printf("[GenericTilesInit] Tile system initialized\n");
    return 0;
}

INT32 GenericTilesExit()
{
    // Don't call GenericTilemapExit() recursively - call the stub version
    Debug_GenericTilesInitted = 0;
    
    if (pTransDraw) {
        free(pTransDraw);
        pTransDraw = NULL;
    }
    
    if (pPrioDraw) {
        free(pPrioDraw);
        pPrioDraw = NULL;
    }
    
    printf("[GenericTilesExit] Tile system shut down\n");
    return 0;
}

void GenericTilesSetGfx(INT32 nNum, UINT8 *GfxBase, INT32 nDepth, INT32 nTileWidth, INT32 nTileHeight, INT32 nGfxLen, UINT32 nColorOffset, UINT32 nColorMask)
{
    if (nNum >= 0 && nNum < MAX_GFX) {
        pGfxBase[nNum] = GfxBase;
        nGfxDepth[nNum] = nDepth;
        nGfxWidth[nNum] = nTileWidth;
        nGfxHeight[nNum] = nTileHeight;
        ::nGfxLen[nNum] = nGfxLen;
        nGfxColorOffset[nNum] = nColorOffset;
        nGfxColorMask[nNum] = nColorMask;
        
        printf("[GenericTilesSetGfx] GFX %d set: %dx%d, depth=%d\n", nNum, nTileWidth, nTileHeight, nDepth);
    }
}

void BurnTransferClear()
{
    if (pTransDraw) {
        memset(pTransDraw, 0, nTransWidth * nTransHeight * sizeof(UINT16));
    }
}

void BurnTransferClear(UINT16 nFillPattern)
{
    if (pTransDraw) {
        UINT16* p = pTransDraw;
        for (INT32 i = 0; i < nTransWidth * nTransHeight; i++) {
            *p++ = nFillPattern;
        }
    }
}

void BurnPrioClear()
{
    if (pPrioDraw) {
        memset(pPrioDraw, 0, nTransWidth * nTransHeight * sizeof(PRIORITY_TYPE));
    }
}

INT32 BurnTransferCopy(UINT32* pPalette)
{
    if (!pTransDraw || !pBurnDraw) {
        return 1;
    }
    
    // Copy transfer buffer to main screen buffer
    UINT16* pSrc = pTransDraw;
    UINT8* pDst = pBurnDraw;
    
    for (INT32 y = 0; y < nTransHeight; y++) {
        for (INT32 x = 0; x < nTransWidth; x++) {
            UINT16 pixel = *pSrc++;
            
            if (pPalette && pixel < 0x8000) {
                UINT32 color = pPalette[pixel];
                
                if (nBurnBpp == 2) {
                    // 16-bit RGB565
                    UINT16 r = (color >> 19) & 0x1F;
                    UINT16 g = (color >> 10) & 0x3F;
                    UINT16 b = (color >> 3) & 0x1F;
                    *(UINT16*)pDst = (r << 11) | (g << 5) | b;
                    pDst += 2;
                } else if (nBurnBpp == 4) {
                    // 32-bit RGBA
                    *(UINT32*)pDst = color;
                    pDst += 4;
                }
            } else {
                // Direct color
                if (nBurnBpp == 2) {
                    *(UINT16*)pDst = pixel;
                    pDst += 2;
                } else if (nBurnBpp == 4) {
                    *(UINT32*)pDst = pixel;
                    pDst += 4;
                }
            }
        }
        pDst += nBurnPitch - (nTransWidth * nBurnBpp);
    }
    
    return 0;
}

INT32 BurnTransferPartial(UINT32* pPalette, INT32 nStart, INT32 nEnd)
{
    // Transfer partial scanlines
    if (nEnd > nTransHeight) nEnd = nTransHeight;
    if (nStart < 0) nStart = 0;
    
    if (!pTransDraw || !pBurnDraw) {
        return 1;
    }
    
    UINT16* pSrc = pTransDraw + (nStart * nTransWidth);
    UINT8* pDst = pBurnDraw + (nStart * nBurnPitch);
    
    for (INT32 y = nStart; y < nEnd; y++) {
        for (INT32 x = 0; x < nTransWidth; x++) {
            UINT16 pixel = *pSrc++;
            
            if (pPalette && pixel < 0x8000) {
                UINT32 color = pPalette[pixel];
                
                if (nBurnBpp == 2) {
                    UINT16 r = (color >> 19) & 0x1F;
                    UINT16 g = (color >> 10) & 0x3F;
                    UINT16 b = (color >> 3) & 0x1F;
                    *(UINT16*)pDst = (r << 11) | (g << 5) | b;
                    pDst += 2;
                } else if (nBurnBpp == 4) {
                    *(UINT32*)pDst = color;
                    pDst += 4;
                }
            } else {
                if (nBurnBpp == 2) {
                    *(UINT16*)pDst = pixel;
                    pDst += 2;
                } else if (nBurnBpp == 4) {
                    *(UINT32*)pDst = pixel;
                    pDst += 4;
                }
            }
        }
        pDst += nBurnPitch - (nTransWidth * nBurnBpp);
    }
    
    return 0;
}

void BurnTransferSetDimensions(INT32 nWidth, INT32 nHeight)
{
    nTransWidth = nWidth;
    nTransHeight = nHeight;
    nScreenWidth = nWidth;
    nScreenHeight = nHeight;
    
    printf("[BurnTransferSetDimensions] Set to %dx%d\n", nWidth, nHeight);
}

INT32 BurnTransferFindSpill()
{
    // For debugging - check for writes outside buffer
    return 0;
}

void BurnTransferExit()
{
    if (Debug_BurnTransferInitted) {
        // Check for spills
        if (BurnTransferFindSpill()) {
            printf("!!! BurnTransferExit(): Game wrote past pTransDraw's allocated dimensions!\n");
            printf("... Frame %d. Transfer dimensions %d x %d\n", nCurrentFrame, nTransWidth, nTransHeight);
        }
    }
    
    // Clean up transfer buffers directly
    if (pTransDraw) {
        free(pTransDraw);
        pTransDraw = NULL;
    }
    
    if (pPrioDraw) {
        free(pPrioDraw);
        pPrioDraw = NULL;
    }
    
    Debug_BurnTransferInitted = 0;
}

INT32 BurnTransferInit()
{
    Debug_BurnTransferInitted = 1;
    
    // Allocate transfer buffers directly
    if (pTransDraw) {
        free(pTransDraw);
    }
    if (pPrioDraw) {
        free(pPrioDraw);
    }
    
    pTransDraw = (UINT16*)malloc(nTransWidth * nTransHeight * sizeof(UINT16));
    pPrioDraw = (PRIORITY_TYPE*)malloc(nTransWidth * nTransHeight * sizeof(PRIORITY_TYPE));
    
    if (pTransDraw) {
        memset(pTransDraw, 0, nTransWidth * nTransHeight * sizeof(UINT16));
    }
    if (pPrioDraw) {
        memset(pPrioDraw, 0, nTransWidth * nTransHeight * sizeof(PRIORITY_TYPE));
    }
    
    printf("[BurnTransferInit] Transfer system initialized %dx%d\n", nTransWidth, nTransHeight);
    return 0;
}

void BurnTransferFlip(INT32 bFlipX, INT32 bFlipY)
{
    // Handle screen flipping
    if (!pTransDraw) return;
    
    if (bFlipX || bFlipY) {
        // Implement screen flipping if needed
        printf("[BurnTransferFlip] Flipping screen X=%d Y=%d\n", bFlipX, bFlipY);
    }
}

void BurnTransferRealloc()
{
    // Reallocate transfer buffers directly
    if (pTransDraw) {
        free(pTransDraw);
    }
    if (pPrioDraw) {
        free(pPrioDraw);
    }
    
    pTransDraw = (UINT16*)malloc(nTransWidth * nTransHeight * sizeof(UINT16));
    pPrioDraw = (PRIORITY_TYPE*)malloc(nTransWidth * nTransHeight * sizeof(PRIORITY_TYPE));
    
    if (pTransDraw) {
        memset(pTransDraw, 0, nTransWidth * nTransHeight * sizeof(UINT16));
    }
    if (pPrioDraw) {
        memset(pPrioDraw, 0, nTransWidth * nTransHeight * sizeof(PRIORITY_TYPE));
    }
    
    printf("[BurnTransferRealloc] Reallocated %dx%d\n", nTransWidth, nTransHeight);
}

// Clipping functions
void GenericTilesSetClip(INT32 nMinx, INT32 nMaxx, INT32 nMiny, INT32 nMaxy)
{
    nClipX1 = nMinx; nClipX2 = nMaxx;
    nClipY1 = nMiny; nClipY2 = nMaxy;
}

void GenericTilesGetClip(INT32 *nMinx, INT32 *nMaxx, INT32 *nMiny, INT32 *nMaxy)
{
    if (nMinx) *nMinx = nClipX1;
    if (nMaxx) *nMaxx = nClipX2;
    if (nMiny) *nMiny = nClipY1;
    if (nMaxy) *nMaxy = nClipY2;
}

void GenericTilesClearClip()
{
    nClipX1 = 0; nClipX2 = nScreenWidth;
    nClipY1 = 0; nClipY2 = nScreenHeight;
}

void GenericTilesSetClipRaw(INT32 nMinx, INT32 nMaxx, INT32 nMiny, INT32 nMaxy)
{
    GenericTilesSetClip(nMinx, nMaxx, nMiny, nMaxy);
}

void GenericTilesClearClipRaw()
{
    GenericTilesClearClip();
}

void GenericTilesSetScanline(INT32 nScanline)
{
    // Set current scanline for interlaced rendering
}

// GfxDecode functions - simplified for Metal
void GfxDecode(INT32 num, INT32 numPlanes, INT32 xSize, INT32 ySize, INT32 planeoffsets[], INT32 xoffsets[], INT32 yoffsets[], INT32 modulo, UINT8 *pSrc, UINT8 *pDest)
{
    // Simple tile decoding for Metal build
    if (!pSrc || !pDest) return;
    
    printf("[GfxDecode] Decoding GFX %d: %dx%d, %d planes\n", num, xSize, ySize, numPlanes);
    
    // For now, just copy data directly
    memcpy(pDest, pSrc, xSize * ySize * numPlanes);
}

void GfxDecodeSingle(INT32 which, INT32 numPlanes, INT32 xSize, INT32 ySize, INT32 planeoffsets[], INT32 xoffsets[], INT32 yoffsets[], INT32 modulo, UINT8 *pSrc, UINT8 *pDest)
{
    // Decode single tile
    GfxDecode(which, numPlanes, xSize, ySize, planeoffsets, xoffsets, yoffsets, modulo, pSrc, pDest);
}

// Basic tile rendering function
void Render8x8Tile(UINT16* pDestDraw, INT32 nTileNumber, INT32 StartX, INT32 StartY, INT32 nTilePalette, INT32 nColourDepth, INT32 nPaletteOffset, UINT8 *pTile)
{
    if (!pDestDraw || !pTile) return;
    
    // Simple 8x8 tile rendering
    for (INT32 y = 0; y < 8; y++) {
        for (INT32 x = 0; x < 8; x++) {
            INT32 pixel = pTile[y * 8 + x];
            if (pixel) {
                INT32 dx = StartX + x;
                INT32 dy = StartY + y;
                
                if (dx >= nClipX1 && dx < nClipX2 && dy >= nClipY1 && dy < nClipY2) {
                    pDestDraw[dy * nScreenWidth + dx] = (nTilePalette << nColourDepth) + pixel + nPaletteOffset;
                }
            }
        }
    }
}

// Additional rendering functions as simplified versions
void Render8x8Tile_Clip(UINT16* pDestDraw, INT32 nTileNumber, INT32 StartX, INT32 StartY, INT32 nTilePalette, INT32 nColourDepth, INT32 nPaletteOffset, UINT8 *pTile)
{
    Render8x8Tile(pDestDraw, nTileNumber, StartX, StartY, nTilePalette, nColourDepth, nPaletteOffset, pTile);
}

void Render16x16Tile(UINT16* pDestDraw, INT32 nTileNumber, INT32 StartX, INT32 StartY, INT32 nTilePalette, INT32 nColourDepth, INT32 nPaletteOffset, UINT8 *pTile)
{
    if (!pDestDraw || !pTile) return;
    
    // Simple 16x16 tile rendering
    for (INT32 y = 0; y < 16; y++) {
        for (INT32 x = 0; x < 16; x++) {
            INT32 pixel = pTile[y * 16 + x];
            if (pixel) {
                INT32 dx = StartX + x;
                INT32 dy = StartY + y;
                
                if (dx >= nClipX1 && dx < nClipX2 && dy >= nClipY1 && dy < nClipY2) {
                    pDestDraw[dy * nScreenWidth + dx] = (nTilePalette << nColourDepth) + pixel + nPaletteOffset;
                }
            }
        }
    }
}

void Render16x16Tile_Clip(UINT16* pDestDraw, INT32 nTileNumber, INT32 StartX, INT32 StartY, INT32 nTilePalette, INT32 nColourDepth, INT32 nPaletteOffset, UINT8 *pTile)
{
    Render16x16Tile(pDestDraw, nTileNumber, StartX, StartY, nTilePalette, nColourDepth, nPaletteOffset, pTile);
} 