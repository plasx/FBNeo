#include "burnint.h"
#include "nec_intf.h"

// Stub implementation for Metal build
#ifdef METAL_BUILD

// NEC CPU specific
void necInit(INT32 nCPU, INT32 type) {}
void necCpuOpen(INT32 nCPU) {}
void necCpuClose() {}
INT32 necGetActive() { return 0; }
void necSetIRQLine(INT32 line, INT32 state) {}
void necReset() {}
INT32 necRun(INT32 nCycles) { return 0; }
void necRunEnd() {}
void necIdle(INT32 nCycles) {}
INT32 necTotalCycles() { return 0; }
void necNewFrame() {}
INT32 necGetPendingIRQ() { return 0; }
void necSetPendingIRQ(INT32 nIRQ) {}
void necSetTotalCycles(INT32 nCycles) {}
void necExit() {}
INT32 necGetPC(INT32 n) { return 0; }
void necScan(INT32 nAction) {}

// Memory management
void necMapMemory(UINT8 *pMemory, UINT32 nStart, UINT32 nEnd, INT32 nType) {}
void necSetReadHandler(UINT8 (*pHandler)(UINT32)) {}
void necSetWriteHandler(void (*pHandler)(UINT32, UINT8)) {}
void necSetReadPortHandler(UINT8 (*pHandler)(UINT32)) {}
void necSetWritePortHandler(void (*pHandler)(UINT32, UINT8)) {}
void necSetReadIoHandler(UINT8 (*pHandler)(UINT32)) {}
void necSetWriteIoHandler(void (*pHandler)(UINT32, UINT8)) {}

#endif 