#include "burnint.h"
#include "metal_fixes.h"

// Global arrays for tilemap system
GenericTilesGfx GenericGfxData[MAX_TILEMAPS];

void GenericTilemapInit(INT32 which, INT32 (*pScan)(INT32 col, INT32 row), void (*pTile)(INT32 offs, GenericTilemapCallbackStruct *sTile), UINT32 tile_width, UINT32 tile_height, UINT32 map_width, UINT32 map_height)
{
    // No-op for Metal build
}

void GenericTilemapSetGfx(INT32 num, UINT8 *gfxbase, INT32 depth, INT32 tile_width, INT32 tile_height, INT32 gfxlen, UINT32 color_offset, UINT32 color_mask)
{
    // No-op for Metal build
}

void GenericTilemapSetScrollX(INT32 which, INT32 scrollx)
{
    // No-op for Metal build
}

void GenericTilemapSetScrollY(INT32 which, INT32 scrolly)
{
    // No-op for Metal build
}

void GenericTilemapSetTransparent(INT32 which)
{
    // No-op for Metal build
}

void GenericTilemapDraw(INT32 which, UINT16 *dest, INT32 flags)
{
    // No-op for Metal build
}

void GenericTilemapExit()
{
    // No-op for Metal build
} 