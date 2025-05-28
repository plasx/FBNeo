#include "burnint.h"
#include "burn.h"
#include "../metal_intf.h"

#ifdef __cplusplus
extern "C" {
#endif
// Basic driver implementation for Metal build
INT32 is_netgame_or_recording() { return 0; }
#ifdef __cplusplus
}
#endif

bool bDoIpsPatch = false;
UINT32 nIpsMemExpLen[9];
char szAppHiscorePath[MAX_PATH] = "";
TCHAR szAppRomPaths[DIRS_MAX][MAX_PATH] = { 
    { "/usr/local/share/roms/" }, 
    { "roms/" }, 
};
int bDrvOkay = 0;
int nBurnDrvActive = 0;
int nBurnDrvCount = 1;

int DrvInit(int nDrvNum, bool bRestore) {
    printf("Initializing driver %d\n", nDrvNum);
    nBurnDrvActive = nDrvNum;
    nMaxPlayers = BurnDrvGetMaxPlayers();
    InputMake(true);
    if (BurnDrvInit()) { 
        BurnDrvExit(); 
        return 1; 
    }
    
    printf("Driver initialized successfully\n");
    bDrvOkay = 1;
    return 0;
}

int DrvExit() {
    if (bDrvOkay) {
        if (nBurnDrvActive < nBurnDrvCount) {
            BurnDrvExit();
        }
    }
    bDrvOkay = 0;
    return 0;
}

int MediaInit() { return 0; }
int MediaExit() { return 0; }
void RunReset() {}

struct MovieInfo { bool bRecord; bool bReadOnly; bool bFrameLossy; };
MovieInfo MovieInfo = { false, false, false }; 