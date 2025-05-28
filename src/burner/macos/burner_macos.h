#ifndef _BURNER_MACOS_H_
#define _BURNER_MACOS_H_

#include "../../intf/interface.h" // Include interface.h first to get RECT definition
#include "../../burn/burn.h"
#include "../platform_macros.h" // Include shared macros

#ifdef __cplusplus
extern "C" {
#endif

// Basic type definitions
typedef unsigned int UINT32;
typedef unsigned short UINT16;
typedef unsigned char UINT8;
typedef signed int INT32;
typedef signed short INT16;
typedef signed char INT8;

// Pointer types for RECT
typedef RECT* PRECT;
typedef RECT* LPRECT;
typedef const RECT* LPCRECT;

// Maximum DIP options
#define MAXDIPOPTIONS 32

// DIP switch group structure
struct GroupOfDIPSwitches {
    BurnDIPInfo dipSwitch;
    UINT16 DefaultDIPOption;
    UINT16 SelectedDIPOption;
    char OptionsNamesWithCheckBoxes[MAXDIPOPTIONS][64];
    BurnDIPInfo dipSwitchesOptions[MAXDIPOPTIONS];
};

// String set class
class StringSet {
public:
    TCHAR* szText;
    int nLen;
    int __cdecl Add(TCHAR* szFormat, ...);
    int Reset();
    StringSet();
    ~StringSet();
};

// Function declarations
int InputInit();
int InputExit();
int InputMake(bool bCopy);

// Array for ROM paths
extern TCHAR szAppRomPaths[DIRS_MAX][MAX_PATH];

// SDL functions
unsigned int SDL_GetTicks();
void SDL_Delay(unsigned int ms);

// Version string
#define szAppBurnVer "1"

#ifdef __cplusplus
}
#endif

#endif // BURNER_MACOS_H
