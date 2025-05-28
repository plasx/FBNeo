#include "burnint.h"
#include "m6502_intf.h"

// Stub implementation for Metal build
#ifdef METAL_BUILD

void m6502Init(INT32 nCPU) {}
void m6502Exit() {}
void m6502Open(INT32 nCPU) {}
void m6502Close() {}
INT32 m6502GetActive() { return 0; }
void m6502Reset() {}
INT32 m6502Run(INT32 nCycles) { return 0; }
void m6502RunEnd() {}
INT32 m6502MapMemory(UINT8* pMemory, UINT16 nStart, UINT16 nEnd, INT32 nType) { return 0; }
void m6502SetReadHandler(UINT8 (*pHandler)(UINT16)) {}
void m6502SetWriteHandler(void (*pHandler)(UINT16, UINT8)) {}
void m6502SetReadOpHandler(UINT8 (*pHandler)(UINT16)) {}
void m6502SetReadOpArgHandler(UINT8 (*pHandler)(UINT16)) {}
void m6502SetReadPortHandler(UINT8 (*pHandler)(UINT16)) {}
void m6502SetWritePortHandler(void (*pHandler)(UINT16, UINT8)) {}
UINT8 m6502ReadPort(UINT16 Address) { return 0; }
void m6502WritePort(UINT16 Address, UINT8 Data) {}
UINT8 m6502ReadByte(UINT16 Address) { return 0; }
void m6502WriteByte(UINT16 Address, UINT8 Data) {}
void m6502WriteRom(UINT16 Address, UINT8 Data) {}
UINT32 m6502GetPC(INT32 nCPU) { return 0; }
void m6502SetIRQLine(INT32 nLine, INT32 nState) {}
void m6502SetIRQCallback(INT32 (*pCallback)(INT32)) {}
INT32 m6502Scan(INT32 nAction) { return 0; }
INT32 m6502TotalCycles() { return 0; }
void m6502NewFrame() {}

// Variants
void m65c02Init(INT32 nCPU) {}
void m65c02Open(INT32 nCPU) {}
void m65c02Reset() {}
INT32 m65c02Run(INT32 nCycles) { return 0; }
void m65c02SetIRQLine(INT32 nLine, INT32 nState) {}
UINT32 m65c02GetPC(INT32 nCPU) { return 0; }

void n2a03Init(INT32 nCPU) {}
void n2a03Open(INT32 nCPU) {}
void n2a03Reset() {}
INT32 n2a03Run(INT32 nCycles) { return 0; }
void n2a03SetIRQLine(INT32 nLine, INT32 nState) {}
UINT32 n2a03GetPC(INT32 nCPU) { return 0; }

void m65sc02Init(INT32 nCPU) {}
void m65sc02Open(INT32 nCPU) {}
void m65sc02Reset() {}
INT32 m65sc02Run(INT32 nCycles) { return 0; }
void m65sc02SetIRQLine(INT32 nLine, INT32 nState) {}
UINT32 m65sc02GetPC(INT32 nCPU) { return 0; }

void deco16Init(INT32 nCPU) {}
void deco16Open(INT32 nCPU) {}
void deco16Reset() {}
INT32 deco16Run(INT32 nCycles) { return 0; }
void deco16SetIRQLine(INT32 nLine, INT32 nState) {}
UINT32 deco16GetPC(INT32 nCPU) { return 0; }

#endif 