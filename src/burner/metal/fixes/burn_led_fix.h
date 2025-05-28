// Fix for burn_led.cpp issues
#ifndef BURN_LED_FIX_H
#define BURN_LED_FIX_H

// Define missing debug variable
#ifndef Debug_BurnLedInitted
int Debug_BurnLedInitted = 0;
#endif

// Fix for the screen dimensions conflict
// We need to modify burn_led.cpp to use the global variables instead of static ones
#define static INT32 nScreenWidth, nScreenHeight /* extern INT32 nScreenWidth, nScreenHeight */

// The extern INT32 nScreenWidth, nScreenHeight are already defined in tiles_generic.h
// We're including this file to avoid the redefinition errors

#endif // BURN_LED_FIX_H
