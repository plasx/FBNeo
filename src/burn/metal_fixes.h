#ifndef METAL_FIXES_H_INCLUDED
#define METAL_FIXES_H_INCLUDED

#include "metal_common.h"

// Always include Z80 types - needed for build compatibility

// Network/recording functions
int is_netgame_or_recording();

// Input interface
extern int nInputIntfMouseDivider;

// Timer function types
typedef double (*BurnTimerTotalCycles)();
typedef void (*BurnTimerRun)(INT32 nCycles);
typedef void (*BurnTimerOverCallback)();
typedef double (*BurnTimerTimeCallback)();

// Global timer function pointers - match timer.h signatures
extern BurnTimerRun BurnTimerCPURun;
extern BurnTimerOverCallback BurnTimerCPUOver;
extern BurnTimerTimeCallback BurnTimerCPUTime;

// Dummy functions - use consistent signatures
UINT32 dummy_total_cycles(UINT32 cycles);
void dummy_newframe(UINT32 cycles, UINT32 cycles_per_frame);
void dummy_idle(INT32 cycles);  // Fixed: use void return type
double dummy_time();
void dummy_time(INT32 a, INT32 b);

// Dummy CPU functions for cheat system
extern double dummy_BurnCpuGetTotalCycles();
extern UINT32 dummy_BurnCpuGetNextIRQLine();
extern void dummy_open(INT32 nCPU);
extern void dummy_close();
extern UINT32 dummy_read(UINT32 address);
extern void dummy_write(UINT32 address, UINT32 data);
extern double dummy_totalcycles();
extern void dummy_run(INT32 nCycles);
extern void dummy_runend();
extern int dummy_active();
extern double dummy_time();

// Dummy configuration
extern struct cpu_core_config dummy_config;

// Timer-related globals
extern void (*pCPURun)(INT32);
extern void (*pTimerTimeCallback)();

// Debug flags
extern int Debug_GenericTilesInitted;
extern int Debug_BurnTransferInitted;

// Add missing sound debug variables
extern int DebugSnd_K053260Initted;
extern int DebugSnd_NamcoSndInitted; 
extern int DebugSnd_K054539Initted;
extern int DebugSnd_SamplesInitted;
extern int DebugSnd_DACInitted;
extern int DebugSnd_YMZ280BInitted;
extern int DebugSnd_MSM6295Initted;
extern int DebugSnd_SegaPCMInitted;
extern int DebugSnd_SAA1099Initted;
extern int DebugSnd_SN76496Initted;
extern int DebugSnd_VLM5030Initted;
extern int DebugSnd_UPD7759Initted;
extern int DebugSnd_MSM5232Initted;
extern int DebugSnd_NESAPUSndInitted;
extern int DebugSnd_X1010Initted;
extern int DebugSnd_RF5C68Initted;

// Constants for tilemap
#define MAX_GFX 8
#define MAX_TILEMAPS 8
#define TMAP_GLOBAL -1
#define TMAP_TRANSPARENT 1

// CPS2 constants
#define MAX_RASTER 16

// Structures for tilemap system
struct GenericTilemapCallbackStruct {
    INT32 nTileNumber;
    INT32 nTilePalette;
    INT32 nFlipx;
    INT32 nFlipy;
    INT32 nCategory;
    UINT32 nPaletteOffset;
    UINT16 *pTile;
};

struct GenericTilesGfx {
    UINT8 *gfxbase;
    INT32 depth;
    INT32 width;
    INT32 height;
    INT32 gfx_len;
    INT32 code_mask;
    UINT32 color_offset;
    UINT32 color_mask;
};

// Additional tilemap constants
#define TMAP_FLIPX 1
#define TMAP_FLIPY 2
#define TMAP_TRANSMASK 8
#define TMAP_TRANSSPLIT 16

extern GenericTilesGfx GenericGfxData[MAX_TILEMAPS];

// Missing function declarations - use consistent signatures with burn.h
void BurnTransferSetDimensions(INT32 width, INT32 height);
void BurnTransferRealloc();
void PutPix(void* pDst, UINT32 c);
void BurnBitmapExit();
void BurnBitmapAllocate(INT32 num, INT32 width, INT32 height, bool allocPrio);
UINT16* BurnBitmapGetBitmap(INT32 num);
UINT8* BurnBitmapGetPriomap(INT32 num);
void GenericTilemapExit();
void GenericTilemapSetGfx(INT32 nGfx, UINT8 *pGfx, INT32 nDepth, INT32 nTileWidth, INT32 nTileHeight, INT32 nGfxLen, UINT32 nColorOffset, UINT32 nColorMask);

