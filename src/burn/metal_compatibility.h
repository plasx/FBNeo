#ifndef METAL_COMPATIBILITY_H
#define METAL_COMPATIBILITY_H

// Metal compatibility header to resolve function signature conflicts
// This header should be included before other FBNeo headers

// Define TCHAR first
#ifndef TCHAR_DEFINED
#define TCHAR_DEFINED 1
typedef char TCHAR;
#define _T(x) x
#define _tcscmp strcmp
#define _tcscpy strcpy
#define _tcslen strlen
#endif

// Forward declarations to prevent conflicts
extern "C" {
    typedef int32_t INT32;
    typedef uint8_t UINT8;
    typedef uint16_t UINT16;
    typedef uint32_t UINT32;
    typedef uint64_t UINT64;
    
    // ROM loading function - use the burn.h signature
    INT32 BurnLoadRom(UINT8* Dest, INT32* pnWrote, INT32 i);
    
    // Graphics transfer functions - use void return type
    void BurnTransferCopy(UINT32 *pPalette);
    void BurnTransferInit();
    void BurnTransferExit();
    
    // Timer functions - use void return type for consistency
    void BurnTimerUpdate(INT32 nCycles);
    UINT64 BurnTimerCPUTotalCycles();
    
    // Prevent redefinition conflicts
    #define BURN_COMPATIBILITY_DEFINED 1
}

#endif // METAL_COMPATIBILITY_H 