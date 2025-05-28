// Patched version of d_megadrive.cpp for Metal build
// NOTE: This file is compiled without the c_cpp_fixes.h header!

#include "burnint.h"

// Forward declarations of Megadrive-specific functions
void MegadriveInit();
void MegadriveExit();
INT32 MegadriveFrame();
INT32 MegadriveScan(INT32 nAction, INT32* pnMin);
const char* MegadriveGetZipName(UINT32 nZip);
extern bool bMegadriveRecalcPalette;

// Additional init functions for multiplayer modes
void MegadriveInit3p();
void MegadriveInit4p();
void MegadriveInit5p();

// Define input structures directly
static struct BurnInputInfo MegadriveInputList[] = {
    {"P1 Start",		BIT_DIGITAL,	0,	"p1 start"	},
    {"P1 Up",		    BIT_DIGITAL,	0,	"p1 up"		},
    {"P1 Down",		    BIT_DIGITAL,	0,	"p1 down"	},
    {"P1 Left",		    BIT_DIGITAL,	0,	"p1 left"	},
    {"P1 Right",		BIT_DIGITAL,	0,	"p1 right"	},
    {"P1 Button A",		BIT_DIGITAL,	0,	"p1 fire 1"	},
    {"P1 Button B",		BIT_DIGITAL,	0,	"p1 fire 2"	},
    {"P1 Button C",		BIT_DIGITAL,	0,	"p1 fire 3"	},
    {"P1 Button X",		BIT_DIGITAL,	0,	"p1 fire 4"	},
    {"P1 Button Y",		BIT_DIGITAL,	0,	"p1 fire 5"	},
    {"P1 Button Z",		BIT_DIGITAL,	0,	"p1 fire 6"	},
    {"P1 Button Mode",	BIT_DIGITAL,	0,	"p1 select"	},
    {"P2 Start",		BIT_DIGITAL,	0,	"p2 start"	},
    {"P2 Up",		    BIT_DIGITAL,	0,	"p2 up"		},
    {"P2 Down",		    BIT_DIGITAL,	0,	"p2 down"	},
    {"P2 Left",		    BIT_DIGITAL,	0,	"p2 left"	},
    {"P2 Right",		BIT_DIGITAL,	0,	"p2 right"	},
    {"P2 Button A",		BIT_DIGITAL,	0,	"p2 fire 1"	},
    {"P2 Button B",		BIT_DIGITAL,	0,	"p2 fire 2"	},
    {"P2 Button C",		BIT_DIGITAL,	0,	"p2 fire 3"	},
    {"P2 Button X",		BIT_DIGITAL,	0,	"p2 fire 4"	},
    {"P2 Button Y",		BIT_DIGITAL,	0,	"p2 fire 5"	},
    {"P2 Button Z",		BIT_DIGITAL,	0,	"p2 fire 6"	},
    {"P2 Button Mode",	BIT_DIGITAL,	0,	"p2 select"	},
    {"Reset",		    BIT_DIGITAL,	0,	"reset"		},
    {"Service",		    BIT_DIGITAL,	0,	"service"	},
    {"Dip A",		    BIT_DIPSWITCH,	0,	"dip"		},
    {"Region",		    BIT_DIPSWITCH,	0,	"dip"		},
    {NULL, 0, 0, 0}
};

