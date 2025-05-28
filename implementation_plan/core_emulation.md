# Core Emulation Implementation

This document details the implementation plan for replacing stub functions in the core emulation components of FBNeo Metal.

## CPU Emulation

### M68K (SekInit/SekRun/SekExit)

```c
// Replace stub implementations in metal_cpu_stubs.c
// Real implementation connecting to Musashi M68K core:

#include "m68000/m68k.h"

// Initialize M68K CPU
INT32 SekInit(INT32 nCount, INT32 nCPUType)
{
    // Set up the M68K CPU core
    m68k_init();
    
    // Configure CPU type (68000, 68010, etc)
    m68k_set_cpu_type(nCPUType == 0 ? M68K_CPU_TYPE_68000 : nCPUType);
    
    // Initialize memory maps
    m68k_memory_map_init();
    
    return 0;
}

// Run M68K CPU for specified cycles
INT32 SekRun(INT32 cycles)
{
    // Execute for the specified number of cycles
    return m68k_execute(cycles);
}

// Reset M68K CPU
INT32 SekReset()
{
    m68k_pulse_reset();
    return 0;
}

// Set memory read/write handlers
void SekSetReadByteHandler(INT32 i, pSekReadByteHandler pHandler)
{
    m68k_set_read_byte_handler(i, pHandler);
}

void SekSetWriteByteHandler(INT32 i, pSekWriteByteHandler pHandler)
{
    m68k_set_write_byte_handler(i, pHandler);
}

// Similar implementations for word and long handlers
```

### Z80 (ZetInit/ZetRun/ZetExit)

```c
// Replace stub implementations in metal_cpu_stubs.c
// Real implementation connecting to MAME Z80 core:

#include "z80/z80.h"

static Z80_Regs Z80;
static int nOpenedCPU = -1;
static int nCPUCount = 0;

// Initialize Z80 CPU
INT32 ZetInit(INT32 nCPU)
{
    // Ensure we don't exceed max CPUs
    if (nCPU >= MAX_Z80) return 1;
    
    // Initialize the CPU context
    memset(&Z80, 0, sizeof(Z80));
    z80_init(&Z80, nCPU);
    
    nCPUCount = nCPU + 1;
    
    // Set the active CPU
    ZetOpen(0);
    
    return 0;
}

// Run Z80 CPU for specified cycles
INT32 ZetRun(INT32 cycles)
{
    if (nOpenedCPU < 0) return 0;
    
    // Execute for the specified number of cycles
    return z80_execute(&Z80, cycles);
}

// Set memory read/write handlers
void ZetSetReadHandler(unsigned char (*pHandler)(unsigned short))
{
    Z80.ReadHandler = pHandler;
}

void ZetSetWriteHandler(void (*pHandler)(unsigned short, unsigned char))
{
    Z80.WriteHandler = pHandler;
}

// Similar implementations for port I/O handlers
```

## ROM Loading and Management

### CPS2 ROM Loading

```cpp
// Implementation for CPS2-specific ROM loading:
INT32 Cps2LoadRoms(bool bLoad)
{
    struct BurnRomInfo ri;
    unsigned char* rom_data = NULL;
    
    // Get ROM info for the current driver
    for (int i = 0; BurnDrvGetRomInfo(&ri, i) == 0; i++) {
        if (ri.nType & BRF_ESS) {  // Essential ROM
            // Allocate memory for ROM
            if (bLoad) {
                rom_data = (unsigned char*)malloc(ri.nLen);
                if (!rom_data) return 1;
                
                // Read ROM data from file
                char* filename;
                BurnDrvGetRomName(&filename, i, 0);
                
                // Open the file and read its contents
                if (LoadFromZip(filename, rom_data, ri.nLen)) {
                    free(rom_data);
                    return 1;
                }
                
                // Process the ROM data (decrypt if necessary for CPS2)
                if (ri.nType & BRF_ENCRYPT) {
                    Cps2Decrypt(rom_data, ri.nLen);
                }
                
                // Load the ROM data into memory
                if (ri.nType & BRF_PRG) {  // Program ROM
                    CpsLoadOne(rom_data, &Cps68kProg, i, ri.nLen);
                } else if (ri.nType & BRF_GRA) {  // Graphics ROM
                    CpsLoadTiles(rom_data, CpsGfx, i);
                } else if (ri.nType & BRF_SND) {  // Sound ROM
                    CpsLoadOneSound(rom_data, i, ri.nLen);
                }
                
                free(rom_data);
            }
        }
    }
    
    return 0;
}
```

### Memory Mapping

