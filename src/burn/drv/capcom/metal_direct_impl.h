#ifndef METAL_DIRECT_IMPL_H
#define METAL_DIRECT_IMPL_H

// We don't need to include any headers here, they're handled in the .c file

// Forward declarations
#ifndef TYPE_INT32
typedef int INT32;
#define TYPE_INT32
#endif

#ifndef TYPE_UINT8
typedef unsigned char UINT8;
#define TYPE_UINT8
#endif

#ifdef BUILD_METAL

// Direct Metal implementation namespace
namespace MetalDirect {
    // Initialization
    INT32 Init();
    
    // Exit/cleanup
    void Exit();
    
    // Frame rendering
    INT32 Frame();
    
    // Memory management
    void SetGfx(UINT8* gfx);
    void SetPalette(UINT8* pal);
    void SetRom(UINT8* rom);
    void SetZRom(UINT8* zrom);
    
    // State management
    INT32 Scan(INT32 nAction);
}

#endif // BUILD_METAL
#endif // METAL_DIRECT_IMPL_H 