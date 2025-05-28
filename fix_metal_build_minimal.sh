#!/bin/bash

# Minimal build script for FBNeo Metal
echo "FBNeo Metal Minimal Build"
echo "========================="

# Create directories
mkdir -p src/dep/generated
mkdir -p src/burner/metal/fixes
mkdir -p obj/metal/burner/metal/fixes
mkdir -p obj/metal/ai

# Copy required header files
cp src/burn/burnint.h src/dep/generated/burnint.h
cp src/burn/tiles_generic.h src/dep/generated/tiles_generic.h

# Create minimal fix files
echo "Creating fixes for missing symbols..."

# Create burn fixes
cat > src/burner/metal/fixes/burn_fixes.cpp << 'EOF'
#include "burnint.h"
#include "burn.h"   // Add for DIRS_MAX and bprintf

// Define DIRS_MAX if it's not defined
#ifndef DIRS_MAX
#define DIRS_MAX 8
#endif

// Stubs for missing symbols in burnint.h
// These are simplified implementations for Metal backend

// Rom loading stubs
bool bDoIpsPatch = false;
TCHAR szAppRomPaths[DIRS_MAX][MAX_PATH] = { { _T("") }, };
// nBurnDrvActive is already defined in burn.h, so we don't redefine it
bool bSaveRAM = false;
bool bSaveCROM = false;

// These should be defined in Interface.h
INT32 VID_LEN = 16;  // Max video plugins
int nRendModActive = 0;
bool bHasFocus = true;
int nAppVirtualFps = 6000;

// Define hardfx_config structure directly 
struct hardfx_config { 
    char *szFileName;
    int nOptions;
    float fDefaults[4];
    float fOptions[4];
    char *szOptions[4];
    
    void hardfx_config_load_defaults() {
        for (int i = 0; i < 4; i++) {
            fOptions[i] = fDefaults[i];
        }
    }
};

// CPS fixes
struct hardfx_config HardFXConfigs_Metal[10];

// QSound fixes
INT8* qsound_rom;
UINT32 qsound_rom_size;

// Input/controller stubs
UINT32 nPrevInput[4] = {0, 0, 0, 0};   // Previous input states for each player
UINT32 nCurrentInput[4] = {0, 0, 0, 0}; // Current input states
INT32 nAnalogAxis[4][2] = {{0,0}, {0,0}, {0,0}, {0,0}}; // Analog axis values
INT32 nFireButtons = 0;
EOF

# Create vid_interface_fixes.cpp
cat > src/burner/metal/fixes/vid_interface_fixes.cpp << 'EOF'
#include "burnint.h"

// Define MAX_HARDCODED_FX if not defined
#ifndef MAX_HARDCODED_FX
#define MAX_HARDCODED_FX 10
#endif

// External variables we need
extern bool bDrvOkay;

// Metal video interface - simplified version with stubs
INT32 VidSelect = 0;
INT32 nVidSelect = 0;
INT32 nVidWidth  = 0;
INT32 nVidHeight = 0;
INT32 nVidRefresh = 0;
INT32 nVidDepth  = 0;
INT32 nVidBlitterOpt[4] = {0, 0, 0, 0};
bool bVidOkay = false;

// More video variables
INT32 nVidFullscreen = 0;
INT32 bVidBilinear = 0;
INT32 bVidScanlines = 0;
INT32 bVidScanRotate = 0;
INT32 bVidScanBilinear = 0;
INT32 nVidScanIntensity = 0;
INT32 bVidScanHalf = 0;
INT32 bVidScanDelay = 0;
INT32 nVidFeedbackIntensity = 0;
INT32 nVidFeedbackOverSaturation = 0;
INT32 bVidCorrectAspect = 0;
INT32 bVidArcaderes = 0;
INT32 nVidDX9HardFX = 0;
INT32 bVidArcaderesHor = 0;
INT32 bVidArcaderesVer = 0;
INT32 nVidRotationAdjust = 0;
INT32 bVidForceFlip = 0;
INT32 nVidTransferMethod = 0;
INT32 bVidFullStretch = 0;
INT32 bVidTripleBuffer = 0;
INT32 bVidVSync = 0;

