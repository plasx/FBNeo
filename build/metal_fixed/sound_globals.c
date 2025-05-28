#include <stdint.h>
#include <stddef.h>
#include <string.h>

// Define the global variables for audio - main implementation for FBNeo Metal
int16_t* pBurnSoundOut = NULL;
int nBurnSoundLen = 0;
int nBurnSoundRate = 44100;
int nBurnSoundPos = 0;
int nBurnSoundBufferSize = 0;

// Single implementation of BurnSoundRender to avoid duplicates
int BurnSoundRender(int16_t* pDest, int nLen) {
    // Just fill with silence in the stub implementation
    if (pDest && nLen > 0) {
        memset(pDest, 0, nLen * 2 * sizeof(int16_t));
    }
    return 0;
}
