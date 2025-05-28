// Burn - Drivers module

#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
#include "version.h"
#include "burnint.h"
#include "timer.h"
#include "driverlist.h"
#include "dac.h"
#include "burn_memory.h"
#include "burn_sound.h"
#include "metal_fixes.h"

#ifdef BUILD_A68K
#include "cpu/a68k/a68k.h"
#endif

// Include our generated sourcefile header
#include "../dep/generated/sourcefile.h"

#ifndef __LIBRETRO__
// filler function, used if the application is not printing debug messages
static INT32 BurnbprintfFiller(INT32, TCHAR* , ...) { return 0; }
// pointer to burner printing function
#ifndef bprintf
INT32 (*bprintf)(INT32 nStatus, TCHAR* szFormat, ...) = BurnbprintfFiller;
#endif
#endif

INT32 nBurnVer = BURN_VERSION;	// Version number of the library

UINT32 nBurnDrvCount     = 0;	// Count of game drivers
int nBurnDrvActive    = ~0U;	// Which game driver is selected
INT32 nBurnDrvSubActive  = -1;	// Which sub-game driver is selected
UINT32 nBurnDrvSelect[8] = { ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U }; // Which games are selected (i.e. loaded but not necessarily active)

char* pszCustomNameA = NULL;
char szBackupNameA[MAX_PATH];
TCHAR szBackupNameW[MAX_PATH];

char** szShortNamesExArray = NULL;
TCHAR** szLongNamesExArray = NULL;
UINT32 nNamesExArray       = 0;

bool bBurnUseMMX;
#if defined BUILD_A68K
bool bBurnUseASMCPUEmulation = 0; // use mame 68000 emulation by default
#endif

// Just so we can start using FBNEO_DEBUG and keep backwards compatablity should whatever is left of FB Alpha rise from it's grave.
#if defined (FBNEO_DEBUG) && (!defined FBA_DEBUG)
#define FBA_DEBUG 1
#endif

#if defined(BUILD_WIN32) || defined(BUILD_SDL) || defined(BUILD_SDL2) || defined(BUILD_MACOS)
#define INCLUDE_RUNAHEAD_SUPPORT
#endif

#if defined(BUILD_WIN32)
#define INCLUDE_REWIND_SUPPORT
#endif

#if defined (FBNEO_DEBUG)
 clock_t starttime = 0;
#endif

UINT32 nCurrentFrame;				// Framecount for emulated game

UINT32 nFramesEmulated;				// Counters for FPS	display
UINT32 nFramesRendered;
bool bForce60Hz           = false;
bool bSpeedLimit60hz      = true;
double dForcedFrameRate   = 60.00;
bool bBurnUseBlend        = true;
INT32 nBurnFPS            = 6000;
INT32 nBurnCPUSpeedAdjust = 0x0100;	// CPU speed adjustment (clock * nBurnCPUSpeedAdjust / 0x0100)

// Burn Draw:
UINT8* pBurnDraw = NULL;			// Pointer to correctly sized bitmap
INT32 nBurnPitch = 0;				// Pitch between each line
INT32 nBurnBpp;						// Bytes per pixel (2, 3, or 4)

INT32 nBurnSoundRate = 0;			// sample rate of sound or zero for no sound
INT32 nBurnSoundLen  = 0;			// length in samples per frame
INT16* pBurnSoundOut = NULL;		// pointer to output buffer

INT32 nInterpolation   = 1;			// Desired interpolation level for ADPCM/PCM sound
INT32 nFMInterpolation = 0;			// Desired interpolation level for FM sound

UINT8 nBurnLayer    = 0xFF;			// Can be used externally to select which layers to show
UINT8 nSpriteEnable = 0xFF;			// Can be used externally to select which layers to show

INT32 bRunAhead = 0;

INT32 nMaxPlayers;

bool bSaveCRoms = 0;

UINT32 *pBurnDrvPalette;

static char** pszShortName = NULL, ** pszFullNameA = NULL;
static wchar_t** pszFullNameW = NULL;

// Fix _tcscmp for non-Windows builds
#ifndef _WIN32
#define _tcscmp strcmp
#endif

bool BurnCheckMMXSupport()
{
#if defined BUILD_X86_ASM
	UINT32 nSignatureEAX = 0, nSignatureEBX = 0, nSignatureECX = 0, nSignatureEDX = 0;

	CPUID(1, nSignatureEAX, nSignatureEBX, nSignatureECX, nSignatureEDX);

	return (nSignatureEDX >> 23) & 1;						// bit 23 of edx indicates MMX support
#else
	return 0;
#endif
}