// Define hardfx_config structure directly to avoid dependency on interface.h
struct hardfx_config { 
    char *szFileName;
    int nOptions;
    float fDefaults[4];
    float fOptions[4];
    char *szOptions[4];
    
    void hardfx_config_load_defaults() {
        for (int i = 0; i < 4; i++) {
            fOptions[i] = fDefaults[i];
        }
    }
};

// Define hardfx_config array for Metal builds
struct hardfx_config HardFXConfigs[MAX_HARDCODED_FX];

// Define a rectangle structure 
typedef struct {
    int left;
    int top;
    int right;
    int bottom;
} RECT;

// Interface info structure
struct InterfaceInfo {
    const TCHAR* pszModuleName;
    TCHAR** ppszInterfaceSettings;
    TCHAR** ppszModuleSettings;
};

// Interface functions
INT32 IntInfoFree(InterfaceInfo* pInfo) { return 0; }
INT32 IntInfoInit(InterfaceInfo* pInfo) { 
    if (pInfo) {
        pInfo->pszModuleName = NULL;
        pInfo->ppszInterfaceSettings = NULL;
        pInfo->ppszModuleSettings = NULL;
    }
    return 0; 
}
INT32 IntInfoAddStringInterface(InterfaceInfo* pInfo, TCHAR* szString) { return 0; }
INT32 IntInfoAddStringModule(InterfaceInfo* pInfo, TCHAR* szString) { return 0; }

// Video core functions (stubs)
INT32 VidInit() { 
    bVidOkay = true;
    return 0; 
}

INT32 VidExit() { 
    bVidOkay = false;
    return 0; 
}

INT32 VidRedraw() { return 0; }

INT32 VidFrame() { return 0; }

INT32 VidPaint(INT32 bValidate) { return 0; }

// Additional Metal specific stubs
INT32 VidRecalcPal() { return 0; }

INT32 VidReInitialise() { return 0; }

const TCHAR* VidGetModuleName() { return _T("Metal"); }

// High color conversion
UINT32 VidHighCol(INT32 r, INT32 g, INT32 b, INT32 i) {
    return (r << 16) | (g << 8) | b;
}

// Screen image size calculation
INT32 VidImageSize(RECT* pRect, INT32 nGameWidth, INT32 nGameHeight) {
    if (pRect) {
        pRect->left = 0;
        pRect->top = 0;
        pRect->right = nGameWidth;
        pRect->bottom = nGameHeight;
    }
    return 0;
}

// Add status display functions (stubs)
INT32 VidSNewTinyMsg(const TCHAR* pText, INT32 nRGB, INT32 nDuration, INT32 nPriority) { return 0; }
INT32 VidSNewJoystickMsg(const TCHAR* pText, INT32 nRGB, INT32 nDuration, INT32 nLineNo) { return 0; }
INT32 VidSNewShortMsg(const TCHAR* pText, INT32 nRGB, INT32 nDuration, INT32 nPriority) { return 0; }
void VidSKillShortMsg() { }
void VidSKillTinyMsg() { }
INT32 VidSAddChatMsg(const TCHAR* pID, INT32 nIDRGB, const TCHAR* pMain, INT32 nMainRGB) { return 0; }

// InterfaceInfo for Metal video driver
InterfaceInfo* VidGetInfo() {
    static InterfaceInfo vidInfo;
    
    // Initialize if needed
    if (vidInfo.pszModuleName == NULL) {
        IntInfoInit(&vidInfo);
        vidInfo.pszModuleName = _T("FBNeo Metal Renderer");
        
        // Add settings
        IntInfoAddStringModule(&vidInfo, _T("Metal renderer"));
        IntInfoAddStringModule(&vidInfo, _T("Hardware accelerated Metal based renderer"));
    }
    
    return &vidInfo;
}
EOF

# Create rom_fixes.cpp with stubs
cat > src/burner/metal/fixes/rom_fixes.cpp << 'EOF'
#include "burnint.h"

// ROM management stubs for Metal minimal build

// Rom scan and support
UINT32 nBurnDrvSelect[8] = {~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U};

