#ifndef _BURN_CROSSPLATFORM_H
#define _BURN_CROSSPLATFORM_H

// Cross-platform definitions for compatibility between different platforms
#include <stdint.h>

// First include burn.h to get data type definitions
#include "burn.h"

#ifdef __cplusplus
extern "C" {
#endif

// Define PAIR type for 32-bit registers with 16-bit parts access
typedef union {
#ifdef LSB_FIRST
    struct { UINT8 l,h,h2,h3; } b;
    struct { UINT16 l,h; } w;
#else
    struct { UINT8 h3,h2,h,l; } b;
    struct { UINT16 h,l; } w;
#endif
    UINT32 d;
} PAIR;

// Byte swapping function - canonical definition for all files
// This must be used in all places that need to swap bytes
static inline void BurnByteswap(UINT8* pData, INT32 nLen) {
    int i;
    if (!pData) return;
    
    for (i = 0; i < nLen; i += 2) {
        if (i + 1 >= nLen) break; // Prevent buffer overrun
        UINT8 temp = pData[i];
        pData[i] = pData[i+1];
        pData[i+1] = temp;
    }
}

// Provide these functions if they don't exist
#ifndef _WIN32
// BurnSetRefreshRate - Sets emulator frame rate
void BurnSetRefreshRate(double dRefreshRate);

// BurnLoadRom - Load a ROM with byteswapping option
INT32 BurnLoadRom(UINT8* pDest, INT32 nNum, INT32 nPass);

// HiscoreReset - Reset high score system for a new game
INT32 HiscoreReset();
#endif

// Undefine Windows-specific macros that might cause issues
#ifdef _MSC_VER
#undef _T
#undef _TEXT
#undef _tfopen
#undef _stprintf
#endif

// Include platform-specific tchar.h 
#include "tchar.h"

#ifdef __cplusplus
}
#endif

#endif // _BURN_CROSSPLATFORM_H