static struct BurnInputInfo Megadrive3pInputList[] = {
    {"P1 Start",		BIT_DIGITAL,	0,	"p1 start"	},
    {"P1 Up",		    BIT_DIGITAL,	0,	"p1 up"		},
    {"P1 Down",		    BIT_DIGITAL,	0,	"p1 down"	},
    {"P1 Left",		    BIT_DIGITAL,	0,	"p1 left"	},
    {"P1 Right",		BIT_DIGITAL,	0,	"p1 right"	},
    {"P1 Button A",		BIT_DIGITAL,	0,	"p1 fire 1"	},
    {"P1 Button B",		BIT_DIGITAL,	0,	"p1 fire 2"	},
    {"P1 Button C",		BIT_DIGITAL,	0,	"p1 fire 3"	},
    {"P1 Button X",		BIT_DIGITAL,	0,	"p1 fire 4"	},
    {"P1 Button Y",		BIT_DIGITAL,	0,	"p1 fire 5"	},
    {"P1 Button Z",		BIT_DIGITAL,	0,	"p1 fire 6"	},
    {"P1 Button Mode",	BIT_DIGITAL,	0,	"p1 select"	},
    {"P2 Start",		BIT_DIGITAL,	0,	"p2 start"	},
    {"P2 Up",		    BIT_DIGITAL,	0,	"p2 up"		},
    {"P2 Down",		    BIT_DIGITAL,	0,	"p2 down"	},
    {"P2 Left",		    BIT_DIGITAL,	0,	"p2 left"	},
    {"P2 Right",		BIT_DIGITAL,	0,	"p2 right"	},
    {"P2 Button A",		BIT_DIGITAL,	0,	"p2 fire 1"	},
    {"P2 Button B",		BIT_DIGITAL,	0,	"p2 fire 2"	},
    {"P2 Button C",		BIT_DIGITAL,	0,	"p2 fire 3"	},
    {"P2 Button X",		BIT_DIGITAL,	0,	"p2 fire 4"	},
    {"P2 Button Y",		BIT_DIGITAL,	0,	"p2 fire 5"	},
    {"P2 Button Z",		BIT_DIGITAL,	0,	"p2 fire 6"	},
    {"P2 Button Mode",	BIT_DIGITAL,	0,	"p2 select"	},
    {"P3 Start",		BIT_DIGITAL,	0,	"p3 start"	},
    {"P3 Up",		    BIT_DIGITAL,	0,	"p3 up"		},
    {"P3 Down",		    BIT_DIGITAL,	0,	"p3 down"	},
    {"P3 Left",		    BIT_DIGITAL,	0,	"p3 left"	},
    {"P3 Right",		BIT_DIGITAL,	0,	"p3 right"	},
    {"P3 Button A",		BIT_DIGITAL,	0,	"p3 fire 1"	},
    {"P3 Button B",		BIT_DIGITAL,	0,	"p3 fire 2"	},
    {"P3 Button C",		BIT_DIGITAL,	0,	"p3 fire 3"	},
    {"P3 Button X",		BIT_DIGITAL,	0,	"p3 fire 4"	},
    {"P3 Button Y",		BIT_DIGITAL,	0,	"p3 fire 5"	},
    {"P3 Button Z",		BIT_DIGITAL,	0,	"p3 fire 6"	},
    {"P3 Button Mode",	BIT_DIGITAL,	0,	"p3 select"	},
    {"Reset",		    BIT_DIGITAL,	0,	"reset"		},
    {"Service",		    BIT_DIGITAL,	0,	"service"	},
    {"Dip A",		    BIT_DIPSWITCH,	0,	"dip"		},
    {"Region",		    BIT_DIPSWITCH,	0,	"dip"		},
    {NULL, 0, 0, 0}
};

