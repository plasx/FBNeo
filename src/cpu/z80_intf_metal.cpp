#include "burnint.h"
#ifdef USE_METAL_FIXES
#include "metal_fixes.h"
#endif

// Simplified Z80 interface for Metal build
struct ZetExt {
    UINT8* pZetMemMap;
    INT32 nZetCycles;
    INT32 nZetCyclesTotal;
};

static ZetExt* ZetCPU = NULL;
static INT32 nZetCPUCount = 0;
static INT32 nZetCPUActive = -1;

// Z80 handler stubs
static UINT8 ZetReadProgStub(UINT32 a) { return 0; }
static void ZetWriteProgStub(UINT32 a, UINT8 d) { }
static UINT8 ZetReadIOStub(UINT32 a) { return 0; }
static void ZetWriteIOStub(UINT32 a, UINT8 d) { }
static UINT8 ZetReadOpStub(UINT32 a) { return 0; }
static UINT8 ZetReadOpArgStub(UINT32 a) { return 0; }

// Simplified Z80 interface functions
INT32 ZetInit(INT32 nCount) {
    printf("[ZetInit] Z80 init (Metal stub) - count: %d\n", nCount);
    nZetCPUCount = nCount;
    ZetCPU = (ZetExt*)malloc(nCount * sizeof(ZetExt));
    memset(ZetCPU, 0, nCount * sizeof(ZetExt));
    return 0;
}

INT32 ZetExit() {
    printf("[ZetExit] Z80 exit (Metal stub)\n");
    if (ZetCPU) {
        free(ZetCPU);
        ZetCPU = NULL;
    }
    nZetCPUCount = 0;
    nZetCPUActive = -1;
    return 0;
}

void ZetOpen(INT32 nCPU) {
    printf("[ZetOpen] Z80 open (Metal stub) - CPU: %d\n", nCPU);
    nZetCPUActive = nCPU;
}

void ZetClose() {
    printf("[ZetClose] Z80 close (Metal stub)\n");
    nZetCPUActive = -1;
}

INT32 ZetRun(INT32 nCycles) {
    printf("[ZetRun] Z80 run (Metal stub) - cycles: %d\n", nCycles);
    return nCycles;
}

void ZetReset() {
    printf("[ZetReset] Z80 reset (Metal stub)\n");
}

INT32 ZetScan(INT32 nAction) {
    printf("[ZetScan] Z80 scan (Metal stub) - action: %d\n", nAction);
    return 0;
}

// Handler setters (simplified)
INT32 ZetSetReadHandler(UINT8 (*pHandler)(UINT16)) {
    printf("[ZetSetReadHandler] Z80 set read handler (Metal stub)\n");
    return 0;
}

INT32 ZetSetWriteHandler(void (*pHandler)(UINT16, UINT8)) {
    printf("[ZetSetWriteHandler] Z80 set write handler (Metal stub)\n");
    return 0;
}

INT32 ZetSetInHandler(UINT8 (*pHandler)(UINT16)) {
    printf("[ZetSetInHandler] Z80 set in handler (Metal stub)\n");
    return 0;
}

INT32 ZetSetOutHandler(void (*pHandler)(UINT16, UINT8)) {
    printf("[ZetSetOutHandler] Z80 set out handler (Metal stub)\n");
    return 0;
}

// Additional Z80 functions
INT32 ZetCheatScan() {
    printf("[ZetCheatScan] Z80 cheat scan (Metal stub)\n");
    return 0;
}

UINT32 ZetReadProg(UINT32 a) {
    return ZetReadProgStub(a);
}

void ZetWriteProg(UINT32 a, UINT8 d) {
    ZetWriteProgStub(a, d);
}

UINT32 ZetReadIO(UINT32 a) {
    return ZetReadIOStub(a);
}

void ZetWriteIO(UINT32 a, UINT8 d) {
    ZetWriteIOStub(a, d);
}

UINT32 ZetReadOp(UINT32 a) {
    return ZetReadOpStub(a);
}

UINT32 ZetReadOpArg(UINT32 a) {
    return ZetReadOpArgStub(a);
}

// Memory mapping stubs
INT32 ZetMapMemory(UINT8* pMemory, UINT32 nStart, UINT32 nEnd, INT32 nType) {
    printf("[ZetMapMemory] Z80 map memory (Metal stub) - start: 0x%X, end: 0x%X\n", nStart, nEnd);
    return 0;
}

INT32 ZetMemCallback(UINT32 nStart, UINT32 nEnd, INT32 nType) {
    printf("[ZetMemCallback] Z80 memory callback (Metal stub) - start: 0x%X, end: 0x%X\n", nStart, nEnd);
    return 0;
}

void ZetSetEDFECallback(void (*pCallback)()) {
    printf("[ZetSetEDFECallback] Z80 set EDFE callback (Metal stub)\n");
} 