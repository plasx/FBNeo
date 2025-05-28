#include "burnint.h"
#include "cps.h"

// CTV function implementations for Metal port
// These implementations draw the CPS tiles for the Metal backend

// Global variables needed for drawing
extern UINT8 *CpsGfx;
extern UINT8 *pBurnDraw;
extern INT32 nBurnPitch;
extern INT32 nBurnBpp;
extern INT32 nCpsScreenWidth;
extern INT32 nCpsScreenHeight;

// Palette look-up variables
extern UINT32 *CpsPal;

// The pCtvTile points to the tile code (16 bits) and the palette (8 bits)
extern UINT8 *pCtvTile;

// CPS tile position variables (needed for rendering)
extern INT32 nCpstPosX;
extern INT32 nCpstPosY;

// Actual implementation of the CtvDo functions for drawing tiles
// Basic version - 2 BPP (4 colors)
extern "C" INT32 CtvDo2() {
    UINT32 nTileNumber = *(UINT16*)(pCtvTile);
    UINT8 nPalette = *(pCtvTile + 2);
    
    if (nBurnBpp < 2 || nBurnBpp > 4) {
        return 1; // Unsupported bit depth
    }
    
    // Calculate tile data address (each 8x8 tile takes 32 bytes in 2bpp mode)
    UINT8* pTileData = CpsGfx + (nTileNumber * 32);
    UINT32* pPalette = CpsPal + (nPalette << 4); // 16 colors per palette
    
    // Get current screen position
    UINT8* pPixel = pBurnDraw + (nCpstPosY * nBurnPitch) + (nCpstPosX * nBurnBpp);
    
    // Draw the 8x8 tile
    for (INT32 y = 0; y < 8; y++) {
        UINT8* pTile = pTileData + (y << 2); // Move down the tile data (4 bytes per line in 2bpp)
        UINT8* pLine = pPixel;
        
        for (INT32 x = 0; x < 8; x += 4) {
            UINT8 data = *pTile++;
            
            // Handle 4 pixels at a time (2 bits per pixel)
            for (INT32 b = 0; b < 4; b++) {
                UINT8 c = (data >> (6 - b * 2)) & 3; // Get color index (0-3)
                
                if (c) {
                    if (nBurnBpp == 2) {
                        *(UINT16*)(pLine) = (UINT16)pPalette[c];
                    } else if (nBurnBpp == 3) {
                        UINT32 color = pPalette[c];
                        pLine[0] = color & 0xFF;         // Red
                        pLine[1] = (color >> 8) & 0xFF;  // Green
                        pLine[2] = (color >> 16) & 0xFF; // Blue
                    } else if (nBurnBpp == 4) {
                        *(UINT32*)(pLine) = pPalette[c];
                    }
                }
                
                pLine += nBurnBpp;
            }
        }
        
        pPixel += nBurnPitch;
    }
    
    return 0;
}

// 4 BPP version (16 colors)
extern "C" INT32 CtvDo4() {
    UINT32 nTileNumber = *(UINT16*)(pCtvTile);
    UINT8 nPalette = *(pCtvTile + 2);
    
    if (nBurnBpp < 2 || nBurnBpp > 4) {
        return 1; // Unsupported bit depth
    }
    
    // Calculate tile data address (each 8x8 tile takes 64 bytes in 4bpp mode)
    UINT8* pTileData = CpsGfx + (nTileNumber * 64);
    UINT32* pPalette = CpsPal + (nPalette << 4); // 16 colors per palette
    
    // Get current screen position
    UINT8* pPixel = pBurnDraw + (nCpstPosY * nBurnPitch) + (nCpstPosX * nBurnBpp);
    
    // Draw the 8x8 tile
    for (INT32 y = 0; y < 8; y++) {
        UINT8* pTile = pTileData + (y << 3); // Move down the tile data (8 bytes per line in 4bpp)
        UINT8* pLine = pPixel;
        
        for (INT32 x = 0; x < 8; x += 2) {
            UINT8 data = *pTile++;
            
            // Handle 2 pixels at a time (4 bits per pixel)
            UINT8 c1 = data >> 4;   // First pixel color (high nibble)
            UINT8 c2 = data & 0x0F; // Second pixel color (low nibble)
            
            if (c1) {
                if (nBurnBpp == 2) {
                    *(UINT16*)(pLine) = (UINT16)pPalette[c1];
                } else if (nBurnBpp == 3) {
                    UINT32 color = pPalette[c1];
                    pLine[0] = color & 0xFF;         // Red
                    pLine[1] = (color >> 8) & 0xFF;  // Green
                    pLine[2] = (color >> 16) & 0xFF; // Blue
                } else if (nBurnBpp == 4) {
                    *(UINT32*)(pLine) = pPalette[c1];
                }
            }
            
            pLine += nBurnBpp;
            
            if (c2) {
                if (nBurnBpp == 2) {
                    *(UINT16*)(pLine) = (UINT16)pPalette[c2];
                } else if (nBurnBpp == 3) {
                    UINT32 color = pPalette[c2];
                    pLine[0] = color & 0xFF;         // Red
                    pLine[1] = (color >> 8) & 0xFF;  // Green
                    pLine[2] = (color >> 16) & 0xFF; // Blue
                } else if (nBurnBpp == 4) {
                    *(UINT32*)(pLine) = pPalette[c2];
                }
            }
            
            pLine += nBurnBpp;
        }
        
        pPixel += nBurnPitch;
    }
    
    return 0;
}