static void BurnGameListInit()
{	// Avoid broken references, RomData requires separate string storage
	if (0 == nBurnDrvCount) return;
	
		pszShortName = (char**   )malloc(nBurnDrvCount * sizeof(char*));
		pszFullNameA = (char**   )malloc(nBurnDrvCount * sizeof(char*));
		pszFullNameW = (wchar_t**)malloc(nBurnDrvCount * sizeof(wchar_t*));

		if ((NULL != pszShortName) && (NULL != pszFullNameA) && (NULL != pszFullNameW)) {
			for (UINT32 i = 0; i < nBurnDrvCount; i++) {
				pszShortName[i] = (char*   )malloc(100      * sizeof(char));
				pszFullNameA[i] = (char*   )malloc(MAX_PATH * sizeof(char));
				pszFullNameW[i] = (wchar_t*)malloc(MAX_PATH * sizeof(wchar_t));

				memset(pszShortName[i], '\0', 100      * sizeof(char));
				memset(pszFullNameA[i], '\0', MAX_PATH * sizeof(char));
				memset(pszFullNameW[i], '\0', MAX_PATH * sizeof(wchar_t));

				if (NULL != pszShortName[i]) {
					strcpy(pszShortName[i], pDriver[i]->szShortName);
					pDriver[i]->szShortName = pszShortName[i];
				}
				if (NULL != pszFullNameA[i]) {
					strcpy(pszFullNameA[i], pDriver[i]->szFullNameA);
					pDriver[i]->szFullNameA = pszFullNameA[i];
				}
#if defined (_UNICODE)
				if (NULL != pDriver[i]->szFullNameW) {
					wmemcpy(pszFullNameW[i], pDriver[i]->szFullNameW, MAX_PATH);	// Include '\0'
				}
				pDriver[i]->szFullNameW = pszFullNameW[i];
#endif
			}
		}

}

static void BurnGameListExit()
{
	// Release of storage space
	for (UINT32 i = 0; i < nBurnDrvCount; i++) {
		if ((NULL != pszShortName) && (NULL != pszShortName[i])) free(pszShortName[i]);
		if ((NULL != pszFullNameA) && (NULL != pszFullNameA[i])) free(pszFullNameA[i]);
		if ((NULL != pszFullNameW) && (NULL != pszFullNameW[i])) free(pszFullNameW[i]);
	}
	if (NULL != pszShortName) free(pszShortName);
	if (NULL != pszFullNameA) free(pszFullNameA);
	if (NULL != pszFullNameW) free(pszFullNameW);
}

extern "C" INT32 BurnLibInit()
{
	BurnLibExit();

#if defined(METAL_BUILD)
    nBurnDrvCount = 0;
    while (pDriver[nBurnDrvCount]) ++nBurnDrvCount;
#else
    nBurnDrvCount = sizeof(pDriver) / sizeof(pDriver[0]);   // count available drivers
#endif

	BurnGameListInit();

	BurnSoundInit();

	bBurnUseMMX = BurnCheckMMXSupport();

	return 0;
}

extern "C" INT32 BurnLibExit()
{
	BurnGameListExit();

	nBurnDrvCount = 0;

	return 0;
}

INT32 BurnGetZipName(char** pszName, UINT32 i)
{
	static char szFilename[MAX_PATH];
	const char* pszGameName = NULL;

	if (pszName == NULL) {
		return 1;
	}

	if (i == 0) {
		pszGameName = pDriver[nBurnDrvActive]->szShortName;
	} else {
		INT32 nOldBurnDrvSelect = nBurnDrvActive;
		UINT32 j = pDriver[nBurnDrvActive]->szBoardROM ? 1 : 0;

		// Try BIOS/board ROMs first
		if (i == 1 && j == 1) {										// There is a BIOS/board ROM
			pszGameName = pDriver[nBurnDrvActive]->szBoardROM;
		}

		if (pszGameName == NULL) {
			// Go through the list to seek out the parent
			while (j < i) {
				const char* pszParent = pDriver[nBurnDrvActive]->szParent;
				pszGameName = NULL;

				if (pszParent == NULL) {							// No parent
					break;
				}

				for (nBurnDrvActive = 0; nBurnDrvActive < nBurnDrvCount; nBurnDrvActive++) {
		            if (strcmp(pszParent, pDriver[nBurnDrvActive]->szShortName) == 0) {	// Found parent
						pszGameName = pDriver[nBurnDrvActive]->szShortName;
						break;
					}
				}

				j++;
			}
		}

		nBurnDrvActive = nOldBurnDrvSelect;
	}

	if (pszGameName == NULL) {
		*pszName = NULL;
		return 1;
	}

	strcpy(szFilename, pszGameName);

	*pszName = szFilename;

	return 0;
}

// ----------------------------------------------------------------------------
// Static functions which forward to each driver's data and functions

INT32 BurnStateMAMEScan(INT32 nAction, INT32* pnMin);
void BurnStateExit();
INT32 BurnStateInit();  // Declaration - implementation is in state.cpp

// Get the text fields for the driver in TCHARs
extern "C" TCHAR* BurnDrvGetText(UINT32 i)
{
    static TCHAR szString[MAX_PATH];
    TCHAR* pszStringW = NULL;

    if (i == 0) {
        pszStringW = (TCHAR*)pDriver[nBurnDrvActive]->szFullNameW;
    } else {
        INT32 nOldBurnDrvSelect = nBurnDrvActive;
        UINT32 j = pDriver[nBurnDrvActive]->szBoardROM ? 1 : 0;

        // Try BIOS/board ROMs first
        if (i == 1 && j == 1) {
            pszStringW = (TCHAR*)pDriver[nBurnDrvActive]->szBoardROM;
        }

        if (pszStringW == NULL) {
            // Go through the list to seek out the parent
            while (j < i) {
                const TCHAR* pszParent = pDriver[nBurnDrvActive]->szParent;
                pszStringW = NULL;

                if (pszParent == NULL) {
                    break;
                }

                for (nBurnDrvActive = 0; nBurnDrvActive < nBurnDrvCount; nBurnDrvActive++) {
                    if (_tcscmp(pszParent, pDriver[nBurnDrvActive]->szShortName) == 0) {
                        pszStringW = (TCHAR*)pDriver[nBurnDrvActive]->szFullNameW;
                        break;
                    }
                }

                j++;
            }
        }

        nBurnDrvActive = nOldBurnDrvSelect;
    }

    if (pszStringW == NULL) {
        return NULL;
    }

    _tcscpy(szString, pszStringW);

    return szString;
}


