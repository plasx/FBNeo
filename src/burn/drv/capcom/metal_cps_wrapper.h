#ifndef METAL_CPS_WRAPPER_H
#define METAL_CPS_WRAPPER_H

// Include our compatibility layer first
#include "../../../burner/metal/fbneo_compat.h"

// Define TCHAR before any FBNeo headers
#ifndef _TCHAR_DEFINED
#define _TCHAR_DEFINED
typedef char TCHAR;
#endif

// Fix BurnLoadRom signature conflict
#define BURNLOADROM_FIXED 1

// Override problematic function declarations
#define bprintf bprintf_impl
extern INT32 (*bprintf)(INT32 nStatus, TCHAR* szFormat, ...);

// Fix timer function conflicts
#define BurnTimerUpdate BurnTimerUpdateMetal
#define BurnTimerCPUTotalCycles BurnTimerCPUTotalCyclesMetal
#define BurnTransferCopy BurnTransferCopyMetal
#define BurnTransferInit BurnTransferInitMetal

// Function declarations with correct signatures
INT32 BurnTimerUpdateMetal(INT32 nCycles);
UINT64 BurnTimerCPUTotalCyclesMetal();
INT32 BurnTransferCopyMetal(UINT32* pPalette);
INT32 BurnTransferInitMetal();

// BurnLoadRom wrapper to fix signature mismatch
inline INT32 BurnLoadRom(UINT8* pDest, INT32 nNum, INT32 nPass) {
    INT32 nWrote = 0;
    extern INT32 BurnLoadRom(UINT8* Dest, INT32* pnWrote, INT32 i);
    return BurnLoadRom(pDest, &nWrote, nNum);
}

// Now include the real CPS header
#include "cps.h"

#endif // METAL_CPS_WRAPPER_H 