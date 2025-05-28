#!/bin/bash

# Create a temp directory
mkdir -p tmp

# Create a C file that includes the necessary header files and defines implementation stubs
cat > tmp/linkage_fix.c << 'EOF'
#include <stdio.h>

// Type definitions needed for our function signatures
typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef int INT32;
typedef short INT16;

#define EXTERNC extern

// Z80 functions
EXTERNC void Z80StopExecute(void) { /* stub */ }
EXTERNC INT32 z80TotalCycles(void) { return 0; }
EXTERNC INT32 ZetInit(INT32 nCPU) { return 0; }
EXTERNC void ZetExit(void) { }
EXTERNC void ZetOpen(INT32 nCPU) { }
EXTERNC void ZetClose(void) { }
EXTERNC void ZetReset(void) { }
EXTERNC INT32 ZetScan(INT32 nAction) { return 0; }
EXTERNC INT32 ZetTotalCycles(void) { return 0; }
EXTERNC INT32 ZetMemCallback(INT32 nStart, INT32 nEnd, INT32 nMode) { return 0; }
EXTERNC void ZetSetReadHandler(UINT8 (*pHandler)(UINT16)) { }
EXTERNC void ZetSetWriteHandler(void (*pHandler)(UINT16, UINT8)) { }

// CPS functions
EXTERNC INT32 Cps2Init(void) { return 0; }
EXTERNC INT32 CpsInit(void) { return 0; }
EXTERNC INT32 CpsExit(void) { return 0; }

// Sound functions
EXTERNC INT32 PsndInit(void) { return 0; }
EXTERNC INT32 PsndExit(void) { return 0; }
EXTERNC void PsndNewFrame(void) { }
EXTERNC void PsndEndFrame(void) { }
EXTERNC INT32 PsndSyncZ80(INT32 nCycles) { return 0; }
EXTERNC INT32 PsmUpdateEnd(void) { return 0; }
EXTERNC INT32 QsndInit(void) { return 0; }
EXTERNC void QsndReset(void) { }
EXTERNC void QsndExit(void) { }
EXTERNC void QsndNewFrame(void) { }
EXTERNC void QsndEndFrame(void) { }
EXTERNC void QsndSyncZ80(void) { }
EXTERNC INT32 QsndScan(INT32 nAction) { return 0; }
EXTERNC void QsndSetRoute(INT32 nIndex, double nVolume, INT32 nRouteDir) { }

// Sample functions
EXTERNC void BurnSampleInit(INT32 nSampleRate) { }
EXTERNC void BurnSampleSetRoute(INT32 nSample, INT32 nChannel, double flVolume, INT32 nRouteDir) { }
EXTERNC INT32 BurnSoundRender(INT16 *pDst, INT32 nLen) { return 0; }

// Trackball functions
EXTERNC void BurnTrackballInit(INT32 nPlayer) { }
EXTERNC void BurnTrackballConfig(INT32 nPlayer, INT32 nDeviceType, INT32 nDevID) { }
EXTERNC void BurnTrackballFrame(int iPlayer, int x1, int y1, int x2, int y2, int *a4, int *a5) { }
EXTERNC void BurnTrackballUpdate(INT32 nPlayer) { }
EXTERNC INT32 BurnTrackballReadSigned(INT32 nPlayer) { return 0; }
EXTERNC INT32 BurnTrackballGetDirection(INT32 nPlayer) { return 0; }
EXTERNC void BurnTrackballReadReset(void) { }

// NeoCD functions
EXTERNC INT32 NeoCDInfo_ID(void) { return 0; }
EXTERNC INT32 NeoCDInfo_Text(int nID) { return 0; }

// Input functions
EXTERNC void FreezeInput(UINT8 **buf, INT32 *size) { }
EXTERNC INT32 UnfreezeInput(const UINT8 *buf, INT32 size) { return 0; }

// Gun functions
EXTERNC INT32 BurnGunInit(INT32 nNumPlayers, INT32 bDrawTargets) { return 0; }
EXTERNC void BurnGunExit(void) { }
EOF

# Compile the object file
clang -c tmp/linkage_fix.c -o link_fix.o

# Create a simple C++ file to include the symbols
cat > link_fix.cpp << 'EOF'
// Empty file to ensure linking
EOF

echo "Linkage fix object created: link_fix.o" 