```cpp
// Implementation for memory mapping:
void CpsMapMemory(unsigned char* pMemory, unsigned int nStart, unsigned int nEnd, int nType)
{
    unsigned char** pAddressBase;
    
    // Select the appropriate memory map based on type
    switch (nType) {
        case MAP_SPRITES:
            pAddressBase = &CpsSpriteMem;
            break;
        case MAP_MEMORY:
            pAddressBase = &CpsRamStart;
            break;
        case MAP_PALETTE:
            pAddressBase = &CpsPalSrc;
            break;
        default:
            return;
    }
    
    *pAddressBase = pMemory;
    
    // Map the memory region in the M68K address space
    for (unsigned int i = nStart; i <= nEnd; i += 0x1000) {
        SekMapMemory(pMemory + (i - nStart), i, i + 0xFFF, nType);
    }
}
```

## Game State Management

```cpp
// Implementation for save state functionality:
INT32 BurnStateLoad(TCHAR* szName, INT32 bAll, INT32 (*pLoadGame)())
{
    // Open the file
    FILE* fp = fopen(szName, "rb");
    if (!fp) return 1;
    
    // Read the header
    char header[4];
    fread(header, 1, 4, fp);
    
    // Verify the file format
    if (memcmp(header, "FB S", 4) != 0) {
        fclose(fp);
        return 1;
    }
    
    // Read the version
    unsigned int nVersion;
    fread(&nVersion, 1, 4, fp);
    
    // Check for compatibility
    if (nVersion < BURN_STATE_VERSION) {
        fclose(fp);
        return 1;
    }
    
    // Read the game ID
    char szGameId[16];
    fread(szGameId, 1, 16, fp);
    
    // Verify game ID matches
    if (strcmp(szGameId, BurnDrvGetTextA(DRV_NAME)) != 0) {
        fclose(fp);
        return 1;
    }
    
    // Load CPU registers
    SekLoadState(fp);
    ZetLoadState(fp);
    
    // Load RAM contents
    fread(CpsRam, 1, 0x10000, fp);
    fread(CpsSpriteRam, 1, 0x4000, fp);
    fread(CpsPalSrc, 1, 0x2000, fp);
    
    // Call the game-specific loading function if provided
    if (pLoadGame) {
        pLoadGame();
    }
    
    fclose(fp);
    return 0;
}

INT32 BurnStateSave(TCHAR* szName, INT32 bAll)
{
    // Open the file
    FILE* fp = fopen(szName, "wb");
    if (!fp) return 1;
    
    // Write the header
    fwrite("FB S", 1, 4, fp);
    
    // Write the version
    unsigned int nVersion = BURN_STATE_VERSION;
    fwrite(&nVersion, 1, 4, fp);
    
    // Write the game ID
    char szGameId[16];
    strcpy(szGameId, BurnDrvGetTextA(DRV_NAME));
    fwrite(szGameId, 1, 16, fp);
    
    // Save CPU registers
    SekSaveState(fp);
    ZetSaveState(fp);
    
    // Save RAM contents
    fwrite(CpsRam, 1, 0x10000, fp);
    fwrite(CpsSpriteRam, 1, 0x4000, fp);
    fwrite(CpsPalSrc, 1, 0x2000, fp);
    
    fclose(fp);
    return 0;
}
```

## Integrating with Metal Frontend

```cpp
// Implementation of BurnDrvFrame for Metal integration:
INT32 BurnDrvFrame()
{
    // Process inputs
    InputMake(true);
    
    // Run one frame of emulation
    CpsFrame();
    
    // Update audio
    BurnSoundUpdate();
    
    // Render the frame to the Metal frame buffer
    if (nBurnLayer & 1) {
        if (Metal_GetFrameBuffer()) {
            // Render the background layer
            CpsRenderBgLayer(0, Metal_GetFrameBuffer());
            
            // Render sprites
            CpsRenderSprites(Metal_GetFrameBuffer());
            
            // Render the foreground layer
            CpsRenderFgLayer(0, Metal_GetFrameBuffer());
            
            // Update the Metal texture
            Metal_UpdateTexture(Metal_GetFrameBuffer(), nBurnBpp, nBurnPitch);
        }
    }
    
    return 0;
}
```

## Implementation Steps

1. **Replace CPU stubs first**
   - Start with M68K implementation
   - Proceed to Z80 implementation
   - Test CPU emulation in isolation

2. **Implement Memory Management**
   - Create memory mapping functions
   - Implement read/write handlers
   - Test memory access

3. **Add ROM Loading**
   - Implement CPS2-specific ROM loading
   - Add decompression and decryption routines
   - Test ROM loading with sample ROMs

4. **Integrate with Metal Frontend**
   - Connect frame buffer from emulation core to Metal renderer
   - Implement frame rendering functions
   - Test rendering pipeline

5. **Add State Management**
   - Implement save/load state functionality
   - Add serialization for all components
   - Test state saving and loading

## Testing Strategy

For each component:
1. Create unit tests to verify functionality
2. Compare output against known good values
3. Test with real ROM images
4. Benchmark performance against reference implementation
5. Verify integration with Metal frontend

## Dependencies

- Musashi M68K core
- MAME Z80 core
- FBNeo core headers
- Metal rendering interface
- CPS2 ROM specifications 