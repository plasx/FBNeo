#include <stdio.h>

// Type definitions needed for our function signatures
typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef int INT32;
typedef short INT16;

// Define all the public C functions
extern "C" {
    // Z80 functions
    __attribute__((visibility("default"))) void Z80StopExecute(void) { /* stub */ }
    __attribute__((visibility("default"))) INT32 z80TotalCycles(void) { return 0; }
    __attribute__((visibility("default"))) INT32 ZetInit(INT32 nCPU) { return 0; }
    __attribute__((visibility("default"))) void ZetExit(void) { }
    __attribute__((visibility("default"))) void ZetOpen(INT32 nCPU) { }
    __attribute__((visibility("default"))) void ZetClose(void) { }
    __attribute__((visibility("default"))) void ZetReset(void) { }
    __attribute__((visibility("default"))) INT32 ZetScan(INT32 nAction) { return 0; }
    __attribute__((visibility("default"))) INT32 ZetTotalCycles(void) { return 0; }
    __attribute__((visibility("default"))) INT32 ZetMemCallback(INT32 nStart, INT32 nEnd, INT32 nMode) { return 0; }
    __attribute__((visibility("default"))) void ZetSetReadHandler(UINT8 (*pHandler)(UINT16)) { }
    __attribute__((visibility("default"))) void ZetSetWriteHandler(void (*pHandler)(UINT16, UINT8)) { }
    
    // CPS functions
    __attribute__((visibility("default"))) INT32 Cps2Init(void) { return 0; }
    __attribute__((visibility("default"))) INT32 CpsInit(void) { return 0; }
    __attribute__((visibility("default"))) INT32 CpsExit(void) { return 0; }
    
    // Sound functions
    __attribute__((visibility("default"))) INT32 PsndInit(void) { return 0; }
    __attribute__((visibility("default"))) INT32 PsndExit(void) { return 0; }
    __attribute__((visibility("default"))) void PsndNewFrame(void) { }
    __attribute__((visibility("default"))) void PsndEndFrame(void) { }
    __attribute__((visibility("default"))) INT32 PsndSyncZ80(INT32 nCycles) { return 0; }
    __attribute__((visibility("default"))) INT32 PsmUpdateEnd(void) { return 0; }
    __attribute__((visibility("default"))) INT32 QsndInit(void) { return 0; }
    __attribute__((visibility("default"))) void QsndReset(void) { }
    __attribute__((visibility("default"))) void QsndExit(void) { }
    __attribute__((visibility("default"))) void QsndNewFrame(void) { }
    __attribute__((visibility("default"))) void QsndEndFrame(void) { }
    __attribute__((visibility("default"))) void QsndSyncZ80(void) { }
    __attribute__((visibility("default"))) INT32 QsndScan(INT32 nAction) { return 0; }
    __attribute__((visibility("default"))) void QsndSetRoute(INT32 nIndex, double nVolume, INT32 nRouteDir) { }
    
    // Sample functions
    __attribute__((visibility("default"))) void BurnSampleInit(INT32 nSampleRate) { }
    __attribute__((visibility("default"))) void BurnSampleSetRoute(INT32 nSample, INT32 nChannel, double flVolume, INT32 nRouteDir) { }
    __attribute__((visibility("default"))) INT32 BurnSoundRender(INT16 *pDst, INT32 nLen) { return 0; }
    
    // Trackball functions
    __attribute__((visibility("default"))) void BurnTrackballInit(INT32 nPlayer) { }
    __attribute__((visibility("default"))) void BurnTrackballConfig(INT32 nPlayer, INT32 nDeviceType, INT32 nDevID) { }
    __attribute__((visibility("default"))) void BurnTrackballFrame(int iPlayer, int x1, int y1, int x2, int y2, int *a4, int *a5) { }
    __attribute__((visibility("default"))) void BurnTrackballUpdate(INT32 nPlayer) { }
    __attribute__((visibility("default"))) INT32 BurnTrackballReadSigned(INT32 nPlayer) { return 0; }
    __attribute__((visibility("default"))) INT32 BurnTrackballGetDirection(INT32 nPlayer) { return 0; }
    __attribute__((visibility("default"))) void BurnTrackballReadReset(void) { }
    
    // NeoCD functions
    __attribute__((visibility("default"))) INT32 NeoCDInfo_ID(void) { return 0; }
    __attribute__((visibility("default"))) INT32 NeoCDInfo_Text(int nID) { return 0; }
    
    // Input functions
    __attribute__((visibility("default"))) void FreezeInput(UINT8 **buf, INT32 *size) { }
    __attribute__((visibility("default"))) INT32 UnfreezeInput(const UINT8 *buf, INT32 size) { return 0; }
    
    // Gun functions
    __attribute__((visibility("default"))) INT32 BurnGunInit(INT32 nNumPlayers, INT32 bDrawTargets) { return 0; }
    __attribute__((visibility("default"))) void BurnGunExit(void) { }
} 