// Global variables for Metal implementation
#include <stdint.h>
#include <stdlib.h> // For NULL definition
#include "metal_declarations.h"

// Basic types
typedef unsigned char UINT8;
typedef int INT32;
typedef unsigned int UINT32;
typedef short INT16;
typedef unsigned short UINT16;
typedef long long INT64;
typedef unsigned long long UINT64;

// Global variables that are accessed from various files
extern "C" {
    // Core engine variables that need to be accessible
    UINT8* pBurnDraw = NULL;
    INT32 nBurnPitch = 0;
    INT32 nBurnBpp = 32;
    INT32 nBurnDrvCount = 1;
    UINT32 nBurnDrvActive = 0;
    INT32 nBurnSoundRate = 44100;
    INT32 nBurnSoundLen = 0;
    INT16* pBurnSoundOut = NULL;
    
    // System hardware flags
    UINT32 nSystemIrqLine = 0;
    UINT32 nSoundBufferPos = 0;
    
    // ROM loading variables
    UINT32 nBurnRomLen = 0;
    UINT8* pBurnRom = NULL;
    
    // Display dimensions
    INT32 nBurnGameWidth = 0;
    INT32 nBurnGameHeight = 0;
    
    // Input variables
    UINT8 keybAnalogSpeed[4] = {0x7F, 0x7F, 0x7F, 0x7F};
    UINT32 nCurrentFrame = 0;
    
    // FPS counter
    INT32 nFramesEmulated = 0;
    INT32 nFramesRendered = 0;
    
    // Path variables
    char szAppHomePath[512] = ".";
    char szAppEEPROMPath[512] = ".";
    
    // Additional variables required by various modules
    INT32 nAudSegLen = 0;
    INT32 nAudSegCount = 0;
    INT32 nMaxPlayers = 0;
    
    // Machine hardware flags
    UINT32 nMachineRomType = 0;
    UINT32 nMachineType = 0;
}

// Define the global emulator frame buffer
EmulatorFrameBuffer g_frameBuffer = {0};