// Get the title of the driver
char* BurnDrvGetTextA(unsigned int nIndex) {
    static char szText[256] = "";
    return szText;
}

// Get the title of the driver (UNICODE)
TCHAR* BurnDrvGetText(unsigned int nIndex) {
    static TCHAR szText[256] = _T("");
    return szText;
}

// Return true if the driver is working
bool BurnDrvIsWorking() {
    return true;
}

// Return the date / year of the driver
char* BurnDrvGetReleaseDateA() {
    return "2023";
}

// Return the date / year of the driver (UNICODE)
TCHAR* BurnDrvGetReleaseDate() {
    return _T("2023");
}

// Return the manufacturer of the driver
char* BurnDrvGetManufacturerA() {
    return "CAPCOM";
}

// Return the manufacturer of the driver (UNICODE)
TCHAR* BurnDrvGetManufacturer() {
    return _T("CAPCOM");
}

// Return the flags for the driver
int BurnDrvGetFlags() {
    return 0;
}

// Init game emulation (loading ROMs, etc)
int BurnDrvInit() {
    return 0;
}

// Exit game emulation
int BurnDrvExit() {
    return 0;
}

// Do one frame of emulation
int BurnDrvFrame() {
    return 0;
}

// Refresh our palette
int BurnDrvRedraw() {
    return 0;
}
EOF

# Create interface fixes
cat > src/burner/metal/fixes/interface_fixes.cpp << 'EOF'
#include "burnint.h"

// Interface stubs for Metal minimal build
UINT32 nInputSelect = 0;
int ArcadeJoystick = 0;
bool bInputOkay = false;

// Audio related
UINT32 nAudSampleRate[8] = {48000, 48000, 48000, 48000, 48000, 48000, 48000, 48000};
INT32 nAudVolume = 10000;
INT32 nAudSegCount = 6;
INT32 nAudSegLen = 0;
INT32 nAudAllocSegLen = 0;
INT16 *nAudNextSound = NULL;
UINT8 bAudOkay = 1;
UINT8 bAudPlaying = 0;
INT32 nAudDSPModule[8] = {0, 0, 0, 0, 0, 0, 0, 0};
UINT32 nAudSelect = 0;
int nVidScrnAspectX = 4;
int nVidScrnAspectY = 3;

// Get analog input for a particular input
INT32 InputGetAxis(INT32 nJoy, INT32 nAxis) {
    return 0;
}

// Get digital input for a particular input
bool InputGetState(INT32 nJoy) {
    return false;
}

// Initialize input system
INT32 InputInit() {
    return 0;
}

// Exit input system
INT32 InputExit() {
    return 0;
}

// Set cooperative level for input
INT32 InputSetCooperativeLevel(bool bExclusive, bool bForeground) {
    return 0;
}

// Get digital input for a particular switch
INT32 InputState(INT32 nCode) {
    return 0;
}

// Initialize audio system
INT32 AudSoundInit() {
    return 0;
}

// Exit audio system
INT32 AudSoundExit() {
    return 0;
}

// Play audio
INT32 AudSoundPlay() {
    return 0;
}

// Stop audio
INT32 AudSoundStop() {
    return 0;
}

// Set audio volume
INT32 AudSoundSetVolume() {
    return 0;
}

// Audio callback
INT32 AudCallback(INT32 nSeg) {
    return 0;
}

// Blank sound
INT32 AudBlankSound() {
    return 0;
}

// Audio check
INT32 AudSoundCheck() {
    return 0;
}

// Audio chip interfaces
int SekInit(int nCount, int nCPUType);
int SekExit();
void SekNewFrame();
int SekReset();
int SekOpen(const int i);
int SekClose();
INT32 SekGetPC(INT32 n);
int SekMemory(unsigned int nStart, unsigned int nEnd, unsigned int nType);
INT32 SekIdle(INT32 nCycles);
int SekSegmentRead(const int i, const unsigned int nStart, const unsigned int nCount, unsigned char* pDest);
int SekSegmentWrite(const int i, const unsigned int nStart, const unsigned int nCount, unsigned char* pSrc);

