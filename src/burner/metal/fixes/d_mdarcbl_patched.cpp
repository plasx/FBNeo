// Patched version of d_mdarcbl.cpp for Metal build
// NOTE: This file is compiled without the c_cpp_fixes.h header!

#include "burnint.h"

// Forward declarations of Megadrive-specific functions
void MegadriveInit();
void MegadriveExit();
INT32 MegadriveFrame();
INT32 MegadriveScan(INT32 nAction, INT32* pnMin);
const char* MegadriveGetZipName(UINT32 nZip);
extern bool bMegadriveRecalcPalette;

// Define the input information structs directly
static struct BurnInputInfo SbubsmInputList[] = {
    {"P1 Coin",		BIT_DIGITAL,	0,	"p1 coin"	},
    {"P1 Start",		BIT_DIGITAL,	0,	"p1 start"	},
    {"P1 Up",		BIT_DIGITAL,	0,	"p1 up"		},
    {"P1 Down",		BIT_DIGITAL,	0,	"p1 down"	},
    {"P1 Left",		BIT_DIGITAL,	0,	"p1 left"	},
    {"P1 Right",		BIT_DIGITAL,	0,	"p1 right"	},
    {"P1 Button 1",		BIT_DIGITAL,	0,	"p1 fire 1"	},
    {"P1 Button 2",		BIT_DIGITAL,	0,	"p1 fire 2"	},
    {"P1 Button 3",		BIT_DIGITAL,	0,	"p1 fire 3"	},
    {"P2 Coin",		BIT_DIGITAL,	0,	"p2 coin"	},
    {"P2 Start",		BIT_DIGITAL,	0,	"p2 start"	},
    {"P2 Up",		BIT_DIGITAL,	0,	"p2 up"		},
    {"P2 Down",		BIT_DIGITAL,	0,	"p2 down"	},
    {"P2 Left",		BIT_DIGITAL,	0,	"p2 left"	},
    {"P2 Right",		BIT_DIGITAL,	0,	"p2 right"	},
    {"P2 Button 1",		BIT_DIGITAL,	0,	"p2 fire 1"	},
    {"P2 Button 2",		BIT_DIGITAL,	0,	"p2 fire 2"	},
    {"P2 Button 3",		BIT_DIGITAL,	0,	"p2 fire 3"	},
    {"Reset",		BIT_DIGITAL,	0,	"reset"		},
    {"Service",		BIT_DIGITAL,	0,	"service"	},
    {"Dip A",		BIT_DIPSWITCH,	0,	"dip"		},
    {"Region",		BIT_DIPSWITCH,	0,	"dip"		},
    {NULL, 0, 0, 0}
};

static struct BurnInputInfo TopshootInputList[] = {
    {"P1 Coin",		BIT_DIGITAL,	0,	"p1 coin"	},
    {"P1 Start",		BIT_DIGITAL,	0,	"p1 start"	},
    {"P1 Up",		BIT_DIGITAL,	0,	"p1 up"		},
    {"P1 Down",		BIT_DIGITAL,	0,	"p1 down"	},
    {"P1 Left",		BIT_DIGITAL,	0,	"p1 left"	},
    {"P1 Right",		BIT_DIGITAL,	0,	"p1 right"	},
    {"P1 Button 1",		BIT_DIGITAL,	0,	"p1 fire 1"	},
    {"P1 Button 2",		BIT_DIGITAL,	0,	"p1 fire 2"	},
    {"P1 Button 3",		BIT_DIGITAL,	0,	"p1 fire 3"	},
    {"Reset",		BIT_DIGITAL,	0,	"reset"		},
    {"Service",		BIT_DIGITAL,	0,	"service"	},
    {"Dip A",		BIT_DIPSWITCH,	0,	"dip"		},
    {"Region",		BIT_DIPSWITCH,	0,	"dip"		},
    {NULL, 0, 0, 0}
};