static struct BurnInputInfo Megadrive4pInputList[] = {
    // Player 1 inputs
    {"P1 Start",		BIT_DIGITAL,	0,	"p1 start"	},
    {"P1 Up",		    BIT_DIGITAL,	0,	"p1 up"		},
    {"P1 Down",		    BIT_DIGITAL,	0,	"p1 down"	},
    {"P1 Left",		    BIT_DIGITAL,	0,	"p1 left"	},
    {"P1 Right",		BIT_DIGITAL,	0,	"p1 right"	},
    {"P1 Button A",		BIT_DIGITAL,	0,	"p1 fire 1"	},
    {"P1 Button B",		BIT_DIGITAL,	0,	"p1 fire 2"	},
    {"P1 Button C",		BIT_DIGITAL,	0,	"p1 fire 3"	},
    {"P1 Button X",		BIT_DIGITAL,	0,	"p1 fire 4"	},
    {"P1 Button Y",		BIT_DIGITAL,	0,	"p1 fire 5"	},
    {"P1 Button Z",		BIT_DIGITAL,	0,	"p1 fire 6"	},
    {"P1 Button Mode",	BIT_DIGITAL,	0,	"p1 select"	},
    // Player 2 inputs
    {"P2 Start",		BIT_DIGITAL,	0,	"p2 start"	},
    {"P2 Up",		    BIT_DIGITAL,	0,	"p2 up"		},
    {"P2 Down",		    BIT_DIGITAL,	0,	"p2 down"	},
    {"P2 Left",		    BIT_DIGITAL,	0,	"p2 left"	},
    {"P2 Right",		BIT_DIGITAL,	0,	"p2 right"	},
    {"P2 Button A",		BIT_DIGITAL,	0,	"p2 fire 1"	},
    {"P2 Button B",		BIT_DIGITAL,	0,	"p2 fire 2"	},
    {"P2 Button C",		BIT_DIGITAL,	0,	"p2 fire 3"	},
    {"P2 Button X",		BIT_DIGITAL,	0,	"p2 fire 4"	},
    {"P2 Button Y",		BIT_DIGITAL,	0,	"p2 fire 5"	},
    {"P2 Button Z",		BIT_DIGITAL,	0,	"p2 fire 6"	},
    {"P2 Button Mode",	BIT_DIGITAL,	0,	"p2 select"	},
    // Player 3 inputs
    {"P3 Start",		BIT_DIGITAL,	0,	"p3 start"	},
    {"P3 Up",		    BIT_DIGITAL,	0,	"p3 up"		},
    {"P3 Down",		    BIT_DIGITAL,	0,	"p3 down"	},
    {"P3 Left",		    BIT_DIGITAL,	0,	"p3 left"	},
    {"P3 Right",		BIT_DIGITAL,	0,	"p3 right"	},
    {"P3 Button A",		BIT_DIGITAL,	0,	"p3 fire 1"	},
    {"P3 Button B",		BIT_DIGITAL,	0,	"p3 fire 2"	},
    {"P3 Button C",		BIT_DIGITAL,	0,	"p3 fire 3"	},
    {"P3 Button X",		BIT_DIGITAL,	0,	"p3 fire 4"	},
    {"P3 Button Y",		BIT_DIGITAL,	0,	"p3 fire 5"	},
    {"P3 Button Z",		BIT_DIGITAL,	0,	"p3 fire 6"	},
    {"P3 Button Mode",	BIT_DIGITAL,	0,	"p3 select"	},
    // Player 4 inputs
    {"P4 Start",		BIT_DIGITAL,	0,	"p4 start"	},
    {"P4 Up",		    BIT_DIGITAL,	0,	"p4 up"		},
    {"P4 Down",		    BIT_DIGITAL,	0,	"p4 down"	},
    {"P4 Left",		    BIT_DIGITAL,	0,	"p4 left"	},
    {"P4 Right",		BIT_DIGITAL,	0,	"p4 right"	},
    {"P4 Button A",		BIT_DIGITAL,	0,	"p4 fire 1"	},
    {"P4 Button B",		BIT_DIGITAL,	0,	"p4 fire 2"	},
    {"P4 Button C",		BIT_DIGITAL,	0,	"p4 fire 3"	},
    {"P4 Button X",		BIT_DIGITAL,	0,	"p4 fire 4"	},
    {"P4 Button Y",		BIT_DIGITAL,	0,	"p4 fire 5"	},
    {"P4 Button Z",		BIT_DIGITAL,	0,	"p4 fire 6"	},
    {"P4 Button Mode",	BIT_DIGITAL,	0,	"p4 select"	},
    // System inputs
    {"Reset",		    BIT_DIGITAL,	0,	"reset"		},
    {"Service",		    BIT_DIGITAL,	0,	"service"	},
    {"Dip A",		    BIT_DIPSWITCH,	0,	"dip"		},
    {"Region",		    BIT_DIPSWITCH,	0,	"dip"		},
    {NULL, 0, 0, 0}
};

