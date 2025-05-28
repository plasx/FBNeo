#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// EEPROM functions
struct _eeprom_interface;
int EEPROMInit(const struct _eeprom_interface* interface);
void EEPROMExit();
void EEPROMReset();
int EEPROMRead();
void EEPROMWriteBit(int bit);
void EEPROMSetCSLine(int state);
void EEPROMSetClockLine(int state);
int EEPROMScan(int nAction, int* pnMin);

#ifdef __cplusplus
}
#endif 