#define USE_METAL 1
#pragma once

// Wrapper for EEPROM functions for CPS games
// This avoids direct dependencies on the EEPROM interface struct
// Provides a clean interface for the CPS drivers

#ifdef __cplusplus
extern "C" {
#endif

// If we're in the Metal build, use its EEPROM interface and types
#ifdef USE_METAL
// Use Metal's type definitions
typedef unsigned char METAL_UINT8;
typedef unsigned short METAL_UINT16;
typedef unsigned int METAL_UINT32;
typedef int METAL_INT32;
typedef short METAL_INT16;
typedef signed char METAL_INT8;

// Match Metal's EEPROM function declarations exactly
void EEPROMInit(const struct metal_eeprom_interface* interface);
void EEPROMExit(void);
void EEPROMReset(void);
METAL_INT32 EEPROMRead(void);
void EEPROMWriteBit(METAL_INT32 bit);
void EEPROMSetCSLine(METAL_INT32 state);
void EEPROMSetClockLine(METAL_INT32 state);
void EEPROMScan(METAL_INT32 nAction, METAL_INT32* pnMin);

// Provide a macro for EEPROMWrite to match what Metal expects
#define EEPROMWrite(val, cs, clk) { EEPROMWriteBit(val); EEPROMSetCSLine(cs); EEPROMSetClockLine(clk); }

#else
// Regular non-Metal build - original interface
// void EEPROMInit(const void *interface);
// void EEPROMExit(void);
// void EEPROMReset(void);
// int EEPROMRead(void);
// void EEPROMWriteCustom(int bit, int cs, int clk);

#ifdef __cplusplus
// C++ inline function
// inline void EEPROMWrite(int val, int cs, int clk) { EEPROMWriteCustom(val, cs, clk); }
#else
// C macro
// #define EEPROMWrite(val, cs, clk) EEPROMWriteCustom(val, cs, clk)
#endif

#endif // USE_METAL

#ifdef __cplusplus
}
#endif 