// Get the ASCII text fields for the driver in ASCII format;
extern "C" char* BurnDrvGetTextA(UINT32 i)
{
	static char szString[MAX_PATH];
	char* pszStringA = NULL;

	if (i == 0) {
		pszStringA = (char*)pDriver[nBurnDrvActive]->szFullNameA;
	} else {
		INT32 nOldBurnDrvSelect = nBurnDrvActive;
		UINT32 j = pDriver[nBurnDrvActive]->szBoardROM ? 1 : 0;

		// Try BIOS/board ROMs first
		if (i == 1 && j == 1) {
			pszStringA = (char*)pDriver[nBurnDrvActive]->szBoardROM;
		}

		if (pszStringA == NULL) {
			// Go through the list to seek out the parent
			while (j < i) {
				const char* pszParent = pDriver[nBurnDrvActive]->szParent;
				pszStringA = NULL;

				if (pszParent == NULL) {
					break;
				}

				for (nBurnDrvActive = 0; nBurnDrvActive < nBurnDrvCount; nBurnDrvActive++) {
					if (strcmp(pszParent, pDriver[nBurnDrvActive]->szShortName) == 0) {
						pszStringA = (char*)pDriver[nBurnDrvActive]->szFullNameA;
						break;
					}
				}

				j++;
			}
		}

		nBurnDrvActive = nOldBurnDrvSelect;
	}

	if (pszStringA == NULL) {
		return NULL;
	}

	strcpy(szString, pszStringA);

	return szString;
}

static INT32 BurnDrvSetFullNameA(char* szName, UINT32 i = nBurnDrvActive)
{
	// Preventing the emergence of ~0U
	// If not NULL, then FullNameA is customized
	if ((i >= 0) && (NULL != szName)) {
		memset(pszFullNameA[i], '\0', MAX_PATH * sizeof(char));
		strcpy(pszFullNameA[i], szName);

		return 0;
	}

	return -1;
}

INT32 BurnDrvSetFullNameW(TCHAR* szName, INT32 i = nBurnDrvActive)
{
	if ((-1 == i) || (NULL == szName)) return -1;

#if defined (_UNICODE)
	memset(pszFullNameW[i], '\0', MAX_PATH * sizeof(wchar_t));
	wcscpy(pszFullNameW[i], szName);
#endif

	return 0;
}

#if defined (_UNICODE)
void BurnLocalisationSetName(char* szName, TCHAR* szLongName)
{
	for (UINT32 i = 0; i < nBurnDrvCount; i++) {
		nBurnDrvActive = i;
		if (!strcmp(szName, pDriver[i]->szShortName)) {
//			pDriver[i]->szFullNameW = szLongName;
			memset(pszFullNameW[i], '\0', MAX_PATH * sizeof(wchar_t));
			_tcscpy(pszFullNameW[i], szLongName);
		}
	}
}
#endif

static void BurnLocalisationSetNameEx()
{
	if (-1 == nBurnDrvSubActive) return;

	memset(szBackupNameA, '\0', sizeof(szBackupNameA));
	strcpy(szBackupNameA, BurnDrvGetTextA(DRV_FULLNAME));
	BurnDrvSetFullNameA(pszCustomNameA);

#if defined (_UNICODE)

	const TCHAR* _str1 = _T(""), * _str2 = BurnDrvGetFullNameW(nBurnDrvActive);

	if (0 != _tcscmp(_str1, _str2)) {
		memset(szBackupNameW, _T('\0'), sizeof(szBackupNameW));
		_tcscpy(szBackupNameW, _str2);
	}

	char szShortNames[256] = { '\0'};

	snprintf(szShortNames, 256, "%s[0x%02x]", pDriver[nBurnDrvActive]->szShortName, nBurnDrvSubActive);

	for (INT32 nIndex = 0; nIndex < nNamesExArray; nIndex++) {
		if (0 == strcmp(szShortNamesExArray[nIndex], szShortNames)) {
			BurnDrvSetFullNameW(szLongNamesExArray[nIndex]);
			return;
		}
	}
#endif
}

extern "C" INT32 BurnDrvGetIndex(char* szName)
{
	if (NULL == szName) return -1;

	for (UINT32 i = 0; i < nBurnDrvCount; i++) {
		if (0 == strcmp(szName, pDriver[i]->szShortName)) {
//			nBurnDrvActive = i;
			return i;
		}
	}

	return -1;
}

extern "C" const char* BurnDrvGetFullNameW(UINT32 i)
{
	return pDriver[i]->szFullNameW;
}

// Get the zip names for the driver
extern "C" INT32 BurnDrvGetZipName(char** pszName, UINT32 i)
{
	if (pDriver[nBurnDrvActive]->pGetZipName) {									// Forward to drivers function
		return ((INT32 (*)(char**, UINT32))pDriver[nBurnDrvActive]->pGetZipName)(pszName, i);
	}

	return BurnGetZipName(pszName, i);											// Forward to general function
}

extern "C" INT32 BurnDrvSetZipName(char* szName, INT32 i)
{
	if ((NULL == szName) || (-1 == i)) return -1;

	strcpy(pszShortName[i], szName);

	return 0;
}

