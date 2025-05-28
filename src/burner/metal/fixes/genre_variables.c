#include <stdint.h>

/*
 * Game genre variables for Metal build
 * 
 * This file defines variables for game genres that are normally
 * defined as macros with bit-shift expressions in burn.h.
 * 
 * For C compatibility, we define them as actual variables
 * that can be used in variable contexts.
 */

// Define the genre values as unsigned integer constants
const unsigned int GENRE_HORSHOOT    = 1U << 0;   // 1
const unsigned int GENRE_VERSHOOT    = 1U << 1;   // 2
const unsigned int GENRE_SCRFIGHT    = 1U << 2;   // 4
const unsigned int GENRE_VSFIGHT     = 1U << 3;   // 8
const unsigned int GENRE_BIOS        = 1U << 4;   // 16
const unsigned int GENRE_PUZZLE      = 1U << 5;   // 32
const unsigned int GENRE_BREAKOUT    = 1U << 6;   // 64
const unsigned int GENRE_CASINO      = 1U << 7;   // 128
const unsigned int GENRE_BALLPADDLE  = 1U << 8;   // 256
const unsigned int GENRE_MAZE        = 1U << 9;   // 512
const unsigned int GENRE_MINIGAMES   = 1U << 10;  // 1024
const unsigned int GENRE_PLATFORM    = 1U << 11;  // 2048
const unsigned int GENRE_QUIZ        = 1U << 13;  // 8192
const unsigned int GENRE_SPORTSMISC  = 1U << 14;  // 16384
const unsigned int GENRE_SPORTSFOOTBALL = 1U << 15; // 32768
const unsigned int GENRE_MISC        = 1U << 16;  // 65536
const unsigned int GENRE_RACING      = 1U << 17;  // 131072
const unsigned int GENRE_SHOOT       = 1U << 18;  // 262144
const unsigned int GENRE_SPORTS      = 1U << 19;  // 524288

// The U suffix ensures unsigned integer constant
// The explicit typecast is not required but adds clarity for readers 