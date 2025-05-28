#pragma once

// Wrapper for cps.h

// Define header guards to prevent the original headers from being included
#define m68000_intf_h
#define msm6295_h
#define samples_h
#define z80_intf_h
#define timer_h
#define eeprom_h
#define INCLUDE_EEPROM_H

// Include only the headers we need for our wrapper
#include "burnint.h"
#include "../../devices/eeprom.h"
#include "wrappers/cps_functions.h" // Include our function declarations

// Now include the actual cps.h content with our guards in place
#include "../cps.h" 