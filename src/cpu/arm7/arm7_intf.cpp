#include "burnint.h"
#include "arm7_intf.h"

// Stub implementation for Metal build
#ifdef METAL_BUILD

// ARM7 CPU specific
void Arm7Init(INT32 nCPU) {}
void Arm7Open(INT32 nCPU) {}
void Arm7Close() {}
INT32 Arm7GetActive() { return 0; }
void Arm7SetIRQLine(INT32 line, INT32 state) {}
void Arm7Reset() {}
INT32 Arm7Run(INT32 nCycles) { return 0; }
INT32 Arm7Idle(INT32 nCycles) { return 0; }
INT32 Arm7TotalCycles() { return 0; }
void Arm7RunEnd() {}
void Arm7Exit() {}
void Arm7Scan(INT32 nAction) {}

// Memory handlers
void Arm7SetReadHandler(UINT32 (*pHandler)(UINT32)) {}
void Arm7SetWriteHandler(void (*pHandler)(UINT32, UINT32)) {}
void Arm7SetReadOpHandler(UINT32 (*pHandler)(UINT32)) {}
void Arm7MapMemory(UINT8 *pMemory, UINT32 nStart, UINT32 nEnd, INT32 nType) {}

#endif 