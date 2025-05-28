// Final Burn Neo - Metal build wrapper for burn.cpp
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "burnint.h"
#include "burner/metal/metal_declarations.h"

// Direct connection to real FBNeo functions
extern "C" {
    // These functions directly call the real FBNeo implementation
    INT32 BurnLibInit_Metal() {
        printf("BurnLibInit_Metal: Initializing FBNeo core\n");
        return BurnLibInit();
    }
    
    INT32 BurnLibExit_Metal() {
        printf("BurnLibExit_Metal: Shutting down FBNeo core\n");
        return BurnLibExit();
    }
    
    INT32 BurnDrvInit_Metal(INT32 nDrvNum) {
        printf("BurnDrvInit_Metal: Loading driver #%d\n", nDrvNum);
        nBurnDrvActive = nDrvNum;
        return BurnDrvInit();
    }
    
    INT32 BurnDrvExit_Metal() {
        printf("BurnDrvExit_Metal: Unloading driver\n");
        return BurnDrvExit();
    }
    
    INT32 BurnDrvFrame_Metal(INT32 bDraw) {
        // Just pass through to the real implementation
        return BurnDrvFrame();
    }
    
    INT32 BurnDrvReset_Metal() {
        printf("BurnDrvReset_Metal: Resetting driver\n");
        return BurnDrvReset();
    }
    
    char* BurnDrvGetTextA_Metal(UINT32 i) {
        return BurnDrvGetTextA(i);
    }
    
    TCHAR* BurnDrvGetText_Metal(UINT32 i) {
        return BurnDrvGetText(i);
    }
    
    INT32 BurnDrvGetZipName_Metal(char** pszName, UINT32 i) {
        return BurnDrvGetZipName(pszName, i);
    }
} 