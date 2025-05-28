#include "burnint.h"
#include "v60_intf.h"

// Stub implementation for Metal build
#ifdef METAL_BUILD

void v60Init() {}
void v60Reset() {}
void v60Exit() {}
INT32 v60Run(INT32 cycles) { return 0; }
void v60SetIRQLine(INT32 irqline, INT32 state) {}
INT32 v60GetPC() { return 0; }
void v60SetIRQCallback(INT32 (*callback)(INT32)) {}
INT32 v60MapMemory(UINT8 *mem, UINT32 start, UINT32 end, INT32 type) { return 0; }
void v60SetReadByteHandler(UINT8 (*pHandler)(UINT32)) {}
void v60SetWriteByteHandler(void (*pHandler)(UINT32, UINT8)) {}
void v60SetReadWordHandler(UINT16 (*pHandler)(UINT32)) {}
void v60SetWriteWordHandler(void (*pHandler)(UINT32, UINT16)) {}
void v60SetReadLongHandler(UINT32 (*pHandler)(UINT32)) {}
void v60SetWriteLongHandler(void (*pHandler)(UINT32, UINT32)) {}
INT32 v60Scan(INT32 nAction) { return 0; }

#endif 