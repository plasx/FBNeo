#include "burnint.h"

// EEPROM function stubs for CPS2 Metal build
void EEPROMReset() {
    // Stub function for EEPROM reset
}

void EEPROMWrite(INT32 nData) {
    // Stub function for EEPROM write
}

INT32 EEPROMRead() {
    // Stub function for EEPROM read
    return 0;
}

void EEPROMFill(UINT8* pData, INT32 nLen) {
    // Stub function to fill EEPROM data
    if (pData && nLen > 0) {
        memset(pData, 0, nLen);
    }
}

void EEPROMScan(INT32 nAction, INT32* pnMin) {
    // Stub function for EEPROM state scanning
} 