// Patched version of cheat.cpp for Metal build
// NOTE: This file is compiled without the c_cpp_fixes.h header!

#include "burnint.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Cheat-related globals from cheat.h
bool bCheatsAllowed;
CheatInfo* pCheatInfo = NULL;
UINT32 CheatSearchShowResultAddresses[CHEATSEARCH_SHOWRESULTS];
UINT32 CheatSearchShowResultValues[CHEATSEARCH_SHOWRESULTS];
CheatSearchInitCallback CheatSearchInitCallbackFunction = NULL;

// Stub implementations of all required functions
INT32 CheatInit()
{
    printf("CheatInit() called (stubbed for Metal build)\n");
    return 0;
}

void CheatExit()
{
    printf("CheatExit() called (stubbed for Metal build)\n");
}

INT32 CheatUpdate()
{
    return 0;
}

INT32 CheatApply()
{
    return 0;
}

INT32 CheatEnable(INT32 nCheat, INT32 nOption)
{
    return 0;
}

INT32 CheatSearchInit()
{
    return 0;
}

void CheatSearchExit()
{
    // Empty stub implementation
}

INT32 CheatSearchStart()
{
    return 0;
}

UINT32 CheatSearchValueNoChange()
{
    return 0;
}

UINT32 CheatSearchValueChange()
{
    return 0;
}

UINT32 CheatSearchValueDecreased()
{
    return 0;
}

UINT32 CheatSearchValueIncreased()
{
    return 0;
}

void CheatSearchDumptoFile()
{
    // Empty stub implementation
}

void CheatSearchExcludeAddressRange(UINT32 nStart, UINT32 nEnd)
{
    // Empty stub implementation
}

unsigned int ReadValueAtHardwareAddress(HWAddressType address, unsigned int size, int isLittleEndian)
{
    return 0;
}

unsigned int ReadValueAtHardwareAddress_audio(HWAddressType address, unsigned int size, int isLittleEndian)
{
    return 0;
}

bool WriteValueAtHardwareAddress(HWAddressType address, unsigned int value, unsigned int size, int isLittleEndian)
{
    return false;
}

bool WriteValueAtHardwareAddress_audio(HWAddressType address, unsigned int value, unsigned int size, int isLittleEndian)
{
    return false;
}

bool IsHardwareAddressValid(HWAddressType address)
{
    return false;
}

// End of patched cheat.cpp 