// Sample functions
#ifdef __cplusplus
extern "C" {
#endif
void BurnSampleExit();
#ifdef __cplusplus
}
#endif

// CPS2 core variables
extern INT32 Cps2Turbo;
extern INT32 Cps2DisableQSnd;
extern INT32 Cps1DisablePSnd;
extern INT32 Cps1Qs;
extern INT32 PangEEP;
extern INT32 CpsBootlegEEPROM;
extern INT32 nCpsObjectBank;
extern INT32 nCpsScreenWidth;
extern INT32 nCpsScreenHeight;
extern INT32 nCpsGlobalXOffset;
extern INT32 nCpsGlobalYOffset;
extern INT32 nCpsGfxScroll[4];
extern INT32 nCpsCyclesExtra;
extern INT32 nCpsZ80Cycles;
extern INT32 nCpsPalCtrlReg;

// CPS2 memory pointers
extern UINT8* CpsRam708;
extern UINT8* CpsFrg;
extern UINT8* CpsSaveReg[MAX_RASTER + 1];
extern UINT8* CpsSaveFrg[MAX_RASTER + 1];
extern UINT8* CpsReg;
extern UINT8* CpsEncZRom;
extern UINT16* ZBuf;

// CPS2 screen tile variables
extern INT32 nBgHi;

// CPS2 callback types
typedef void (*CpsMemScanCallback)(INT32 nAction, INT32* pnMin);
typedef INT32 (*Cps1ObjGetCallback)(void);
typedef INT32 (*Cps1ObjDrawCallback)(INT32, INT32);
typedef void (*CpsRunInitCallback)(void);
typedef void (*CpsRunResetCallback)(void);
typedef void (*CpsRunFrameStartCallback)(void);
typedef void (*CpsRunFrameMiddleCallback)(void);
typedef void (*CpsRunFrameEndCallback)(void);

// CPS2 callback instances
extern CpsMemScanCallback CpsMemScanCallbackFunction;
extern CpsRunInitCallback CpsRunInitCallbackFunction;
extern CpsRunInitCallback CpsRunExitCallbackFunction;
extern CpsRunResetCallback CpsRunResetCallbackFunction;
extern CpsRunFrameStartCallback CpsRunFrameStartCallbackFunction;
extern CpsRunFrameMiddleCallback CpsRunFrameMiddleCallbackFunction;
extern CpsRunFrameEndCallback CpsRunFrameEndCallbackFunction;
extern Cps1ObjGetCallback Cps1ObjGetCallbackFunction;
extern Cps1ObjDrawCallback Cps1ObjDrawCallbackFunction;

// Prevent CPS2 driver from defining conflicting typedefs
#ifndef CPS_DRAW_TYPEDEFS_DEFINED
#define CPS_DRAW_TYPEDEFS_DEFINED

// CPS2 drawing function types - match signatures from real CPS2 driver
typedef INT32 (*CpsObjDrawDoFn)(INT32,INT32);
typedef INT32 (*CpsScrXDrawDoFn)(UINT8 *,INT32,INT32);
typedef void (*CpsLayersDoFn)();
typedef INT32 (*CpsrPrepareDoFn)();
typedef INT32 (*CpsrRenderDoFn)();

#endif // CPS_DRAW_TYPEDEFS_DEFINED

// CPS2 drawing function pointers - declared by real CPS2 driver
extern CpsScrXDrawDoFn CpsScr1DrawDoX;
extern CpsScrXDrawDoFn CpsScr3DrawDoX;
extern CpsObjDrawDoFn CpsObjDrawDoX;
extern CpsrPrepareDoFn CpsrPrepareDoX;
extern CpsrRenderDoFn CpsrRenderDoX;

// CPS2 drawing function implementations - match real CPS2 driver signatures  
INT32 Cps1Scr1Draw(UINT8* base, INT32 sx, INT32 sy);
INT32 Cps1Scr3Draw(UINT8* base, INT32 sx, INT32 sy);
INT32 Cps1ObjDraw(INT32 nLevelFrom, INT32 nLevelTo);
INT32 Cps1rPrepare();
INT32 Cps1rRender();
INT32 Cps2Scr1Draw(UINT8* base, INT32 sx, INT32 sy);
INT32 Cps2Scr3Draw(UINT8* base, INT32 sx, INT32 sy);
INT32 Cps2ObjDraw(INT32 nLevelFrom, INT32 nLevelTo);
INT32 Cps2rPrepare();
INT32 Cps2rRender();

// CPS2 utility functions
void cps2_decrypt_game_data();
UINT8* CpsFindGfxRam(UINT32 nAddress, UINT32 nSize);

