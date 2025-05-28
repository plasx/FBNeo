// This file directly exports all functions with C linkage needed by the linker
// No includes to avoid conflicts with core headers

// Type definitions needed for our function signatures
typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef int INT32;
typedef short INT16;

#define MY_EXPORT __attribute__((visibility("default")))

// Direct function exports with C linkage
extern "C" {
    // CPS functions
    MY_EXPORT int Cps2Init() { return 0; }
    MY_EXPORT int CpsInit() { return 0; }
    MY_EXPORT int CpsExit() { return 0; }
    
    // Z80 functions
    MY_EXPORT int ZetInit(int nCount) { return 0; }
    MY_EXPORT void ZetExit() {}
    MY_EXPORT void ZetOpen(int nCPU) {}
    MY_EXPORT void ZetClose() {}
    MY_EXPORT void ZetReset() {}
    MY_EXPORT int ZetScan(int nAction) { return 0; }
    MY_EXPORT int ZetTotalCycles() { return 0; }
    MY_EXPORT int ZetMemCallback(int nStart, int nEnd, int nMode) { return 0; }
    MY_EXPORT void ZetSetReadHandler(unsigned char (*pHandler)(unsigned short)) {}
    MY_EXPORT void ZetSetWriteHandler(void (*pHandler)(unsigned short, unsigned char)) {}
    MY_EXPORT void Z80StopExecute() {}
    MY_EXPORT int z80TotalCycles() { return 0; }
    
    // Sound functions
    MY_EXPORT int PsndInit() { return 0; }
    MY_EXPORT int PsndExit() { return 0; }
    MY_EXPORT void PsndNewFrame() {}
    MY_EXPORT void PsndEndFrame() {}
    MY_EXPORT int PsndSyncZ80(int nCycles) { return 0; }
    MY_EXPORT int PsmUpdateEnd() { return 0; }
    MY_EXPORT int QsndInit() { return 0; }
    MY_EXPORT void QsndReset() {}
    MY_EXPORT void QsndExit() {}
    MY_EXPORT void QsndNewFrame() {}
    MY_EXPORT void QsndEndFrame() {}
    MY_EXPORT void QsndSyncZ80() {}
    MY_EXPORT int QsndScan(int nAction) { return 0; }
    MY_EXPORT void QsndSetRoute(int nIndex, double nVolume, int nRouteDir) {}
    
    // Sample functions
    MY_EXPORT void BurnSampleInit(int nSampleRate) {}
    MY_EXPORT void BurnSampleSetRoute(int nSample, int nChannel, double flVolume, int nRouteDir) {}
    MY_EXPORT int BurnSoundRender(short* pDst, int nLen) { return 0; }
    MY_EXPORT void BurnSampleExit() {}
    MY_EXPORT int BurnSampleScan(int nAction, int* pnMin) { return 0; }
    MY_EXPORT void BurnSampleRender(short* pDest, unsigned int nLen) {}
    MY_EXPORT int BurnSampleGetChannelStatus(int nSample) { return 0; }
    MY_EXPORT void BurnSampleChannelPlay(int channel, int sample, bool loop) {}
    
    // Trackball functions
    MY_EXPORT void BurnTrackballInit(int nNumPlayers) {}
    MY_EXPORT void BurnTrackballConfig(int nPlayer, int nDeviceType, int nDevID) {}
    MY_EXPORT void BurnTrackballFrame(int iPlayer, int x1, int y1, int x2, int y2, int* a4, int* a5) {}
    MY_EXPORT void BurnTrackballUpdate(int nPlayer) {}
    MY_EXPORT int BurnTrackballReadSigned(int nPlayer) { return 0; }
    MY_EXPORT int BurnTrackballGetDirection(int nPlayer) { return 0; }
    MY_EXPORT void BurnTrackballReadReset() {}
    
    // NeoCD functions
    MY_EXPORT int NeoCDInfo_ID() { return 0; }
    MY_EXPORT int NeoCDInfo_Text(int nID) { return 0; }
    
    // Input functions
    MY_EXPORT void FreezeInput(unsigned char** buf, int* size) {}
    MY_EXPORT int UnfreezeInput(const unsigned char* buf, int size) { return 0; }
    
    // Gun functions
    MY_EXPORT void BurnGunExit() {}
    MY_EXPORT int BurnGunInit(int nNumPlayers, int bDrawTargets) { return 0; }
    MY_EXPORT void BurnGunMakeInputs(int nPlayer, short nXPosRaw, short nYPosRaw) {}
    MY_EXPORT void BurnGunDrawTarget(int num, int x, int y) {}
    MY_EXPORT unsigned char BurnGunReturnX(int nPlayer) { return 0; }
    MY_EXPORT unsigned char BurnGunReturnY(int nPlayer) { return 0; }
    MY_EXPORT void BurnGunScan() {}
    
    // Sek functions
    MY_EXPORT int SekSetResetCallback(int (*pCallback)()) { return 0; }
    MY_EXPORT int SekSetReadByteHandler(int i, unsigned char (*pHandler)(unsigned int)) { return 0; }
    MY_EXPORT int SekSetReadWordHandler(int i, unsigned short (*pHandler)(unsigned int)) { return 0; }
    MY_EXPORT int SekScan(int nAction) { return 0; }
    
    // CTV functions
    MY_EXPORT int CtvReady() { return 0; }
} 