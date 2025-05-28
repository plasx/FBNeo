#ifndef METAL_COMPAT_LAYER_H
#define METAL_COMPAT_LAYER_H

// Metal Compatibility Layer for FBNeo Phase 3
// This header provides all necessary types and function declarations
// to avoid conflicts with complex FBNeo headers

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// Basic Type Definitions
// ============================================================================

// Define TCHAR and related macros - always define these
typedef char TCHAR;
#define _T(x) x
#define _tcscmp strcmp
#define _tcscpy strcpy
#define _tcslen strlen
#define _tcsncat strncat
#define _tcsncpy strncpy
#define _tcsstr strstr
#define _tcstol strtol
#define _tcstoul strtoul
#define _tprintf printf
#define _stprintf sprintf
#define _sntprintf snprintf
#define _tfopen fopen
#define _tremove remove
#define _trename rename

// Mark that TCHAR is defined
#ifndef TCHAR_DEFINED
#define TCHAR_DEFINED 1
#endif

// Basic integer types
typedef int8_t INT8;
typedef uint8_t UINT8;
typedef int16_t INT16;
typedef uint16_t UINT16;
typedef int32_t INT32;
typedef uint32_t UINT32;
typedef int64_t INT64;
typedef uint64_t UINT64;

// Path and string constants
#ifndef MAX_PATH
#define MAX_PATH 512
#endif

// ============================================================================
// FBNeo Core Constants
// ============================================================================

// Driver text constants
#define DRV_NAME            0
#define DRV_FULLNAME        1
#define DRV_COMMENT         2
#define DRV_MANUFACTURER    3
#define DRV_SYSTEM          4
#define DRV_PARENT          5
#define DRV_BOARD           6
#define DRV_SAMPLE          7
#define DRV_DATE            8
#define DRV_NEXTNAME        0x8000

// Hardware constants
#define HARDWARE_CAPCOM_CPS2    0x08000000
#define HARDWARE_PREFIX_CARTRIDGE 0x10000000

// ROM type constants
#define CPS2_PRG_68K            0x01
#define CPS2_GFX                0x02
#define CPS2_PRG_Z80            0x03
#define CPS2_QSND               0x04
#define CPS2_ENCRYPTION_KEY     0x05

// BRF (Burn ROM Flags) constants
#define BRF_PRG                 0x01
#define BRF_GRA                 0x02
#define BRF_SND                 0x04
#define BRF_ESS                 0x08
#define BRF_BIOS                0x10
#define BRF_SELECT              0x20
#define BRF_OPT                 0x40
#define BRF_NODUMP              0x80

// Input bit types
#define BIT_DIGITAL             0x01
#define BIT_ANALOG_REL          0x02
#define BIT_DIPSWITCH           0x04

// ============================================================================
// Structure Definitions
// ============================================================================

// ROM information structure
struct BurnRomInfo {
    const char *szName;
    UINT32 nLen;
    UINT32 nCrc;
    UINT32 nType;
};

// Input information structure
struct BurnInputInfo {
    const char* szName;
    UINT8 nType;
    union {
        UINT8* pVal;
        UINT16* pShortVal;
    };
    const char* szInfo;
};

// DIP switch information structure
struct BurnDIPInfo {
    INT32 nInput;
    UINT8 nFlags;
    UINT8 nMask;
    UINT8 nSetting;
    const char* szText;
};

// Sample information structure
struct BurnSampleInfo {
    const char* szName;
    UINT32 nFlags;
};

// Driver structure (simplified for Metal)
struct BurnDriver {
    const char* szShortName;
    const char* szFullNameA;
    const char* szComment;
    const char* szManufacturer;
    const char* szSystemName;
    const char* szParentName;
    const char* szBoardROM;
    const char* szSampleName;
    const char* szDate;
    UINT32 nFlags;
    INT32 nGenreFlags;
    INT32 nFamilyFlags;
    INT32 nMaxPlayers;
    UINT32 nHardwareCode;
    