static struct BurnInputInfo Megadrive5pInputList[] = {
    // Players 1 and 2 inputs
    {"P1 Start",		BIT_DIGITAL,	0,	"p1 start"	},
    {"P1 Up",		    BIT_DIGITAL,	0,	"p1 up"		},
    {"P1 Down",		    BIT_DIGITAL,	0,	"p1 down"	},
    {"P1 Left",		    BIT_DIGITAL,	0,	"p1 left"	},
    {"P1 Right",		BIT_DIGITAL,	0,	"p1 right"	},
    {"P1 Button A",		BIT_DIGITAL,	0,	"p1 fire 1"	},
    {"P1 Button B",		BIT_DIGITAL,	0,	"p1 fire 2"	},
    {"P1 Button C",		BIT_DIGITAL,	0,	"p1 fire 3"	},
    {"P1 Button X",		BIT_DIGITAL,	0,	"p1 fire 4"	},
    {"P1 Button Y",		BIT_DIGITAL,	0,	"p1 fire 5"	},
    {"P1 Button Z",		BIT_DIGITAL,	0,	"p1 fire 6"	},
    {"P1 Button Mode",	BIT_DIGITAL,	0,	"p1 select"	},
    
    {"P2 Start",		BIT_DIGITAL,	0,	"p2 start"	},
    {"P2 Up",		    BIT_DIGITAL,	0,	"p2 up"		},
    {"P2 Down",		    BIT_DIGITAL,	0,	"p2 down"	},
    {"P2 Left",		    BIT_DIGITAL,	0,	"p2 left"	},
    {"P2 Right",		BIT_DIGITAL,	0,	"p2 right"	},
    {"P2 Button A",		BIT_DIGITAL,	0,	"p2 fire 1"	},
    {"P2 Button B",		BIT_DIGITAL,	0,	"p2 fire 2"	},
    {"P2 Button C",		BIT_DIGITAL,	0,	"p2 fire 3"	},
    {"P2 Button X",		BIT_DIGITAL,	0,	"p2 fire 4"	},
    {"P2 Button Y",		BIT_DIGITAL,	0,	"p2 fire 5"	},
    {"P2 Button Z",		BIT_DIGITAL,	0,	"p2 fire 6"	},
    {"P2 Button Mode",	BIT_DIGITAL,	0,	"p2 select"	},
    
    // Player 3-5 inputs
    {"P3 Start",		BIT_DIGITAL,	0,	"p3 start"	},
    {"P3 Up",		    BIT_DIGITAL,	0,	"p3 up"		},
    {"P3 Down",		    BIT_DIGITAL,	0,	"p3 down"	},
    {"P3 Left",		    BIT_DIGITAL,	0,	"p3 left"	},
    {"P3 Right",		BIT_DIGITAL,	0,	"p3 right"	},
    {"P3 Button A",		BIT_DIGITAL,	0,	"p3 fire 1"	},
    {"P3 Button B",		BIT_DIGITAL,	0,	"p3 fire 2"	},
    {"P3 Button C",		BIT_DIGITAL,	0,	"p3 fire 3"	},
    
    {"P4 Start",		BIT_DIGITAL,	0,	"p4 start"	},
    {"P4 Up",		    BIT_DIGITAL,	0,	"p4 up"		},
    {"P4 Down",		    BIT_DIGITAL,	0,	"p4 down"	},
    {"P4 Left",		    BIT_DIGITAL,	0,	"p4 left"	},
    {"P4 Right",		BIT_DIGITAL,	0,	"p4 right"	},
    {"P4 Button A",		BIT_DIGITAL,	0,	"p4 fire 1"	},
    {"P4 Button B",		BIT_DIGITAL,	0,	"p4 fire 2"	},
    {"P4 Button C",		BIT_DIGITAL,	0,	"p4 fire 3"	},
    