extern "C" INT32 BurnDrvGetRomInfo(struct BurnRomInfo* pri, UINT32 i)		// Forward to drivers function
{
	return ((INT32 (*)(struct BurnRomInfo*, UINT32))pDriver[nBurnDrvActive]->pGetRomInfo)(pri, i);
}

extern "C" INT32 BurnDrvGetRomName(char** pszName, UINT32 i, INT32 nAka)		// Forward to drivers function
{
	return ((INT32 (*)(char**, UINT32, INT32))pDriver[nBurnDrvActive]->pGetRomName)(pszName, i, nAka);
}

extern "C" INT32 BurnDrvGetInputInfo(struct BurnInputInfo* pii, UINT32 i)	// Forward to drivers function
{
	return ((INT32 (*)(struct BurnInputInfo*, UINT32))pDriver[nBurnDrvActive]->pGetInputInfo)(pii, i);
}

// Set input state for a specific input
extern "C" INT32 BurnDrvSetInput(INT32 i, INT32 nState) {
    printf("[BurnDrvSetInput] Setting input %d to state %d\n", i, nState);
    // For Metal build, just return success
    return 0;
}

extern "C" INT32 BurnDrvGetDIPInfo(struct BurnDIPInfo* pdi, UINT32 i)
{
	if (pDriver[nBurnDrvActive]->pGetDIPInfo) {									// Forward to drivers function
		return ((INT32 (*)(struct BurnDIPInfo*, UINT32))pDriver[nBurnDrvActive]->pGetDIPInfo)(pdi, i);
	}

	return 1;																	// Fail automatically
}

extern "C" INT32 BurnDrvGetSampleInfo(struct BurnSampleInfo* pri, UINT32 i)		// Forward to drivers function
{
	if (pDriver[nBurnDrvActive]->GetSampleInfo) {
		return pDriver[nBurnDrvActive]->GetSampleInfo(pri, i);
	}

	return 0;
}

// Fix parameter types for GetSampleName and GetHDDName
INT32 BurnDrvGetSampleName(char** pszName, UINT32 i, INT32 nAka)
{
    if (nBurnDrvActive < nBurnDrvCount) {
        return pDriver[nBurnDrvActive]->GetSampleName((TCHAR*)pszName, i, nAka);
    }
    return 1;
}

INT32 BurnDrvGetHDDName(char** pszName, UINT32 i, INT32 nAka)
{
    if (nBurnDrvActive < nBurnDrvCount) {
        return pDriver[nBurnDrvActive]->GetHDDName((TCHAR*)pszName, i, nAka);
    }
    return 1;
}

// Get the screen size
extern "C" INT32 BurnDrvGetVisibleSize(INT32* pnWidth, INT32* pnHeight)
{
	*pnWidth =pDriver[nBurnDrvActive]->nWidth;
	*pnHeight=pDriver[nBurnDrvActive]->nHeight;

	return 0;
}

extern "C" INT32 BurnDrvGetVisibleOffs(INT32* pnLeft, INT32* pnTop)
{
	*pnLeft = 0;
	*pnTop = 0;

	return 0;
}

extern "C" INT32 BurnDrvGetFullSize(INT32* pnWidth, INT32* pnHeight)
{
	if (pDriver[nBurnDrvActive]->nFlags & BDF_ORIENTATION_VERTICAL) {
		*pnWidth =pDriver[nBurnDrvActive]->nHeight;
		*pnHeight=pDriver[nBurnDrvActive]->nWidth;
	} else {
		*pnWidth =pDriver[nBurnDrvActive]->nWidth;
		*pnHeight=pDriver[nBurnDrvActive]->nHeight;
	}

	return 0;
}

// Get screen aspect ratio
extern "C" INT32 BurnDrvGetAspect(INT32* pnXAspect, INT32* pnYAspect)
{
	*pnXAspect = pDriver[nBurnDrvActive]->nXAspect;
	*pnYAspect = pDriver[nBurnDrvActive]->nYAspect;

	return 0;
}

extern "C" INT32 BurnDrvSetVisibleSize(INT32 pnWidth, INT32 pnHeight)
{
	if (pDriver[nBurnDrvActive]->nFlags & BDF_ORIENTATION_VERTICAL) {
		pDriver[nBurnDrvActive]->nHeight = pnWidth;
		pDriver[nBurnDrvActive]->nWidth = pnHeight;
	} else {
		pDriver[nBurnDrvActive]->nWidth = pnWidth;
		pDriver[nBurnDrvActive]->nHeight = pnHeight;
	}

	return 0;
}

extern "C" INT32 BurnDrvSetAspect(INT32 pnXAspect,INT32 pnYAspect)
{
	pDriver[nBurnDrvActive]->nXAspect = pnXAspect;
	pDriver[nBurnDrvActive]->nYAspect = pnYAspect;

	return 0;
}

// Get the hardware code
extern "C" INT32 BurnDrvGetHardwareCode()
{
	return pDriver[nBurnDrvActive]->Hardware;
}

// Get flags, including BDF_GAME_WORKING flag
extern "C" INT32 BurnDrvGetFlags()
{
	return pDriver[nBurnDrvActive]->nFlags;
}

// Return BDF_WORKING flag
extern "C" bool BurnDrvIsWorking()
{
	return pDriver[nBurnDrvActive]->nFlags & BDF_GAME_WORKING;
}

// Return max. number of players
extern "C" INT32 BurnDrvGetMaxPlayers()
{
	return pDriver[nBurnDrvActive]->nPlayers;
}

