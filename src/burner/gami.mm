// Burner Game Input
#include "burner.h"

#include <cstring>
#include <cctype>

#ifdef _WIN32
#include <Windows.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "metal/tchar.h"

struct GameInp* GameInp = NULL;
UINT32 nGameInpCount = 0;
UINT32 nMacroCount = 0;
UINT32 nMaxMacro = 0;

// Add the InputSetCooperativeLevel function for Metal build with INT32 return type
#ifdef BUILD_METAL
INT32 InputSetCooperativeLevel(const bool bExclusive, const bool bForeground)
{
    // Stub implementation for Metal build
    return 0;
}
#endif