    {"P5 Start",		BIT_DIGITAL,	0,	"p5 start"	},
    {"P5 Up",		    BIT_DIGITAL,	0,	"p5 up"		},
    {"P5 Down",		    BIT_DIGITAL,	0,	"p5 down"	},
    {"P5 Left",		    BIT_DIGITAL,	0,	"p5 left"	},
    {"P5 Right",		BIT_DIGITAL,	0,	"p5 right"	},
    {"P5 Button A",		BIT_DIGITAL,	0,	"p5 fire 1"	},
    {"P5 Button B",		BIT_DIGITAL,	0,	"p5 fire 2"	},
    {"P5 Button C",		BIT_DIGITAL,	0,	"p5 fire 3"	},
    
    // System inputs
    {"Reset",		    BIT_DIGITAL,	0,	"reset"		},
    {"Service",		    BIT_DIGITAL,	0,	"service"	},
    {"Dip A",		    BIT_DIPSWITCH,	0,	"dip"		},
    {"Region",		    BIT_DIPSWITCH,	0,	"dip"		},
    {NULL, 0, 0, 0}
};

// Define DIP switch information
struct BurnDIPInfo MegadriveDIPList[] = {
    // Auto-detect console region
    {0, 0xf0, 0xf0, 0x00, NULL },
    {0, 0xf0, 0xf0, 0x00, "Auto" },
    {0, 0xf0, 0xf0, 0x80, "Japan (NTSC)" },
    {0, 0xf0, 0xf0, 0x40, "Japan (PAL)" },
    {0, 0xf0, 0xf0, 0x20, "USA (NTSC)" },
    {0, 0xf0, 0xf0, 0x10, "Europe (PAL)" },
    {0, 0xff, 0xff, 0x02, NULL },
    {0, 0xff, 0xff, 0x02, "Auto" },
    {0, 0xff, 0xff, 0x00, "Game" },
    {0, 0xff, 0xff, 0x01, "svp" },
    {NULL, 0, 0, 0, NULL }
};

// ROM information for specific games
static struct BurnRomInfo md_gametoRomDesc[] = {
    { "Bare Game", 0x400000, 0x00000000, BRF_PRG | BRF_ESS },
    { NULL, 0, 0, 0 }
};

// Define ROM access function directly
INT32 md_gametoRomInfo(struct BurnRomInfo* pri, UINT32 i) {
    if (i >= 2) return 1;
    if (i < 1) {
        pri->nLen = md_gametoRomDesc[i].nLen;
        pri->nCrc = md_gametoRomDesc[i].nCrc;
        pri->nType = md_gametoRomDesc[i].nType;
        
        if (md_gametoRomDesc[i].szName)
            strcpy(pri->szName, md_gametoRomDesc[i].szName);
        else
            pri->szName[0] = '\0';
    }
    
    return 0;
}

// ROM info for micromc2
static struct BurnRomInfo md_micromc2RomDesc[] = {
    { "MicroMC2 BIOS", 0x20000, 0x00000000, BRF_PRG | BRF_ESS },
    { NULL, 0, 0, 0 }
};

// ROM access function for micromc2
INT32 md_micromc2RomInfo(struct BurnRomInfo* pri, UINT32 i) {
    if (i >= 2) return 1;
    if (i < 1) {
        pri->nLen = md_micromc2RomDesc[i].nLen;
        pri->nCrc = md_micromc2RomDesc[i].nCrc;
        pri->nType = md_micromc2RomDesc[i].nType;
        
        if (md_micromc2RomDesc[i].szName)
            strcpy(pri->szName, md_micromc2RomDesc[i].szName);
        else
            pri->szName[0] = '\0';
    }
    
    return 0;
}

// ROM info for microm96
static struct BurnRomInfo md_microm96RomDesc[] = {
    { "MicroM96 BIOS", 0x20000, 0x00000000, BRF_PRG | BRF_ESS },
    { NULL, 0, 0, 0 }
};

