// Patched version of load.cpp for Metal build
// This version includes stubs for ROM loading functions

#include "burnint.h"
#include "metal_declarations.h"
#include "../../../burn/crossplatform.h" // Include for the canonical BurnByteswap
#include <string.h>
#include <stdio.h>

// Simplified definitions for Metal
#define LD_NIBBLES      (1<<0)
#define LD_INVERT       (1<<1)
#define LD_BYTESWAP     (1<<2)
#define LD_BYTESWAP_END (1<<3)
#define LD_REVERSE      (1<<4)
#define LD_SWAP_ODD     (1<<5)
#define LD_SWAP_EVEN    (1<<6)
#define LD_XOR          (1<<7)
#define LD_GROUP_MANY   (1<<15)

#define LD_GROUP(a)     (((a) & 15) << 8)
#define LD_GROUPSIZE(a) (((a) >> 8) & 15)

// ROM-related stubs needed for the Metal build

// BurnExtLoadRom callback
INT32 BurnExtLoadRomStub(UINT8* Dest, INT32* pnWrote, INT32 i)
{
    printf("BurnExtLoadRomStub called for ROM %d\n", i);
    
    // Pretend we wrote something
    if (pnWrote) {
        *pnWrote = 0x1000;  // Arbitrary size
    }
    
    // Just fill with a pattern so we know it loaded
    if (Dest) {
        for (INT32 j = 0; j < 0x1000; j++) {
            Dest[j] = (UINT8)(j & 0xFF);
        }
    }
    
    return 0;
}

// Set the callback
INT32 (*BurnExtLoadRom)(UINT8* Dest, INT32* pnWrote, INT32 i) = BurnExtLoadRomStub;

// Set globals needed by other modules
UINT8* BurnMallocMemPattern = NULL;
bool bDoIpsPatch = false;

// Metal-specific memory allocation functions - don't use the macros
void* Metal_BurnMalloc(INT32 size)
{
    printf("Metal_BurnMalloc called for %d bytes\n", size);
    return malloc(size);
}

// Metal-specific memory free function - don't use the macros
void Metal_BurnFree(void* ptr)
{
    printf("Metal_BurnFree called\n");
    free(ptr);
}

// ROM loading function
INT32 BurnDrvLoadRom(UINT8* Dest, INT32 i, INT32 nGap)
{
    printf("BurnDrvLoadRom called: ROM #%d, gap %d\n", i, nGap);
    
    // Fill with pattern so we know it loaded
    if (Dest) {
        for (INT32 j = 0; j < 0x1000; j++) {
            Dest[j] = (UINT8)((i * 3 + j) & 0xFF);
        }
    }
    
    return 0;
}

// Callbacks for memory management
static void* MetalMemAlloc(size_t size)
{
    return malloc(size);
}

static void MetalMemFree(void* ptr)
{
    free(ptr);
}

// Function to set memory allocation callbacks
void BurnSetMemAlloc(void* (*pAlloc)(size_t), void (*pFree)(void*))
{
    // Implementation not needed for Metal build
}

// Initialize memory management
void InitMemoryManager(void)
{
    BurnSetMemAlloc(MetalMemAlloc, MetalMemFree);
}

// Support for BurnState (save states)
const char* BurnStateGetDescription()
{
    return "Metal Build";
}

INT32 BurnStateInit()
{
    printf("BurnStateInit called\n");
    return 0;
}

INT32 BurnStateExit()
{
    printf("BurnStateExit called\n");
    return 0;
}

INT32 BurnStateSave(const char* filename)
{
    printf("BurnStateSave called: %s\n", filename);
    return 0;
}

INT32 BurnStateLoad(const char* filename)
{
    printf("BurnStateLoad called: %s\n", filename);
    return 0;
}

INT32 BurnStateRegister(const char* section, INT32 size, void* data)
{
    printf("BurnStateRegister called: %s, size %d\n", section, size);
    return 0;
}

// Support for IPS patching
INT32 IpsApplyPatches(UINT8* base, char* filename)
{
    printf("IpsApplyPatches called for %s\n", filename);
    return 0;
}

// Additional ROM-related functions
char* BurnDrvGetRomName(INT32 i)
{
    static char szBuffer[32];
    snprintf(szBuffer, sizeof(szBuffer), "ROM%d", i);
    return szBuffer;
}

UINT8* RomFind(UINT32 nByteSize)
{
    printf("RomFind called for %u bytes\n", nByteSize);
    return (UINT8*)Metal_BurnMalloc(nByteSize); // Use our custom function instead of the macro
}

// No need for BurnByteswap function here - use the one from crossplatform.h

// Simplified ROM loading for Metal build
extern "C" INT32 BurnLoadRom(uint8_t* Dest, INT32 i, INT32 nGap) {
    printf("BurnLoadRom called: i=%d, nGap=%d\n", i, nGap);
    
    // In the Metal stub implementation, we just fill the ROM area with a test pattern
    if (Dest) {
        for (INT32 j = 0; j < 0x10000; j++) {
            if (nGap > 1) {
                Dest[j * nGap] = j & 0xFF;
            } else {
                Dest[j] = j & 0xFF;
            }
        }
    }
    
    return 0;
}

extern "C" INT32 BurnLoadRomExt(uint8_t* Dest, INT32 i, INT32 nGap, INT32 nFlags) {
    printf("BurnLoadRomExt called: i=%d, nGap=%d, nFlags=%08x\n", i, nGap, nFlags);
    
    // In the Metal stub implementation, we just fill the ROM area with a test pattern
    if (Dest) {
        for (INT32 j = 0; j < 0x10000; j++) {
            if (nGap > 1) {
                Dest[j * nGap] = j & 0xFF;
            } else {
                Dest[j] = j & 0xFF;
            }
        }
        
        // Apply flags as needed
        if (nFlags & LD_INVERT) {
            for (INT32 j = 0; j < 0x10000; j++) {
                if (nGap > 1) {
                    Dest[j * nGap] ^= 0xFF;
                } else {
                    Dest[j] ^= 0xFF;
                }
            }
        }
        
        if (nFlags & LD_BYTESWAP) {
            BurnByteswap(Dest, 0x10000);
        }
    }
    
    return 0;
}

extern "C" INT32 BurnXorRom(uint8_t* Dest, INT32 i, INT32 nGap) {
    printf("BurnXorRom called: i=%d, nGap=%d\n", i, nGap);
    
    return BurnLoadRomExt(Dest, i, nGap, LD_XOR);
}

extern "C" INT32 BurnLoadBitField(uint8_t* pDest, uint8_t* pSrc, INT32 nField, INT32 nSrcLen) {
    printf("BurnLoadBitField called: nField=%d, nSrcLen=%d\n", nField, nSrcLen);
    
    // Simple stub implementation for bit field loading
    if (pDest && pSrc && nSrcLen > 0) {
        memset(pDest, 0, nSrcLen * 8);
        
        for (INT32 i = 0; i < nSrcLen * 8; i++) {
            INT32 nSrcPos = (i >> 3);
            INT32 nSrcBit = 7 - (i & 7);
            INT32 nBit = (nField >> nSrcBit) & 1;
            
            pDest[i] = (pSrc[nSrcPos] >> nBit) & 1;
        }
    }
    
    return 0;
}

// End of patched load.cpp 