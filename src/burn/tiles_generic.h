#ifndef _TILES_GENERIC_H_
#define _TILES_GENERIC_H_

#include "burnint.h"

// Type definitions
typedef unsigned short PRIORITY_TYPE;
extern PRIORITY_TYPE* pPrioDraw;

// Function declarations
int GenericTilesInit();
int GenericTilesExit();
void GenericTilesSetGfx(INT32 nNum, UINT8 *GfxBase, INT32 nDepth, INT32 nTileWidth, INT32 nTileHeight, INT32 nGfxLen, UINT32 nColorOffset, UINT32 nColorMask);
void BurnTransferClear();
INT32 BurnTransferCopy(UINT32* pPalette);

// For tile drawing
extern unsigned char* pTileData;
// Screen dimensions - declared once here and externally referenced
extern INT32 nScreenWidth, nScreenHeight;
extern int nTileXPos, nTileYPos;

// For Zooming
extern unsigned short* pZBuffer;
extern unsigned short* pTransDraw;

// For Transparency unit sieve
extern unsigned char nBit[8];
extern unsigned char nMaskAnd[8];
extern unsigned char nMaskOr[8];

inline static unsigned int alpha_blend(unsigned int d, unsigned int s, unsigned int p)
{
	int a = 256 - p;

	return (((((s & 0xff00ff) * p) + ((d & 0xff00ff) * a)) & 0xff00ff00) |
		  ((((s & 0x00ff00) * p) + ((d & 0x00ff00) * a)) & 0x00ff0000)) >> 8;
}

