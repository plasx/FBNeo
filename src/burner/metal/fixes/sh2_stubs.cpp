#include "burnint.h"

// Type definitions for SH2 handlers
typedef unsigned char (*pSh2ReadByteHandler)(unsigned int a);
typedef void (*pSh2WriteByteHandler)(unsigned int a, unsigned char d);
typedef unsigned short (*pSh2ReadWordHandler)(unsigned int a);
typedef void (*pSh2WriteWordHandler)(unsigned int a, unsigned short d);
typedef unsigned int (*pSh2ReadLongHandler)(unsigned int a);
typedef void (*pSh2WriteLongHandler)(unsigned int a, unsigned int d);

// Simple SH2 CPU stubs for Metal backend
int Sh2Init(int nCount) { return 0; }
void Sh2Exit() {}
void Sh2Open(const int i) {}
void Sh2Close() {}
int Sh2GetActive() { return 0; }
void Sh2Reset() {}
void Sh2SetIRQLine(const int line, const int state) {}
int Sh2Run(const int cycles) { return 0; }
INT32 Sh2TotalCycles() { return 0; }
void Sh2NewFrame() {}
void Sh2BurnUntilInt() {}
int Sh2MapMemory(unsigned char* pMemory, unsigned int nStart, unsigned int nEnd, int nType) { return 0; }
int Sh2MapHandler(uintptr_t nHandler, unsigned int nStart, unsigned int nEnd, int nType) { return 0; }
void Sh2SetReadByteHandler(unsigned int i, pSh2ReadByteHandler pHandler) {}
void Sh2SetWriteByteHandler(unsigned int i, pSh2WriteByteHandler pHandler) {}
void Sh2SetReadWordHandler(unsigned int i, pSh2ReadWordHandler pHandler) {}
void Sh2SetWriteWordHandler(unsigned int i, pSh2WriteWordHandler pHandler) {}
void Sh2SetReadLongHandler(unsigned int i, pSh2ReadLongHandler pHandler) {}
void Sh2SetWriteLongHandler(unsigned int i, pSh2WriteLongHandler pHandler) {}
unsigned char Sh2ReadByte(unsigned int a) { return 0; }
unsigned short Sh2ReadWord(unsigned int a) { return 0; }
unsigned int Sh2ReadLong(unsigned int a) { return 0; }
void Sh2WriteByte(unsigned int a, unsigned char d) {}
void Sh2WriteWord(unsigned int a, unsigned short d) {}
void Sh2WriteLong(unsigned int a, unsigned int d) {}
int Sh2Scan(int nAction) { return 0; }
void Sh2SetEatCycles(int i) {}
void Sh2StopRun() {}
void Sh2SetIRQCallback(int (*pCallback)(int)) {}
unsigned int Sh2GetPC(int) { return 0; }
void Sh2RunEnd() {} 