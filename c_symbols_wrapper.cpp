// Wrapper file to properly link C functions from c_symbols.c with C++ code
// This provides the correct extern "C" linkage for the linker

// Include types and basic definitions
#include "src/burner/metal/fixes/c_symbols_header.h"

// Explicitly declare all symbols with extern "C" to ensure proper linkage
extern "C" {
    // Z80 functions
    void Z80StopExecute();
    INT32 z80TotalCycles();
    INT32 ZetInit(INT32 nCPU);
    void ZetExit();
    void ZetOpen(INT32 nCPU);
    void ZetClose();
    void ZetReset();
    INT32 ZetScan(INT32 nAction);
    INT32 ZetTotalCycles();
    INT32 ZetMemCallback(INT32 nStart, INT32 nEnd, INT32 nMode);
    void ZetSetReadHandler(UINT8 (*pHandler)(UINT16));
    void ZetSetWriteHandler(void (*pHandler)(UINT16, UINT8));
    
    // CPS functions
    INT32 Cps2Init();
    INT32 CpsInit();
    INT32 CpsExit();
    
    // Sound functions
    INT32 PsndInit();
    INT32 PsndExit();
    void PsndNewFrame();
    void PsndEndFrame();
    INT32 PsndSyncZ80(INT32 nCycles);
    INT32 PsmUpdateEnd();
    INT32 QsndInit();
    void QsndReset();
    void QsndExit();
    void QsndNewFrame();
    void QsndEndFrame();
    void QsndSyncZ80();
    INT32 QsndScan(INT32 nAction);
    void QsndSetRoute(INT32 nIndex, double nVolume, INT32 nRouteDir);
    
    // Sample functions
    void BurnSampleInit(INT32 nSampleRate);
    void BurnSampleSetRoute(INT32 nSample, INT32 nChannel, double flVolume, INT32 nRouteDir);
    INT32 BurnSoundRender(INT16 *pDst, INT32 nLen);
    void BurnSampleExit();
    INT32 BurnSampleScan(INT32 nAction, INT32 *pnMin);
    void BurnSampleRender(INT16 *pDest, UINT32 nLen);
    INT32 BurnSampleGetChannelStatus(INT32 nSample);
    void BurnSampleChannelPlay(INT32 channel, INT32 sample, bool loop);
    
    // Trackball functions
    void BurnTrackballInit(INT32 nPlayer);
    void BurnTrackballConfig(INT32 nPlayer, INT32 nDeviceType, INT32 nDevID);
    void BurnTrackballFrame(int iPlayer, int x1, int y1, int x2, int y2, int *a4, int *a5);
    void BurnTrackballUpdate(INT32 nPlayer);
    INT32 BurnTrackballReadSigned(INT32 nPlayer);
    INT32 BurnTrackballGetDirection(INT32 nPlayer);
    void BurnTrackballReadReset();
    
    // NeoCD functions
    INT32 NeoCDInfo_ID();
    INT32 NeoCDInfo_Text(int nID);
    
    // Input functions
    void FreezeInput(UINT8 **buf, INT32 *size);
    INT32 UnfreezeInput(const UINT8 *buf, INT32 size);
    
    // Gun functions
    INT32 BurnGunInit(INT32 nNumPlayers, INT32 bDrawTargets);
    void BurnGunExit();
    void BurnGunMakeInputs(INT32 nPlayer, INT16 nXPosRaw, INT16 nYPosRaw);
    void BurnGunDrawTarget(INT32 num, INT32 x, INT32 y);
    UINT8 BurnGunReturnX(INT32 nPlayer);
    UINT8 BurnGunReturnY(INT32 nPlayer);
    void BurnGunScan();

    // SEK functions
    INT32 SekInit(INT32 nCount, INT32 nCPUType);
    void SekExit();
    void SekNewFrame();
    INT32 SekReset();
    INT32 SekOpen(const INT32 i);
    INT32 SekClose();
    INT32 SekScan(INT32 nAction);

    // CTV function
    INT32 CtvReady();
    INT32 PsndScan(INT32 nAction, INT32* pnMin);
} 