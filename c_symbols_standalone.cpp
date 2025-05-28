#include "src/burner/metal/fixes/c_symbols_header.h"

// This file provides standalone C linkage exports for all the symbols
// needed by the linker, with proper extern "C" linkage

// Note: We must keep the extern "C" OUTSIDE the function definitions
// to ensure proper linkage in C++

extern "C" {
    // Z80 functions
    __attribute__((visibility("default"))) void Z80StopExecute() {}
    __attribute__((visibility("default"))) INT32 z80TotalCycles() { return 0; }
    __attribute__((visibility("default"))) INT32 ZetInit(INT32 nCPU) { return 0; }
    __attribute__((visibility("default"))) void ZetExit() {}
    __attribute__((visibility("default"))) void ZetOpen(INT32 nCPU) {}
    __attribute__((visibility("default"))) void ZetClose() {}
    __attribute__((visibility("default"))) void ZetReset() {}
    __attribute__((visibility("default"))) INT32 ZetScan(INT32 nAction) { return 0; }
    __attribute__((visibility("default"))) INT32 ZetTotalCycles() { return 0; }
    __attribute__((visibility("default"))) INT32 ZetMemCallback(INT32 nStart, INT32 nEnd, INT32 nMode) { return 0; }
    __attribute__((visibility("default"))) void ZetSetReadHandler(UINT8 (*pHandler)(UINT16)) {}
    __attribute__((visibility("default"))) void ZetSetWriteHandler(void (*pHandler)(UINT16, UINT8)) {}

    // CPS functions
    __attribute__((visibility("default"))) INT32 Cps2Init() { return 0; }
    __attribute__((visibility("default"))) INT32 CpsInit() { return 0; }
    __attribute__((visibility("default"))) INT32 CpsExit() { return 0; }

    // Sound functions
    __attribute__((visibility("default"))) INT32 PsndInit() { return 0; }
    __attribute__((visibility("default"))) INT32 PsndExit() { return 0; }
    __attribute__((visibility("default"))) void PsndNewFrame() {}
    __attribute__((visibility("default"))) void PsndEndFrame() {}
    __attribute__((visibility("default"))) INT32 PsndSyncZ80(INT32 nCycles) { return 0; }
    __attribute__((visibility("default"))) INT32 PsmUpdateEnd() { return 0; }
    __attribute__((visibility("default"))) INT32 QsndInit() { return 0; }
    __attribute__((visibility("default"))) void QsndReset() {}
    __attribute__((visibility("default"))) void QsndExit() {}
    __attribute__((visibility("default"))) void QsndNewFrame() {}
    __attribute__((visibility("default"))) void QsndEndFrame() {}
    __attribute__((visibility("default"))) void QsndSyncZ80() {}
    __attribute__((visibility("default"))) INT32 QsndScan(INT32 nAction) { return 0; }
    __attribute__((visibility("default"))) void QsndSetRoute(INT32 nIndex, double nVolume, INT32 nRouteDir) {}

    // Sample functions
    __attribute__((visibility("default"))) void BurnSampleInit(INT32 nSampleRate) {}
    __attribute__((visibility("default"))) void BurnSampleSetRoute(INT32 nSample, INT32 nChannel, double flVolume, INT32 nRouteDir) {}
    __attribute__((visibility("default"))) INT32 BurnSoundRender(INT16 *pDst, INT32 nLen) { return 0; }
    __attribute__((visibility("default"))) void BurnSampleExit() {}
    __attribute__((visibility("default"))) INT32 BurnSampleScan(INT32 nAction, INT32 *pnMin) { return 0; }
    __attribute__((visibility("default"))) void BurnSampleRender(INT16 *pDest, UINT32 nLen) {}
    __attribute__((visibility("default"))) INT32 BurnSampleGetChannelStatus(INT32 nSample) { return 0; }
    __attribute__((visibility("default"))) void BurnSampleChannelPlay(INT32 channel, INT32 sample, bool loop) {}

    // Trackball functions
    __attribute__((visibility("default"))) void BurnTrackballInit(INT32 nPlayer) {}
    __attribute__((visibility("default"))) void BurnTrackballConfig(INT32 nPlayer, INT32 nDeviceType, INT32 nDevID) {}
    __attribute__((visibility("default"))) void BurnTrackballFrame(int iPlayer, int x1, int y1, int x2, int y2, int *a4, int *a5) {}
    __attribute__((visibility("default"))) void BurnTrackballUpdate(INT32 nPlayer) {}
    __attribute__((visibility("default"))) INT32 BurnTrackballReadSigned(INT32 nPlayer) { return 0; }
    __attribute__((visibility("default"))) INT32 BurnTrackballGetDirection(INT32 nPlayer) { return 0; }
    __attribute__((visibility("default"))) void BurnTrackballReadReset() {}

    // NeoCD functions
    __attribute__((visibility("default"))) INT32 NeoCDInfo_ID() { return 0; }
    __attribute__((visibility("default"))) INT32 NeoCDInfo_Text(int nID) { return 0; }

    // Input functions
    __attribute__((visibility("default"))) void FreezeInput(UINT8 **buf, INT32 *size) {}
    __attribute__((visibility("default"))) INT32 UnfreezeInput(const UINT8 *buf, INT32 size) { return 0; }

    // Gun functions
    __attribute__((visibility("default"))) INT32 BurnGunInit(INT32 nNumPlayers, INT32 bDrawTargets) { return 0; }
    __attribute__((visibility("default"))) void BurnGunExit() {}
    __attribute__((visibility("default"))) void BurnGunMakeInputs(INT32 nPlayer, INT16 nXPosRaw, INT16 nYPosRaw) {}
    __attribute__((visibility("default"))) void BurnGunDrawTarget(INT32 num, INT32 x, INT32 y) {}
    __attribute__((visibility("default"))) UINT8 BurnGunReturnX(INT32 nPlayer) { return 0; }
    __attribute__((visibility("default"))) UINT8 BurnGunReturnY(INT32 nPlayer) { return 0; }
    __attribute__((visibility("default"))) void BurnGunScan() {}

    // SEK functions
    __attribute__((visibility("default"))) INT32 SekInit(INT32 nCount, INT32 nCPUType) { return 0; }
    __attribute__((visibility("default"))) void SekExit() {}
    __attribute__((visibility("default"))) void SekNewFrame() {}
    __attribute__((visibility("default"))) INT32 SekReset() { return 0; }
    __attribute__((visibility("default"))) INT32 SekOpen(const INT32 i) { return 0; }
    __attribute__((visibility("default"))) INT32 SekClose() { return 0; }
    __attribute__((visibility("default"))) INT32 SekScan(INT32 nAction) { return 0; }

    // CTV function
    __attribute__((visibility("default"))) INT32 CtvReady() { return 0; }
    __attribute__((visibility("default"))) INT32 PsndScan(INT32 nAction, INT32* pnMin) { return 0; }
} 