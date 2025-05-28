#include <stdint.h>
#include <stdlib.h>
#include "../metal_declarations.h"

// Define MAX_RASTER used for CPS operations
#define MAX_RASTER 16

// Basic CPS system variables for Metal port
// These are simplified versions of the variables used in the main FBNeo core

// CPS2 system variables
INT32 nCpsZ80Cycles = 0;
INT32 nCpsCycles = 0;

// Global variables for graphics handling
UINT8* CpsGfx = NULL;
UINT8* CpsZRam = NULL;
UINT8* CpsQSam = NULL;
UINT8* CpsAd = NULL;
UINT8* CpsRam = NULL;
UINT8* CpsReg = NULL;
UINT8* CpsSaveReg = NULL;
UINT8* CpsSaveFight = NULL;
UINT8* CpsSaveReg2 = NULL;
UINT8* CpsZRamC0 = NULL;
UINT8* CpsZRamF0 = NULL;

// CPS1 variables
INT32 Cps = 0;
INT32 Cps1Qs = 0;
INT32 Cps1Pic = 0;
INT32 nCpsNumScanlines = 224;

// Game configuration flags
INT32 Sf2Hack = 0;
INT32 Sf2tHack = 0;
INT32 Ssf2tHack = 0;
INT32 Forgottn = 0;
INT32 Cps1QsHack = 0;
INT32 Cps2Turbo = 0;

// Rendering flags
INT32 nBurnCPUSpeedAdjust = 0;
INT32 nBgCount = 0;
INT32 nFgCount = 0;
INT32 nSpCount = 0;
UINT8* CpsStar = NULL;

// CPS2 variables
INT32 nCurrentFrame = 0;
INT32 nCpsCurrentFrame = 0;

// Memory map variables
INT32 nCpsRomLen = 0;
INT32 nCpsCodeLen = 0;
INT32 nCpsGfxLen = 0;
INT32 nCpsZRomLen = 0;
INT32 nCpsQSamLen = 0;
INT32 nCpsAdLen = 0;

// Metal port specific variables
INT32 CpsMetal = 1;          // Flag to indicate we're running in Metal mode

// CPS screen variables
INT32 nCpsScreenWidth = 384;
INT32 nCpsScreenHeight = 224;
INT32 nCpsGlobalXOffset = 0;
INT32 nCpsGlobalYOffset = 0;
INT32 nBgHi = 0;

// ROM path for Metal port
char g_szROMPath[260] = {0};

// CPS sprite and tile related variables
UINT32 nCpsGfxMask = 0;
UINT32 nRowScroll[17] = {0};
INT32 nCpsGfxScroll[4] = {0};
UINT8* CpsSaveFrg[MAX_RASTER][0x10] = {{NULL}};
UINT8* MaskAddr[4] = {NULL};
UINT8* CpsZRom = NULL;

// CPS video rendering variables
INT32 nCtvRollX = 0;
INT32 nCtvRollY = 0;
UINT8* pCtvTile = NULL;
INT32 nCtvTileAdd = 0;
UINT8* pCtvLine = NULL;

// CPS type variables
INT32 nCpstType = 0;
INT32 nCpstX = 0;
INT32 nCpstY = 0;
INT32 nCpstTile = 0;
INT32 nCpstFlip = 0;
INT32 CpstRowShift = 0;
UINT16 CpstPmsk = 0;
INT32 nFirstY = 0;
INT32 nLastY = 0;
INT32 nFirstX = 0;
INT32 nLastX = 0;
INT32 nStartline = 0;
INT32 nEndline = 224;

// CPS input variables
UINT8 CpsInp000[0x10] = {0};
UINT8 CpsInp001[0x10] = {0};
UINT8 CpsInp011[0x10] = {0};
UINT8 CpsInp020[0x10] = {0};
UINT8 CpsInp021[0x10] = {0};
UINT8 CpsInp029[0x10] = {0};
UINT8 CpsInp176[0x10] = {0};

// Missing variables needed for CPS driver compilation
INT32 CpsrLineInfo[16][16] = {{0}}; // Raster line info for all scanlines
UINT8* CpsrBase = NULL;            // Pointer to the scan-line table
INT32 nCpsrScrY = 0;                // Offset to current scan-line
UINT8 CpsRaster[MAX_RASTER][0x10] = {{0}}; // Raster RAM buffer

// Stubs for CTV rendering functions
INT32 CtvDoStub() { return 0; }
INT32 (*CtvDoX[32])() = {CtvDoStub};
INT32 (*CtvDoXB[32])() = {CtvDoStub};
INT32 (*CtvDoXM[32])() = {CtvDoStub}; 