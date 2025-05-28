// Z80 (Zed Eight-Ty) Interface

#ifndef _Z80_INTF_H
#define _Z80_INTF_H

#ifdef __cplusplus
extern "C" {
#endif

// Remove duplicate __fastcall definition
#undef __fastcall
#define __fastcall

// These are not defined in z80.h, but they should be
#define Z80_CLEAR_LINE		0
#define Z80_ASSERT_LINE		1
#define Z80_HOLD_LINE		2

// This way of setting of IRQs doesn't work with the z80 core
#define Z80_IRQSTATUS_NONE	0
#define Z80_IRQSTATUS_ACK	1
#define Z80_IRQSTATUS_AUTO	2

#include "z80.h"

// Z80 Burn Timer Helpers
#define ZetROMSize 0
#define ZetRAMSize 1

// For backwards compatibility
#define ZET_IRQSTATUS_NONE	Z80_IRQSTATUS_NONE
#define ZET_IRQSTATUS_ACK	Z80_IRQSTATUS_ACK
#define ZET_IRQSTATUS_AUTO	Z80_IRQSTATUS_AUTO

extern INT32 nZetCycle[4];
extern INT32 nZ80ICount;

INT32 ZetInit(INT32 nCount);
void ZetExit();
void ZetNewFrame();
void ZetOpen(INT32 nCPU);
void ZetClose();
INT32 ZetGetActive();
void ZetAllCpusIdle();

// CPU Core functions
INT32 ZetRun(INT32 nCycles);
void ZetRunEnd();
void ZetRunAdjust(INT32 nCycles);
INT32 ZetSegmentCycles();
INT32 ZetTotalCycles();
void ZetReset();

// Status register functions
INT32 ZetGetPC(INT32 n);
void ZetSetPC(INT32 n);
void ZetSetSP(INT32 n);
INT32 ZetGetSP();
void ZetSetReg(INT32 nReg, UINT16 nValue);
UINT16 ZetGetReg(INT32 nReg);
INT32 ZetGetRealPC();
INT32 ZetGetBC();
INT32 ZetGetHL();
INT32 ZetGetSP2();
void ZetSetBC(INT32 n);
void ZetSetHL(INT32 n);
void ZetSetDE(INT32 n);

// IRQ functionality
void ZetSetIRQLine(INT32 nStatus, INT32 nLine);
void ZetRaiseIrq(INT32 nCPU, INT32 nStatus);
void ZetLowerIrq(INT32 nCPU);

// For debugging/logging
INT32 ZetGetAF();
INT32 ZetGetAF2();
INT32 ZetGetBC2();
INT32 ZetGetDE();
INT32 ZetGetDE2();
INT32 ZetGetHL2();
INT32 ZetGetI();
INT32 ZetGetIM();
INT32 ZetGetIX();
INT32 ZetGetIY();
INT32 ZetGetR();
INT32 ZetGetPrevPC();
INT32 ZetGetVector();
void ZetSetAF(INT32 n);
void ZetSetAF2(INT32 n);
void ZetSetBC2(INT32 n);
void ZetSetDE2(INT32 n);
void ZetSetHL2(INT32 n);
void ZetSetI(INT32 n);
void ZetSetIX(INT32 n);
void ZetSetIY(INT32 n);
void ZetSetR(INT32 n);
void ZetSetVector(INT32 n);

// Memory management
INT32 ZetMapArea(INT32 nStart, INT32 nEnd, INT32 nMode, UINT8* Mem);
// Different version with separate read/write memory maps - in METAL implementation we'll use only the first version
#if !defined(USE_METAL_RENDERER)
INT32 ZetMapArea(INT32 nStart, INT32 nEnd, INT32 nMode, UINT8* Mem01, UINT8* Mem02);
#endif
void ZetMapMemory(UINT8* Mem, INT32 nStart, INT32 nEnd, INT32 nMode);
INT32 ZetMemCallback(INT32 nStart, INT32 nEnd, INT32 nMode);
INT32 ZetUnmapArea(INT32 nStart, INT32 nEnd, INT32 nMode);

// Cheat helpers and state machine
void ZetCheatCpuConfig();
INT32 ZetCheatScan();
void ZetScan(INT32 nAction);

// Handler functions
INT32 ZetSetReadHandler(UINT8 (*pHandler)(UINT16));
INT32 ZetSetWriteHandler(void (*pHandler)(UINT16, UINT8));
INT32 ZetSetInHandler(UINT8 (*pHandler)(UINT16));
INT32 ZetSetOutHandler(void (*pHandler)(UINT16, UINT8));
// Set the BUSREQ line state (0 = clear, 1 = assert)
INT32 ZetSetBUSREQLine(INT32 nStatus);
// Void return version for METAL implementation compatibility
#if !defined(USE_METAL_RENDERER)
void ZetSetBUSREQLine(INT32 nStatus);
#endif
INT32 ZetDaisyChain(INT32 nStatus);

// For Burn Timer
void ZetIdle(INT32 nCycles);
INT32 ZetNmi();

// Useful for debugging
extern void (*ZetDebugWriteCallback)(UINT16 address, UINT8 data);

#ifdef __cplusplus
}
#endif

#endif // _Z80_INTF_H