// Return genre flags
extern "C" INT32 BurnDrvGetGenreFlags()
{
	return pDriver[nBurnDrvActive]->nGenre;
}

// Return family flags
extern "C" INT32 BurnDrvGetFamilyFlags()
{
	return pDriver[nBurnDrvActive]->nFamily;
}

// Return sourcefile
extern "C" const char* BurnDrvGetSourcefile()
{
	const char* szShortName = pDriver[nBurnDrvActive]->szShortName;
	for (INT32 i = 0; sourcefile_table[i].game_name[0] != '\0'; i++) {
		if (!strcmp(sourcefile_table[i].game_name, szShortName)) {
			return sourcefile_table[i].sourcefile;
		}
	}
	return "";
}

// Save Aspect & Screensize in BurnDrvInit(), restore in BurnDrvExit()
// .. as games may need to change modes, etc.
static INT32 DrvAspectX, DrvAspectY;
static INT32 DrvX, DrvY;
static INT32 DrvCached = 0;

static void BurnCacheSizeAspect_Internal()
{
	BurnDrvGetFullSize(&DrvX, &DrvY);
	BurnDrvGetAspect(&DrvAspectX, &DrvAspectY);
	DrvCached = 1;
}

static void BurnRestoreSizeAspect_Internal()
{
	if (DrvCached) {
		BurnDrvSetVisibleSize(DrvX, DrvY);
		BurnDrvSetAspect(DrvAspectX, DrvAspectY);
		DrvCached = 0;
	}
}

// Init game emulation (loading any needed roms)
extern "C" INT32 BurnDrvInit()
{
	INT32 nReturnValue;

	if (nBurnDrvActive >= nBurnDrvCount) {
		return 1;
	}

#if defined (FBNEO_DEBUG)
	{
		TCHAR szText[1024] = _T("");
		TCHAR* pszPosition = szText;
		TCHAR* pszName = BurnDrvGetText(DRV_FULLNAME);
		INT32 nName = 1;

		while ((pszName = BurnDrvGetText(DRV_NEXTNAME | DRV_FULLNAME)) != NULL) {
			nName++;
		}

		// Print the title

		bprintf(PRINT_IMPORTANT, _T("*** Starting emulation of %s - %s.\n"), BurnDrvGetText(DRV_NAME), BurnDrvGetText(DRV_FULLNAME));

#ifdef BUILD_A68K
		if (bBurnUseASMCPUEmulation)
			bprintf(PRINT_ERROR, _T("*** WARNING: Assembly MC68000 core is enabled for this session!\n"));
#endif

		// Then print the alternative titles

		if (nName > 1) {
			bprintf(PRINT_IMPORTANT, _T("    Alternative %s "), (nName > 2) ? _T("titles are") : _T("title is"));
			pszName = BurnDrvGetText(DRV_FULLNAME);
			nName = 1;
			while ((pszName = BurnDrvGetText(DRV_NEXTNAME | DRV_FULLNAME)) != NULL) {
				if (pszPosition + _tcslen(pszName) - 1022 > szText) {
					break;
				}
				if (nName > 1) {
					bprintf(PRINT_IMPORTANT, _T(SEPERATOR_1));
				}
				bprintf(PRINT_IMPORTANT, _T("%s"), pszName);
				nName++;
			}
			bprintf(PRINT_IMPORTANT, _T(".\n"));
		}
	}
#endif

	BurnSetMouseDivider(1);

	BurnSetRefreshRate(60.0);

	BurnCacheSizeAspect_Internal();

	CheatInit();
	HiscoreInit();
	BurnStateInit();
#if defined (INCLUDE_RUNAHEAD_SUPPORT)
	StateRunAheadInit();
#endif
#if defined (INCLUDE_REWIND_SUPPORT)
	StateRewindInit();
#endif
	BurnInitMemoryManager();
	BurnRandomInit();
	BurnSoundDCFilterReset();
	BurnTimerPreInit();

	nReturnValue = pDriver[nBurnDrvActive]->Init();	// Forward to drivers function

	if (-1 != nBurnDrvSubActive) {
		BurnLocalisationSetNameEx();
	}

	nMaxPlayers = pDriver[nBurnDrvActive]->nPlayers;

	nCurrentFrame = 0;

#if defined (FBNEO_DEBUG)
	if (!nReturnValue) {
		starttime = clock();
		nFramesEmulated = 0;
		nFramesRendered = 0;
	} else {
		starttime = 0;
	}
#endif

	return nReturnValue;
}

