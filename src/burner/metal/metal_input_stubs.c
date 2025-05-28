#include "metal_declarations.h"
#include "metal_error_handling.h"

// Stub implementation for input functions
void InputMake(void) {
    Metal_LogMessage(LOG_LEVEL_INFO, "Stub implementation of InputMake called");
}

void Metal_ProcessKeyDown(int keyCode) {
    Metal_LogMessage(LOG_LEVEL_INFO, "Stub implementation of Metal_ProcessKeyDown called: %d", keyCode);
}

void Metal_ProcessKeyUp(int keyCode) {
    Metal_LogMessage(LOG_LEVEL_INFO, "Stub implementation of Metal_ProcessKeyUp called: %d", keyCode);
}

void Metal_UpdateInputState(void) {
    Metal_LogMessage(LOG_LEVEL_INFO, "Stub implementation of Metal_UpdateInputState called");
}

// Initialize graphics components
void Graphics_InitComponents() {
    Metal_LogMessage(LOG_LEVEL_INFO, "Stub implementation of Graphics_InitComponents called");
}

// Generate test pattern
int Metal_GenerateTestPattern(int patternType) {
    Metal_LogMessage(LOG_LEVEL_INFO, "Stub implementation of Metal_GenerateTestPattern called: %d", patternType);
    return 0;
}

// Process audio
int Metal_ProcessAudio() {
    Metal_LogMessage(LOG_LEVEL_INFO, "Stub implementation of Metal_ProcessAudio called");
    return 0;
}

// Unload ROM
void Metal_UnloadROM() {
    Metal_LogMessage(LOG_LEVEL_INFO, "Stub implementation of Metal_UnloadROM called");
}

// Global sound variables
int nBurnSoundLen = 0;
int nBurnSoundRate = 44100;
short* pBurnSoundOut = NULL; 