// CPS2 mapper enums
typedef enum {
    mapper_TKSGZB,
    mapper_QD63B,
    mapper_TN2292,
    mapper_RCM63B,
    mapper_PKB10B,
    mapper_pang3,
    mapper_CP1B1F,
    mapper_CP1B1F_boot,
    mapper_pokon,
    mapper_gulun,
    mapper_sfzch,
    mapper_cps2,
    mapper_frog,
    mapper_KNM10B,
    mapper_pang3b4
} CpsMapperType;

// Additional CPS2 variables that were missing
extern INT32 CpsDrawSpritesInReverse;
extern INT32 nCpsBlend;
extern INT32 nCpsCycles;
extern INT32 nCPS68KClockspeed;
extern INT32 nRasterline[MAX_RASTER + 2];

// Stub function declarations (no definitions here)
INT32 CpsObjInit();
INT32 CpsObjExit();
INT32 CpsObjGet();
INT32 PsndInit();
void PsndExit();
void PsndNewFrame();
void PsndScan(INT32 nAction, INT32* pnMin);

// CPS2 ROM type constants (with correct values from cps.h)
#ifndef CPS2_PRG_68K
#define CPS2_PRG_68K  0x01
#endif
#ifndef CPS2_GFX
#define CPS2_GFX      0x02
#endif
#ifndef CPS2_PRG_Z80
#define CPS2_PRG_Z80  0x03
#endif
#ifndef CPS2_QSND
#define CPS2_QSND     0x04
#endif
#ifndef CPS2_ENCRYPTION_KEY
#define CPS2_ENCRYPTION_KEY 0x05
#endif
#ifndef CPS2_PRG_68K_SIMM
#define CPS2_PRG_68K_SIMM 0x06
#endif
#ifndef CPS2_PRG_68K_XOR_TABLE
#define CPS2_PRG_68K_XOR_TABLE 0x07
#endif
#ifndef CPS2_GFX_SIMM
#define CPS2_GFX_SIMM 0x08
#endif
#ifndef CPS2_GFX_SPLIT4
#define CPS2_GFX_SPLIT4 0x09
#endif
#ifndef CPS2_GFX_SPLIT8
#define CPS2_GFX_SPLIT8 0x0A
#endif
#ifndef CPS2_GFX_19XXJ
#define CPS2_GFX_19XXJ CPS2_GFX
#endif
#ifndef CPS2_QSND_SIMM
#define CPS2_QSND_SIMM 0x0B
#endif
#ifndef CPS2_QSND_SIMM_BYTESWAP
#define CPS2_QSND_SIMM_BYTESWAP 0x0C
#endif

// CPS board constants (with correct values from cps.h)
#ifndef CPS_B_21_DEF
#define CPS_B_21_DEF 13
#endif

// Add missing CPS2 variables that were causing compilation errors
extern INT32 CpsLayEn[6];
extern INT32 nCpsLcReg;
extern INT32 Scroll1TileMask;
extern INT32 Scroll2TileMask; 
extern INT32 Scroll3TileMask;
extern INT32 bCpsUpdatePalEveryFrame;
extern INT32 nCpsNumScanlines;
extern UINT8 AspectDIP;
extern UINT8* CpsRam90;
extern INT32 Cps2VolUp;
extern INT32 Cps2VolDwn;
extern INT32 Cps2Volume;

// CPS2 function declarations
void PsndSyncZ80(INT32 nCycles);

// CPS2 variables to resolve language linkage conflicts
extern UINT16 nCpstPal;
extern INT32 nEndline;

// Palette compatibility - fix type to match real CPS2 driver
extern UINT32* CpstPal;
extern UINT32* CpstPal32;

// Fix callback function signatures to match CPS2 driver expectations
typedef INT32 (*Cps1ObjGetCallback)(void);
typedef INT32 (*Cps1ObjDrawCallback)(INT32, INT32);

// Add back missing CPS2 variables that were accidentally removed
extern UINT16 ZValue;
extern UINT8 CpsReset;
extern UINT8 Cpi01A, Cpi01C, Cpi01E;

// Add missing variables needed by CPS2 driver
extern INT32 nBgHi;
extern INT32 fFakeDip;

// Add missing CPS2 drawing functions
void CpsObjDrawInit();

// CPS2 variables that need to be accessible to all CPS2 driver files
extern INT32 Cps1OverrideLayers;
extern INT32 nCps1Layers[4];
extern INT32 nCps1LayerOffs[3];
extern UINT8 CpsRecalcPal;

// Add missing CPS2 variables and functions needed by real CPS2 driver
extern UINT8* CpsSavePal;
extern INT32 MaskAddr[4];
void CtvReady();

