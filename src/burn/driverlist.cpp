#include "metal_common.h"
#include "metal_fixes.h"
#include "burn.h"
#include "stdfunc.h"
#include "burnint.h"
#include <stdio.h>
#include <string.h>

// Forward declarations for CPS2 functions
extern "C" {
    INT32 Cps2Init();
    INT32 DrvExit();
    INT32 Cps2Frame();
    INT32 CpsRedraw();
    INT32 CpsAreaScan(INT32 nAction, INT32* pnMin);
    extern UINT8 CpsRecalcPal;
}

// Marvel vs. Capcom ROM information (from d_cps2.cpp)
static struct BurnRomInfo MvscRomDesc[] = {
    { "mvce.03a",      0x080000, 0x824e4a90, CPS2_PRG_68K | BRF_ESS | BRF_PRG },
    { "mvce.04a",      0x080000, 0x436c5a4e, CPS2_PRG_68K | BRF_ESS | BRF_PRG },
    { "mvc.05a",       0x080000, 0x2d8c8e86, CPS2_PRG_68K | BRF_ESS | BRF_PRG },
    { "mvc.06a",       0x080000, 0x8528e1f5, CPS2_PRG_68K | BRF_ESS | BRF_PRG },
    { "mvc.07",        0x080000, 0xc3baa32b, CPS2_PRG_68K | BRF_ESS | BRF_PRG },
    { "mvc.08",        0x080000, 0xbc002fcd, CPS2_PRG_68K | BRF_ESS | BRF_PRG },
    { "mvc.09",        0x080000, 0xc67b26df, CPS2_PRG_68K | BRF_ESS | BRF_PRG },
    { "mvc.10",        0x080000, 0x0fdd1e26, CPS2_PRG_68K | BRF_ESS | BRF_PRG },

    { "mvc.13m",       0x400000, 0xfa5f74bc, CPS2_GFX | BRF_GRA },
    { "mvc.15m",       0x400000, 0x71938a8f, CPS2_GFX | BRF_GRA },
    { "mvc.17m",       0x400000, 0x92741d07, CPS2_GFX | BRF_GRA },
    { "mvc.19m",       0x400000, 0xbcb72fc6, CPS2_GFX | BRF_GRA },
    { "mvc.14m",       0x400000, 0x7f1df4e4, CPS2_GFX | BRF_GRA },
    { "mvc.16m",       0x400000, 0x90bd3203, CPS2_GFX | BRF_GRA },
    { "mvc.18m",       0x400000, 0x67aaf727, CPS2_GFX | BRF_GRA },
    { "mvc.20m",       0x400000, 0x8b0bade8, CPS2_GFX | BRF_GRA },

    { "mvc.01",        0x020000, 0x41629e95, CPS2_PRG_Z80 | BRF_ESS | BRF_PRG },
    { "mvc.02",        0x020000, 0x963abf6b, CPS2_PRG_Z80 | BRF_ESS | BRF_PRG },

    { "mvc.11m",       0x400000, 0x850fe663, CPS2_QSND | BRF_SND },
    { "mvc.12m",       0x400000, 0x7ccb1896, CPS2_QSND | BRF_SND },
    
    { "mvsc.key",      0x000014, 0x7e101e09, CPS2_ENCRYPTION_KEY },
    
    { NULL, 0, 0, 0 }
};

// ROM info function for Marvel vs. Capcom
INT32 MvscRomInfo(struct BurnRomInfo* pri, UINT32 i, INT32 nAka) {
    if (!pri) {
        return 1;
    }
    
    if (i >= (sizeof(MvscRomDesc) / sizeof(struct BurnRomInfo)) - 1) {
        return 1; // End of ROM list
    }
    
    *pri = MvscRomDesc[i];
    return 0;
}

// ROM name function for Marvel vs. Capcom
INT32 MvscRomName(char** pszName, UINT32 i, INT32 nAka) {
    if (!pszName) {
        return 1;
    }
    
    if (i >= (sizeof(MvscRomDesc) / sizeof(struct BurnRomInfo)) - 1) {
        return 1; // End of ROM list
    }
    
    *pszName = MvscRomDesc[i].szName;
    return 0;
}

// Input info for Marvel vs. Capcom (CPS2 Fighting style)
static struct BurnInputInfo MvscInputList[] = {
    {"P1 Coin"          , BIT_DIGITAL  , CpsInp020+4, "p1 coin"   },
    {"P1 Start"         , BIT_DIGITAL  , CpsInp020+0, "p1 start"  },
    {"P1 Up"            , BIT_DIGITAL  , CpsInp001+3, "p1 up"     },
    {"P1 Down"          , BIT_DIGITAL  , CpsInp001+2, "p1 down"   },
    {"P1 Left"          , BIT_DIGITAL  , CpsInp001+1, "p1 left"   },
    {"P1 Right"         , BIT_DIGITAL  , CpsInp001+0, "p1 right"  },
    {"P1 Weak Punch"    , BIT_DIGITAL  , CpsInp001+4, "p1 fire 1" },
    {"P1 Medium Punch"  , BIT_DIGITAL  , CpsInp001+5, "p1 fire 2" },
    {"P1 Strong Punch"  , BIT_DIGITAL  , CpsInp001+6, "p1 fire 3" },
    {"P1 Weak Kick"     , BIT_DIGITAL  , CpsInp011+0, "p1 fire 4" },
    {"P1 Medium Kick"   , BIT_DIGITAL  , CpsInp011+1, "p1 fire 5" },
    {"P1 Strong Kick"   , BIT_DIGITAL  , CpsInp011+2, "p1 fire 6" },

