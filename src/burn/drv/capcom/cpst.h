#ifndef _CPST_H_
#define _CPST_H_

// CPS Tile rendering constants
// This file defines constants needed for CPS rendering

#ifdef __cplusplus
extern "C" {
#endif

// Tile type definitions
#define CTT_FLIPX      0x01
#define CTT_FLIPY      0x02
#define CTT_CARE       0x04    // Clip tile when partially visible
#define CTT_ROWS       0x08    // Use row scroll
#define CTT_8X8        0x10    // 8x8 tile size
#define CTT_16X16      0x20    // 16x16 tile size (standard)
#define CTT_32X32      0x40    // 32x32 tile size

// Rendering parameters
extern INT32 nCpstType;         // Tile type (CTT_*)
extern INT32 nCpstX;            // X position
extern INT32 nCpstY;            // Y position
extern INT32 nCpstTile;         // Tile number
extern INT32 nCpstFlip;         // Flip state
extern INT32 CpstRowShift;      // Row shift amount
extern UINT16 CpstPmsk;         // Priority mask
#ifndef USE_METAL_FIXES
extern UINT16 nCpstPal;         // Palette - avoid conflict with cps.h in Metal build
#endif
extern INT32 nFirstY;           // First Y position to draw
extern INT32 nLastY;            // Last Y position to draw
extern INT32 nFirstX;           // First X position to draw
extern INT32 nLastX;            // Last X position to draw
extern INT32 nStartline;        // Starting scan line
#ifndef USE_METAL_FIXES
extern INT32 nEndline;          // Ending scan line - avoid conflict with cps.h in Metal build
#endif

// Tile rendering constants for RENDER_ROW
#define RENDER_ROW_ROW           32  // Maximum number of rows
#define RENDER_ROW_BLOCK         8   // Block width
#define RENDER_ROW_ROWMAX      256   // Maximum row count
#define RENDER_ROW_COUNT       384   // Total tile count

// Inline function to set the palette - only if not in Metal build to avoid conflict
#ifndef USE_METAL_FIXES
inline static void CpstSetPal(INT32 nPal)
{
    nCpstPal = nPal;
}
#endif

#ifdef __cplusplus
}
#endif

#endif // _CPST_H_ 