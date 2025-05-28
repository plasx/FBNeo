#ifndef _CPS_INPUT_BRIDGE_H_
#define _CPS_INPUT_BRIDGE_H_

// =============================================================================
// FBNeo Metal - CPS Input System Bridge
// =============================================================================
// This header provides proper C/C++ interoperability for the CPS input system
// =============================================================================

#include "metal_interop.h"

METAL_BEGIN_C_DECL

// -----------------------------------------------------------------------------
// CPS Input Arrays
// -----------------------------------------------------------------------------

// Core CPS input arrays
extern UINT8 CpsInp000[0x10];  // P1 inputs
extern UINT8 CpsInp001[0x10];  // P2 inputs
extern UINT8 CpsInp011[0x10];  // System inputs
extern UINT8 CpsInp020[0x10];  // P3 inputs
extern UINT8 CpsInp021[0x10];  // P4 inputs
extern UINT8 CpsInp029[0x10];  // Additional CPS inputs
extern UINT8 CpsInp176[0x10];  // Extra CPS inputs

// Extended CPS input arrays
extern UINT8 CpsInp018[0x10];
extern UINT8 CpsInp177[0x10];
extern UINT8 CpsInp179[0x10];
extern UINT8 CpsInp186[0x10];
extern UINT8 CpsInp187[0x10];
extern UINT8 CpsInp188[0x10];
extern UINT8 CpsInp189[0x10];
extern UINT8 CpsInp190[0x10];

// -----------------------------------------------------------------------------
// CPS System Variables
// -----------------------------------------------------------------------------

// CPS2 flags and system variables
extern INT32 Cps2Turbo;
extern INT32 nCpsZ80Cycles;
extern INT32 nCpsCycles;
extern UINT8* CpsZRamC0;
extern UINT8* CpsZRamF0;

// Dials and paddle inputs
extern UINT16 CpsInp055;
extern UINT16 CpsInp05d;
extern INT32 nDial055;
extern INT32 nDial05d;
extern INT32 nDial055_dir;
extern INT32 nDial05d_dir;
extern UINT8 CpsDigUD[4];

// Puzzle loop paddle inputs
extern INT16 CpsInpPaddle1;
extern INT16 CpsInpPaddle2; 
extern INT32 CpsPaddle1Value;
extern INT32 CpsPaddle2Value;
extern INT32 CpsPaddle1;
extern INT32 CpsPaddle2;

// CPS raster line info
extern INT32 CpsrLineInfo[16][16];
extern UINT8* CpsrBase;
extern INT32 nCpsrScrY;
extern UINT8 CpsRaster[16][0x10];

// -----------------------------------------------------------------------------
// CPS Flag Variables
// -----------------------------------------------------------------------------

extern INT32 PangEEP;
extern INT32 Forgottn;
extern INT32 Cps1QsHack;
extern INT32 Kodh;
extern INT32 Cawingb;
extern INT32 Sf2thndr;
extern INT32 Pzloop2;
extern INT32 Hkittymp;
extern INT32 Ssf2tb;
extern INT32 Dinohunt;
extern INT32 Port6SoundWrite;
extern INT32 CpsBootlegEEPROM;
extern INT32 Jurassic99;
extern INT32 Dinoh;
extern INT32 Wofhfh;
extern INT32 Wofsgzb;
extern INT32 Wof3js;
extern INT32 Knightsh;
extern INT32 Ecofght;

// -----------------------------------------------------------------------------
// CPS Input Functions
// -----------------------------------------------------------------------------

// Read inputs from devices and update CPS input arrays
void CpsReadInputs();

// Process input values into game-specific formats
void CpsMakeInputs();

// Initialize CPS input system
INT32 CpsInputInit();

// Shut down CPS input system
INT32 CpsInputExit();

// Metal-specific functions for integration with GameController
INT32 Metal_UpdateCpsInputs();

METAL_END_C_DECL

#endif // _CPS_INPUT_BRIDGE_H_ 