// 8 BPP version (256 colors)
extern "C" INT32 CtvDo8() {
    UINT32 nTileNumber = *(UINT16*)(pCtvTile);
    UINT8 nPalette = *(pCtvTile + 2);
    
    if (nBurnBpp < 2 || nBurnBpp > 4) {
        return 1; // Unsupported bit depth
    }
    
    // Calculate tile data address (each 8x8 tile takes 64 bytes in 8bpp mode)
    UINT8* pTileData = CpsGfx + (nTileNumber * 64);
    UINT32* pPalette = CpsPal + (nPalette << 8); // 256 colors per palette
    
    // Get current screen position
    UINT8* pPixel = pBurnDraw + (nCpstPosY * nBurnPitch) + (nCpstPosX * nBurnBpp);
    
    // Draw the 8x8 tile
    for (INT32 y = 0; y < 8; y++) {
        UINT8* pTile = pTileData + (y << 3); // Move down the tile data (8 bytes per line in 8bpp)
        UINT8* pLine = pPixel;
        
        for (INT32 x = 0; x < 8; x++) {
            UINT8 c = *pTile++;
            
            if (c) {
                if (nBurnBpp == 2) {
                    *(UINT16*)(pLine) = (UINT16)pPalette[c];
                } else if (nBurnBpp == 3) {
                    UINT32 color = pPalette[c];
                    pLine[0] = color & 0xFF;         // Red
                    pLine[1] = (color >> 8) & 0xFF;  // Green
                    pLine[2] = (color >> 16) & 0xFF; // Blue
                } else if (nBurnBpp == 4) {
                    *(UINT32*)(pLine) = pPalette[c];
                }
            }
            
            pLine += nBurnBpp;
        }
        
        pPixel += nBurnPitch;
    }
    
    return 0;
}

// Implement the rest of the CtvDo functions as requested
// Each just re-routes to the main implementations above
extern "C" INT32 CtvDo2b() { return CtvDo2(); }
extern "C" INT32 CtvDo2f() { return CtvDo2(); }
extern "C" INT32 CtvDo2fb() { return CtvDo2(); }
extern "C" INT32 CtvDo2x() { return CtvDo2(); }
extern "C" INT32 CtvDo2xb() { return CtvDo2(); }
extern "C" INT32 CtvDo2m() { return CtvDo2(); }
extern "C" INT32 CtvDo2mb() { return CtvDo2(); }
extern "C" INT32 CtvDo2mf() { return CtvDo2(); }
extern "C" INT32 CtvDo2mfb() { return CtvDo2(); }
extern "C" INT32 CtvDo2mx() { return CtvDo2(); }
extern "C" INT32 CtvDo2mxb() { return CtvDo2(); }

extern "C" INT32 CtvDo4b() { return CtvDo4(); }
extern "C" INT32 CtvDo4f() { return CtvDo4(); }
extern "C" INT32 CtvDo4fb() { return CtvDo4(); }
extern "C" INT32 CtvDo4x() { return CtvDo4(); }
extern "C" INT32 CtvDo4xb() { return CtvDo4(); }
extern "C" INT32 CtvDo4m() { return CtvDo4(); }
extern "C" INT32 CtvDo4mb() { return CtvDo4(); }
extern "C" INT32 CtvDo4mf() { return CtvDo4(); }
extern "C" INT32 CtvDo4mfb() { return CtvDo4(); }
extern "C" INT32 CtvDo4mx() { return CtvDo4(); }
extern "C" INT32 CtvDo4mxb() { return CtvDo4(); }

extern "C" INT32 CtvDo8b() { return CtvDo8(); }
extern "C" INT32 CtvDo8f() { return CtvDo8(); }
extern "C" INT32 CtvDo8fb() { return CtvDo8(); }
extern "C" INT32 CtvDo8x() { return CtvDo8(); }
extern "C" INT32 CtvDo8xb() { return CtvDo8(); }
extern "C" INT32 CtvDo8m() { return CtvDo8(); }
extern "C" INT32 CtvDo8mb() { return CtvDo8(); }
extern "C" INT32 CtvDo8mf() { return CtvDo8(); }
extern "C" INT32 CtvDo8mfb() { return CtvDo8(); }
extern "C" INT32 CtvDo8mx() { return CtvDo8(); }
extern "C" INT32 CtvDo8mxb() { return CtvDo8(); }

// Simple stubs for the rest of the CtvDoYYY variants
// All of them route to one of the three main implementations above
extern "C" INT32 CtvDoCmd(INT32 Cmd)
{
    switch (Cmd) {
        case 0x02: return CtvDo2();
        case 0x04: return CtvDo4();
        case 0x08: return CtvDo8();
        default: return 1;
    }
} 