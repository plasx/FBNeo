#include "burnint.h"
#include "burn_memory.h"
// Don't include cheat.h directly as it's already included by burnint.h

// This file provides dummy implementations of missing functions and variables
// needed to successfully build the Metal implementation of FBNeo

// Global variables
char g_szAppPath[MAX_PATH] = "";
int nInputIntfMouseDivider = 1;
bool bDoIpsPatch = false;
UINT32 nIpsMemExpLen[SND2_ROM + 1] = {0};
UINT32 nIpsDrvDefine = 0;

// Input related variables
int nAnalogDeadZone = 0;    // Changed from AnalogDeadZone to avoid conflict

// Make ProcessAnalog extern "C" with correct signature
extern "C" {
    UINT8 ProcessAnalog(INT16 anaval, INT32 reversed, INT32 flags, UINT8 scalemin, UINT8 scalemax) { return 0; }
}

// Implement the correct AnalogDeadZone function
INT32 AnalogDeadZone(INT32 anaval) { return anaval; }

// Timer functions
extern "C" {
    INT32 BurnTimerPreInit() { return 0; }
    INT32 BurnTimerCPUTotalCycles() { return 0; }
}
INT32 BurnTimerCPUClockspeed = 12000000; // 12MHz default

// Netgame/recording functions
INT32 is_netgame_or_recording() { return 0; }

// IPS patch functions
extern "C" {
    void IpsApplyPatches(unsigned char* base, char* rom_name, unsigned int rom_crc, bool readonly) {}
}

// Debug variables
UINT8 Debug_BurnGunInitted = 0;
UINT8 Debug_BurnLedInitted = 0;

// Movie info
struct MovieExtInfo {
    INT32 placeholder;
};
struct MovieExtInfo MovieInfo = { 0 };

// Driver related
extern "C" {
    INT32 BurnDrvReset() { return 0; }
} 