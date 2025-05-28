#include "metal_exports.h"
#include "burner_metal.h"
#include <stdio.h>
#include <string.h>

// External burn function declarations
extern "C" {
    int BurnLibInit();
    int BurnLibExit();
    int BurnDrvInit();
    int BurnDrvExit();
    int BurnDrvFrame();
    int BurnDrvReset();
    char* BurnDrvGetTextA(unsigned int i);
    unsigned int BurnDrvGetFlags();
}

// Global variables (normally from burn.h)
unsigned int nBurnDrvActive = 0;
unsigned int nBurnDrvCount = 0;

// BurnLib initialization (Metal version)
int BurnLibInit_Metal() {
    // Initialize main burn library
    int nRet = BurnLibInit();
    if (nRet != 0) {
        printf("Error initializing BurnLib: %d\n", nRet);
        return nRet;
    }
    
    // Print burn library info
    printf("FBNeo Metal initialization completed.\n");
    printf("Version: %s\n", szAppBurnVer);
    printf("Available drivers: %d\n", nBurnDrvCount);
    
    return 0;
}

// BurnLib exit (Metal version)
int BurnLibExit_Metal() {
    // Cleanup burn library
    int nRet = BurnLibExit();
    if (nRet != 0) {
        printf("Error during BurnLib exit: %d\n", nRet);
        return nRet;
    }
    
    printf("FBNeo Metal shutdown completed.\n");
    return 0;
}

// Driver initialization wrapper
int BurnDrvInit_Metal(int nDrvNum) {
    nBurnDrvActive = nDrvNum;
    return BurnDrvInit();
}

// Driver exit wrapper
int BurnDrvExit_Metal() {
    return BurnDrvExit();
}

// Driver frame wrapper
int BurnDrvFrame_Metal(int bDraw) {
    return BurnDrvFrame();
}

// Driver reset wrapper
int BurnDrvReset_Metal() {
    return BurnDrvReset();
}

// Get driver index wrapper
int BurnGetDriverIndex_Metal(const char* szName) {
    for (unsigned int i = 0; i < nBurnDrvCount; i++) {
        nBurnDrvActive = i;
        if (BurnDrvGetTextA(DRV_NAME) && strcmp(BurnDrvGetTextA(DRV_NAME), szName) == 0) {
            return i;
        }
    }
    return -1;
}

// Get driver text wrapper
char* BurnDrvGetTextA_Metal(unsigned int i) {
    return BurnDrvGetTextA(i);
}

// Get driver flags wrapper
unsigned int BurnDrvGetFlags_Metal() {
    return BurnDrvGetFlags();
} 