#include <stdint.h>
#include <stddef.h>
#include <string.h>  // For memset

// Define the global variables for audio
int16_t* pBurnSoundOut = NULL;
int nBurnSoundLen = 0;
int nBurnSoundRate = 44100;
int nBurnSoundPos = 0;
int nBurnSoundBufferSize = 0;

// Stub implementations for sound functions
int Metal_ProcessAudio() {
    // Process audio for current frame (stub implementation)
    return 0;
}

int Metal_InitAudio() {
    return 0;
}

void Metal_ShutdownAudio() {
    // Nothing to do
}

int BurnSoundRender(int16_t* pDest, int nLen) {
    // Just fill with silence
    if (pDest && nLen > 0) {
        memset(pDest, 0, nLen * 2 * sizeof(int16_t));
    }
    return 0;
}