    // Function pointers
    INT32 (*pGetRomInfo)(struct BurnRomInfo* pri, UINT32 i);
    INT32 (*pGetRomName)(const char** pszName, UINT32 i, INT32 nAka);
    INT32 (*pGetInputInfo)(struct BurnInputInfo* pii, UINT32 i);
    INT32 (*pGetDIPInfo)(struct BurnDIPInfo* pdi, UINT32 i);
    INT32 (*pGetSampleInfo)(struct BurnSampleInfo* psi, UINT32 i);
    INT32 (*pGetSampleName)(const char** pszName, UINT32 i, INT32 nAka);
    INT32 (*pGetHDDName)(const char** pszName, UINT32 i, INT32 nAka);
    INT32 (*pGetZipName)(const char** pszName, UINT32 i);
    INT32 (*Init)();
    INT32 (*Exit)();
    INT32 (*Frame)();
    INT32 (*Redraw)();
    INT32 (*AreaScan)(INT32*, INT32*);
    UINT8* pRecalcPal;
    UINT32 nPaletteEntries;
    INT32 nWidth, nHeight;
    INT32 nAspectX, nAspectY;
    
    // Additional fields for compatibility
    const char* szParent;
    const char* szFullNameW;
};

// ============================================================================
// Global Variables (External Declarations)
// ============================================================================

