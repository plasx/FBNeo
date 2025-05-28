// Fixes for const qualifier issues in burn.cpp and other files
#ifndef METAL_CONST_FIXES_H
#define METAL_CONST_FIXES_H

// Add definition that tells burnint.h to skip BurnDriver definition
#define SKIP_DRIVER_DEFINITION

// Replace char* with const char* in BurnDriver struct
// This is necessary because many string literals are passed to non-const char*
#ifdef __cplusplus
#define NO_CONST_CHAR const_cast<char*>
#define NO_CONST_WCHAR const_cast<wchar_t*>
#else
#define NO_CONST_CHAR (char*)
#define NO_CONST_WCHAR (wchar_t*)
#endif

// For Metal build, we need to handle the const qualifier issues differently
#define BDF_HISCORE_SUPPORTED (1 << 17)

// Fix struct initializers that need void* vs int conversion
typedef void* GameGenre;

// Define genre constants as void* instead of int
#define GENRE_CONST(x) ((GameGenre)(void*)(x))

// Redefine game genres
#undef GBF_HORSHOOT
#undef GBF_VERSHOOT
#undef GBF_SCRFIGHT
#undef GBF_PLATFORM
#undef GBF_VSFIGHT
#undef GBF_BIOS
#undef GBF_BREAKOUT
#undef GBF_CASINO
#undef GBF_BALLPADDLE
#undef GBF_MAZE
#undef GBF_MINIGAMES
#undef GBF_PINBALL
#undef GBF_PUZZLE
#undef GBF_QUIZ
#undef GBF_SPORTSFOOTBALL
#undef GBF_SPORTSMISC
#undef GBF_MISC
#undef GBF_MAHJONG
#undef GBF_RACING
#undef GBF_SHOOT

// Define game genres as void* for correct struct initialization
#define GBF_HORSHOOT GENRE_CONST(1 << 0)
#define GBF_VERSHOOT GENRE_CONST(1 << 1)
#define GBF_SCRFIGHT GENRE_CONST(1 << 2)
#define GBF_PLATFORM GENRE_CONST(1 << 11)
#define GBF_VSFIGHT GENRE_CONST(1 << 4)
#define GBF_BIOS GENRE_CONST(1 << 5)
#define GBF_BREAKOUT GENRE_CONST(1 << 6)
#define GBF_CASINO GENRE_CONST(1 << 7)
#define GBF_BALLPADDLE GENRE_CONST(1 << 8)
#define GBF_MAZE GENRE_CONST(1 << 9)
#define GBF_MINIGAMES GENRE_CONST(1 << 10)
#define GBF_PINBALL GENRE_CONST(1 << 11)
#define GBF_PUZZLE GENRE_CONST(1 << 12)
#define GBF_QUIZ GENRE_CONST(1 << 13)
#define GBF_SPORTSFOOTBALL GENRE_CONST(1 << 14)
#define GBF_SPORTSMISC GENRE_CONST(1 << 14)
#define GBF_MISC GENRE_CONST(1 << 15)
#define GBF_MAHJONG GENRE_CONST(1 << 16)
#define GBF_RACING GENRE_CONST(1 << 17)
#define GBF_SHOOT GENRE_CONST(1 << 18)

#endif // METAL_CONST_FIXES_H