// Z80 core interface
int ZetInit(int nCPU);
int ZetExit();
void ZetNewFrame();
int ZetOpen(int nCPU);
int ZetClose();
int ZetReset();
int ZetPc(int n);
int ZetBc(int n);
int ZetDe(int n);
int ZetHL(int n);
int ZetScan(int nAction);
int ZetIdle(int nCycles);
int ZetSegmentAccess(int nStart, int nEnd);
int ZetMemCallback(int nStart, int nEnd, int nMode);
int ZetMemory(unsigned char *Mem, int nStart, int nEnd, int nType);
EOF

# Create qsound fixes
cat > src/burner/metal/fixes/qsound_fixes.cpp << 'EOF'
#include "burnint.h"

// QSound stubs
void QscInit(int nRateHz) {}
void QscExit() {}
void QscReset() {}
void QscNewFrame() {}
void QscWrite(int nAddress, int nValue) {}
void QscPlayQsndSample(signed short* pSoundBuf, int nSample) {}
void QscScan(int nAction) {}
void QscSetRoute(int nIndex, double nVolume, int nRouteDir) {}
EOF

# Create M68K fixes
cat > src/burner/metal/fixes/m68k_fixes.cpp << 'EOF'
#include "burnint.h"

extern "C" {
    
// Define stub M68K functions to allow compilation
int nSekCount;
int nSekActive;

// Implements the basic functions needed from Musashi
int SekInit(int nCount, int nCPUType) { return 0; }
int SekExit() { return 0; }
void SekNewFrame() {}
int SekReset() { return 0; }
int SekOpen(const int i) { return 0; }
int SekClose() { return 0; }
INT32 SekGetPC(INT32 n) { return 0; }
int SekMemory(unsigned int nStart, unsigned int nEnd, unsigned int nType) { return 0; }
INT32 SekIdle(INT32 nCycles) { return 0; }
int SekSegmentRead(const int i, const unsigned int nStart, const unsigned int nCount, unsigned char* pDest) { return 0; }
int SekSegmentWrite(const int i, const unsigned int nStart, const unsigned int nCount, unsigned char* pSrc) { return 0; }

} // extern "C"
EOF

# Create YM2203 fixes
cat > src/burner/metal/fixes/ym2203_fixes.cpp << 'EOF'
#include "burnint.h"

// Define a callback type for FM interrupt handling
typedef void (*IrqHandlerType)(INT32, INT32);

