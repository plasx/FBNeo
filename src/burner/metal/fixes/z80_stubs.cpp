#include "burnint.h"
#include "z80_intf.h"

// Z80 CPU core stub implementations for Metal backend
int ZetInit(int nCPU) { return 0; }
void ZetExit() {}
void ZetNewFrame() {}
void ZetOpen(int nCPU) {}
void ZetClose() {}
int ZetMemCallback(int nStart, int nEnd, int nMode) { return 0; }
int ZetMapArea(int nStart, int nEnd, int nMode, unsigned char *pMemSpace) { return 0; }
int ZetMapArea(int nStart, int nEnd, int nMode, unsigned char *pMemSpace, unsigned char *pDefSpace) { return 0; }
void ZetSetReadHandler(UINT8 (__fastcall *pHandler)(UINT16)) {}
void ZetSetWriteHandler(void (__fastcall *pHandler)(UINT16, UINT8)) {}
void ZetSetInHandler(UINT8 (__fastcall *pHandler)(UINT16)) {}
void ZetSetOutHandler(void (__fastcall *pHandler)(UINT16, UINT8)) {}
void ZetReset() {}
void ZetSetIRQLine(const int line, const int status) {}
void ZetSetVector(int vector) {}
int ZetRun(const int cycles) { return 0; }
void ZetRunEnd() {}
int ZetGetActive() { return 0; }
int ZetTotalCycles() { return 0; }
INT32 ZetIdle(INT32 nCycles) { return 0; }
int ZetNmi() { return 0; }
void ZetGetRegs(Z80_Regs* Regs) {}
void ZetSetRegs(Z80_Regs* Regs) {}
UINT32 ZetGetPC(INT32 n) { return 0; }
INT32 ZetScan(INT32 nAction) { return 0; } 