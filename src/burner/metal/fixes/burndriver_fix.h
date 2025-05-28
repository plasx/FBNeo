#pragma once

// Compatibility fixes for BurnDriver initialization in Metal build

// Include necessary types
#include <stdint.h>

// Define the types directly to avoid circular inclusion issues
typedef uint8_t UINT8;
typedef int8_t INT8;
typedef uint16_t UINT16;
typedef int16_t INT16;
typedef uint32_t UINT32;
typedef int32_t INT32;
typedef uint64_t UINT64;
typedef int64_t INT64;

#ifdef METAL_BUILD

// 1. Undefine the original BurnDriver struct if it's already defined
#ifdef BurnDriver
#undef BurnDriver
#endif

// 2. Define a modified version for Metal that fixes the initialization issues
struct BurnDriver {
    // Original fields
    char* szShortName;
    char* szParent;
    char* szBoardROM;
    char* szDate;
    
    // For Metal, convert function pointers to void* to avoid type errors during initialization
    void* GetRomInfo;
    void* GetRomName;
    void* GetInputInfo;
    void* GetDIPInfo;
    
    // Game functions (as void* for Metal build)
    void* Init;
    void* Exit;
    void* Frame;
    void* Draw;
    void* Scan;
    
    // Additional fields
    UINT32 Flags;
    UINT16 Players;
    UINT16 Hardware;
    UINT16 Genre;
    UINT16 Family;
    UINT16 Manufacturer;
    
    // Note for Metal: Actual function types for reference:
    // INT32 (*GetRomInfo)(struct BurnRomInfo* pri, UINT32 i);
    // INT32 (*GetRomName)(char** pszName, UINT32 i, INT32 nAka);
    // INT32 (*GetInputInfo)(struct BurnInputInfo* pii, UINT32 i);
    // INT32 (*GetDIPInfo)(struct BurnDIPInfo* pdi, UINT32 i);
    // INT32 (*Init)();
    // INT32 (*Exit)();
    // INT32 (*Frame)();
    // INT32 (*Draw)();
    // INT32 (*Scan)(INT32 nAction, INT32* pnMin);
};

// 3. Suppress warnings for this approach
#pragma GCC diagnostic ignored "-Wmissing-braces"
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"

#endif // METAL_BUILD 