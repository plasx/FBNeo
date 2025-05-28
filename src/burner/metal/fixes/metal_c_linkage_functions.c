#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

// Basic type definitions 
typedef int INT32;
typedef unsigned int UINT32;
typedef unsigned short UINT16; 
typedef unsigned char UINT8;
typedef int BOOL;
typedef char TCHAR;  // Define TCHAR as char for our Metal build

// Define MAX_PATH
#ifndef PATH_MAX
#define PATH_MAX 512
#endif

#ifndef MAX_PATH
#define MAX_PATH PATH_MAX
#endif

// Linkage functions required by CPS but not needed in the standalone version
// Just use our own simplified implementations

// Sound interface
void wav_open(char* szFile) {
    printf("[SOUND] wav_open(%s)\n", szFile);
}

void wav_close() {
    printf("[SOUND] wav_close()\n");
}

// OSD FPS display
void FBAWriteProfileData() {
    // Empty implementation
}

// Display metrics
int GetClientScreenWidth() {
    return 0;
}

int GetClientScreenHeight() {
    return 0;
}

// Path info
int BurnUseASMCPUEmulation = 0;
int bHardwareDebug = 0;
int bDoIpsPatch = 0;
int nIpsMaxFileLen = 0;

// Key mapping
unsigned int CinpState() {
    return 0;
}

// Generic emulation memory handling
unsigned char* BurnMalloc(int size) {
    printf("[MEMORY] BurnMalloc(%d)\n", size);
    return (unsigned char*)malloc(size);
}

void _BurnFree(void *ptr) {
    printf("[MEMORY] _BurnFree()\n");
    free(ptr);
}

int AppDebugPrintf(int nStatus, char* pszFormat, ...) {
    // Empty implementation
    return 0;
}

// String processing functions
INT32 DoStrDec(char* s) { return 0; }
INT32 DoStrHex(char* s) { return 0; }

// AVI writing
int AviRecordStart() {
    return 0;
}

int AviRecordRom() {
    return 0;
}

int AviRecordFrame() {
    return 0;
}

void CloseAvi() {
    // Empty implementation
}

void AviRecordStop() {
    // Empty implementation
}

// File functions
void GetCurrentSaveSlot(void* slot) {
    // Empty implementation
}

void RefreshPalette() {
    // Empty implementation
}

// Cheats
void AsciiToGame() {
    // Empty implementation
}

void CheatSearchInit() {
    // Empty implementation
}

// Zip handling
void FlushDumpingBIOSRepository() {
    // Empty implementation
}

void GetZIPEncoding(char* szEncoding) {
    // Empty implementation
}

// String conversion
char* TCHARToANSI(const char* pszInString, char* pszOutString, int nOutSize)
{
    if (pszOutString) {
        strncpy(pszOutString, pszInString, nOutSize);
        pszOutString[nOutSize - 1] = '\0';
        return pszOutString;
    }
    return NULL;
} 