// YM2203 interface stubs
void BurnYM2203Update(INT16* pSoundBuf, INT32 nSegmentEnd) {}
void BurnYM2203Reset() {}
void BurnYM2203Exit() {}
INT32 BurnYM2203Init(INT32 num, INT32 nClockFrequency, IrqHandlerType IRQCallback, INT32 (*StreamCallback)(INT32), double RefreshRate, INT32 bAddSignal) { return 0; }
void BurnYM2203SetRoute(INT32 nChip, INT32 nIndex, double nVolume, INT32 nRouteDir) {}
void BurnYM2203Scan(INT32 nAction, INT32* pnMin) {}
void BurnYM2203Write(INT32 i, UINT8 nReg, UINT8 nValue) {}
UINT8 BurnYM2203Read(INT32 i, INT32 nReg) { return 0; }
void BurnTimerEndFrame(INT32 nCycles) {}
void BurnTimerUpdate(INT32 nCycles) {}
void BurnTimerUpdateEnd() {}
void BurnTimerUpdateYM3526(INT32 nCycles) {}
void BurnTimerUpdateYM3812(INT32 nCycles) {}
void BurnTimerUpdateYM2203(INT32 nCycles) {}
void BurnTimerUpdateYM2413(INT32 nCycles) {}
void BurnTimerUpdateYM2608(INT32 nCycles) {}
void BurnTimerUpdateYM2610(INT32 nCycles) {}
void BurnTimerUpdateYM2612(INT32 nCycles) {}
void BurnTimerUpdateYMF2612(INT32 nCycles) {}
void BurnTimerUpdateYMF278B(INT32 nCycles) {}
INT32 BurnTimerAttachSek(INT32 nClockspeed) { return 0; }
INT32 BurnTimerAttachZet(INT32 nClockspeed) { return 0; }
INT32 BurnTimerAttachM6809(INT32 nClockspeed) { return 0; }
INT32 BurnTimerAttachHD6309(INT32 nClockspeed) { return 0; }
INT32 BurnTimerAttachM6800(INT32 nClockspeed) { return 0; }
INT32 BurnTimerAttachHD63701(INT32 nClockspeed) { return 0; }
INT32 BurnTimerAttachM6803(INT32 nClockspeed) { return 0; }
INT32 BurnTimerAttachM6502(INT32 nClockspeed) { return 0; }
INT32 BurnTimerAttachH6280(INT32 nClockspeed) { return 0; }
INT32 BurnTimerAttachSh2(INT32 nClockspeed) { return 0; }
INT32 BurnTimerAttachI8039(INT32 nClockspeed) { return 0; }
void BurnTimerScan(INT32 nAction, INT32* pnMin) {}
void BurnTimerReset() {}
void BurnTimerExit() {}
INT32 BurnTimerInit(INT32 (*pOverCallback)(INT32, INT32), INT32 (*pZeroCallback)(INT32)) { return 0; }
UINT8 BurnYM2151ReadStatus() { return 0; }
void BurnYM2151SetAllRoutes(double nVolume, INT32 nRouteDir) {}
void BurnYM2151Scan(INT32 nAction, INT32 *pnMin) {}
void BurnYM2151Reset() {}
void BurnYM2151Exit() {}
void BurnYM2151Write(UINT32 nAddress, UINT8 nData) {}
INT32 BurnYM2151Init(INT32 nClockFrequency) { return 0; }
void BurnYM2151SetRoute(INT32 nIndex, double nVolume, INT32 nRouteDir) {}
void BurnYM2151Render(INT16* pSoundBuf, INT32 nSegmentLength) {}
EOF

# Create CPS fixes
cat > src/burner/metal/fixes/cps_fixes.cpp << 'EOF'
#include "burnint.h"

// CPS1/2 stub variables and functions
unsigned char *CpsGfx = NULL;
unsigned int nCpsGfxLen = 0;
unsigned char *CpsRom = NULL;
unsigned int nCpsRomLen = 0;
unsigned char *CpsCode = NULL;
unsigned int nCpsCodeLen = 0;
unsigned char *CpsZRom = NULL;
unsigned int nCpsZRomLen = 0;
unsigned char *CpsQSamples = NULL;
unsigned int nCpsQSamplesLen = 0;
unsigned char *CpsStar = NULL;
unsigned int nCpsStarLen = 0;

// CPS1 & CPS2 sprites
struct CpsSprite {
	short x, y;
	unsigned short n;
	unsigned char attr;
	unsigned char flip;
	unsigned char pal;
};

// Initialize CPS sprite list
void CpsInitSpriteLists() {}

// Transfer one sprite from the CPS sprite list
void CpsObjGetActive() {}

// Transfer CPS sprites to the screen
void CpsObjDrawInit() {}

// Draw a sprite to the screen
void CpsObjDraw(CpsSprite* pSpr, int nFlag) {}

// Finish drawing sprites
void CpsObjDrawShutdown() {}

// Initialize sprite lists for CPS
int CpsObjInit() { return 0; }

// Exit sprite lists for CPS
int CpsObjExit() { return 0; }
EOF

# Compile everything
echo "Compiling all fix files..."
clang++ -O2 -fomit-frame-pointer -Wno-write-strings -std=c++17 -DUSE_METAL -DLSB_FIRST -DTCHAR_IS_CHAR -DUSE_AI \
    -I./src/dep/generated -I./src/burn -I./src/burn/devices -I./src/burn/sound -I./src/burn/snd \
    -I./src/cpu -I./src/cpu/m68k -I./src/cpu/z80 -I./src/intf -I./src/intf/video -I./src/intf/audio \
    -I./src/burner -I./src/burner/metal -I./src/burner/metal/fixes -I./src/ai \
    -c src/burner/metal/fixes/burn_fixes.cpp -o obj/metal/burner/metal/fixes/burn_fixes.o

