#include "burnint.h"

// Minimal driver implementation for Metal build
// Empty stubs of driver-related functions

// Driver callbacks with correct signatures
static INT32 MvscInit() { return 0; }
static INT32 MvscExit() { return 0; }
static INT32 MvscFrame() { return 0; }
static INT32 MvscRedraw() { return 0; }
static INT32 MvscAreaScan(INT32 nAction, INT32* pnMin) { return 0; }

// Descriptive data for the driver
static struct BurnInputInfo MvscInputList[] = { { NULL, 0 } };
static struct BurnDIPInfo MvscDIPList[] = { { NULL, 0 } };

// Required callbacks with proper signatures
static INT32 MvscGetZipName(char** pszName, UINT32 i) { return 1; }
static INT32 MvscGetRomInfo(struct BurnRomInfo* pri, UINT32 i) { return 1; }
static INT32 MvscGetRomName(char** pszName, UINT32 i, INT32 nAka) { return 1; }
static INT32 MvscGetInputInfo(struct BurnInputInfo* pii, UINT32 i) { return 1; }
static INT32 MvscGetDIPInfo(struct BurnDIPInfo* pdi, UINT32 i) { return 1; }
static INT32 MvscGetHDDInfo(struct BurnHDDInfo* pri, UINT32 i) { return 1; }
static INT32 MvscGetHDDName(char** pszName, UINT32 i, INT32 nAka) { return 1; }
static INT32 MvscGetSampleInfo(struct BurnSampleInfo* pri, UINT32 i) { return 1; }
static INT32 MvscGetSampleName(char** pszName, UINT32 i, INT32 nAka) { return 1; }

// Driver structure - defining necessary callbacks to prevent linker errors
struct BurnDriver BurnDrvCpsMvsc = {
    // Basic info
    (char*)"mvsc",         // Short name
    NULL,                  // Parent
    NULL,                  // Board ROM
    NULL,                  // Sample name
    (char*)"1998",         // Date
    
    // Names
    (char*)"Marvel vs. Capcom: Clash of Super Heroes (USA 980123)", // Full name (A)
    NULL,                  // Comment (A)
    (char*)"Capcom",       // Manufacturer (A)
    (char*)"CPS2",         // System (A)
    
    // Names (Unicode W variants)
    NULL,                  // Full name (W)
    NULL,                  // Comment (W)
    NULL,                  // Manufacturer (W)
    NULL,                  // System (W)
    
    // Flags and type info
    BDF_GAME_WORKING,      // Flags
    2,                     // Max players
    HARDWARE_CAPCOM_CPS2,  // Hardware code
    GBF_VSFIGHT,           // Genre
    FBF_SF,                // Family
    
    // ROM/HDD/Sample info functions
    MvscGetZipName,        // GetZipName
    MvscGetRomInfo,        // GetRomInfo
    MvscGetRomName,        // GetRomName
    MvscGetHDDInfo,        // GetHDDInfo
    MvscGetHDDName,        // GetHDDName
    MvscGetSampleInfo,     // GetSampleInfo
    MvscGetSampleName,     // GetSampleName
    
    // Input info functions
    MvscGetInputInfo,      // GetInputInfo
    MvscGetDIPInfo,        // GetDIPInfo
    
    // Core functions
    MvscInit,              // Init
    MvscExit,              // Exit
    MvscFrame,             // Frame
    MvscRedraw,            // Redraw
    MvscAreaScan,          // AreaScan
    
    // Palette related
    NULL,                  // pRecalcPal
    0,                     // nPaletteEntries
    
    // Screen dimensions and aspect ratio
    320,                   // nWidth
    240,                   // nHeight
    4,                     // nXAspect
    3                      // nYAspect
};
