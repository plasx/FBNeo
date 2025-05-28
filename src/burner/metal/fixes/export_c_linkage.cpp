#include "burnint.h"
#include "burn_ym2151.h"

// Avoid 'UINT8 undeclared identifier' error
typedef unsigned char UINT8;
typedef int INT32;

// Metal wrapper for C API exports
extern "C" {
    // These functions have different signatures between Metal and non-Metal builds
    // and need explicit C linkage exports for Metal compatibility.
    
    // YM2151 sound chip functions
    extern void BurnYM2151Write(INT32 chip, INT32 offset, UINT8 data);
    
    // MSM6295 ADPCM sound chip functions
    extern void MSM6295Command(INT32 chip, UINT8 data);
    extern UINT8 MSM6295Read(INT32 chip);
    extern INT32 MSM6295Init(INT32 chip, INT32 samplerate, INT32 adjuster);
    extern void MSM6295Reset(INT32 chip);
    extern void MSM6295Exit(INT32 chip);
    extern void MSM6295SetRoute(INT32 chip, double volume, INT32 route);
    extern INT32 MSM6295Scan(INT32 chip, INT32 action);
    extern INT32 MSM6295RenderDirect(INT32 chip, INT16* buffer, INT32 samples, INT32 routes);
} 