clang++ -O2 -fomit-frame-pointer -Wno-write-strings -std=c++17 -DUSE_METAL -DLSB_FIRST -DTCHAR_IS_CHAR -DUSE_AI \
    -I./src/dep/generated -I./src/burn -I./src/burn/devices -I./src/burn/sound -I./src/burn/snd \
    -I./src/cpu -I./src/cpu/m68k -I./src/cpu/z80 -I./src/intf -I./src/intf/video -I./src/intf/audio \
    -I./src/burner -I./src/burner/metal -I./src/burner/metal/fixes -I./src/ai \
    -c src/burner/metal/fixes/rom_fixes.cpp -o obj/metal/burner/metal/fixes/rom_fixes.o

clang++ -O2 -fomit-frame-pointer -Wno-write-strings -std=c++17 -DUSE_METAL -DLSB_FIRST -DTCHAR_IS_CHAR -DUSE_AI \
    -I./src/dep/generated -I./src/burn -I./src/burn/devices -I./src/burn/sound -I./src/burn/snd \
    -I./src/cpu -I./src/cpu/m68k -I./src/cpu/z80 -I./src/intf -I./src/intf/video -I./src/intf/audio \
    -I./src/burner -I./src/burner/metal -I./src/burner/metal/fixes -I./src/ai \
    -c src/burner/metal/fixes/qsound_fixes.cpp -o obj/metal/burner/metal/fixes/qsound_fixes.o

clang++ -O2 -fomit-frame-pointer -Wno-write-strings -std=c++17 -DUSE_METAL -DLSB_FIRST -DTCHAR_IS_CHAR -DUSE_AI \
    -I./src/dep/generated -I./src/burn -I./src/burn/devices -I./src/burn/sound -I./src/burn/snd \
    -I./src/cpu -I./src/cpu/m68k -I./src/cpu/z80 -I./src/intf -I./src/intf/video -I./src/intf/audio \
    -I./src/burner -I./src/burner/metal -I./src/burner/metal/fixes -I./src/ai \
    -c src/burner/metal/fixes/m68k_fixes.cpp -o obj/metal/burner/metal/fixes/m68k_fixes.o

clang++ -O2 -fomit-frame-pointer -Wno-write-strings -std=c++17 -DUSE_METAL -DLSB_FIRST -DTCHAR_IS_CHAR -DUSE_AI \
    -I./src/dep/generated -I./src/burn -I./src/burn/devices -I./src/burn/sound -I./src/burn/snd \
    -I./src/cpu -I./src/cpu/m68k -I./src/cpu/z80 -I./src/intf -I./src/intf/video -I./src/intf/audio \
    -I./src/burner -I./src/burner/metal -I./src/burner/metal/fixes -I./src/ai \
    -c src/burner/metal/fixes/cps_fixes.cpp -o obj/metal/burner/metal/fixes/cps_fixes.o

clang++ -O2 -fomit-frame-pointer -Wno-write-strings -std=c++17 -DUSE_METAL -DLSB_FIRST -DTCHAR_IS_CHAR -DUSE_AI \
    -I./src/dep/generated -I./src/burn -I./src/burn/devices -I./src/burn/sound -I./src/burn/snd \
    -I./src/cpu -I./src/cpu/m68k -I./src/cpu/z80 -I./src/intf -I./src/intf/video -I./src/intf/audio \
    -I./src/burner -I./src/burner/metal -I./src/burner/metal/fixes -I./src/ai \
    -c src/burner/metal/fixes/ym2203_fixes.cpp -o obj/metal/burner/metal/fixes/ym2203_fixes.o

clang++ -O2 -fomit-frame-pointer -Wno-write-strings -std=c++17 -DUSE_METAL -DLSB_FIRST -DTCHAR_IS_CHAR -DUSE_AI \
    -I./src/dep/generated -I./src/burn -I./src/burn/devices -I./src/burn/sound -I./src/burn/snd \
    -I./src/cpu -I./src/cpu/m68k -I./src/cpu/z80 -I./src/intf -I./src/intf/video -I./src/intf/audio \
    -I./src/burner -I./src/burner/metal -I./src/burner/metal/fixes -I./src/ai \
    -c src/burner/metal/fixes/interface_fixes.cpp -o obj/metal/burner/metal/fixes/interface_fixes.o