// Add missing CPS2 variables needed by d_cps2.cpp driver
extern INT32 Cps2DisableDigitalVolume;
extern INT32 Pzloop2;
extern INT32 Sfa2ObjHack;
extern INT32 Ssf2tb;
extern INT32 Ssf2t;
extern INT32 Xmcota;
extern INT32 Ecofght;
extern INT32 CpsLayer1XOffs;
extern INT32 CpsLayer2XOffs;
extern INT32 CpsLayer3XOffs;
extern INT32 CpsLayer1YOffs;
extern INT32 CpsLayer2YOffs;
extern INT32 CpsLayer3YOffs;

// Add proper extern "C" declarations for CPS functions needed by compilation
#ifdef __cplusplus
extern "C" {
#endif

void SetCpsBId(int, int);
INT32 CpsRunInit();
INT32 CpsRunExit();
INT32 CpsAreaScan(INT32 nAction, INT32* pnMin);

// Add CpsRamFF declaration inside extern "C" block to avoid language linkage conflicts
extern UINT8* CpsRamFF;

// Add missing CPU interface types for Z80
typedef UINT8 (*Z80ReadIoHandler)(UINT16);
typedef void (*Z80WriteIoHandler)(UINT16, UINT8);
typedef UINT8 (*Z80ReadProgHandler)(UINT16);
typedef void (*Z80WriteProgHandler)(UINT16, UINT8);
typedef UINT8 (*Z80ReadOpHandler)(UINT16);
typedef UINT8 (*Z80ReadOpArgHandler)(UINT16);

// Add missing debug variables
extern INT32 DebugCPU_ZetInitted;
extern INT32 DebugCPU_SekInitted;

// Add missing CPU functions
extern void ZetCPUPush();
extern void ZetCPUPop();
extern UINT8 ZetCheatRead(UINT32);
extern void ZetCheatWriteROM(UINT32, UINT8);
extern void ZetWriteRom(UINT32, UINT8);
extern UINT8 ZetReadByte(UINT32);
extern void Z80StopExecute();

// Add missing M68K CPU functions - fix the return types to match m68000_intf.h
extern void SekCPUPush(INT32 nCPU);
extern void SekCPUPop();
extern UINT8 SekCheatRead(UINT32);
extern void SekCheatWriteROM(UINT32, UINT8);
extern void SekWriteRom(UINT32, UINT8);
extern UINT32 SekReadByte(UINT32);

// Add missing M68K functions for interface
extern void SekWriteByteROM(UINT32, UINT8);
extern INT32 SekGetActive();
extern INT32 SekTotalCycles(INT32);
extern void SekNewFrame();
extern void SekIdle();
extern void SekSetIRQLine(INT32, INT32);
extern INT32 SekRun(const INT32);
extern void SekRunEnd();
extern void SekReset();
extern INT32 SekScan(INT32);
extern void SekExit();

#ifdef __cplusplus
}
#endif

// Sound function aliases for consistency
#define PsndEndFrame QsndEndFrame
void PsmUpdateEnd();

// Add missing Z80 core types and constants
#define MAX_CM_SCRIPTS 64
#define ULA_VARIANT_NONE 0
#define ULA_VARIANT_SINCLAIR 1
#define ULA_VARIANT_AMSTRAD 2
#define RWINFO_READ 1
#define RWINFO_WRITE 2
#define RWINFO_IO_PORT 4
#define RWINFO_MEMORY 8
#define RWINFO_PROCESSED 16
#define CYCLES_EXEC 1
#define CYCLES_ISR 1

typedef struct {
    const char* sinclair;
    const char* amstrad;
} CM_SCRIPT_DESCRIPTION;

typedef struct {
    const char* field_name;
    int offset;
    int bits;
} CM_SCRIPT_BREAKDOWN;

typedef struct {
    int id;
    const char* desc;
    char script[256];
    int length;
    CM_SCRIPT_BREAKDOWN breakdown;
} CM_SCRIPT;

// RW info structure for Z80 memory access tracking
typedef struct {
    UINT16 addr;
    UINT8 val;
    UINT16 flags;
    const char* dbg;
} RWINFO_ENTRY;

typedef struct {
    UINT32 address;
    UINT8 data;
    UINT32 cycles;
    bool do_optional;
    int element;
    bool capturing;
    int uncontended_cycles_predicted;
    int uncontended_cycles_eaten;
    int rw_count;
    CM_SCRIPT* script;
    RWINFO_ENTRY rw[32];  // Array for tracking memory accesses
} OPCODE_HISTORY;

// DIP switch variables
extern UINT8 CpsDipA[8];
extern UINT8 CpsDipB[8];
extern UINT8 CpsDipC[8];

#endif // METAL_FIXES_H_INCLUDED 