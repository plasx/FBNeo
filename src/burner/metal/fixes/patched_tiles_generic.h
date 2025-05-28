#ifndef _PATCHED_TILES_GENERIC_H_
#define _PATCHED_TILES_GENERIC_H_

/*
 * Patched version of tiles_generic.h for Metal build
 * 
 * This header provides C-compatible declarations without
 * including the original tiles_generic.h file.
 */

#include <stdint.h>
#include <stdbool.h>

// Basic type definitions (also in patched_burn.h but included here for completeness)
typedef int32_t INT32;
typedef uint32_t UINT32;
typedef int16_t INT16;
typedef uint16_t UINT16;
typedef int8_t INT8;
typedef uint8_t UINT8;

// Define the GenericTilesGfx structure
struct GenericTilesGfx {
    UINT8 *gfxbase;
    INT32 depth;
    INT32 width;
    INT32 height;
    INT32 pitch;
    INT32 entry_count;
    INT32 color_offset;
    INT32 color_depth;
    INT32 flags;
};

// Define the clip_struct structure
struct clip_struct {
    INT32 left;
    INT32 top;
    INT32 right;
    INT32 bottom;
};

#ifdef __cplusplus
extern "C" {
#endif

// Clean declaration of GenericTilesGfx with struct tag
extern struct GenericTilesGfx GenericGfxData[];

// Clean declaration for clip_struct with struct tag
extern struct clip_struct* BurnBitmapClipDims(INT32 nBitmapNumber);

// Clean function declarations for Generic Tilemap functions
extern void GenericTilemapDraw_C(INT32 which, UINT16* Bitmap, INT32 priority, INT32 priority_mask);
extern void GenericTilemapSetOffsets_3Param(INT32 which, INT32 x, INT32 y);
extern void GenericTilemapSetOffsets_5Param(INT32 which, INT32 x, INT32 y, INT32 x_flipped, INT32 y_flipped);

// Define wrapper function for GenericTilemapDraw with priority_mask parameter
#define GenericTilemapDraw(which, Bitmap, priority, priority_mask) \
    GenericTilemapDraw_C(which, Bitmap, priority, priority_mask)

// Define wrapper macro for GenericTilemapSetOffsets with different parameter counts
#define GenericTilemapSetOffsets_3Args(which, x, y) \
    GenericTilemapSetOffsets_3Param(which, x, y)

#define GenericTilemapSetOffsets_5Args(which, x, y, x_flipped, y_flipped) \
    GenericTilemapSetOffsets_5Param(which, x, y, x_flipped, y_flipped)

// Helper macros to count arguments
#define TILES_NARGS_SEQ(_1,_2,_3,_4,_5,N,...) N
#define TILES_NARGS(...) TILES_NARGS_SEQ(__VA_ARGS__, 5, 4, 3, 2, 1)

// Dispatch macro based on number of arguments
#define GenericTilemapSetOffsets(...) \
    TILES_DISPATCH(GenericTilemapSetOffsets, TILES_NARGS(__VA_ARGS__))(__VA_ARGS__)

#define TILES_DISPATCH(func, nargs) TILES_DISPATCH_(func, nargs)
#define TILES_DISPATCH_(func, nargs) func ## _ ## nargs ## Args

// Other necessary functions from tiles_generic.h
extern void GenericTilesInit();
extern void GenericTilesExit();
extern void GenericTilesClearScreen();
extern void GenericTilesClearClipRect(INT32 nBitmap);
extern void GenericTileSetClipRect(INT32 nBitmap, INT32 left, INT32 top, INT32 right, INT32 bottom);
extern void GenericTileGetClipRect(INT32 nBitmap, INT32* left, INT32* top, INT32* right, INT32* bottom);

#ifdef __cplusplus
}
#endif

#endif /* _PATCHED_TILES_GENERIC_H_ */ 