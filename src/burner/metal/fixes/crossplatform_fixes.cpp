#include "../../../burn/crossplatform.h"
#include "burn_memory.h"
#include <stdio.h>

// Global variables required for FBNeo Metal build
// Define these only once
double dBurnFPS = 60.0;          // Current refresh rate
INT32 nBurnSoundRate = 44100;    // Sound sample rate
unsigned char* pBurnDraw = NULL; // Screen buffer
int nBurnPitch = 0;              // Screen pitch
int nBurnBpp = 0;                // Bytes per pixel
bool bBurnOkay = false;          // Emulator ready flag

// Define screen dimensions (used by multiple files)
INT32 nScreenWidth = 320;       // Default screen width
INT32 nScreenHeight = 240;      // Default screen height

// Set emulator refresh rate
void BurnSetRefreshRate(double dRefreshRate)
{
    dBurnFPS = dRefreshRate;
}

// System reset function
INT32 HiscoreReset()
{
    printf("HiscoreReset called\n");
    return 0;
}

// YM2151 Sound core functions
INT32 BurnYM2151Reset() { return 0; }
INT32 BurnYM2151Exit() { return 0; }
INT32 BurnYM2151Init(INT32 nClockFrequency) { return 0; }
void BurnYM2151SetRoute(INT32 nChip, INT32 nIndex, double nVolume, INT32 nRouteDir) {}
void BurnYM2151SetAllRoutes(INT32 nChip, double nVolume, INT32 nRouteDir) {}
UINT8 BurnYM2151Read(INT32 nChip, INT32 nAddress) { return 0; }
void BurnYM2151Write(INT32 nChip, INT32 nAddress, UINT8 nData) {}
void BurnYM2151Scan(INT32 nAction, INT32* pnMin) {} 