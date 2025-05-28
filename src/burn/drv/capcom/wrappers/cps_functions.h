#pragma once

// CPS functions declaration for Metal build
// This header declares the CPS functions needed for the Metal build

#ifdef USE_METAL

// Include our type definitions
#include "metal_linkage_wrapper.h"

#ifdef __cplusplus
extern "C" {
#endif

// CPS rendering functions
METAL_INT32 Cps1rRender();
METAL_INT32 Cps2rRender();
METAL_INT32 Cps1rPrepare();
METAL_INT32 Cps2rPrepare();
METAL_INT32 Cps1Scr1Draw(unsigned char* pSrc, METAL_INT32 nSrcPitch, METAL_INT32 nSize);
METAL_INT32 Cps1Scr3Draw(unsigned char* pSrc, METAL_INT32 nSrcPitch, METAL_INT32 nSize);
METAL_INT32 Cps2Scr1Draw(unsigned char* pSrc, METAL_INT32 nSrcPitch, METAL_INT32 nSize);
METAL_INT32 Cps2Scr3Draw(unsigned char* pSrc, METAL_INT32 nSrcPitch, METAL_INT32 nSize);
METAL_INT32 CtvReady();

// Sound functions
METAL_INT32 PsndInit();
METAL_INT32 PsndExit();
METAL_INT32 PsndScan(METAL_INT32 nAction, METAL_INT32* pnMin);
void PsndNewFrame();
void PsndEndFrame();
METAL_INT32 PsmUpdateEnd();
METAL_INT32 PsndSyncZ80(METAL_INT32 nCycles);

#ifdef __cplusplus
}
#endif

#endif // USE_METAL 