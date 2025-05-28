#include "burnint.h"
#include "burn_debug.h"

// Debug flags
INT32 Debug_BurnGunInitted = 0;
INT32 Debug_BurnShiftInitted = 0;
INT32 Debug_HiscoreInitted = 0;

// Memory management functions
// Input functions
void BurnSetMouseDivider(INT32 nDivider)
{
    // Set mouse divider
}

// Analog input functions
INT32 AnalogDeadZone(INT32 nInput)
{
    if (nInput > INPUT_DEADZONE) {
        return nInput - INPUT_DEADZONE;
    } else if (nInput < -INPUT_DEADZONE) {
        return nInput + INPUT_DEADZONE;
    }
    return 0;
}

// Process analog input
INT32 ProcessAnalog(INT32 nInput, INT32 nMin, INT32 nMax, INT32 nDeadZone, INT32 nRange)
{
    // Apply deadzone
    if (nInput > nDeadZone) {
        nInput -= nDeadZone;
    } else if (nInput < -nDeadZone) {
        nInput += nDeadZone;
    } else {
        nInput = 0;
    }

    // Scale to range
    if (nInput != 0) {
        nInput = (nInput * nRange) / (nMax - nMin);
    }

    return nInput;
} 