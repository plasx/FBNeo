#include "burnint.h"
#include "i386_intf.h"

// Stub implementation for Metal build
#ifdef METAL_BUILD

void i386Init(INT32 type) {}
void i386Reset() {}
void i386Exit() {}
INT32 i386Run(INT32 cycles) { return 0; }
void i386SetIRQLine(INT32 irqline, INT32 state) {}
void i386SetIRQCallback(INT32 (*irqcallback)(INT32)) {}
INT32 i386GetActive() { return 0; }
INT32 i386TotalCycles() { return 0; }
void i386NewFrame() {}
void i386RunEnd() {}
INT32 i386CpuScan(INT32 nAction) { return 0; }

UINT8 i386_read8(UINT32 address) { return 0; }
UINT16 i386_read16(UINT32 address) { return 0; }
UINT32 i386_read32(UINT32 address) { return 0; }
void i386_write8(UINT32 address, UINT8 value) {}
void i386_write16(UINT32 address, UINT16 value) {}
void i386_write32(UINT32 address, UINT32 value) {}

void i386MapMemory(UINT8 *mem, UINT32 start, UINT32 end, INT32 flags) {}
void i386SetReadHandler(UINT8 (*read)(UINT32)) {}
void i386SetReadWordHandler(UINT16 (*read)(UINT32)) {}
void i386SetReadLongHandler(UINT32 (*read)(UINT32)) {}
void i386SetWriteHandler(void (*write)(UINT32, UINT8)) {}
void i386SetWriteWordHandler(void (*write)(UINT32, UINT16)) {}
void i386SetWriteLongHandler(void (*write)(UINT32, UINT32)) {}

// i486 variants
void i486Init(INT32 type) {}
void i486Reset() {}
void i486Exit() {}
INT32 i486Run(INT32 cycles) { return 0; }
void i486SetIRQLine(INT32 irqline, INT32 state) {}
void i486SetIRQCallback(INT32 (*irqcallback)(INT32)) {}
INT32 i486GetActive() { return 0; }
INT32 i486TotalCycles() { return 0; }
void i486NewFrame() {}
void i486RunEnd() {}
INT32 i486CpuScan(INT32 nAction) { return 0; }

// pentium
void pentiumInit(INT32 type) {}
void pentiumReset() {}
void pentiumExit() {}
INT32 pentiumRun(INT32 cycles) { return 0; }
void pentiumSetIRQLine(INT32 irqline, INT32 state) {}
void pentiumSetIRQCallback(INT32 (*irqcallback)(INT32)) {}
INT32 pentiumGetActive() { return 0; }
INT32 pentiumTotalCycles() { return 0; }
void pentiumNewFrame() {}
void pentiumRunEnd() {}
INT32 pentiumCpuScan(INT32 nAction) { return 0; }

#endif 