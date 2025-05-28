#ifndef _PATCHED_BURN_H_
#define _PATCHED_BURN_H_

/*
 * Patched version of burn.h for Metal build
 * 
 * This header provides C-compatible declarations of key structures
 * and functions from burn.h without including the original header.
 */

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// Basic type definitions
typedef int32_t INT32;
typedef uint32_t UINT32;
typedef int16_t INT16;
typedef uint16_t UINT16;
typedef int8_t INT8;
typedef uint8_t UINT8;

// Forward declarations for all struct/enum types used
#ifdef __cplusplus
extern "C" {
#endif

// Forward declare structs that need proper tags
extern struct CheatInfo* pCheatInfo;
extern struct RomDataInfo* pRDI;
extern struct BurnRomInfo* pDataRomDesc;
extern struct cpu_core_config MegadriveCPU;
extern struct cpu_core_config FD1094CPU;

// Forward declare enums with proper tags
extern INT32 (*BurnExtCartridgeSetupCallback)(enum BurnCartrigeCommand nCommand);
extern INT32 BurnDrvCartridgeSetup(enum BurnCartrigeCommand nCommand);

// Time-related declarations
extern void BurnGetLocalTime(struct tm* nTime);

// IPS patching
extern void IpsApplyPatches_C(unsigned char* base, char* rom_name, unsigned int rom_crc, bool readonly);
#define IpsApplyPatches(base, rom_name, rom_crc, readonly) \
    IpsApplyPatches_C(base, rom_name, rom_crc, readonly)

// Core function declarations that might be needed in Metal code
extern INT32 BurnLibInit();
extern INT32 BurnLibExit();
extern INT32 BurnDrvSelect(INT32 nDriver);
extern INT32 BurnDrvInit();
extern INT32 BurnDrvExit();
extern INT32 BurnDrvReset();
extern INT32 BurnDrvFrame();
extern INT32 BurnRecalcPal();
extern INT32 BurnDrvGetIndex(char* szName);
extern char* BurnDrvGetTextA(UINT32 i);
extern INT32 BurnDrvGetVisibleSize(INT32* pnWidth, INT32* pnHeight);
extern INT32 BurnDrvGetAspect(INT32* pnXAspect, INT32* pnYAspect);
extern UINT32 BurnHighCol32(INT32 r, INT32 g, INT32 b, INT32 i);

// External declarations for genre variables
extern const unsigned int GENRE_HORSHOOT;
extern const unsigned int GENRE_VERSHOOT;
extern const unsigned int GENRE_SCRFIGHT;
extern const unsigned int GENRE_VSFIGHT;
extern const unsigned int GENRE_BIOS;
extern const unsigned int GENRE_PUZZLE;
extern const unsigned int GENRE_BREAKOUT;
extern const unsigned int GENRE_CASINO;
extern const unsigned int GENRE_BALLPADDLE;
extern const unsigned int GENRE_PLATFORM;
extern const unsigned int GENRE_QUIZ;
extern const unsigned int GENRE_SPORTSMISC;
extern const unsigned int GENRE_SPORTSFOOTBALL;
extern const unsigned int GENRE_MISC;
extern const unsigned int GENRE_RACING;
extern const unsigned int GENRE_SHOOT;
extern const unsigned int GENRE_SPORTS;

#ifdef __cplusplus
}
#endif

#endif /* _PATCHED_BURN_H_ */ 