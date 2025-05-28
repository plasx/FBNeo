#ifndef _BURNER_H
#define _BURNER_H

// Basic constants
#define DIRS_MAX  20  // Maximum number of directories to search

// Basic types
#ifndef _TCHAR_DEFINED
#define _TCHAR_DEFINED
typedef char TCHAR;
#endif

typedef int INT32;

#define TEXT(a) a
#define MAX_PATH 260

// Forward declarations for required components
extern TCHAR szAppBurnVer[16];
extern int AppInitialise();
extern int AppExit();
extern int GameInpInit();
extern int GameInpExit();
extern int InpInit();
extern int InpExit();

// Function prototypes
INT32 BurnLibInit();
INT32 BurnLibExit();

#endif // _BURNER_H 