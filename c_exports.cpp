#include "src/burner/metal/fixes/c_symbols_header.h"

// We define a special symbol that will be used by the Metal framework to force-include this file
// This ensures our exports are always included
extern "C" __attribute__((visibility("default"))) void *__metal_force_include_symbol = 0;

// These functions are defined as C linkage to ensure they are exported with the correct names
extern "C" {
    // Z80 functions
    void Z80StopExecute() {}
    INT32 z80TotalCycles() { return 0; }
    INT32 ZetInit(INT32 nCPU) { return 0; }
    void ZetExit() {}
    void ZetOpen(INT32 nCPU) {}
    void ZetClose() {}
    void ZetReset() {}
    INT32 ZetScan(INT32 nAction) { return 0; }
    INT32 ZetTotalCycles() { return 0; }
    INT32 ZetMemCallback(INT32 nStart, INT32 nEnd, INT32 nMode) { return 0; }
    void ZetSetReadHandler(UINT8 (*pHandler)(UINT16)) {}
    void ZetSetWriteHandler(void (*pHandler)(UINT16, UINT8)) {}

    // CPS functions
    INT32 Cps2Init() { return 0; }
    INT32 CpsInit() { return 0; }
    INT32 CpsExit() { return 0; }

    // Sound functions
    INT32 PsndInit() { return 0; }
    INT32 PsndExit() { return 0; }
    void PsndNewFrame() {}
    void PsndEndFrame() {}
    INT32 PsndSyncZ80(INT32 nCycles) { return 0; }
    INT32 PsmUpdateEnd() { return 0; }
    INT32 QsndInit() { return 0; }
    void QsndReset() {}
    void QsndExit() {}
    void QsndNewFrame() {}
    void QsndEndFrame() {}
    void QsndSyncZ80() {}
    INT32 QsndScan(INT32 nAction) { return 0; }
    void QsndSetRoute(INT32 nIndex, double nVolume, INT32 nRouteDir) {}

    // Sample functions
    void BurnSampleInit(INT32 nSampleRate) {}
    void BurnSampleSetRoute(INT32 nSample, INT32 nChannel, double flVolume, INT32 nRouteDir) {}
    INT32 BurnSoundRender(INT16 *pDst, INT32 nLen) { return 0; }
    void BurnSampleExit() {}
    INT32 BurnSampleScan(INT32 nAction, INT32 *pnMin) { return 0; }
    void BurnSampleRender(INT16 *pDest, UINT32 nLen) {}
    INT32 BurnSampleGetChannelStatus(INT32 nSample) { return 0; }
    void BurnSampleChannelPlay(INT32 channel, INT32 sample, bool loop) {}

    // Trackball functions
    void BurnTrackballInit(INT32 nPlayer) {}
    void BurnTrackballConfig(INT32 nPlayer, INT32 nDeviceType, INT32 nDevID) {}
    void BurnTrackballFrame(int iPlayer, int x1, int y1, int x2, int y2, int *a4, int *a5) {}
    void BurnTrackballUpdate(INT32 nPlayer) {}
    INT32 BurnTrackballReadSigned(INT32 nPlayer) { return 0; }
    INT32 BurnTrackballGetDirection(INT32 nPlayer) { return 0; }
    void BurnTrackballReadReset() {}

    // NeoCD functions
    INT32 NeoCDInfo_ID() { return 0; }
    INT32 NeoCDInfo_Text(int nID) { return 0; }

    // Input functions
    void FreezeInput(UINT8 **buf, INT32 *size) {}
    INT32 UnfreezeInput(const UINT8 *buf, INT32 size) { return 0; }

    // Gun functions
    INT32 BurnGunInit(INT32 nNumPlayers, INT32 bDrawTargets) { return 0; }
    void BurnGunExit() {}
    void BurnGunMakeInputs(INT32 nPlayer, INT16 nXPosRaw, INT16 nYPosRaw) {}
    void BurnGunDrawTarget(INT32 num, INT32 x, INT32 y) {}
    UINT8 BurnGunReturnX(INT32 nPlayer) { return 0; }
    UINT8 BurnGunReturnY(INT32 nPlayer) { return 0; }
    void BurnGunScan() {}
    
    // SEK functions
    INT32 SekInit(INT32 nCount, INT32 nCPUType) { return 0; }
    void SekExit() {}
    void SekNewFrame() {}
    INT32 SekReset() { return 0; }
    INT32 SekOpen(const INT32 i) { return 0; }
    INT32 SekClose() { return 0; }
    INT32 SekScan(INT32 nAction) { return 0; }

    // CTV function
    INT32 CtvReady() { return 0; }
    INT32 PsndScan(INT32 nAction, INT32* pnMin) { return 0; }
} 