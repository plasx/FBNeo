#ifndef _BURN_FIXES_H_
#define _BURN_FIXES_H_

// Include this header in Metal build before including burn.cpp
// It adds necessary declarations for the Metal port

#include "burnint.h"
#include "burn_sourcefile_stubs.h"

// Declare the driver list array - defined in driverlist.cpp
extern struct BurnDriver* const pDriverList[];

// Stub implementations for some less-critical functions
static const char* BurnDrvGetTextA(UINT32 i, UINT32 nIndex)
{
    return "";
}

static UINT32 BurnDrvGetAudioCPUClockRate(int cpu)
{
    return 0;
}

// Fix for pDriver vs pDriverList confusion - in burn.cpp it uses pDriver
// but our implementation uses pDriverList
#define pDriver pDriverList

#endif // _BURN_FIXES_H_ 