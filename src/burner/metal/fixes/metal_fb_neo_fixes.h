#pragma once

// Metal FBNeo compatibility fixes for macOS
// This header is designed to fix compatibility issues between C and C++ code in FBNeo

// Include our base compatibility header first
#include "c_cpp_fixes.h"

// Include standard headers
#include <stddef.h>  // For size_t
#include <stdint.h>  // For fixed-width integer types
#include <time.h>    // For time_t, clock_t, etc.

// C/C++ compatibility helpers
#ifdef __cplusplus
extern "C" {
#endif

// Basic Metal-specific constants
#define MAX_PATH 260
#define DIRS_MAX 20

// Basic Metal types - ensure consistent across C/C++
typedef uint8_t UINT8;
typedef int8_t INT8;
typedef uint16_t UINT16;
typedef int16_t INT16;
typedef uint32_t UINT32;
typedef int32_t INT32;
typedef uint64_t UINT64;
typedef int64_t INT64;

// Needed for compatibility with Metal frameworks
#ifdef __OBJC__
    #define FBNEO_BOOL bool
#else
    #define FBNEO_BOOL int
    typedef FBNEO_BOOL BOOL;
#endif

// Forward declaration of struct cheat_core 
// (defined in c_cpp_fixes.h to avoid redefinition)
struct cheat_core;

// Metal string type macros for handling const qualifiers
// These turn non-const char* to const char* for safer usage
#define METAL_CONST_CHAR(x) ((const char*)(x))
#define METAL_CONST_WCHAR(x) ((const wchar_t*)(x))
#define METAL_COPY_STRING(dst, src) do { if (src) { size_t len = strlen(src) + 1; dst = (char*)malloc(len); if (dst) memcpy(dst, src, len); } else { dst = NULL; } } while(0)

// Frame buffer global variables
extern UINT8* pBurnDraw_Metal;  // Pointer to the current frame buffer
extern INT32 nBurnPitch_Metal;  // Pitch (bytes per line) for the frame buffer
extern INT32 nBurnBpp_Metal;    // Bytes Per Pixel (2, 3, or 4)

// Metal render function signature
INT32 Metal_RenderFrame(void* frameData, int width, int height);

// Driver stub functions for Metal
INT32 Metal_InitDriver();
INT32 Metal_ExitDriver();
INT32 Metal_LoadRom(char* romName);
INT32 Metal_Frame();
INT32 Metal_Draw();
INT32 Metal_Scan(INT32 nAction, INT32* pnMin);

// State management
INT32 Metal_SaveState(char* fileName);
INT32 Metal_LoadState(char* fileName);

// Palette handling
void Metal_CalcPalette();
void Metal_SetPalette(UINT32 index, UINT32 rgb);

// System initialization functions
void Metal_InitSystem();
void Metal_ExitSystem();

#ifdef __cplusplus
}
#endif 