#ifdef __cplusplus
extern "C" {
#endif

void GenericTilesSetClip(INT32 nMinx, INT32 nMaxx, INT32 nMiny, INT32 nMaxy);
void GenericTilesGetClip(INT32 *nMinx, INT32 *nMaxx, INT32 *nMiny, INT32 *nMaxy);
void GenericTilesClearClip();
void GenericTilesSetClipRaw(INT32 nMinx, INT32 nMaxx, INT32 nMiny, INT32 nMaxy);
void GenericTilesClearClipRaw();
void GenericTilesSetScanline(INT32 nScanline);

void GfxDecode(int num, int numPlanes, int xSize, int ySize, int planeOffsets[], int xOffsets[], int yOffsets[], int modulo, unsigned char *pSrc, unsigned char *pDest);

void NMK112_init(int game_type);
void NMK112_okibank_write(int chip, int bank, int val);
void NMK112_state_save();

// Declare each function with a unique signature to avoid conflicts
INT32 RenderZoomedTile1(unsigned short *dest, unsigned char *gfx, int code, int color, int drawmode, int sx, int sy, int fx, int fy, int width, int height, int zoomx, int zoomy);
INT32 RenderCustomTile_Mask1(unsigned short* pDestDraw, int nWidth, int nHeight, int nTileWidth, int nTileHeight, unsigned int nTilePalette, int nColourDepth, int nMaskColour, int nPaletteOffset, unsigned char *pTile);
void RenderPrioTile(unsigned short* pDestDraw, unsigned char* pTile, int nWidth, int nHeight, int nTileWidth, int nTileHeight, unsigned int nTilePalette, int nColourDepth, int nMaskColour, int nPaletteOffset, int nPriority);
void RenderPrioSprite(unsigned short* pDestDraw, unsigned char* pTile, int nWidth, int nHeight, int nTileWidth, int nTileHeight, unsigned int nTilePalette, int nColourDepth, int nMaskColour, int nPaletteOffset, int nPriority);

// These functions need to be used when access to the palette is not available and the tile data is 4bpp
void RenderTileClipped(unsigned short* pDestDraw, int nWidth, int nHeight, int nTileWidth, int nTileHeight, int nTilePalette, int nColourDepth, int nPaletteOffset, unsigned char *pTile);
void RenderSpriteClipped(unsigned short* pDestDraw, int nWidth, int nHeight, int nTileWidth, int nTileHeight, int nTilePalette, int nColourDepth, int nPaletteOffset, unsigned char *pTile);

// These functions need to be used when access to the palette is not available and the tile data is 8bpp
void RenderTile_indirectclipped(unsigned short* pDestDraw, int nWidth, int nHeight, int nTileWidth, int nTileHeight, int nTilePalette, int nColourDepth, int nPaletteOffset, unsigned char *pTile);
void RenderSprite_indirectclipped(unsigned short* pDestDraw, int nWidth, int nHeight, int nTileWidth, int nTileHeight, int nTilePalette, int nColourDepth, int nPaletteOffset, unsigned char *pTile);
void RenderTile_indirect(unsigned short* pDestDraw, int nWidth, int nHeight, int nTileWidth, int nTileHeight, int nTilePalette, int nColourDepth, int nPaletteOffset, unsigned char *pTile);

// These functions need to be used when access to the palette is not available and the sprite data is 8bpp
void RenderSprite_indirect(unsigned short* pDestDraw, int nWidth, int nHeight, int nTileWidth, int nTileHeight, int nTilePalette, int nColourDepth, int nPaletteOffset, unsigned char *pTile);
// and 8bpp w/masking
void RenderSprite_indirect_mask(unsigned short* pDestDraw, int nWidth, int nHeight, int nTileWidth, int nTileHeight, int nTilePalette, int nColourDepth, int nMaskColor, int nPaletteOffset, unsigned char *pTile);

// These functions need to be used when the graphics data has already been decoded to the destination colour-depth
void RenderTile_Mask(unsigned short* pDestDraw, int nWidth, int nHeight, int nTileWidth, int nTileHeight, int nMaskColour, int nPaletteOffset, unsigned char *pTile);
void RenderTile(unsigned short* pDestDraw, int nWidth, int nHeight, int nTileWidth, int nTileHeight, int nPaletteOffset, unsigned char *pTile);
void RenderSprite_Mask(unsigned short* pDestDraw, int nWidth, int nHeight, int nTileWidth, int nTileHeight, int nMaskColour, int nPaletteOffset, unsigned char *pTile);
void RenderSprite(unsigned short* pDestDraw, int nWidth, int nHeight, int nTileWidth, int nTileHeight, int nPaletteOffset, unsigned char *pTile);
void RenderZoomedTile_Mask(unsigned short* pDestDraw, int nWidth, int nHeight, int nTileWidth, int nTileHeight, int nMaskColour, int nPaletteOffset, int nZoomX, int nZoomY, unsigned char *pTile);
void RenderZoomedSprite_Mask(unsigned short* pDestDraw, int nWidth, int nHeight, int nTileWidth, int nTileHeight, int nMaskColour, int nPaletteOffset, int nZoomX, int nZoomY, unsigned char *pTile);
void RenderZoomedTile2(unsigned short* pDestDraw, int nWidth, int nHeight, int nTileWidth, int nTileHeight, int nPaletteOffset, int nZoomX, int nZoomY, unsigned char *pTile);
void RenderZoomedSprite(unsigned short* pDestDraw, int nWidth, int nHeight, int nTileWidth, int nTileHeight, int nPaletteOffset, int nZoomX, int nZoomY, unsigned char *pTile);

void RenderTileTranstab(UINT16 *dest, UINT8 *gfx, INT32 code, INT32 color, INT32 trans_col, INT32 sx, INT32 sy, INT32 flipx, INT32 flipy, INT32 width, INT32 height, UINT8 *tab);

INT32 BurnTransferInit();
void BurnTransferExit();

#ifdef __cplusplus
}
#endif

// Tile decoding macros
// Undefine any existing STEP macros from burnint.h
#undef STEP1
#undef STEP2
#undef STEP4
#undef STEP8
#undef STEP16
#undef STEP32

// Define our function-like macros
#define STEP1(start, step)	(start)
#define STEP2(start, step)	(start), (start)+(step)
#define STEP4(start, step)	STEP2(start, step), STEP2((start)+((step)*2), step)
#define STEP8(start, step)	STEP4(start, step), STEP4((start)+((step)*4), step)
#define STEP16(start, step)	STEP8(start, step), STEP8((start)+((step)*8), step)
#define STEP32(start, step)	STEP16(start, step), STEP16((start)+((step)*16), step)
#define STEP64(start, step)	STEP32(start, step), STEP32((start)+((step)*32), step)

// Aliases for compatibility with existing code
#define RenderZoomedTile RenderZoomedTile1
#define RenderCustomTile_Mask RenderCustomTile_Mask1
#define RenderZoomedTile_Portable RenderZoomedTile2

#endif // _TILES_GENERIC_H_
