#ifndef _BURN_DEBUG_H
#define _BURN_DEBUG_H

// Debug flags
extern INT32 Debug_BurnGunInitted;
extern INT32 Debug_BurnShiftInitted;
extern INT32 Debug_HiscoreInitted;

// Memory management functions
void BurnInitMemoryManager();
void BurnExitMemoryManager();

// Input functions
void BurnSetMouseDivider(INT32 nDivider);

// Analog input functions
INT32 AnalogDeadZone(INT32 nInput);
#define INPUT_DEADZONE 0x10

// Process analog input
INT32 ProcessAnalog(INT32 nInput, INT32 nMin, INT32 nMax, INT32 nDeadZone, INT32 nRange);

#endif // _BURN_DEBUG_H 