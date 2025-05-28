#pragma once

#ifndef CTV_FIXES_H
#define CTV_FIXES_H

#include "../../../burn/burnint.h"

#ifdef __cplusplus
extern "C" {
#endif

// CTV variables
extern UINT32 _nCtvRollX;
extern UINT32 _nCtvRollY;
extern int _nCtvTileAdd;
extern UINT8* _pCtvLine;
extern unsigned char* _pCtvTile;

// CTV functions
extern void CtvDoX(INT32 nField);
extern void CtvDoXB(INT32 nField);
extern void CtvDoXM(INT32 nField);

// Other missing function declarations
extern void BurnGunExit();
extern INT32 BurnGunInit(INT32 nNumPlayers, INT32 bDrawTargets);
extern void BurnGunMakeInputs(INT32 nPlayer, INT16 nXPosRaw, INT16 nYPosRaw);
extern void BurnGunRender(INT32 nPlayer, INT32 x, INT32 y);
extern void BurnGunScan(void);
extern INT32 BurnGunReturnX(INT32 nPlayer);
extern INT32 BurnGunReturnY(INT32 nPlayer);
extern void FreezeInput(unsigned char **buf, int *size);
extern int UnfreezeInput(const unsigned char *buf, int size);
extern void PsndSyncZ80(INT32 cycles);
extern void SekNewFrame(void);
extern INT32 TimerInit(void);
extern void TimerExit(void);
extern INT32 TimerReset(void);
extern INT32 TimerScan(INT32 nAction);
extern void TimerSetTimeOut(INT32 nCpuNum, INT32 (*pTimeoutHandler)(INT32), double nTimeOut);
extern void TimerUpdateEndThisFrame(void);

#ifdef __cplusplus
}
#endif

#endif // CTV_FIXES_H