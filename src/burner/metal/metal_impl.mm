#import <Cocoa/Cocoa.h>
#include "burnint.h"

typedef bool BOOL_OBJC;  // To avoid conflict with the Objective-C BOOL

// Define NULL if not already defined
#ifndef NULL
#define NULL 0
#endif

// Metal implementation stubs
TCHAR szAppBurnVer[16] = TEXT("Metal 0.1");
TCHAR szAppRomPaths[DIRS_MAX][MAX_PATH] = { { _T("roms/") } };

int AppInitialise() {
    printf("Metal backend initialized\n");
    return 0;
}

int AppExit() {
    printf("Metal backend exited\n");
    return 0;
}

int GameInpInit() {
    return 0;
}

int GameInpExit() {
    return 0;
} 