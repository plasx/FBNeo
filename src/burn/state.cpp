#include "burnint.h"

// Minimal state.cpp implementation for Metal build
// This file provides state saving/loading functionality

// Function prototypes
INT32 BurnStateLoadEmbed(FILE* fp, INT32 nOffset, INT32 nSize, INT32 nVer);
INT32 BurnStateLoad(TCHAR* szName, INT32 nVer, INT32 bAll);
INT32 BurnStateSaveEmbed(FILE* fp, INT32 nOffset, INT32 nSize, INT32 nVer);
INT32 BurnStateSave(TCHAR* szName, INT32 nVer);

// Empty implementation of state saving/loading functions
INT32 BurnStateLoadEmbed(FILE* fp, INT32 nOffset, INT32 nSize, INT32 nVer)
{
    return 0;
}

INT32 BurnStateLoad(TCHAR* szName, INT32 nVer, INT32 bAll)
{
    return 0;
}

INT32 BurnStateSaveEmbed(FILE* fp, INT32 nOffset, INT32 nSize, INT32 nVer)
{
    return 0;
}

INT32 BurnStateSave(TCHAR* szName, INT32 nVer)
{
    return 0;
}

// Initialize state system
INT32 BurnStateInit()
{
    return 0;
}

// Exit state system
void BurnStateExit()
{
    // Nothing to do in minimal implementation
}

// Minimal implementation for state manangement
static INT32 BurnStateCompress(UINT8* Def, INT32 nDefLen, INT32 nMaxLen, UINT8* Src, INT32 nSrcLen)
{
    // Simplified - just return source length
    return nSrcLen;
} 