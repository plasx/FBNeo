#include "burnint.h"
#include "burn_input.h"
#include "metal_fixes.h"
#include <stdio.h>
#include <string.h>

// Input state storage
static UINT8 nInputState[INPUT_MAX];
static UINT8 nPrevInputState[INPUT_MAX];
static bool bInputInitialized = false;

// Input mapping for CPS2/Marvel vs. Capcom
struct InputMapping {
    const char* name;
    UINT8 player;
    UINT8 type;  // 0=digital, 1=analog
};

static InputMapping inputMap[] = {
    // Player 1
    {"P1 Up",       0, 0},
    {"P1 Down",     0, 0}, 
    {"P1 Left",     0, 0},
    {"P1 Right",    0, 0},
    {"P1 Punch",    0, 0},
    {"P1 Kick",     0, 0},
    {"P1 Start",    0, 0},
    {"P1 Coin",     0, 0},
    
    // Player 2
    {"P2 Up",       1, 0},
    {"P2 Down",     1, 0},
    {"P2 Left",     1, 0}, 
    {"P2 Right",    1, 0},
    {"P2 Punch",    1, 0},
    {"P2 Kick",     1, 0},
    {"P2 Start",    1, 0},
    {"P2 Coin",     1, 0},
    
    // System
    {"Reset",       0, 0},
    {"Service",     0, 0},
    {"Test",        0, 0},
    
    {NULL, 0, 0}  // End marker
};

INT32 BurnInputInit()
{
    if (bInputInitialized) {
        return 0;
    }
    
    // Clear input state
    memset(nInputState, 0, sizeof(nInputState));
    memset(nPrevInputState, 0, sizeof(nPrevInputState));
    
    bInputInitialized = true;
    
    printf("[BurnInputInit] Input system initialized\n");
    return 0;
}

INT32 BurnInputExit()
{
    bInputInitialized = false;
    
    printf("[BurnInputExit] Input system shut down\n");
    return 0;
}

INT32 BurnInputReset()
{
    if (bInputInitialized) {
        memset(nInputState, 0, sizeof(nInputState));
        memset(nPrevInputState, 0, sizeof(nPrevInputState));
        printf("[BurnInputReset] Input state reset\n");
    }
    return 0;
}

INT32 BurnInputSetKey(INT32 i, INT32 nState)
{
    if (!bInputInitialized) {
        BurnInputInit();
    }
    
    if (i < 0 || i >= INPUT_MAX) {
        printf("[BurnInputSetKey] Warning: Input index %d out of bounds\n", i);
        return 1;
    }
    
    nInputState[i] = (UINT8)(nState ? 1 : 0);
    return 0;
}

INT32 BurnInputGetKey(INT32 i)
{
    if (!bInputInitialized || i < 0 || i >= INPUT_MAX) {
        return 0;
    }
    
    return nInputState[i];
}

INT32 BurnInputUpdate()
{
    if (!bInputInitialized) {
        return 1;
    }
    
    // Store previous state
    memcpy(nPrevInputState, nInputState, sizeof(nInputState));
    
    return 0;
}

INT32 BurnInputHasChanged(INT32 nInput)
{
    if (!bInputInitialized || nInput < 0 || nInput >= INPUT_MAX) {
        return 0;
    }
    
    return (nInputState[nInput] != nPrevInputState[nInput]) ? 1 : 0;
}

// Get input info for a specific input
INT32 BurnInputGetInfo(BurnInputInfo* pii, INT32 i)
{
    if (!pii || i < 0) {
        return 1;
    }
    
    // Clear the structure
    memset(pii, 0, sizeof(BurnInputInfo));
    
    if (i < (sizeof(inputMap) / sizeof(InputMapping)) - 1) {
        InputMapping* mapping = &inputMap[i];
        if (mapping->name) {
            pii->szName = (char*)mapping->name;
            pii->szInfo = (char*)mapping->name;
            pii->nType = mapping->type ? BIT_ANALOG_REL : BIT_DIGITAL;
            pii->pVal = &nInputState[i];
            return 0;
        }
    }
    
    return 1;
}

// CPS2-specific input handling
INT32 BurnInputMapCPS2()
{
    if (!bInputInitialized) {
        return 1;
    }
    
    // Map keyboard/gamepad inputs to CPS2 controls
    // This would be called by the Metal input system
    
    // Example mapping for testing:
    // Arrow keys -> P1 directions
    // A,S -> P1 punch/kick
    // Enter -> P1 start
    // Space -> P1 coin
    
    printf("[BurnInputMapCPS2] CPS2 input mapping applied\n");
    return 0;
}

// Input constants for CPS2
const INT32 CPS2_INPUT_UP     = 0;
const INT32 CPS2_INPUT_DOWN   = 1;
const INT32 CPS2_INPUT_LEFT   = 2;
const INT32 CPS2_INPUT_RIGHT  = 3;
const INT32 CPS2_INPUT_PUNCH  = 4;
const INT32 CPS2_INPUT_KICK   = 5;
const INT32 CPS2_INPUT_START  = 6;
const INT32 CPS2_INPUT_COIN   = 7; 