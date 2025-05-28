#ifndef _BURNINT_METAL_H_
#define _BURNINT_METAL_H_

/*
 * C-compatible version of burnint.h
 * This provides all the necessary type definitions and declarations
 * for C code to interface with FBNeo's C++ codebase
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "../metal_declarations.h"
#include "c_cpp_fixes.h"

// Forward declarations for C compatibility
struct CheatInfo;
struct RomDataInfo;
struct BurnRomInfo;
struct tm;
enum BurnCartrigeCommand;

// Type definitions
typedef unsigned char UINT8;
typedef signed char INT8;
typedef unsigned short UINT16;
typedef signed short INT16;
typedef unsigned int UINT32;
typedef signed int INT32;
typedef unsigned long long UINT64;
typedef signed long long INT64;

// Macros
#define MAX_PATH 260

// Driver text constants
#define DRV_NAME        0
#define DRV_DATE        1
#define DRV_FULLNAME    2

// Constants for hardware addresses
typedef UINT32 HWAddressType;

// Game genre constants 
// These replace the macros in burn.h that are causing problems
#define GENRE_HORSHOOT  (1U << 0)
#define GENRE_VERSHOOT  (1U << 1)
#define GENRE_SCRFIGHT  (1U << 2)
#define GENRE_VSFIGHT   (1U << 3)
#define GENRE_BIOS      (1U << 4)
#define GENRE_PUZZLE    (1U << 5)
#define GENRE_PLATFORM  (1U << 11)

// Core FBNeo functions we need to call from C code
// Declare all functions with proper C parameter types

// ROM and driver handling
INT32 BurnLibInit(void);
INT32 BurnLibExit(void);
INT32 BurnDrvGetIndex(char* szName);
INT32 BurnDrvSelect(INT32 nDriver);
INT32 BurnDrvInit(void);
INT32 BurnDrvExit(void);
INT32 BurnDrvFrame(void);
INT32 BurnDrvReset(void);
char* BurnDrvGetTextA(UINT32 i);
INT32 BurnDrvGetVisibleSize(INT32* pnWidth, INT32* pnHeight);
INT32 BurnDrvGetAspect(INT32* pnXAspect, INT32* pnYAspect);

// Memory handling
void* BurnMalloc(size_t size);
void BurnFree(void* mem);

// Metal-specific implementation functions
INT32 BurnLibInit_Metal(void);
INT32 BurnLibExit_Metal(void);
INT32 BurnDrvInit_Metal(INT32 nDrvNum);
INT32 BurnDrvExit_Metal(void);
INT32 Metal_RunFrame(int bDraw);

// High color conversion functions
UINT32 BurnHighCol32(INT32 r, INT32 g, INT32 b, INT32 i);

// External variables declared as opaque pointers in C code
extern struct CheatInfo* pCheatInfo;
extern struct RomDataInfo* pRDI;
extern struct BurnRomInfo* pDataRomDesc;
extern INT32 nBurnDrvCount;  

// Frame buffer variables
extern UINT8* pBurnDraw;
extern INT32 nBurnPitch;
extern INT32 nBurnBpp;
extern INT32 nBurnSoundLen;
extern INT16* pBurnSoundOut;

// Input handling
extern UINT8 DrvJoy1[8];
extern UINT8 DrvJoy2[8];
extern UINT8 DrvJoy3[8];
extern UINT8 DrvJoy4[8];
extern UINT8 DrvDips[8];
extern UINT8 DrvReset;

extern INT32 nCurrentFrame;
extern UINT32 nFramesEmulated;
extern UINT32 nFramesRendered;
extern UINT32 nBurnFPS;

#endif // _BURNINT_METAL_H_
