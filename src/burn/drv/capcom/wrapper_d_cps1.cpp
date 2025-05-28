#ifndef WRAPPER_D_CPS1_CPP
#define WRAPPER_D_CPS1_CPP

#include <stdint.h>
#include "../../../burner/metal/fixes/simple_metal_wrapper.h"
#include "../../../burner/metal/fixes/cps_fixes.h" // Include our CPS fixes
#include "../../../burner/metal/fixes/wrapper_fixes.h" // Include wrapper fixes

// This is a wrapper file that includes the original d_cps1.cpp
// after applying fixes for the Metal build

// Define all header guards to prevent multiple inclusions
#define eeprom_h
#define INCLUDE_EEPROM_H
#define m68000_intf_h
#define msm6295_h
#define samples_h
#define z80_intf_h
#define timer_h
#define burn_ym2151_h
#define burnint_h

// Include our eeprom wrapper
#include "../../devices/eeprom.h"

// Include extern "C" declarations for all the functions needed by d_cps1.cpp
extern "C" {
    // CPS drawing functions
    int Cps1rRender();
    int Cps2rRender();
    int Cps1rPrepare();
    int Cps2rPrepare();
    int Cps1Scr1Draw(unsigned char* pSrc, int nSrcPitch, int nSize);
    int Cps1Scr3Draw(unsigned char* pSrc, int nSrcPitch, int nSize);
    int Cps2Scr1Draw(unsigned char* pSrc, int nSrcPitch, int nSize);
    int Cps2Scr3Draw(unsigned char* pSrc, int nSrcPitch, int nSize);
    int CtvReady();

    // CPS2 functions for wrappers
    UINT8* CpsFindGfxRam(INT32 nOffset, INT32 nLen);
    void GetPalette(INT32 nStart, INT32 nCount);

    // CPU-related functions
    void SekNewFrame();
    int SekInit(int nCount, int nCPUType);
    void SekExit();
    void SekOpen(int i);
    void SekClose();
    int SekScan(int nAction);
    int SekRun(int nCycles);
    void SekSetIRQLine(const int line, const int status);
    void SekSetCyclesScanline(int nCycles);
    int SekSetResetCallback(int (*pCallback)());
    int SekMapMemory(unsigned char* pMemory, unsigned int nStart, unsigned int nEnd, int nType);
    int SekMapHandler(unsigned long nHandler, unsigned int nStart, unsigned int nEnd, int nType);
    int SekSetReadByteHandler(int i, unsigned char (*pHandler)(unsigned int));
    int SekSetWriteByteHandler(int i, void (*pHandler)(unsigned int, unsigned char));
    int SekSetReadWordHandler(int i, unsigned short (*pHandler)(unsigned int));
    int SekSetWriteWordHandler(int i, void (*pHandler)(unsigned int, unsigned short));
    void SekReset();

    // Z80-related functions
    void ZetNewFrame();
    int ZetInit(int nCount);
    void ZetExit();
    void ZetOpen(int nCPU);
    void ZetClose();
    void ZetReset();
    int ZetIdle(int nCycles);
    int ZetTotalCycles();
    void ZetSetIRQLine(const int line, const int status);
    int ZetScan(int nAction);
    int ZetMemCallback(int nStart, int nEnd, int nMode);
    void ZetSetReadHandler(unsigned char (*pHandler)(unsigned short));
    void ZetSetWriteHandler(void (*pHandler)(unsigned short, unsigned char));

    // Sound-related functions
    int PsndInit();
    int PsndExit();
    int PsndScan(int nAction, int* pnMin);
    void PsndNewFrame();
    void PsndEndFrame();
    int PsmUpdateEnd();
    int PsndSyncZ80(int nCycles);

    // Timer functions
    int BurnTimerInit(int (*pOverCallback)(int, int), double (*pTimeCallback)(), int nCPU);
    int BurnTimerAttach(void* pCC, int nClockspeed);
    void BurnTimerSetRetrig(int c, double period);
    int BurnSoundRender(short* pDest, int nLen);

    // Sample functions - fix return types to match actual implementations
    void BurnSampleExit();
    void BurnSampleInit(int bAdd);
    void BurnSampleScan(int nAction, int* pnMin);
    void BurnSampleRender(short* pDest, unsigned int nLen);
    int BurnSampleGetChannelStatus(int nSample);
    void BurnSampleChannelPlay(int channel, int sample, bool loop);
    void BurnSampleSetRoute(int nSample, int nOutput, double nVolume, int nRouteType);

    // Input functions
    void FreezeInput(unsigned char** buf, int* size);
    int UnfreezeInput(const unsigned char* buf, int size);

    // NeoCD info functions
    int NeoCDInfo_ID();
    int NeoCDInfo_Text(int nID);  // Changed to int to match implementation
}

// Include the original file
#include "d_cps1.cpp" 

#endif // WRAPPER_D_CPS1_CPP 