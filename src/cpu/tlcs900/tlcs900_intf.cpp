#include "burnint.h"
#include "tlcs900_intf.h"

// Stub implementation for Metal build
#ifdef METAL_BUILD

void tlcs900SetReadByteHandler(UINT8 (*read)(UINT32)) {}
void tlcs900SetWriteByteHandler(void (*write)(UINT32, UINT8)) {}
void tlcs900SetReadWordHandler(UINT16 (*read)(UINT32)) {}
void tlcs900SetWriteWordHandler(void (*write)(UINT32, UINT16)) {}
void tlcs900SetReadLongHandler(UINT32 (*read)(UINT32)) {}
void tlcs900SetWriteLongHandler(void (*write)(UINT32, UINT32)) {}

INT32 tlcs900Init(INT32 type) { return 0; }
void tlcs900Reset() {}
void tlcs900Exit() {}
INT32 tlcs900Run(INT32 cycles) { return 0; }
void tlcs900SetIRQLine(INT32 line, INT32 state) {}
INT32 tlcs900GetPC() { return 0; }
INT32 tlcs900MapMemory(UINT8* pMemory, UINT32 nStart, UINT32 nEnd, INT32 nType) { return 0; }
void tlcs900Scan(INT32 nAction) {}

#endif 