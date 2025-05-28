#ifndef _FIX_SOUND_ROUTING_H_
#define _FIX_SOUND_ROUTING_H_

// Only apply these fixes when using Metal renderer
#ifdef USE_METAL_RENDERER

// Override the default sound routing macros with Metal-compatible versions
#undef BURN_SND_ROUTE_LEFT
#undef BURN_SND_ROUTE_RIGHT
#undef BURN_SND_ROUTE_BOTH

// Metal-specific sound routing definitions
#define METAL_SND_ROUTE_LEFT   0
#define METAL_SND_ROUTE_RIGHT  1
#define METAL_SND_ROUTE_BOTH   2

#endif // USE_METAL_RENDERER

#ifdef __cplusplus
extern "C" {
#endif

// Some sound-related functions needed by the Metal build
void BurnYM2151SetRoute(int nChip, double nVolume, int nRoute);
void QscSetRoute(int nChip, double nVolume, int nRoute);

#ifdef __cplusplus
}
#endif

#endif // _FIX_SOUND_ROUTING_H_ 