// Define the ROM information structs directly
static struct BurnRomInfo sbubsmRomDesc[] = {
    { "mpr-19271.ic1",	0x100000, 0x9a08cb9d, BRF_PRG | BRF_ESS },	// Supposedly position 2
    { "mpr-19272.ic2",	0x100000, 0x39e5b28b, BRF_PRG | BRF_ESS },
    { "epr-19269.ic28",	0x040000, 0x4f2e5fd5, BRF_PRG | BRF_ESS },
    { "epr-19270.ic29",	0x040000, 0xb7aab08a, BRF_PRG | BRF_ESS },
    { NULL, 0, 0, 0 }
};

// Define the ROM access functions directly
INT32 sbubsmRomInfo(struct BurnRomInfo* pri, UINT32 i) {
    if (i >= 5) return 1;
    if (i < 4) {
        pri->nLen = sbubsmRomDesc[i].nLen;
        pri->nCrc = sbubsmRomDesc[i].nCrc;
        pri->nType = sbubsmRomDesc[i].nType;
        
        if (sbubsmRomDesc[i].szName)
            strcpy(pri->szName, sbubsmRomDesc[i].szName);
        else
            pri->szName[0] = '\0';
    }
    
    return 0;
}

// Define the ROM information structs directly
static struct BurnRomInfo topshootRomDesc[] = {
    { "tp2-ep1.bin",	0x040000, 0xc6a5f608, BRF_PRG | BRF_ESS },
    { "tp2-ep2.bin",	0x040000, 0xb6815996, BRF_PRG | BRF_ESS },
    { "tp2-ep3.bin",	0x040000, 0x0293d98e, BRF_PRG | BRF_ESS },
    { "tp2-ep4.bin",	0x040000, 0x911d7da8, BRF_PRG | BRF_ESS },
    { NULL, 0, 0, 0 }
};

// Define the ROM access functions directly
INT32 topshootRomInfo(struct BurnRomInfo* pri, UINT32 i) {
    if (i >= 5) return 1;
    if (i < 4) {
        pri->nLen = topshootRomDesc[i].nLen;
        pri->nCrc = topshootRomDesc[i].nCrc;
        pri->nType = topshootRomDesc[i].nType;
        
        if (topshootRomDesc[i].szName)
            strcpy(pri->szName, topshootRomDesc[i].szName);
        else
            pri->szName[0] = '\0';
    }
    
    return 0;
}

// Define the driver structures manually
struct BurnDriver BurnDrvMDarcadebSbubsm = {
    "sbubsm", "megaplay", "megadriv", NULL, "1987",
    "Megaplay - Super Bubble Bobble (Sun Mixing bootleg)\0", NULL, "Taito (Sun Mixing bootleg)", "Megadrive",
    NULL, NULL, NULL, NULL,
    BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_SEGA_MEGADRIVE, GBF_MISC, 0,
    MegadriveGetZipName, sbubsmRomInfo, sbubsmRomInfo, NULL, NULL, SbubsmInputList, NULL,
    MegadriveInit, MegadriveExit, MegadriveFrame, NULL, MegadriveScan,
    &bMegadriveRecalcPalette, 0x100, 320, 224, 4, 3
};

struct BurnDriver BurnDrvMDarcadebTopshoot = {
    "topshoot", "megaplay", "megadriv", NULL, "199?",
    "Megaplay - Top Shooter (Sun Mixing bootleg)\0", NULL, "Sega (Sun Mixing bootleg)", "Megadrive",
    NULL, NULL, NULL, NULL,
    BDF_GAME_WORKING | BDF_CLONE, 1, HARDWARE_SEGA_MEGADRIVE, GBF_MISC, 0,
    MegadriveGetZipName, topshootRomInfo, topshootRomInfo, NULL, NULL, TopshootInputList, NULL,
    MegadriveInit, MegadriveExit, MegadriveFrame, NULL, MegadriveScan,
    &bMegadriveRecalcPalette, 0x100, 320, 224, 4, 3
}; 