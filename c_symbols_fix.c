/*
 * This file provides C-linkage exports for symbols required by the linker
 * These are simple stub implementations with the proper symbol names
 * that match what the C++ code is looking for.
 */

#include "src/burner/metal/fixes/c_symbols_header.h"

// Explicitly mark all functions as having default visibility for the linker
// and use the standard C calling convention (no name mangling)
#define EXPORT __attribute__((visibility("default")))

// Z80 functions
EXPORT void Z80StopExecute(void) {}
EXPORT INT32 z80TotalCycles(void) { return 0; }
EXPORT INT32 ZetInit(INT32 nCPU) { return 0; }
EXPORT void ZetExit(void) {}
EXPORT void ZetOpen(INT32 nCPU) {}
EXPORT void ZetClose(void) {}
EXPORT void ZetReset(void) {}
EXPORT INT32 ZetScan(INT32 nAction) { return 0; }
EXPORT INT32 ZetTotalCycles(void) { return 0; }
EXPORT INT32 ZetMemCallback(INT32 nStart, INT32 nEnd, INT32 nMode) { return 0; }
EXPORT void ZetSetReadHandler(UINT8 (*pHandler)(UINT16)) {}
EXPORT void ZetSetWriteHandler(void (*pHandler)(UINT16, UINT8)) {}

// CPS functions
EXPORT INT32 Cps2Init(void) { return 0; }
EXPORT INT32 CpsInit(void) { return 0; }
EXPORT INT32 CpsExit(void) { return 0; }

// Sound functions
EXPORT INT32 PsndInit(void) { return 0; }
EXPORT INT32 PsndExit(void) { return 0; }
EXPORT void PsndNewFrame(void) {}
EXPORT void PsndEndFrame(void) {}
EXPORT INT32 PsndSyncZ80(INT32 nCycles) { return 0; }
EXPORT INT32 PsmUpdateEnd(void) { return 0; }
EXPORT INT32 QsndInit(void) { return 0; }
EXPORT void QsndReset(void) {}
EXPORT void QsndExit(void) {}
EXPORT void QsndNewFrame(void) {}
EXPORT void QsndEndFrame(void) {}
EXPORT void QsndSyncZ80(void) {}
EXPORT INT32 QsndScan(INT32 nAction) { return 0; }
EXPORT void QsndSetRoute(INT32 nIndex, double nVolume, INT32 nRouteDir) {}

// Sample functions
EXPORT void BurnSampleInit(INT32 nSampleRate) {}
EXPORT void BurnSampleSetRoute(INT32 nSample, INT32 nChannel, double flVolume, INT32 nRouteDir) {}
EXPORT INT32 BurnSoundRender(INT16 *pDst, INT32 nLen) { return 0; }
EXPORT void BurnSampleExit(void) {}
EXPORT INT32 BurnSampleScan(INT32 nAction, INT32 *pnMin) { return 0; }
EXPORT void BurnSampleRender(INT16 *pDest, UINT32 nLen) {}
EXPORT INT32 BurnSampleGetChannelStatus(INT32 nSample) { return 0; }
EXPORT void BurnSampleChannelPlay(INT32 channel, INT32 sample, bool loop) {}

// Trackball functions
EXPORT void BurnTrackballInit(INT32 nPlayer) {}
EXPORT void BurnTrackballConfig(INT32 nPlayer, INT32 nDeviceType, INT32 nDevID) {}
EXPORT void BurnTrackballFrame(int iPlayer, int x1, int y1, int x2, int y2, int *a4, int *a5) {}
EXPORT void BurnTrackballUpdate(INT32 nPlayer) {}
EXPORT INT32 BurnTrackballReadSigned(INT32 nPlayer) { return 0; }
EXPORT INT32 BurnTrackballGetDirection(INT32 nPlayer) { return 0; }
EXPORT void BurnTrackballReadReset(void) {}

// NeoCD functions
EXPORT INT32 NeoCDInfo_ID(void) { return 0; }
EXPORT INT32 NeoCDInfo_Text(int nID) { return 0; }

// Input functions
EXPORT void FreezeInput(UINT8 **buf, INT32 *size) {}
EXPORT INT32 UnfreezeInput(const UINT8 *buf, INT32 size) { return 0; }

// Gun functions
EXPORT INT32 BurnGunInit(INT32 nNumPlayers, INT32 bDrawTargets) { return 0; }
EXPORT void BurnGunExit(void) {}
EXPORT void BurnGunMakeInputs(INT32 nPlayer, INT16 nXPosRaw, INT16 nYPosRaw) {}
EXPORT void BurnGunDrawTarget(INT32 num, INT32 x, INT32 y) {}
EXPORT UINT8 BurnGunReturnX(INT32 nPlayer) { return 0; }
EXPORT UINT8 BurnGunReturnY(INT32 nPlayer) { return 0; }
EXPORT void BurnGunScan(void) {}

// SEK functions
EXPORT INT32 SekInit(INT32 nCount, INT32 nCPUType) { return 0; }
EXPORT void SekExit(void) {}
EXPORT void SekNewFrame(void) {}
EXPORT INT32 SekReset(void) { return 0; }
EXPORT INT32 SekOpen(const INT32 i) { return 0; }
EXPORT INT32 SekClose(void) { return 0; }
EXPORT INT32 SekScan(INT32 nAction) { return 0; }

// CTV function
EXPORT INT32 CtvReady(void) { return 0; }
EXPORT INT32 PsndScan(INT32 nAction, INT32* pnMin) { return 0; } 