clang++ -O2 -fomit-frame-pointer -Wno-write-strings -std=c++17 -DUSE_METAL -DLSB_FIRST -DTCHAR_IS_CHAR -DUSE_AI \
    -I./src/dep/generated -I./src/burn -I./src/burn/devices -I./src/burn/sound -I./src/burn/snd \
    -I./src/cpu -I./src/cpu/m68k -I./src/cpu/z80 -I./src/intf -I./src/intf/video -I./src/intf/audio \
    -I./src/burner -I./src/burner/metal -I./src/burner/metal/fixes -I./src/ai \
    -c src/burner/metal/fixes/vid_interface_fixes.cpp -o obj/metal/burner/metal/fixes/vid_interface_fixes.o

# Compile AI components
echo "Compiling AI components..."
clang++ -O2 -fomit-frame-pointer -Wno-write-strings -std=c++17 -DUSE_METAL -DLSB_FIRST -DTCHAR_IS_CHAR -DUSE_AI \
    -I./src/dep/generated -I./src/burn -I./src/burn/devices -I./src/burn/sound -I./src/burn/snd \
    -I./src/cpu -I./src/cpu/m68k -I./src/cpu/z80 -I./src/intf -I./src/intf/video -I./src/intf/audio \
    -I./src/burner -I./src/burner/metal -I./src/burner/metal/fixes -I./src/ai \
    -c src/ai/ai_module.cpp -o obj/metal/ai/ai_module.o

clang++ -O2 -fomit-frame-pointer -Wno-write-strings -std=c++17 -DUSE_METAL -DLSB_FIRST -DTCHAR_IS_CHAR -DUSE_AI \
    -I./src/dep/generated -I./src/burn -I./src/burn/devices -I./src/burn/sound -I./src/burn/snd \
    -I./src/cpu -I./src/cpu/m68k -I./src/cpu/z80 -I./src/intf -I./src/intf/video -I./src/intf/audio \
    -I./src/burner -I./src/burner/metal -I./src/burner/metal/fixes -I./src/ai \
    -c src/ai/ai_input_frame.cpp -o obj/metal/ai/ai_input_frame.o

clang++ -O2 -fomit-frame-pointer -Wno-write-strings -std=c++17 -DUSE_METAL -DLSB_FIRST -DTCHAR_IS_CHAR -DUSE_AI \
    -I./src/dep/generated -I./src/burn -I./src/burn/devices -I./src/burn/sound -I./src/burn/snd \
    -I./src/cpu -I./src/cpu/m68k -I./src/cpu/z80 -I./src/intf -I./src/intf/video -I./src/intf/audio \
    -I./src/burner -I./src/burner/metal -I./src/burner/metal/fixes -I./src/ai \
    -c src/ai/ai_memory_mapping.cpp -o obj/metal/ai/ai_memory_mapping.o

clang++ -O2 -fomit-frame-pointer -Wno-write-strings -std=c++17 -DUSE_METAL -DLSB_FIRST -DTCHAR_IS_CHAR -DUSE_AI \
    -I./src/dep/generated -I./src/burn -I./src/burn/devices -I./src/burn/sound -I./src/burn/snd \
    -I./src/cpu -I./src/cpu/m68k -I./src/cpu/z80 -I./src/intf -I./src/intf/video -I./src/intf/audio \
    -I./src/burner -I./src/burner/metal -I./src/burner/metal/fixes -I./src/ai \
    -c src/ai/ai_output_action.cpp -o obj/metal/ai/ai_output_action.o

