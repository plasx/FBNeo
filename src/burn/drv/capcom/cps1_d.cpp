#include "burnint.h"
#include "burn.h"

// Minimal implementation of cps1_d.cpp for the Metal build
// This file contains the driver for CPS1 system

// We need just enough to make the build work
static INT32 MvscInit()
{
    return 0;
}

static INT32 MvscExit()
{
    return 0;
}

static INT32 MvscFrame()
{
    return 0;
}

// Define the driver manually for the Metal build
struct BurnDriver BurnDrvCpsMvsc = {
    (char*)"mvsc", NULL, NULL, NULL, (char*)"1998",
    (char*)"Marvel vs. Capcom: Clash of Super Heroes (USA 980123)",
    NULL, (char*)"Capcom", (char*)"CPS2",
    NULL, NULL, NULL, NULL,
    BDF_GAME_WORKING, 2, HARDWARE_CAPCOM_CPS2, GBF_VSFIGHT, FBF_SF,
    NULL, NULL, NULL, NULL, NULL, NULL, MvscInit, MvscExit, MvscFrame, NULL, NULL, NULL,
    320, 240, 4, 3
}; 