// Exit game emulation
extern "C" INT32 BurnDrvExit()
{
#if defined (FBNEO_DEBUG)
	if (starttime) {
		clock_t endtime;
		clock_t nElapsedSecs;

		endtime = clock();
		nElapsedSecs = (endtime - starttime);
		bprintf(PRINT_IMPORTANT, _T(" ** Emulation ended (running for %.2f seconds).\n"), (float)nElapsedSecs / CLOCKS_PER_SEC);
		bprintf(PRINT_IMPORTANT, _T("    %.2f%% of frames rendered (%d out of a total %d).\n"), (float)nFramesRendered / nFramesEmulated * 100, nFramesRendered, nFramesEmulated);
		bprintf(PRINT_IMPORTANT, _T("    %.2f frames per second (average).\n"), (float)nFramesRendered / nFramesEmulated * nBurnFPS / 100);
		bprintf(PRINT_NORMAL, _T("\n"));
	}
#endif

	HiscoreExit(); // must come before CheatExit() (uses cheat cpu-registry)
	CheatExit();
	CheatSearchExit();
	BurnStateExit();
#if defined (INCLUDE_RUNAHEAD_SUPPORT)
	StateRunAheadExit();
#endif
#if defined (INCLUDE_REWIND_SUPPORT)
	StateRewindExit();
#endif

	nBurnCPUSpeedAdjust = 0x0100;

	pBurnDrvPalette = NULL;

	if (-1 != nBurnDrvSubActive) {
		pszCustomNameA = szBackupNameA;
		BurnDrvSetFullNameA(szBackupNameA);
		pszCustomNameA = NULL;

#if defined (_UNICODE)
		const wchar_t* _str1 = L"", * _str2 = BurnDrvGetFullNameW(nBurnDrvActive);

		if (0 != _tcscmp(_str1, _str2)) {
			BurnDrvSetFullNameW(szBackupNameW);
		}
#endif
	}

	INT32 nRet = pDriver[nBurnDrvActive]->Exit();			// Forward to drivers function

	nBurnDrvSubActive = -1;	// Rest to -1;

	BurnExitMemoryManager();
#if defined FBNEO_DEBUG
	DebugTrackerExit();
#endif

	BurnRestoreSizeAspect_Internal();

	return nRet;
}

INT32 (*BurnExtCartridgeSetupCallback)(BurnCartrigeCommand nCommand) = NULL;

INT32 BurnDrvCartridgeSetup(BurnCartrigeCommand nCommand)
{
	if (nBurnDrvActive >= nBurnDrvCount || BurnExtCartridgeSetupCallback == NULL) {
		return 1;
	}

	if (nCommand == CART_EXIT) {
		return pDriver[nBurnDrvActive]->Exit();
	}

	if (nCommand != CART_INIT_END && nCommand != CART_INIT_START) {
		return 1;
	}

	BurnExtCartridgeSetupCallback(CART_INIT_END);

#if defined FBNEO_DEBUG
		bprintf(PRINT_NORMAL, _T("  * Loading Cartridge\n"));
#endif

	if (BurnExtCartridgeSetupCallback(CART_INIT_START)) {
		return 1;
	}

	if (nCommand == CART_INIT_START) {
		return pDriver[nBurnDrvActive]->Init();
	}

	return 0;
}

// Do one frame of game emulation
extern "C" INT32 BurnDrvFrame()
{
	CheatApply();									// Apply cheats (if any)
	HiscoreApply();
	return pDriver[nBurnDrvActive]->Frame();		// Forward to drivers function
}

// Force redraw of the screen
extern "C" INT32 BurnDrvRedraw()
{
	if (pDriver[nBurnDrvActive]->Redraw) {
		return pDriver[nBurnDrvActive]->Redraw();	// Forward to drivers function
	}

	return 1;										// No funtion provide, so simply return
}

// Refresh Palette
extern "C" INT32 BurnRecalcPal()
{
	if (nBurnDrvActive < nBurnDrvCount) {
		UINT8* pr = pDriver[nBurnDrvActive]->pRecalcPal;
		if (pr == NULL) return 1;
		*pr = 1;									// Signal for the driver to refresh it's palette
	}

	return 0;
}

extern "C" INT32 BurnDrvGetPaletteEntries()
{
	return pDriver[nBurnDrvActive]->nPaletteEntries;
}

// ----------------------------------------------------------------------------

INT32 (*BurnExtProgressRangeCallback)(double fProgressRange) = NULL;
INT32 (*BurnExtProgressUpdateCallback)(double fProgress, const TCHAR* pszText, bool bAbs) = NULL;

INT32 BurnSetProgressRange(double fProgressRange)
{
	if (BurnExtProgressRangeCallback) {
		return BurnExtProgressRangeCallback(fProgressRange);
	}

	return 1;
}

INT32 BurnUpdateProgress(double fProgress, const TCHAR* pszText, bool bAbs)
{
	if (BurnExtProgressUpdateCallback) {
		return BurnExtProgressUpdateCallback(fProgress, pszText, bAbs);
	}

	return 1;
}

// ----------------------------------------------------------------------------
// NOTE: Make sure this is called before any soundcore init!
struct MovieExtInfo
{
	// date & time
	UINT32 year, month;
	UINT16 day, dayofweek;
	UINT32 hour, minute, second;
};

#if !defined(BUILD_SDL) && !defined(BUILD_SDL2) && !defined(BUILD_MACOS)
extern struct MovieExtInfo MovieInfo; // from replay.cpp
#else
struct MovieExtInfo MovieInfo = { 0, 0, 0, 0, 0, 0, 0 };
#endif

void BurnGetLocalTime(tm *nTime)
{
	if (is_netgame_or_recording()) {
		if (is_netgame_or_recording() & 2) { // recording/playback
			nTime->tm_sec = MovieInfo.second;
			nTime->tm_min = MovieInfo.minute;
			nTime->tm_hour = MovieInfo.hour;
			nTime->tm_mday = MovieInfo.day;
			nTime->tm_wday = MovieInfo.dayofweek;
			nTime->tm_mon = MovieInfo.month;
			nTime->tm_year = MovieInfo.year;
		} else {
			nTime->tm_sec = 0; // defaults for netgame
			nTime->tm_min = 0;
			nTime->tm_hour = 0;
			nTime->tm_mday = 1;
			nTime->tm_wday = 3;
			nTime->tm_mon = 6 - 1;
			nTime->tm_year = 2018;
		}
	} else {
		time_t nLocalTime = time(NULL); // query current time from this machine
		tm* tmLocalTime = localtime(&nLocalTime);
		memcpy(nTime, tmLocalTime, sizeof(tm));
	}
}


