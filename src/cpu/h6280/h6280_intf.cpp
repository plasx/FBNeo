#include "burnint.h"
#include "h6280_intf.h"

// Stub implementation for Metal build
#ifdef METAL_BUILD

void h6280Init(INT32 nCPU) {}
void h6280Reset() {}
void h6280Exit() {}
INT32 h6280Run(INT32 cycles) { return 0; }
void h6280SetIRQLine(INT32 irqline, INT32 state) {}
INT32 h6280TotalCycles() { return 0; }
void h6280RunEnd() {}
void h6280NewFrame() {}
INT32 h6280CpuScan(INT32 nAction) { return 0; }
UINT8 h6280_read(UINT16 address) { return 0; }
void h6280_write(UINT16 address, UINT8 data) {}
void h6280_write_rom(UINT16 address, UINT8 data) {}
UINT8 h6280_fetch(UINT16 address) { return 0; }
INT32 h6280GetActive() { return 0; }
void h6280MapMemory(UINT8 *src, UINT16 start, UINT16 finish, INT32 type) {}
void h6280SetWriteHandler(void (*write)(UINT16, UINT8)) {}
void h6280SetReadHandler(UINT8 (*read)(UINT16)) {}
void h6280WritePort(UINT8 port, UINT8 data) {}
UINT8 h6280ReadPort(UINT8 port) { return 0; }
void h6280SetIRQCallback(INT32 (*irqcallback)(INT32)) {}

#endif 