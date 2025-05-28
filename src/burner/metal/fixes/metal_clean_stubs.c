#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/*
 * This file contains clean stub implementations for Metal build
 * that don't depend on problematic header files.
 * 
 * It provides minimal implementations for required functions and
 * uses simplified structures to avoid C/C++ compatibility issues.
 */

// Basic type definitions
typedef int32_t INT32;
typedef uint32_t UINT32;
typedef int16_t INT16;
typedef uint16_t UINT16;
typedef int8_t INT8;
typedef uint8_t UINT8;
typedef char TCHAR;

// Simplified CPU interface implementations
INT32 SekTotalCycles(void) { return 0; }
void SekSetRESETLine(INT32 cpu, INT32 state) {}
void SekClose(void) {}
INT32 SekGetActive(void) { return -1; }
void SekNewFrame(void) {}
INT32 SekIdle(INT32 cycles) { return 0; }
INT32 SekSegmentCycles(void) { return 0; }
void SekRunEnd(void) {}
INT32 SekRun(INT32 cycles) { return 0; }

// Simplified Z80 CPU interface implementations
void ZetClose(void) {}
INT32 ZetGetActive(void) { return -1; }
void ZetNewFrame(void) {}
INT32 ZetIdle(INT32 cycles) { return 0; }
INT32 ZetSegmentCycles(void) { return 0; }
INT32 ZetTotalCycles(void) { return 0; }
void ZetRunEnd(void) {}
INT32 ZetRun(INT32 cycles) { return 0; }
void ZetSetIRQLine(const INT32 line, const INT32 status) {}
void ZetSetVector(INT32 vector) {}
INT32 ZetDaisyChain(INT32 param) { return 0; }
void ZetSetReadHandler(UINT8 (*pHandler)(UINT16)) {}
void ZetSetWriteHandler(void (*pHandler)(UINT16, UINT8)) {}
void ZetSetInHandler(UINT8 (*pHandler)(UINT16)) {}
void ZetSetOutHandler(void (*pHandler)(UINT16, UINT8)) {}

// Simplified CPU configuration structure
struct metal_cpu_config {
    char name[32];
    void* callbacks;
};

// Global variables with clean initialization
struct metal_cpu_config MegadriveCPU = {0};
struct metal_cpu_config FD1094CPU = {0};
struct metal_cpu_config MegadriveZ80 = {0};

// Sound implementation stubs
void BurnSoundClear(void) {}
void BurnSoundRender(INT16* pDst, INT32 nLen) {}

// String helpers that don't depend on TCHAR complexities
char* TCHARToANSI_Clean(const char* pszInString, char* pszOutString, int nOutSize) {
    if (pszInString) {
        strncpy(pszOutString, pszInString, nOutSize - 1);
        pszOutString[nOutSize - 1] = 0; // Null terminate
    } else {
        pszOutString[0] = 0;
    }
    return pszOutString;
}

// Clean driver interface stubs
INT32 BurnDrvGetZipName_Clean(char** pszName, UINT32 i) {
    if (i == 0) {
        *pszName = NULL;
    }
    return 1; // End of list
}

INT32 BurnDrvIsNeogeo_Clean(void) { 
    return 0;
}

INT32 DoLibInit_Clean(void) {
    return 0;
}

INT32 DrvInit_Clean(INT32 nDrvNum, bool bRestore) {
    return 0;
}

INT32 DrvExit_Clean(void) {
    return 0;
}

// Input stubs
INT32 InputInit(void) { return 0; }
INT32 InputExit(void) { return 0; }
INT32 InputMake(bool bCopy) { return 0; }

// Simple default color conversion
UINT32 BurnHighCol32_Clean(INT32 r, INT32 g, INT32 b, INT32 _i __attribute__((unused))) {
    return (r << 16) | (g << 8) | b;
}

// Support for palette recalculation
INT32 BurnRecalcPal_Clean(void) {
    return 0;
}

// Support for burn LED - stub that does nothing
void BurnLEDSetStatus_Clean(INT32 led, UINT32 status) {}

// Empty callback functions
void (*BurnDrvSetColorDepth)(INT32 nDepth) = NULL;
INT32 (*BurnDrvGetFlags)(void) = NULL;

// Game genre variables (replaces macros)
void* GBF_HORSHOOT_CLEAN = (void*)(uintptr_t)(1 << 0);
void* GBF_VERSHOOT_CLEAN = (void*)(uintptr_t)(1 << 1);
void* GBF_SCRFIGHT_CLEAN = (void*)(uintptr_t)(1 << 2);
void* GBF_VSFIGHT_CLEAN = (void*)(uintptr_t)(1 << 3);
void* GBF_BIOS_CLEAN = (void*)(uintptr_t)(1 << 4);
void* GBF_PLATFORM_CLEAN = (void*)(uintptr_t)(1 << 11);
void* GBF_RACING_CLEAN = (void*)(uintptr_t)(1 << 17);
void* GBF_SHOOT_CLEAN = (void*)(uintptr_t)(1 << 18); 