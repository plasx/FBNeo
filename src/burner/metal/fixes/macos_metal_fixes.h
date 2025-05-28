#ifndef _MACOS_METAL_FIXES_H_
#define _MACOS_METAL_FIXES_H_

// ===========================================================================
// Comprehensive macOS/ARM64 Metal Build Fixes
// ===========================================================================
// This header file contains all the fixes needed for compiling FBNeo
// on macOS with Metal backend, particularly for ARM64 architecture.
// Add this to the beginning of any files that need the fixes.
// ===========================================================================

// Standard includes
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// ===========================================================================
// Type definitions
// ===========================================================================
#ifndef TYPES_DEFINED
#define TYPES_DEFINED
typedef int32_t INT32;
typedef uint32_t UINT32;
typedef int16_t INT16;
typedef uint16_t UINT16;
typedef int8_t INT8;
typedef uint8_t UINT8;
#endif

// ===========================================================================
// C/C++ compatibility
// ===========================================================================
#ifndef __cplusplus
    typedef int bool;
    #define true 1
    #define false 0
#endif

// ===========================================================================
// Fix for __fastcall on ARM64/macOS
// ===========================================================================
// On ARM64, the default calling convention is already efficient
// and fastcall is unnecessary and unsupported
#ifndef FASTCALL 
 #undef __fastcall
 #define __fastcall
#endif

// ===========================================================================
// Fix for the EXT_* macros used in m68kdasm.c
// ===========================================================================
#ifndef EXT_BD_SIZE
 #define EXT_BD_SIZE(A)                    (((A)>>4)&0x3)
#endif

#ifndef EXT_INDEX_SUPPRESS
 #define EXT_INDEX_SUPPRESS(A)             ((A)&0x40)
#endif

#ifndef EXT_BASE_SUPPRESS
 #define EXT_BASE_SUPPRESS(A)              ((A)&0x80)
#endif

// ===========================================================================
// Fix function overloading issues with SekGetPC
// ===========================================================================
#ifdef UINT32_SekGetPC
    #undef UINT32_SekGetPC
#endif

// ===========================================================================
// Fix for sprintf deprecation warnings
// ===========================================================================
#define sprintf_safe(buffer, size, format, ...) snprintf(buffer, size, format, ##__VA_ARGS__)

// ===========================================================================
// CPS variables - forward declarations
// ===========================================================================
#ifndef CPS_VARIABLES_DECLARED
#define CPS_VARIABLES_DECLARED
extern UINT8 CpsInp001[0x10];  // P2 inputs
extern INT32 CpsrLineInfo[16][16];  // Raster line info
extern UINT8* CpsrBase;  // Pointer to the scan-line table
extern INT32 nCpsrScrY;  // Offset to current scan-line
#endif

// ===========================================================================
// Disable warnings for build issues we cannot fix
// ===========================================================================
#pragma GCC diagnostic ignored "-Wmissing-braces"
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#endif // _MACOS_METAL_FIXES_H_ 