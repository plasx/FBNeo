// Metal-specific stubs for state.h and other missing functions
// This provides C-compatible implementations to resolve linker errors

// No Foundation.h include to avoid Objective-C issues
#include "burnint.h"
#include "burn_ym2151.h"
#include "timer.h"

// Include needed for tms34010
typedef struct _tms34010_display_params tms34010_display_params;

// Simple TCHAR typedef for Metal without including Foundation.h
typedef char TCHAR;

// Define these essential stubs with proper linkage
// All functions need proper extern "C" linkage to avoid name mangling issues
extern "C" {

// State management stubs
INT32 BurnStateLoad(TCHAR* szName, INT32 bAll, INT32 (*pLoadGame)())
{
    // Stub implementation for Metal
    return 0;
}

INT32 BurnStateSave(TCHAR* szName, INT32 bAll)
{
    // Stub implementation for Metal
    return 0;
}

// String conversion stub
char* TCHARToANSI(const TCHAR* pszIn, char* pszOut, INT32 nOutSize)
{
    // Simple implementation for Metal (already ASCII/char)
    strcpy(pszOut, pszIn);
    return pszOut;
}

// Video interface stubs
INT32 VidRecalcPal()
{
    // Stub implementation for Metal
    return 0;
}

// Sample rendering stub
INT32 BurnSampleRender_INT(UINT32 nChannelMask)
{
    // Stub implementation
    return 0;
}

// TMS34010 stubs - use the exact signature from tms34_intf.cpp
INT32 tms34010_generate_scanline(INT32 line, INT32 (*callback)(INT32, tms34010_display_params*))
{
    return 0;
}

// YM2151 function stubs with exact signatures to match the headers
// We need these declarations to have proper C linkage as well
void BurnYM2151SetRoute(INT32 chip, INT32 nIndex, double nVolume, INT32 nRouteDir) {}
void BurnYM2151SetAllRoutes(INT32 chip, double vol, INT32 route) {}
void BurnYM2151SetIrqHandler(INT32 chip, void (*irq_cb)(INT32)) {}
void BurnYM2151SetPortHandler(INT32 chip, write8_handler port_cb) {}
UINT8 BurnYM2151Read(INT32 chip) { return 0; }

// Note: Global variables like nIpsMemExpLen and szAppEEPROMPath 
// are defined in metal_c_globals.c to avoid language linkage issues

// QSound stubs
INT32 QsndScan(INT32 nAction) { return 0; }

// Support for required PS_M.cpp variables 
int bPsndOkay = 0;
UINT32 nQscLen = 0;

} // end of extern "C" block

// Global variable definitions (outside the extern "C" block)
// These are already declared as extern in their headers
INT32 nInputIntfMouseDivider = 1;
INT32 nFireButtons = 4;
INT32 bVidUseHardwareGamma = 0;
INT32 bStreetFighterLayout = 0;

// Add any other necessary variables here (outside extern "C") 