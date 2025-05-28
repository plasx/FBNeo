#include "burn.h"

#ifndef JOYPROCESS
#define JOYPROCESS

#ifdef __cplusplus
extern "C" {
#endif

extern INT32 bBurnRunAheadFrame;

// ---[ ProcessJoystick() Flags (grep ProcessJoystick in drv/pre90s for examples)
#define INPUT_4WAY              0x02  // convert 8-way inputs to 4-way
#define INPUT_4WAY_ALT          0x22  // alternate method for 4-way processing
#define INPUT_CLEAROPPOSITES    0x04  // disallow up+down or left+right
#define INPUT_MAKEACTIVELOW     0x08  // input is active-high, make active-low after processing
#define INPUT_ISACTIVELOW       0x10  // input is active-low to begin with

// --- [ INPUT_4WAY / INPUT_4WAY_ALT methodology
// INPUT_4WAY: cancels previous frame's direction bit when diagonal is pressed
// INPUT_4WAY_ALT: holds previous non-diagonal direction until diagonal is released

void ProcessJoystick(UINT8 *input, INT8 playernum, INT8 up_bit, INT8 down_bit, INT8 left_bit, INT8 right_bit, UINT8 flags);
void CompileInput(UINT8 **input, void *output, INT32 num, INT32 bits, UINT32 *init);

// ---[ ProcessAnalog()
// Mostly used for self-centering Wheel, brake, accelerator.  Linear (0-ff),
// non-linear (f.ex centered @ 0x80)  grep ProcessAnalog in burn/drv/konami for
// examples :)

// ---[ ProcessAnalog() flags
// by default, it centers at the midpoint between scalemin and scalemax.
// use INPUT_LINEAR to change to linear-mode.
#define INPUT_DEADZONE          0x01
#define INPUT_LINEAR            0x02

// for analog pedals, allows a mapped digital button to work
#define INPUT_MIGHTBEDIGITAL    0x04

// Need something else / looking for:
// analog dial, paddle & trackball, use BurnTrackball device (burn_gun.h)
// lightgun, use BurnGun device (also burn_gun.h)

UINT8 ProcessAnalog(INT16 anaval, INT32 reversed, INT32 flags, UINT8 scalemin, UINT8 scalemax);
INT32 AnalogDeadZone(INT32 anaval);
UINT32 scalerange(UINT32 x, UINT32 in_min, UINT32 in_max, UINT32 out_min, UINT32 out_max);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // JOYPROCESS
