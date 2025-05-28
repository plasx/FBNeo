#include "burnint.h"
#ifdef USE_METAL_FIXES
#include "metal_fixes.h"
#endif

// Simplified M68K interface for Metal build
struct SekExt {
    UINT8* pSekMemMap;
    INT32 nSekCycles;
    INT32 nSekCyclesTotal;
};

// Define SekRegister enum to fix conflicts
enum SekRegister {
    SEK_REG_D0 = 0, SEK_REG_D1, SEK_REG_D2, SEK_REG_D3, SEK_REG_D4, SEK_REG_D5, SEK_REG_D6, SEK_REG_D7,
    SEK_REG_A0, SEK_REG_A1, SEK_REG_A2, SEK_REG_A3, SEK_REG_A4, SEK_REG_A5, SEK_REG_A6, SEK_REG_A7,
    SEK_REG_PC, SEK_REG_SR, SEK_REG_SP, SEK_REG_USP,
    SEK_REG_ISP, SEK_REG_MSP, SEK_REG_SFC, SEK_REG_DFC, SEK_REG_VBR, SEK_REG_CACR, SEK_REG_CAAR
};

static SekExt* SekCPU = NULL;
static INT32 nSekCPUCount = 0;
static INT32 nSekCPUActive = -1;

// M68K handler stubs
static UINT32 SekReadByteStub(UINT32 a) { return 0; }
static UINT16 SekReadWordStub(UINT32 a) { return 0; }
static UINT32 SekReadLongStub(UINT32 a) { return 0; }
static void SekWriteByteStub(UINT32 a, UINT8 d) { }
static void SekWriteWordStub(UINT32 a, UINT16 d) { }
static void SekWriteLongStub(UINT32 a, UINT32 d) { }

// CPU interface structure stub
// static cpu_core_config SekConfig = { ... };

// Simplified M68K interface functions
INT32 SekInit(INT32 nCount, INT32 nCPUType) {
    printf("[SekInit] M68K init (Metal stub) - count: %d, type: %d\n", nCount, nCPUType);
    nSekCPUCount = nCount;
    SekCPU = (SekExt*)malloc(nCount * sizeof(SekExt));
    memset(SekCPU, 0, nCount * sizeof(SekExt));
    return 0;
}

void SekExit() {
    printf("[SekExit] M68K exit (Metal stub)\n");
    if (SekCPU) {
        free(SekCPU);
        SekCPU = NULL;
    }
    nSekCPUCount = 0;
    nSekCPUActive = -1;
}

void SekOpen(INT32 nCPU) {
    printf("[SekOpen] M68K open (Metal stub) - CPU: %d\n", nCPU);
    nSekCPUActive = nCPU;
}

void SekClose() {
    printf("[SekClose] M68K close (Metal stub)\n");
    nSekCPUActive = -1;
}

// Memory access functions
UINT32 SekReadByte(UINT32 a) {
    return SekReadByteStub(a);
}

UINT16 SekReadWord(UINT32 a) {
    return SekReadWordStub(a);
}

UINT32 SekReadLong(UINT32 a) {
    return SekReadLongStub(a);
}

void SekWriteByte(UINT32 a, UINT8 d) {
    SekWriteByteStub(a, d);
}

void SekWriteWord(UINT32 a, UINT16 d) {
    SekWriteWordStub(a, d);
}

void SekWriteLong(UINT32 a, UINT32 d) {
    SekWriteLongStub(a, d);
}

// Memory mapping stubs
INT32 SekMapMemory(UINT8* pMemory, UINT32 nStart, UINT32 nEnd, INT32 nType) {
    printf("[SekMapMemory] M68K map memory (Metal stub) - start: 0x%X, end: 0x%X\n", nStart, nEnd);
    return 0;
}

INT32 SekMapHandler(UINT32 nHandler, UINT32 nStart, UINT32 nEnd, INT32 nType) {
    printf("[SekMapHandler] M68K map handler (Metal stub) - start: 0x%X, end: 0x%X\n", nStart, nEnd);
    return 0;
}

// Register access
UINT32 SekGetReg(SekRegister nRegister) {
    printf("[SekGetReg] M68K get register (Metal stub) - reg: %d\n", nRegister);
    return 0;
}

void SekSetReg(SekRegister nRegister, UINT32 nValue) {
    printf("[SekSetReg] M68K set register (Metal stub) - reg: %d, value: 0x%X\n", nRegister, nValue);
} 