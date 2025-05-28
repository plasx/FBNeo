#pragma once

#ifndef DIRECT_MINIMAL_H
#define DIRECT_MINIMAL_H

#include "../../../burn/burnint.h"
#include "c_linkage_bridge.h"

// Forward declaration for cpu_core_config
struct cpu_core_config;

// All declarations are now in c_linkage_bridge.h
// This header is kept for backward compatibility

// These declarations don't need extern "C" since they're included
// by headers that already have the proper language linkage

// Z80 CPU functions
// INT32 ZetTotalCycles();
// void ZetRunEnd();
// INT32 ZetIdle(INT32 nCycles);

// Gun functions
// INT32 BurnGunInit(INT32 nNumPlayers, INT32 bDrawTargets);
// void BurnGunExit();
// void BurnGunMakeInputs(INT32 nPlayer, INT16 nXPosRaw, INT16 nYPosRaw);
// void BurnGunDrawTarget(INT32 num, INT32 x, INT32 y);
// UINT8 BurnGunReturnX(INT32 nPlayer);
// UINT8 BurnGunReturnY(INT32 nPlayer);
// void BurnGunScan();

#ifdef __cplusplus
extern "C" {
#endif

UINT32 ZetGetPC(INT32 n);
void ZetSetIRQLine(INT32 nIRQLine, INT32 nStatus);

#ifdef __cplusplus
}
#endif

#endif // DIRECT_MINIMAL_H 