    {"P2 Coin"          , BIT_DIGITAL  , CpsInp020+5, "p2 coin"   },
    {"P2 Start"         , BIT_DIGITAL  , CpsInp020+1, "p2 start"  },
    {"P2 Up"            , BIT_DIGITAL  , CpsInp000+3, "p2 up"     },
    {"P2 Down"          , BIT_DIGITAL  , CpsInp000+2, "p2 down"   },
    {"P2 Left"          , BIT_DIGITAL  , CpsInp000+1, "p2 left"   },
    {"P2 Right"         , BIT_DIGITAL  , CpsInp000+0, "p2 right"  },
    {"P2 Weak Punch"    , BIT_DIGITAL  , CpsInp000+4, "p2 fire 1" },
    {"P2 Medium Punch"  , BIT_DIGITAL  , CpsInp000+5, "p2 fire 2" },
    {"P2 Strong Punch"  , BIT_DIGITAL  , CpsInp000+6, "p2 fire 3" },
    {"P2 Weak Kick"     , BIT_DIGITAL  , CpsInp011+4, "p2 fire 4" },
    {"P2 Medium Kick"   , BIT_DIGITAL  , CpsInp011+5, "p2 fire 5" },
    {"P2 Strong Kick"   , BIT_DIGITAL  , CpsInp020+6, "p2 fire 6" },

    {"Reset"            , BIT_DIGITAL  , &CpsReset  , "reset"     },
    {"Diagnostic"       , BIT_DIGITAL  , CpsInp021+1, "diag"      },
    {"Service"          , BIT_DIGITAL  , CpsInp021+2, "service"   },
    
    { NULL, 0, NULL, NULL }
};

// Input info function for Marvel vs. Capcom
INT32 MvscInputInfo(struct BurnInputInfo* pii, UINT32 i) {
    if (!pii) {
        return 1;
    }
    
    if (i >= (sizeof(MvscInputList) / sizeof(struct BurnInputInfo)) - 1) {
        return 1; // End of input list
    }
    
    *pii = MvscInputList[i];
    return 0;
}

// DIP info for Marvel vs. Capcom (none for this game)
INT32 MvscDIPInfo(struct BurnDIPInfo* pdi, UINT32 i) {
    return 1; // No DIP switches
}

// Marvel vs. Capcom driver definition
struct BurnDriver BurnDrvCpsMvsc = {
    "mvsc",                                                    // szShortName
    "Marvel Vs. Capcom: Clash of Super Heroes (Europe 980123)", // szFullNameA
    NULL,                                                      // szComment
    "Capcom",                                                  // szManufacturer
    "CPS2",                                                    // szSystemName
    NULL,                                                      // szParentName
    NULL,                                                      // szBoardROM
    NULL,                                                      // szSampleName
    "1998",                                                    // szDate
    BDF_GAME_WORKING | BDF_HISCORE_SUPPORTED,                 // nFlags
    GBF_VSFIGHT,                                              // nGenreFlags
    FBF_SF,                                                   // nFamilyFlags
    2,                                                        // nMaxPlayers
    HARDWARE_CAPCOM_CPS2,                                     // nHardwareCode
    (void*)MvscRomInfo,                                       // pGetRomInfo
    (void*)MvscRomName,                                       // pGetRomName
    (void*)MvscInputInfo,                                     // pGetInputInfo
    (void*)MvscDIPInfo,                                       // pGetDIPInfo
    Cps2Init,                                                 // Init
    DrvExit,                                                  // Exit
    Cps2Frame,                                                // Frame
    CpsRedraw,                                                // Redraw
    CpsAreaScan,                                              // AreaScan
    &CpsRecalcPal,                                            // pRecalcPal
    0x1000,                                                   // nPaletteEntries
    384,                                                      // nWidth
    224,                                                      // nHeight
    4,                                                        // nAspectX
    3,                                                        // nAspectY
    NULL,                                                     // szParent
    "Marvel Vs. Capcom: Clash of Super Heroes (Europe 980123)", // szFullNameW (same as szFullNameA)
    NULL                                                      // pGetZipName
};

// Driver count and array
UINT32 nBurnDrvCount = 1;
int nBurnDrvActive = -1;

struct BurnDriver* pDriver[] = {
    &BurnDrvCpsMvsc,
    NULL
}; 