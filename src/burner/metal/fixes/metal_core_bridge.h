#ifndef _METAL_CORE_BRIDGE_H_
#define _METAL_CORE_BRIDGE_H_

// =============================================================================
// FBNeo Metal - Core Interface Bridge
// =============================================================================
// This header provides proper C linkage for core FBNeo functions that are
// called across the C/C++ boundary.
// =============================================================================

#include "metal_interop.h"

// Core FBNeo structures that need to be shared across C/C++ boundaries
METAL_BEGIN_C_DECL

// -----------------------------------------------------------------------------
// Core Structures
// -----------------------------------------------------------------------------

// ROM information structure
struct BurnRomInfo {
    char szName[100];
    UINT32 nLen;
    UINT32 nCrc;
    UINT32 nType;
};

// Sound route information
struct BurnSoundRoute {
    INT32 nVolume;
    INT32 nRouteToDevice;
    INT32 nRouteToOutput;
};

// Input structure
struct GameInp {
    UINT8 nInput;
    char* szName;
    UINT16 nVal;
    UINT16 nConst;
};

// -----------------------------------------------------------------------------
// Core Functions
// -----------------------------------------------------------------------------

// ROM/Driver functions
INT32 BurnDrvGetZipName(char** pszName, UINT32 i);
INT32 BurnDrvGetRomInfo(struct BurnRomInfo* pri, UINT32 i);
INT32 BurnDrvGetRomName(char** pszName, UINT32 i, UINT32 j);
INT32 BurnDrvGetText(char** pszText, UINT32 i);
UINT32 BurnDrvGetHardwareCode();
INT32 BurnDrvIsWorking();
UINT8* BurnDrvGetDefaultVisibleRects(INT32* pnNum);

// General emulation functions
void BurnSetRefreshRate(double dRefreshRate);
INT32 BurnRecalcPalette();
INT32 BurnLoadRom(UINT8* Dest, INT32 i, INT32 nGap);

// Sound/Audio functions
void BurnSoundInit();
void BurnSoundExit();
void BurnSoundPlay();
void BurnSoundStop();
void BurnSoundSetVolume(INT32 volume);
void BurnSoundRender(INT16* pSoundBuf, INT32 nSegmentLength);

// State management
INT32 BurnStateLoad(char* szName, INT32 nOffset, INT32 (*pLoadGame)());
INT32 BurnStateSave(char* szName, INT32 nOffset);

// Input system
INT32 BurnInputInit();
INT32 BurnInputExit();
INT32 BurnInputProcess();

METAL_END_C_DECL

#endif // _METAL_CORE_BRIDGE_H_ 