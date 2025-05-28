// FinalBurn Neo - Emulator for MC68000/Z80 based arcade games
//            Refer to the "license.txt" file for more info

// Burner emulation library
#ifndef _BURN_H
#define _BURN_H

// Define TCHAR first, before any other includes
#ifndef TCHAR_DEFINED
#define TCHAR_DEFINED 1
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
#define _tmkdir mkdir
#define _trmdir rmdir
#define _tstat stat
#define _taccess access
#define _tchdir chdir
#define _tgetcwd getcwd
#define _tsplitpath splitpath
#define _tmakepath makepath
#define _tfullpath fullpath
#define _tfindfirst findfirst
#define _tfindnext findnext
#define _tfindclose findclose
#endif

#include <stdio.h>
#include <stdint.h>
#include <time.h>

// Define basic types
typedef int32_t INT32;
typedef int8_t INT8;
typedef uint8_t UINT8;
typedef int16_t INT16;
typedef uint16_t UINT16;
typedef uint32_t UINT32;
typedef int64_t INT64;
typedef uint64_t UINT64;

// Define MAX_PATH
#ifndef MAX_PATH
#define MAX_PATH 512
#endif

// Driver information
extern UINT32 nBurnDrvCount;
extern int nBurnDrvActive;

// Sound rate
extern INT32 nBurnSoundRate;

// CPS2 ROM type constants
#define CPS2_PRG_68K            0x01
#define CPS2_GFX                0x02
#define CPS2_PRG_Z80            0x03
#define CPS2_QSND               0x04
#define CPS2_ENCRYPTION_KEY     0x05
#define CPS2_PRG_68K_SIMM       0x06
#define CPS2_PRG_68K_XOR_TABLE  0x07
#define CPS2_GFX_SIMM           0x08
#define CPS2_GFX_SPLIT4         0x09
#define CPS2_GFX_SPLIT8         0x0A
#define CPS2_QSND_SIMM          0x0B
#define CPS2_QSND_SIMM_BYTESWAP 0x0C

// BRF (Burn ROM Flags) constants
#define BRF_PRG                 0x01
#define BRF_GRA                 0x02
#define BRF_SND                 0x04
#define BRF_ESS                 0x08
#define BRF_BIOS                0x10
#define BRF_SELECT              0x20
#define BRF_OPT                 0x40
#define BRF_NODUMP              0x80

// BDF (Burn Driver Flags) constants
#define BDF_GAME_WORKING        0x01
#define BDF_CLONE               0x02
#define BDF_PROTOTYPE           0x04
#define BDF_BOOTLEG             0x08
#define BDF_HACK                0x10
#define BDF_HOMEBREW            0x20
#define BDF_DEMO                0x40
#define BDF_HISCORE_SUPPORTED   0x80

// Hardware constants
#define HARDWARE_CAPCOM_CPS2    0x08000000
#define HARDWARE_PREFIX_CARTRIDGE 0x10000000

// Game genre flags
#define GBF_VSFIGHT             0x01
#define GBF_HORSHOOT            0x02
#define GBF_PUZZLE              0x04
#define GBF_QUIZ                0x08

// Family flags
#define FBF_SF                  0x01
#define FBF_DSTLK               0x02

// Input bit types
#define BIT_DIGITAL             0x01
#define BIT_ANALOG_REL          0x02
#define BIT_DIPSWITCH           0x04

// ROM information structure
struct BurnRomInfo {
    char *szName;
    UINT32 nLen;
    UINT32 nCrc;
    UINT32 nType;
};

// Input information structure
struct BurnInputInfo {
    char* szName;
    UINT8 nType;
    union {
        UINT8* pVal;
        UINT16* pShortVal;
    };
    char* szInfo;
};

// DIP switch information structure
struct BurnDIPInfo {
    INT32 nInput;
    UINT8 nFlags;
    UINT8 nMask;
    UINT8 nSetting;
    char* szText;
};

// Driver structure - match what burn.cpp expects
struct BurnDriver {
    char* szShortName;
    char* szFullNameA;
    char* szComment;
    char* szManufacturer;
    char* szSystemName;
    char* szParentName;  // Added for burn.cpp compatibility
    char* szBoardROM;
    char* szSampleName;
    char* szDate;
    UINT32 nFlags;
    INT32 nGenreFlags;
    INT32 nFamilyFlags;
    INT32 nMaxPlayers;
    UINT32 nHardwareCode;
    