// ROM access function for microm96
INT32 md_microm96RomInfo(struct BurnRomInfo* pri, UINT32 i) {
    if (i >= 2) return 1;
    if (i < 1) {
        pri->nLen = md_microm96RomDesc[i].nLen;
        pri->nCrc = md_microm96RomDesc[i].nCrc;
        pri->nType = md_microm96RomDesc[i].nType;
        
        if (md_microm96RomDesc[i].szName)
            strcpy(pri->szName, md_microm96RomDesc[i].szName);
        else
            pri->szName[0] = '\0';
    }
    
    return 0;
}

// Define driver descriptors for region auto-detection
struct BurnDriver BurnDrvMegadrive = {
    "megadriv", NULL, NULL, NULL, "1988-1994",
    "Sega Megadrive / Genesis (Auto Region)\0", NULL, "Sega", "Megadrive",
    NULL, NULL, NULL, NULL,
    BDF_GAME_WORKING, 2, HARDWARE_SEGA_MEGADRIVE, GBF_MISC, 0,
    MegadriveGetZipName, md_gametoRomInfo, md_gametoRomInfo, NULL, NULL, MegadriveInputList, MegadriveDIPList,
    MegadriveInit, MegadriveExit, MegadriveFrame, NULL, MegadriveScan,
    &bMegadriveRecalcPalette, 0x100, 320, 224, 4, 3
};

// Define driver for 3-player mode
struct BurnDriver BurnDrvMegadrive3p = {
    "megadriv3p", "megadriv", NULL, NULL, "1988-1994",
    "Sega Megadrive / Genesis (3P, Auto Region)\0", NULL, "Sega", "Megadrive",
    NULL, NULL, NULL, NULL,
    BDF_GAME_WORKING | BDF_CLONE, 3, HARDWARE_SEGA_MEGADRIVE, GBF_MISC, 0,
    MegadriveGetZipName, md_gametoRomInfo, md_gametoRomInfo, NULL, NULL, Megadrive3pInputList, MegadriveDIPList,
    MegadriveInit3p, MegadriveExit, MegadriveFrame, NULL, MegadriveScan,
    &bMegadriveRecalcPalette, 0x100, 320, 224, 4, 3
};

// Define driver for 4-player mode
struct BurnDriver BurnDrvMegadrive4p = {
    "megadriv4p", "megadriv", NULL, NULL, "1988-1994",
    "Sega Megadrive / Genesis (4P, Auto Region)\0", NULL, "Sega", "Megadrive",
    NULL, NULL, NULL, NULL,
    BDF_GAME_WORKING | BDF_CLONE, 4, HARDWARE_SEGA_MEGADRIVE, GBF_MISC, 0,
    MegadriveGetZipName, md_gametoRomInfo, md_gametoRomInfo, NULL, NULL, Megadrive4pInputList, MegadriveDIPList,
    MegadriveInit4p, MegadriveExit, MegadriveFrame, NULL, MegadriveScan,
    &bMegadriveRecalcPalette, 0x100, 320, 224, 4, 3
};

// Define driver for 5-player mode
struct BurnDriver BurnDrvMegadrive5p = {
    "megadriv5p", "megadriv", NULL, NULL, "1988-1994",
    "Sega Megadrive / Genesis (5P, Auto Region)\0", NULL, "Sega", "Megadrive",
    NULL, NULL, NULL, NULL,
    BDF_GAME_WORKING | BDF_CLONE, 5, HARDWARE_SEGA_MEGADRIVE, GBF_MISC, 0,
    MegadriveGetZipName, md_gametoRomInfo, md_gametoRomInfo, NULL, NULL, Megadrive5pInputList, MegadriveDIPList,
    MegadriveInit5p, MegadriveExit, MegadriveFrame, NULL, MegadriveScan,
    &bMegadriveRecalcPalette, 0x100, 320, 224, 4, 3
}; 