#ifdef __cplusplus
extern "C" {
#endif

// Core driver variables
extern UINT32 nBurnDrvCount;
extern int nBurnDrvActive;
extern struct BurnDriver* pDriver[];

// Graphics variables
extern UINT8* pBurnDraw;
extern INT32 nBurnPitch;
extern INT32 nBurnBpp;

// Audio variables
extern INT16* pBurnSoundOut;
extern INT32 nBurnSoundLen;
extern INT32 nBurnSoundRate;

// CPS2 input variables
extern UINT8 CpsInp000[8];
extern UINT8 CpsInp001[8];
extern UINT8 CpsInp010[8];
extern UINT8 CpsInp011[8];
extern UINT8 CpsInp018[8];
extern UINT8 CpsInp020[8];
extern UINT8 CpsInp021[8];
extern UINT8 CpsInp119[8];
extern UINT8 CpsReset;

// Printf function pointer
extern INT32 (*bprintf)(INT32 nStatus, TCHAR* szFormat, ...);

// ============================================================================
// Core Function Declarations
// ============================================================================

// Library management
INT32 BurnLibInit();
INT32 BurnLibExit();

// Driver management
INT32 BurnDrvSelect(INT32 nDrvNum);
INT32 BurnDrvInit();
INT32 BurnDrvExit();
INT32 BurnDrvFrame();
INT32 BurnDrvRedraw();
INT32 BurnDrvFind(const char* szName);

// Driver information
TCHAR* BurnDrvGetText(UINT32 i);
const char* BurnDrvGetTextA(UINT32 i);
INT32 BurnDrvGetIndex(const char* szName);
INT32 BurnDrvGetRomInfo(struct BurnRomInfo *pri, UINT32 i);
INT32 BurnDrvGetInputInfo(struct BurnInputInfo* pii, UINT32 i);
INT32 BurnDrvGetDIPInfo(struct BurnDIPInfo* pdi, UINT32 i);
INT32 BurnDrvGetRomName(const char** pszName, UINT32 i, INT32 nAka);
INT32 BurnDrvGetSampleInfo(struct BurnSampleInfo* psi, UINT32 i);
INT32 BurnDrvGetSampleName(const char** pszName, UINT32 i, INT32 nAka);
INT32 BurnDrvGetHDDName(const char** pszName, UINT32 i, INT32 nAka);
INT32 BurnDrvGetZipName(const char** pszName, UINT32 i);
INT32 BurnDrvGetVisibleSize(INT32* pnWidth, INT32* pnHeight);
INT32 BurnDrvGetVisibleOffs(INT32* pnLeft, INT32* pnTop);
INT32 BurnDrvGetHardwareCode();
INT32 BurnDrvGetFlags();
bool BurnDrvIsWorking();
INT32 BurnDrvGetMaxPlayers();
INT32 BurnDrvGetPaletteEntries();

// Input management
INT32 BurnInputInit();
INT32 BurnInputExit();
INT32 BurnDrvSetInput(INT32 i, INT32 nState);

// ROM loading
INT32 BurnLoadRom(UINT8* Dest, INT32* pnWrote, INT32 i);
INT32 BurnSetROMPath(const char* szPath);
const char* BurnGetROMPath();
INT32 BurnROMInit();
INT32 BurnROMExit();

// Memory management
void* BurnMalloc(size_t size);
void* BurnRealloc(void* ptr, size_t size);
void BurnFree(void* ptr);

// Sound management
INT32 BurnSoundInit();
INT32 BurnSoundExit();
void BurnSoundDCFilterReset();
INT32 BurnSoundRender(INT16* pSoundBuf, INT32 nSegmentLength);

// Graphics transfer
void BurnTransferCopy(UINT32 *pPalette);
void BurnTransferInit();
void BurnTransferExit();
void BurnClearScreen();
void BurnRecalcPal();

// Timer functions
UINT64 BurnTimerCPUTotalCycles();
void BurnTimerUpdate(INT32 nCycles);
void BurnTimerEndFrame(INT32 nCycles);

// Utility functions
void BurnSetRefreshRate(double dFrameRate);
UINT16 BurnRandom();
void BurnRandomInit();
void BurnRandomSetSeed(UINT64 nSeed);
double BurnGetTime();

// ============================================================================
// Metal Bridge Function Declarations
// ============================================================================

// Metal-specific bridge functions
INT32 BurnLibInit_Metal();
INT32 BurnLibExit_Metal();
INT32 BurnDrvInit_Metal(INT32 nDrvNum);
INT32 BurnDrvExit_Metal();
INT32 Metal_RunFrame(int bDraw);

// Frame buffer access
void* Metal_GetFrameBuffer();
bool IsFrameBufferUpdated();
void SetFrameBufferUpdated(bool updated);
void UpdateMetalFrameTexture(void* data, int width, int height);

// ROM validation
INT32 Metal_PrepareROMLoading(const char* romPath);
INT32 Metal_ValidateDriverROMs();
void Metal_PrintROMValidationStatus();

// Input functions
INT32 Metal_InitInput();
INT32 Metal_ExitInput();
INT32 Metal_HandleKeyDown(int keyCode);
INT32 Metal_HandleKeyUp(int keyCode);
void Metal_ProcessInput();

// Audio functions
INT32 Metal_InitAudio();
INT32 Metal_ExitAudio();
void Metal_UpdateAudio();
void Metal_SetAudioEnabled(bool enabled);
void Metal_SetAudioVolume(float volume);
float Metal_GetAudioVolume();
bool Metal_IsAudioInitialized();
void Metal_PrintAudioStatus();

// Phase 7 function declarations
void Metal_DumpFrame(int frameNumber);
void Metal_ToggleDebugOverlay();
INT32 Metal_QuickSave();
INT32 Metal_QuickLoad();
INT32 Metal_SaveState(int slot);
INT32 Metal_LoadState(int slot);
int Metal_GetCurrentSaveSlot();
const char* Metal_GetSaveStateStatus();
int Metal_GetActiveInputs();
void Metal_PrintInputState();
INT32 Metal_HandleCommandKey(int keyCode, bool isDown);
INT32 Metal_RenderAudio(INT16* buffer, INT32 samples);
float Metal_GetAudioLatency();

// C linkage functions
void* Metal_GetRawFrameBuffer();

#ifdef __cplusplus
}
#endif

#endif // METAL_COMPAT_LAYER_H 