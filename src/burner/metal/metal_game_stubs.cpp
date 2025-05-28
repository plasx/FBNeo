#include "metal_bridge.h"
#include <stdio.h>

// Game control stubs
extern "C" int Metal_RunGame() {
    printf("Metal_RunGame() called\n");
    
    // Run a single frame of the game
    int result = Metal_RunFrame(true);
    
    // Create a timer for continuous updates in the app
    extern int CreateGameTimer();
    CreateGameTimer();
    
    return result;
}

extern "C" int Metal_ResetGame() {
    printf("Metal_ResetGame() called\n");
    
    // Call the Metal reset function
    extern INT32 BurnDrvReset_Metal();
    return BurnDrvReset_Metal();
}

extern "C" int Metal_PauseGame(int pause) {
    printf("Metal_PauseGame(%d) called\n", pause);
    
    // Set the global pause flag
    extern bool bRunPause;
    bRunPause = pause ? true : false;
    
    // Pause audio if enabled
    Metal_PauseAudio(pause);
    
    return 0;
} 