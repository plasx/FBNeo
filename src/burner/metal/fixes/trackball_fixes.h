#include "c_symbols_header.h"
#pragma once

typedef int INT32;
// Trackball-related function declarations for Metal build
namespace MetalFixes {

// Private implementations (not exposed directly)
INT32 __BurnTrackballInit(INT32 nPlayers);
void __BurnTrackballConfig(int index, int nA, int nB);
void __BurnTrackballUpdate(int index);
void __BurnTrackballReadReset();
int __BurnTrackballReadSigned(int index);
int __BurnTrackballGetDirection(int index);
void __BurnTrackballFrame(int index, int x, int y, int z, int TurboBananas, int TurboHotdog);
void __BurnGunExit();

// Sound rendering
#ifdef __cplusplus
extern "C" {
#endif
INT32 __BurnSoundRender(INT16 *pDst, INT32 nLen);
INT32 BurnSoundRender(INT16 *pDst, INT32 nLen);
#ifdef __cplusplus
}
#endif

} // namespace MetalFixes

// The extern "C" declarations are in direct_minimal.h and implemented in trackball_fixes.cpp 