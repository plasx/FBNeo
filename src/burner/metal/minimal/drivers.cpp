#include "burnint.h"
#include "burn.h"
#include "../../../dep/generated/driverlist.h"

// This is a simple drivers file for Metal build

struct BurnDriver* BurnDrvGetNext(int nPrev) {
    if (nPrev < 0 || nPrev >= nBurnDrvCount) { return NULL; }
    return pDriver[nPrev];
}

TCHAR* BurnDrvGetText(unsigned int i) {
    if (nBurnDrvActive < nBurnDrvCount) { return pDriver[nBurnDrvActive]->szText[i]; }
    return NULL;
}

int BurnDrvGetMaxPlayers() {
    if (nBurnDrvActive < nBurnDrvCount) { return pDriver[nBurnDrvActive]->nMaxPlayers; }
    return 0;
}

BurnRomInfo* BurnDrvGetRomInfo(unsigned int* pnOffset, unsigned int i) {
    if (nBurnDrvActive < nBurnDrvCount) { return pDriver[nBurnDrvActive]->GetRomInfo(pnOffset, i); }
    return NULL;
}

char* BurnDrvGetRomName(unsigned int i) {
    if (nBurnDrvActive < nBurnDrvCount) { return pDriver[nBurnDrvActive]->GetRomName(i); }
    return NULL;
}

int BurnDrvGetZipName(char** pszName, unsigned int i) {
    if (nBurnDrvActive < nBurnDrvCount) { return pDriver[nBurnDrvActive]->GetZipName(pszName, i); }
    return 1;
}

int BurnDrvInit() {
    if (nBurnDrvActive < nBurnDrvCount) { return pDriver[nBurnDrvActive]->Init(); }
    return 1;
}

int BurnDrvExit() {
    if (nBurnDrvActive < nBurnDrvCount) { return pDriver[nBurnDrvActive]->Exit(); }
    return 0;
}

void BurnDrvGetVisibleSize(int* pnWidth, int* pnHeight) {
    if (nBurnDrvActive < nBurnDrvCount) {
        if (pDriver[nBurnDrvActive]->nWidth && pDriver[nBurnDrvActive]->nHeight) {
            *pnWidth = pDriver[nBurnDrvActive]->nWidth;
            *pnHeight = pDriver[nBurnDrvActive]->nHeight;
            return;
        }
    }
    *pnWidth = 320;
    *pnHeight = 240;
} 