// ----------------------------------------------------------------------------
// State-able random generator, based on early BSD LCG rand
static UINT64 nBurnRandSeed = 0;

UINT16 BurnRandom()
{
	nBurnRandSeed = nBurnRandSeed * 1103515245 + 12345;

	return (UINT32)(nBurnRandSeed / 65536) % 0x10000;
}

void BurnRandomScan(INT32 nAction)
{
	if (nAction & ACB_DRIVER_DATA) {
		SCAN_VAR(nBurnRandSeed);
	}
}

void BurnRandomSetSeed(UINT64 nSeed)
{
	nBurnRandSeed = nSeed;
}

void BurnRandomInit()
{ // for states & input recordings - init before emulation starts
	if (is_netgame_or_recording()) {
		BurnRandomSetSeed(0x303808909313ULL);
	} else {
		BurnRandomSetSeed(time(NULL));
	}
}

// ----------------------------------------------------------------------------
// Handy FM default callbacks

INT32 BurnSynchroniseStream(INT32 nSoundRate)
{
	return (INT64)BurnTimerCPUTotalCycles() * nSoundRate / BurnTimerCPUClockspeed;
}

double BurnGetTime()
{
	return (double)BurnTimerCPUTotalCycles() / BurnTimerCPUClockspeed;
}

// CPU Speed adjuster
INT32 BurnSpeedAdjust(INT32 cyc)
{
	return (INT32)((INT64)cyc * nBurnCPUSpeedAdjust / 0x0100);
}

// ----------------------------------------------------------------------------
// Wrappers for MAME-specific function calls

#include "driver.h"

// ----------------------------------------------------------------------------
// Wrapper for MAME logerror calls

#if defined (FBNEO_DEBUG) && defined (MAME_USE_LOGERROR)
void logerror(char* szFormat, ...)
{
	static char szLogMessage[1024];

	va_list vaFormat;
	va_start(vaFormat, szFormat);

	_vsnprintf(szLogMessage, 1024, szFormat, vaFormat);

	va_end(vaFormat);

	bprintf(PRINT_ERROR, _T("%hs"), szLogMessage);

	return;
}
#endif

#if defined (FBNEO_DEBUG)
void BurnDump_(char *filename, UINT8 *buffer, INT32 bufsize, INT32 append)
{
	FILE *f = fopen(filename, (append) ? "a+b" : "wb+");
	if (f) {
		fwrite(buffer, 1, bufsize, f);
		fclose(f);
	} else {
		bprintf(PRINT_ERROR, _T(" - BurnDump() - Error writing file.\n"));
	}
}

void BurnDumpLoad_(char *filename, UINT8 *buffer, INT32 bufsize)
{
	FILE *f = fopen(filename, "rb+");
	if (f) {
		fread(buffer, 1, bufsize, f);
		fclose(f);
	} else {
		bprintf(PRINT_ERROR, _T(" - BurnDumpLoad() - File not found.\n"));
	}
}
#endif

// ----------------------------------------------------------------------------
// Wrapper for MAME state_save_register_* calls

struct BurnStateEntry { BurnStateEntry* pNext; BurnStateEntry* pPrev; char szName[256]; void* pValue; UINT32 nSize; };

static BurnStateEntry* pStateEntryAnchor = NULL;
typedef void (*BurnPostloadFunction)();
static BurnPostloadFunction BurnPostload[8];

static void BurnStateRegister(const char* module, INT32 instance, const char* name, void* val, UINT32 size)
{
	// Allocate new node
	BurnStateEntry* pNewEntry = (BurnStateEntry*)BurnMalloc(sizeof(BurnStateEntry));
	if (pNewEntry == NULL) {
		return;
	}

	memset(pNewEntry, 0, sizeof(BurnStateEntry));

	// Link the new node
	pNewEntry->pNext = pStateEntryAnchor;
	if (pStateEntryAnchor) {
		pStateEntryAnchor->pPrev = pNewEntry;
	}
	pStateEntryAnchor = pNewEntry;

	snprintf(pNewEntry->szName, 256, "%s:%s %i", module, name, instance);

	pNewEntry->pValue = val;
	pNewEntry->nSize = size;
}


INT32 BurnStateMAMEScan(INT32 nAction, INT32* pnMin)
{
	if (nAction & ACB_VOLATILE) {

		if (pnMin && *pnMin < 0x029418) {						// Return minimum compatible version
			*pnMin = 0x029418;
		}

		if (pStateEntryAnchor) {
			struct BurnArea ba;
			BurnStateEntry* pCurrentEntry = pStateEntryAnchor;

			do {
			   	ba.Data		= pCurrentEntry->pValue;
				ba.nLen		= pCurrentEntry->nSize;
				ba.nAddress = 0;
				ba.szName	= pCurrentEntry->szName;
				BurnAcb(&ba);

			} while ((pCurrentEntry = pCurrentEntry->pNext) != 0);
		}

		if (nAction & ACB_WRITE) {
			for (INT32 i = 0; i < 8; i++) {
				if (BurnPostload[i]) {
					BurnPostload[i]();
				}
			}
		}
	}

	return 0;
}

