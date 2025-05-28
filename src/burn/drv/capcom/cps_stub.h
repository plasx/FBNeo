#ifndef CPS_STUB_H
#define CPS_STUB_H

#include "burnint.h"
#include "cpst.h"

// CPS rendering stubs for Metal implementation
// Define all missing variables referenced in cpsrd.cpp

#ifdef __cplusplus
extern "C" {
#endif

// Only define these variables if they haven't been defined in cps.h
#ifndef CPS_H
extern UINT8* CpsrBase;
extern INT32 nCpsrScrX;
extern INT32 nCpsrScrY;
extern UINT16* CpsrRows;
extern INT32 nCpsrRowStart;
extern INT32 nEndline;
#endif

// Variables that don't appear in cps.h
extern UINT8* CpsSaveReg[0x100];
extern INT32 nBgHi;
extern INT32 nFlyCount;
extern UINT32 Scroll2TileMask;
extern INT32 nCpsGfxScroll[4];
extern INT32 nCpsScreenWidth;
extern INT32 nCpsScreenHeight; 
extern INT32 nStartline;
extern INT32 nCpsGfxLen;
extern INT32 nCpsGfxMask;
extern UINT8* CpsGfx;
extern UINT32 nRowScroll[17];
extern UINT8* CpsStar;
extern INT32 CpsQaNoRender;
extern UINT8* MaskAddr[4];

// CPS input variables
extern UINT8 CpsInp000[0x10];
extern UINT8 CpsInp001[0x10];
extern UINT8 CpsInp011[0x10];
extern UINT8 CpsInp020[0x10];
extern UINT8 CpsInp021[0x10];
extern UINT8 CpsInp029[0x10];
extern UINT8 CpsInp176[0x10];

// Rendering function prototypes
extern INT32 (*CpstOneDoX[3])();
extern INT32 (*CpstOneBgDoX[3])();
extern INT32 (*CpstOneObjDoX[2])();
extern INT32 (*CtvDoX[32])();
extern INT32 (*CtvDoXB[32])();
extern INT32 (*CtvDoXM[32])();

// CTV rendering variables
extern INT32 nCtvRollX;
extern INT32 nCtvRollY;
extern UINT8* pCtvTile;
extern INT32 nCtvTileAdd;
extern UINT8* pCtvLine;

// CPS2 variables
extern INT32 Cps2Turbo;
extern INT32 nCpsZ80Cycles;
extern INT32 nCpsCycles;
extern UINT8* CpsZRamC0;
extern UINT8* CpsZRamF0;

// CpsrLineInfo structure definition for row scrolling
struct CpsrLineInfo {
    INT32 nStart;       // Start position
    INT32 nWidth;       // Width
    INT32 nTileStart;   // Tile Start position
    INT32 nTileEnd;     // Tile End position
    INT16 Rows[16];     // Row shift data
    INT32 nMaxLeft;     // Maximum row scroll left
    INT32 nMaxRight;    // Maximum row scroll right
};

extern struct CpsrLineInfo CpsrLineInfo[32];

// Define these functions only if they're not in cps.h
#ifndef CPS_H
// Stub implementation of CpstSetPal
void CpstSetPal(INT32 nPal);

// Stub implementation for CPS fast video rendering
void CpsFastVidDefault();
#endif

// External function declarations
UINT8* CpsFindGfxRam(INT32 nOffset, INT32 nLen);

#ifdef __cplusplus
}
#endif

#endif // CPS_STUB_H 