clang++ -O2 -fomit-frame-pointer -Wno-write-strings -std=c++17 -DUSE_METAL -DLSB_FIRST -DTCHAR_IS_CHAR -DUSE_AI \
    -I./src/dep/generated -I./src/burn -I./src/burn/devices -I./src/burn/sound -I./src/burn/snd \
    -I./src/cpu -I./src/cpu/m68k -I./src/cpu/z80 -I./src/intf -I./src/intf/video -I./src/intf/audio \
    -I./src/burner -I./src/burner/metal -I./src/burner/metal/fixes -I./src/ai \
    -c src/ai/combo_classifier.cpp -o obj/metal/ai/combo_classifier.o

clang++ -O2 -fomit-frame-pointer -Wno-write-strings -std=c++17 -DUSE_METAL -DLSB_FIRST -DTCHAR_IS_CHAR -DUSE_AI \
    -I./src/dep/generated -I./src/burn -I./src/burn/devices -I./src/burn/sound -I./src/burn/snd \
    -I./src/cpu -I./src/cpu/m68k -I./src/cpu/z80 -I./src/intf -I./src/intf/video -I./src/intf/audio \
    -I./src/burner -I./src/burner/metal -I./src/burner/metal/fixes -I./src/ai \
    -c src/ai/ai_torch_policy.cpp -o obj/metal/ai/ai_torch_policy.o

clang++ -O2 -fomit-frame-pointer -Wno-write-strings -std=c++17 -DUSE_METAL -DLSB_FIRST -DTCHAR_IS_CHAR -DUSE_AI \
    -I./src/dep/generated -I./src/burn -I./src/burn/devices -I./src/burn/sound -I./src/burn/snd \
    -I./src/cpu -I./src/cpu/m68k -I./src/cpu/z80 -I./src/intf -I./src/intf/video -I./src/intf/audio \
    -I./src/burner -I./src/burner/metal -I./src/burner/metal/fixes -I./src/ai \
    -c src/ai/ai_torch_policy_model.cpp -o obj/metal/ai/ai_torch_policy_model.o

# Compile Objective-C++ files with Metal support
clang++ -O2 -fomit-frame-pointer -Wno-write-strings -std=c++17 -DUSE_METAL -DLSB_FIRST -DTCHAR_IS_CHAR -DUSE_AI -fobjc-arc -x objective-c++ \
    -I./src/dep/generated -I./src/burn -I./src/burn/devices -I./src/burn/sound -I./src/burn/snd \
    -I./src/cpu -I./src/cpu/m68k -I./src/cpu/z80 -I./src/intf -I./src/intf/video -I./src/intf/audio \
    -I./src/burner -I./src/burner/metal -I./src/burner/metal/fixes -I./src/ai \
    -c src/ai/metal_debug_overlay.mm -o obj/metal/ai/metal_debug_overlay.o

# Link final executable
echo "Linking final executable..."
clang++ -o fbneo_metal_minimal obj/metal/burner/metal/fixes/burn_fixes.o \
    obj/metal/burner/metal/fixes/interface_fixes.o \
    obj/metal/burner/metal/fixes/rom_fixes.o \
    obj/metal/burner/metal/fixes/m68k_fixes.o \
    obj/metal/burner/metal/fixes/qsound_fixes.o \
    obj/metal/burner/metal/fixes/cps_fixes.o \
    obj/metal/burner/metal/fixes/ym2203_fixes.o \
    obj/metal/burner/metal/fixes/vid_interface_fixes.o \
    obj/metal/burner/metal/metal_minimal.o \
    obj/metal/ai/ai_module.o \
    obj/metal/ai/ai_input_frame.o \
    obj/metal/ai/ai_memory_mapping.o \
    obj/metal/ai/ai_output_action.o \
    obj/metal/ai/combo_classifier.o \
    obj/metal/ai/ai_torch_policy.o \
    obj/metal/ai/ai_torch_policy_model.o \
    obj/metal/ai/metal_debug_overlay.o \
    -framework Metal -framework MetalKit -framework Cocoa -framework CoreGraphics -framework QuartzCore -framework CoreVideo \
    -framework CoreML -framework Vision \
    -lm -lpthread

# Check if build was successful
if [ -f fbneo_metal_minimal ]; then
    echo "Build completed successfully! Executable created: fbneo_metal_minimal"
    # Run the minimal executable
    ./fbneo_metal_minimal
else
    echo "Build failed!"
fi 