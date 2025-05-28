#ifndef FBNEO_COMPAT_H
#define FBNEO_COMPAT_H

// FBNeo compatibility header for Metal build
// This provides all necessary types and fixes for building FBNeo sources

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// Basic types that FBNeo expects
#ifndef INT8
typedef int8_t INT8;
#endif

#ifndef UINT8
typedef uint8_t UINT8;
#endif

#ifndef INT16
typedef int16_t INT16;
#endif

#ifndef UINT16
typedef uint16_t UINT16;
#endif

#ifndef INT32
typedef int32_t INT32;
#endif

#ifndef UINT32
typedef uint32_t UINT32;
#endif

#ifndef INT64
typedef int64_t INT64;
#endif

#ifndef UINT64
typedef uint64_t UINT64;
#endif

// TCHAR type - FBNeo uses this for text
#ifndef _TCHAR_DEFINED
#define _TCHAR_DEFINED
typedef char TCHAR;
#endif

// Fast call convention - not used on ARM
#ifndef __fastcall
#define __fastcall
#endif

// CPU status constants
#ifndef CPU_IRQSTATUS_NONE
#define CPU_IRQSTATUS_NONE 0
#define CPU_IRQSTATUS_ACK  1
#define CPU_IRQSTATUS_AUTO 2
#define CPU_IRQSTATUS_HOLD 4
#endif

// Callback typedefs that are missing
typedef INT32 (*CpsRunInitCallback)();
typedef INT32 (*CpsRunResetCallback)();
typedef INT32 (*CpsRunFrameStartCallback)();
typedef INT32 (*CpsRunFrameMiddleCallback)();
typedef INT32 (*CpsRunFrameEndCallback)();
typedef void (*CpsRWSoundCommandCallback)(UINT16);

// Input set structures
typedef struct {
    UINT8* pVal;
    UINT8 nVal;
    UINT8 nType;
} CPSINPSET;

typedef CPSINPSET CPSINPEX;

// Scan macros
#ifndef SCAN_VAR
#define SCAN_VAR(x) BurnAreaScan(ACB_VOLATILE, &x, sizeof(x), #x)
#endif

// Cheat system stubs
#ifndef CheatSearchExcludeAddressRange
#define CheatSearchExcludeAddressRange(a, b) ((void)0)
#endif

// CPS2 specific defines
#ifndef Cps2Turbo
#define Cps2Turbo 0
#endif

// Memory scan
#ifndef ACB_VOLATILE
#define ACB_VOLATILE 0x01
#endif

#ifndef BurnAreaScan
inline void BurnAreaScan(INT32 nAction, void* pData, INT32 nLen, const char* szName) {
    // Stub for now
}
#endif

// Aspect ratio functions
inline void BurnDrvGetAspect(INT32* x, INT32* y) {
    if (x) *x = 4;
    if (y) *y = 3;
}

#define BurnDrvSetAspect(x, y) ((void)0)

// Disable problematic features
#define USE_REINITIALISE 0
#define Reinitialise() ((void)0)

// Print function
inline INT32 bprintf_impl(INT32 nStatus, const char* szFormat, ...) {
    if (nStatus == 0) {
        va_list args;
        va_start(args, szFormat);
        vprintf(szFormat, args);
        va_end(args);
    }
    return 0;
}

// Endian swapping
#ifndef BURN_ENDIAN_SWAP_INT16
#ifdef __APPLE__
#include <libkern/OSByteOrder.h>
#define BURN_ENDIAN_SWAP_INT16(x) OSSwapInt16(x)
#define BURN_ENDIAN_SWAP_INT32(x) OSSwapInt32(x)
#else
#define BURN_ENDIAN_SWAP_INT16(x) ((((x) & 0xFF) << 8) | (((x) & 0xFF00) >> 8))
#define BURN_ENDIAN_SWAP_INT32(x) ((((x) & 0xFF) << 24) | (((x) & 0xFF00) << 8) | (((x) & 0xFF0000) >> 8) | (((x) & 0xFF000000) >> 24))
#endif
#endif

#endif // FBNEO_COMPAT_H 