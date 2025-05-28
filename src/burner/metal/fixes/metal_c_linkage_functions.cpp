#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int INT32;
typedef unsigned int UINT32;

// TMS34010 display parameter struct
struct _tms34010_display_params {
    int dummy;
};

// BurnState functions
int BurnStateLoad(char* szName, int nOffset, int (*pLoadGame)()) {
    return 0;
}

int BurnStateSave(char* szName, int nOffset) {
    return 0;
}

// TMS34010 functions
int tms34010_generate_scanline(int line, int (*callback)(int, struct _tms34010_display_params*)) {
    return 0;
}

// QSound functions
int QsndScan(int nAction) {
    return 0;
}

// BurnSample functions
void BurnSampleRender_INT(unsigned int nSegmentLength) {
}

// String conversion functions
void TCHARToANSI(const char* pszInString, char* pszOutString, int nOutSize) {
    if (pszOutString && nOutSize > 0) {
        strncpy(pszOutString, pszInString, nOutSize - 1);
        pszOutString[nOutSize - 1] = 0;
    }
}

// 68K helpers
void m68k_modify_timeslice(int value) {
}

// Synchronization for QSound Z80
void QsndSyncZ80() {
}

// Empty EEPROM path
char szAppEEPROMPath[260] = {0};

#ifdef __cplusplus
}
#endif 