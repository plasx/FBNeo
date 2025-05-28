#ifndef _METAL_AUDIO_BRIDGE_H_
#define _METAL_AUDIO_BRIDGE_H_

// =============================================================================
// FBNeo Metal - Audio System Bridge
// =============================================================================
// This header provides proper C/C++ interoperability for the audio system
// =============================================================================

#include "metal_interop.h"

METAL_BEGIN_C_DECL

// -----------------------------------------------------------------------------
// Core Audio Functions
// -----------------------------------------------------------------------------

// These are the standard FBNeo audio functions that need to be bridged
// to the Metal implementation

// Initialize audio subsystem
INT32 BurnSoundInit();

// Clean up audio resources
INT32 BurnSoundExit();

// Start audio playback
INT32 BurnSoundPlay();

// Stop audio playback
INT32 BurnSoundStop();

// Check audio status
INT32 BurnSoundCheck();

// Set audio volume
INT32 BurnSoundSetVolume(INT32 nVolume);

// Render audio frames
INT32 BurnSoundRender(INT16* pSoundBuf, INT32 nSegmentLength);

// -----------------------------------------------------------------------------
// Metal-specific Audio Functions
// -----------------------------------------------------------------------------

// Audio framework integration for Metal
INT32 Metal_AudioInit();
INT32 Metal_AudioFrame();
void Metal_ShutdownAudio();
void Metal_PauseAudio(INT32 pause);
void Metal_SetAudioVolume(float volume);
float Metal_GetAudioCPUUsage();
float Metal_GetBufferFillLevel();

// -----------------------------------------------------------------------------
// Sound Chip Functions
// -----------------------------------------------------------------------------

// YM2151 sound chip functions (with proper linkage)
void BurnYM2151Write(INT32 chip, INT32 offset, UINT8 data);
UINT8 BurnYM2151Read(INT32 chip, INT32 offset);
void BurnYM2151Reset();
void BurnYM2151Exit();
void BurnYM2151SetRoute(INT32 chip, INT32 nIndex, double nVolume, INT32 nRouteDir);

// MSM6295 ADPCM sound chip functions
void MSM6295Command(INT32 chip, UINT8 data);
UINT8 MSM6295Read(INT32 chip);
INT32 MSM6295Init(INT32 chip, INT32 samplerate, INT32 adjuster);
void MSM6295Reset(INT32 chip);
void MSM6295Exit(INT32 chip);
void MSM6295SetRoute(INT32 chip, double volume, INT32 route);
INT32 MSM6295Scan(INT32 chip, INT32 action);
INT32 MSM6295RenderDirect(INT32 chip, INT16* buffer, INT32 samples, INT32 routes);

// Other sound chip functions may be added as needed...

// -----------------------------------------------------------------------------
// Audio Compatbility Layer for FBNeo Core
// -----------------------------------------------------------------------------

// Legacy function wrappers for compatibility with burn.cpp
INT32 AudSoundInit();
INT32 AudSoundExit();
INT32 AudSoundPlay();
INT32 AudSoundStop();
INT32 AudSoundSetVolume(INT32 nVolume);
INT32 AudSetCallback(INT32 (*pCallback)(INT32));

METAL_END_C_DECL

#endif // _METAL_AUDIO_BRIDGE_H_ 