// wrapper functions

extern "C" void state_save_register_func_postload(void (*pFunction)())
{
	for (INT32 i = 0; i < 8; i++) {
		if (BurnPostload[i] == NULL) {
			BurnPostload[i] = pFunction;
			break;
		}
	}
}

extern "C" void state_save_register_INT8(const char* module, INT32 instance, const char* name, INT8* val, UINT32 size)
{
	BurnStateRegister(module, instance, name, (void*)val, size * sizeof(INT8));
}

extern "C" void state_save_register_UINT8(const char* module, INT32 instance, const char* name, UINT8* val, UINT32 size)
{
	BurnStateRegister(module, instance, name, (void*)val, size * sizeof(UINT8));
}

extern "C" void state_save_register_INT16(const char* module, INT32 instance, const char* name, INT16* val, UINT32 size)
{
	BurnStateRegister(module, instance, name, (void*)val, size * sizeof(INT16));
}

extern "C" void state_save_register_UINT16(const char* module, INT32 instance, const char* name, UINT16* val, UINT32 size)
{
	BurnStateRegister(module, instance, name, (void*)val, size * sizeof(UINT16));
}

extern "C" void state_save_register_INT32(const char* module, INT32 instance, const char* name, INT32* val, UINT32 size)
{
	BurnStateRegister(module, instance, name, (void*)val, size * sizeof(INT32));
}

extern "C" void state_save_register_UINT32(const char* module, INT32 instance, const char* name, UINT32* val, UINT32 size)
{
	BurnStateRegister(module, instance, name, (void*)val, size * sizeof(UINT32));
}

extern "C" void state_save_register_int(const char* module, INT32 instance, const char* name, INT32* val)
{
	BurnStateRegister(module, instance, name, (void*)val, sizeof(INT32));
}

extern "C" void state_save_register_float(const char* module, INT32 instance, const char* name, float* val, UINT32 size)
{
	BurnStateRegister(module, instance, name, (void*)val, size * sizeof(float));
}

extern "C" void state_save_register_double(const char* module, INT32 instance, const char* name, double* val, UINT32 size)
{
	BurnStateRegister(module, instance, name, (void*)val, size * sizeof(double));
}

int bDrvOkay = 0;

// Add missing function
int is_netgame_or_recording() {
    return 0;
}

// Add missing declarations
extern int is_netgame_or_recording();
extern int nInputIntfMouseDivider;

// Remove the overloaded version and keep only one implementation
void BurnSetRefreshRate(double dFrameRate)
{
    if (dFrameRate <= 0.0) {
        return;
    }
    dFrameRate = dFrameRate * 100.0;
    dFrameRate = (double)(INT32)(dFrameRate + 0.5);
    dFrameRate = dFrameRate / 100.0;
    nBurnFPS = (INT32)(100.0 / dFrameRate);
    nBurnFPS = 100 * nBurnFPS;
    nBurnFPS = nBurnFPS / 100;
}

// Fix string literal conversion warnings
const TCHAR* GetDriverName(INT32 i) {
    if (i < 0 || i >= nBurnDrvCount) {
        return _T("");
    }
    return (const TCHAR*)pDriver[i]->szFullNameW;
}

// Initialize FBNeo core
INT32 BurnInit() {
    printf("[BurnInit] Initializing FBNeo core\n");
    
    // Initialize memory manager
    BurnInitMemoryManager();
    
    // Initialize input system
    extern INT32 BurnInputInit();
    BurnInputInit();
    
    // Initialize sound system
    BurnSoundInit();
    
    printf("[BurnInit] Core initialization complete\n");
    return 0;
}

// Select a driver by index
extern "C" INT32 BurnDrvSelect(INT32 nDrvNum) {
    printf("[BurnDrvSelect] Selecting driver %d\n", nDrvNum);
    
    if (nDrvNum < 0 || nDrvNum >= nBurnDrvCount) {
        printf("[BurnDrvSelect] ERROR: Invalid driver number %d (max: %d)\n", nDrvNum, nBurnDrvCount - 1);
        return 1;
    }
    
    if (!pDriver || !pDriver[nDrvNum]) {
        printf("[BurnDrvSelect] ERROR: Driver %d is NULL\n", nDrvNum);
        return 1;
    }
    
    nBurnDrvActive = nDrvNum;
    printf("[BurnDrvSelect] Selected driver %d: %s\n", nDrvNum, 
           pDriver[nDrvNum]->szShortName ? pDriver[nDrvNum]->szShortName : "Unknown");
    
    return 0;
}

// Find a driver by short name
extern "C" INT32 BurnDrvFind(const char* szName) {
    printf("[BurnDrvFind] Searching for driver: '%s'\n", szName ? szName : "NULL");
    
    if (!szName) {
        printf("[BurnDrvFind] ERROR: Driver name is NULL\n");
        return -1;
    }
    
    if (!pDriver) {
        printf("[BurnDrvFind] ERROR: pDriver array is NULL\n");
        return -1;
    }
    
    for (UINT32 i = 0; i < nBurnDrvCount; i++) {
        if (pDriver[i] && pDriver[i]->szShortName) {
            if (strcmp(pDriver[i]->szShortName, szName) == 0) {
                printf("[BurnDrvFind] Found driver '%s' at index %d\n", szName, i);
                return i;
            }
        }
    }
    
    printf("[BurnDrvFind] Driver '%s' not found\n", szName);
    return -1;
}

// External declarations