    // Function pointers
    void* pGetRomInfo;   // Will be cast to proper type
    void* pGetRomName;   // Will be cast to proper type
    void* pGetInputInfo; // Will be cast to proper type
    void* pGetDIPInfo;   // Will be cast to proper type
    INT32 (*Init)();
    INT32 (*Exit)();
    INT32 (*Frame)();
    INT32 (*Redraw)();
    INT32 (*AreaScan)(INT32*, INT32*);
    UINT8* pRecalcPal;
    UINT32 nPaletteEntries;
    INT32 nWidth, nHeight;
    INT32 nAspectX, nAspectY;
    
    // Additional fields for burn.cpp compatibility
    char* szParent;      // Parent driver name
    char* szFullNameW;   // Wide character full name (same as szFullNameA for Metal)
    void* pGetZipName;   // ZIP name function pointer
};

extern struct BurnDriver* pDriver[];

// CPS2 input variables (declared as extern)
extern UINT8 CpsInp000[8];
extern UINT8 CpsInp001[8];
extern UINT8 CpsInp010[8];
extern UINT8 CpsInp011[8];
extern UINT8 CpsInp018[8];
extern UINT8 CpsInp020[8];
extern UINT8 CpsInp021[8];
extern UINT8 CpsInp119[8];
extern UINT8 CpsReset;

// Function pointer for printf
extern INT32 (*bprintf)(INT32 nStatus, TCHAR* szFormat, ...);

#ifdef __cplusplus
extern "C" {
#endif

// Core library functions
INT32 BurnLibInit();
INT32 BurnLibExit();
INT32 BurnDrvInit();
INT32 BurnDrvExit();
INT32 BurnDrvFrame();
INT32 BurnDrvRedraw();

// Driver management functions
INT32 BurnDrvSelect(INT32 nDrvNum);
INT32 BurnDrvSetInput(INT32 i, INT32 nState);
INT32 BurnDrvFind(const char* szName);

// Driver information functions
TCHAR* BurnDrvGetText(UINT32 i);
char* BurnDrvGetTextA(UINT32 i);
INT32 BurnDrvGetIndex(char* szName);
INT32 BurnDrvGetRomInfo(struct BurnRomInfo *pri, UINT32 i);
INT32 BurnDrvGetInputInfo(struct BurnInputInfo* pii, UINT32 i);
INT32 BurnDrvGetDIPInfo(struct BurnDIPInfo* pdi, UINT32 i);
INT32 BurnDrvGetRomName(char** pszName, UINT32 i, INT32 nAka);
INT32 BurnDrvGetVisibleSize(INT32* pnWidth, INT32* pnHeight);
INT32 BurnDrvGetHardwareCode();
INT32 BurnDrvGetFlags();
bool BurnDrvIsWorking();
INT32 BurnDrvGetMaxPlayers();

// ROM loading functions - use consistent signature
INT32 BurnLoadRom(UINT8* Dest, INT32* pnWrote, INT32 i);
void* BurnRealloc(void* ptr, size_t size);
void* BurnMalloc(size_t size);
void BurnFree(void* ptr);
INT32 BurnSetROMPath(const char* szPath);
const char* BurnGetROMPath();

// ROM system functions
INT32 BurnROMInit();
INT32 BurnROMExit();

// Timer functions - use consistent signatures
UINT64 BurnTimerCPUTotalCycles();
void BurnTimerUpdate(INT32 nCycles);
void BurnTimerEndFrame(INT32 nCycles);

// Graphics transfer functions - use consistent signatures
void BurnTransferCopy(UINT32 *pPalette);
void BurnTransferInit();
void BurnTransferExit();
void BurnClearScreen();

// Refresh rate function
void BurnSetRefreshRate(double dFrameRate);

// Random number functions
UINT16 BurnRandom();
void BurnRandomInit();
void BurnRandomSetSeed(UINT64 nSeed);

// Time functions
double BurnGetTime();
void BurnGetLocalTime(tm *nTime);

#ifdef __cplusplus
}
#endif

#endif 