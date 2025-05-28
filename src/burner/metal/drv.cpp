#include "burnint.h"

// Driver interface stubs
int bDrvOkay = 0;

int DrvInit(int nDrvNum, bool bRestore) {
    printf("DrvInit() called: %d, restore=%d\n", nDrvNum, bRestore);
    nBurnDrvActive = nDrvNum;
    return 0;
}

int DrvExit() {
    printf("DrvExit() called\n");
    return 0;
}

int DrvFrame() {
    return 0;
} 