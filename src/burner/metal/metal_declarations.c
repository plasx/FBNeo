#include "metal_declarations.h"

// Define a minimal BurnDrvMeta struct for C compatibility
// Just include the fields we need to access in Metal code
struct BurnDrvMeta {
    int nWidth;
    int nHeight;
    int nAspectX;
    int nAspectY;
    char szShortName[32];
    char szFullName[256];
};

// Define global variables declared in metal_declarations.h
char szAppRomPaths[DIRS_MAX][MAX_PATH] = {{0}};
char szAppDirPath[MAX_PATH] = {0};
struct BurnDrvMeta BurnDrvInfo = {0};

// Frame buffer variables
UINT8* pBurnDraw_Metal = NULL;
INT32 nBurnPitch_Metal = 0;
INT32 nBurnBpp_Metal = 0;
