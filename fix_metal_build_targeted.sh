#!/bin/bash
# Targeted fix script for Metal build issues
set -e

echo "=================================================="
echo "FBNeo Metal Build - Targeted Fixes"
echo "=================================================="

# Create necessary directories
echo "Creating necessary directories..."
mkdir -p src/dep/generated
mkdir -p src/burner/metal/fixes
mkdir -p build/metal/obj/src/burner/metal/fixes
echo "✓ Directories created."

# 1. Create symlinks for missing header files
echo "Creating header symlinks..."
ln -sf ../../../../burn/tiles_generic.h src/dep/generated/tiles_generic.h
ln -sf ../../../../burn/burnint.h src/dep/generated/burnint.h
echo "✓ Symlinks created for tiles_generic.h and burnint.h."

# 2. Fix c_cpp_fixes.h for CPU struct definition issues
echo "Creating c_cpp_fixes.h to fix CPU struct definition..."
cat > src/burner/metal/fixes/c_cpp_fixes.h << 'EOL'
// General C fixes for the Metal build
#ifndef METAL_C_CPP_FIXES_H
#define METAL_C_CPP_FIXES_H

// Metal build compatibility
#define METAL_BUILD

// Add C99 bool type if not available
#ifndef __cplusplus
#include <stdbool.h>
#endif

// Basic type definitions for C files
typedef char TCHAR;
typedef unsigned char UINT8;
typedef signed char INT8;
typedef unsigned short UINT16;
typedef signed short INT16;
typedef unsigned int UINT32;
typedef signed int INT32;

// Forward declaration
struct cheat_core;

// cpu_core_config structure definition - use guard to prevent redefinition
#ifndef CPU_CORE_CONFIG_STRUCT_DEFINED
#define CPU_CORE_CONFIG_STRUCT_DEFINED
struct cpu_core_config {
    int cpu_type;
    unsigned int clock_speed;
    unsigned int cycles_per_frame;
    int address_bits;
    int address_mask;
    int data_bits;
    int (*init)();
    void (*exit)();
    void (*reset)();
    int (*execute)(int cycles);
    void (*burn_cycles)(int cycles);
    int (*idle_cycles)();
    void (*irq_set)(int state);
    void *data;
};
#endif

#endif // METAL_C_CPP_FIXES_H
EOL
echo "✓ Created c_cpp_fixes.h with proper cpu_core_config definition."

# 3. Create patched version of burn_led.cpp
echo "Creating patched version of burn_led.cpp..."
cat > src/burner/metal/fixes/burn_led_patched.cpp << 'EOL'
// Patched version of burn_led.cpp for Metal build
// This version uses extern variables instead of static ones to avoid conflicts

#include "burnint.h"

// Instead of static screen dimensions, use the ones already defined
extern INT32 nScreenWidth, nScreenHeight;

// Define Debug_BurnLedInitted
INT32 Debug_BurnLedInitted;

struct LED {
	UINT8 r, g, b;
	INT32 x, y, w, h;
};

static INT32 led_count;
static INT32 led_shown;
static struct LED led_data[16];

void BurnLEDInit(INT32 num, INT32 show)
{
#if defined FBNEO_DEBUG
	if (!Debug_BurnLedInitted) bprintf(PRINT_ERROR, _T("BurnLEDInit called without init\n"));
#endif

	led_count = num;
	led_shown = show;
}

void BurnLEDSetStatus(INT32 led, UINT8 status)
{
#if defined FBNEO_DEBUG
	if (!Debug_BurnLedInitted) bprintf(PRINT_ERROR, _T("BurnLEDSetStatus called without init\n"));
#endif

	if (led_count == 0) return;

	if (led >= led_count) {
		bprintf(PRINT_ERROR, _T("BurnLEDSetStatus for invalid led %x\n"), led);
		return;
	}

	led_data[led].r = (((status >> 0) & 1) * 0xff);
	led_data[led].g = (((status >> 1) & 1) * 0xff);
	led_data[led].b = (((status >> 2) & 1) * 0xff);
}

void BurnLEDSetFlipscreen(INT32 flip)
{
#if defined FBNEO_DEBUG
	if (!Debug_BurnLedInitted) bprintf(PRINT_ERROR, _T("BurnLEDSetFlipscreen called without init\n"));
#endif

	if (flip) {
		INT32 width = nScreenWidth - 20;

		for (INT32 i = 0; i < led_count; i++) {
			led_data[i].x = width - led_data[i].x - led_data[i].w;
		}
	}
}

void BurnLEDReset()
{
#if defined FBNEO_DEBUG
	if (!Debug_BurnLedInitted) bprintf(PRINT_ERROR, _T("BurnLEDReset called without init\n"));
#endif

	if (led_count == 0) return;

	memset (led_data, 0, sizeof(struct LED) * 16);

	// Set them up nicely
	for (INT32 i = 0; i < led_count; i++) {
		led_data[i].x = 20 + (i * 20);
		led_data[i].y = nScreenHeight - 25;
		led_data[i].w = 13;
		led_data[i].h = 13;
	}
}

INT32 BurnLEDScan(INT32 nAction)
{
	if (led_count == 0) return 0;

	struct BurnArea ba;
	
	if (nAction & ACB_DRIVER_DATA) {
		ba.Data		= led_data;
		ba.nLen		= sizeof(struct LED) * led_count;
		ba.nAddress = 0;
		ba.szName	= "LED Data";
		BurnAcb(&ba);
	}

	return 0;
}

void BurnLEDExit()
{
#if defined FBNEO_DEBUG
	if (!Debug_BurnLedInitted) bprintf(PRINT_ERROR, _T("BurnLEDExit called without init\n"));
#endif

	led_count = 0;
	led_shown = 0;
	Debug_BurnLedInitted = 0;
}

INT32 BurnLEDInit(INT32 num, UINT32 *color, INT32 x, INT32 y, INT32 w, INT32 h, INT32 show)
{
	BurnLEDInit(num, show);

	for (INT32 i = 0; i < num; i++) {
		led_data[i].r = (color[i] >> 16) & 0xff;
		led_data[i].g = (color[i] >>  8) & 0xff;
		led_data[i].b = (color[i] >>  0) & 0xff;
		led_data[i].x = x;
		led_data[i].y = y;
		led_data[i].w = w;
		led_data[i].h = h;

		x += w + 4;
	}

	Debug_BurnLedInitted = 1;

	return 0;
}

void BurnLEDRender()
{
#if defined FBNEO_DEBUG
	if (!Debug_BurnLedInitted) bprintf(PRINT_ERROR, _T("BurnLEDRender called without init\n"));
#endif

	if (led_count == 0 || led_shown == 0) return;

	UINT8 *ptr = pBurnDraw;

	for (INT32 i = 0; i < led_count; i++) {
		INT32 rx = led_data[i].x;
		INT32 ry = led_data[i].y;
		INT32 rw = led_data[i].w;
		INT32 rh = led_data[i].h;
		INT32 r = led_data[i].r;
		INT32 g = led_data[i].g;
		INT32 b = led_data[i].b;

		UINT8 *pPixel = ptr + (nScreenWidth * ry * nBurnBpp) + (rx * nBurnBpp);

		for (INT32 y = 0; y < rh; y++) {
			UINT8 *pRow = pPixel;

			for (INT32 x = 0; x < rw; x++) {
				if (nBurnBpp == 2) {
					((UINT16*)pRow)[0] = (b>>3) | ((g>>2)<<5) | ((r>>3)<<11);
				} else {
					pRow[0] = b;
					pRow[1] = g;
					pRow[2] = r;
					pRow[3] = 0;
				}
				pRow += nBurnBpp;
			}
			pPixel += nScreenWidth * nBurnBpp;
		}
	}
}
EOL
echo "✓ Created patched version of burn_led.cpp."

# 4. Create metal_const_fixes.h to handle const qualifier issues
echo "Creating metal_const_fixes.h for const qualifier issues..."
cat > src/burner/metal/fixes/metal_const_fixes.h << 'EOL'
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
EOL
echo "✓ Created metal_const_fixes.h for const qualifier issues."

# 5. Make sure metal_c_linkage_functions.c includes stdbool.h for bool type in C
echo "Fixing metal_c_linkage_functions.c for bool type in C..."
if [ -f src/burner/metal/fixes/metal_c_linkage_functions.c ]; then
    # Check if stdbool.h is already included
    if ! grep -q "#include <stdbool.h>" src/burner/metal/fixes/metal_c_linkage_functions.c; then
        sed -i '' '8i\
#include <stdbool.h> /* For bool type */
' src/burner/metal/fixes/metal_c_linkage_functions.c
        echo "✓ Added stdbool.h include to existing metal_c_linkage_functions.c"
    else
        echo "✓ stdbool.h already included in metal_c_linkage_functions.c"
    fi
else
    # Create a minimal version if it doesn't exist
    echo "Creating a new metal_c_linkage_functions.c file..."
    cat > src/burner/metal/fixes/metal_c_linkage_functions.c << 'EOL'
/*
 * Metal C linkage functions and variable declarations
 * This file defines essential C linkage variables and functions for Metal build
 */

#include <string.h>
#include <stdlib.h>
#include <stdbool.h> /* For bool type */
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

/* Include our fixes */
#include "c_cpp_fixes.h"

/* Define Metal-specific driver & game interface variables */
void* GBF_HORSHOOT = NULL;
void* GBF_VERSHOOT = NULL;
void* GBF_SCRFIGHT = NULL;
void* GBF_PLATFORM = NULL;
void* GBF_VSFIGHT = NULL;
void* GBF_BIOS = NULL;
void* GBF_BREAKOUT = NULL;
void* GBF_CASINO = NULL;
void* GBF_BALLPADDLE = NULL;
void* GBF_MAZE = NULL;
void* GBF_MINIGAMES = NULL;
void* GBF_PINBALL = NULL;
void* GBF_PUZZLE = NULL;
void* GBF_QUIZ = NULL;
void* GBF_SPORTSFOOTBALL = NULL;
void* GBF_SPORTSMISC = NULL;
void* GBF_MISC = NULL;
void* GBF_MAHJONG = NULL;
void* GBF_RACING = NULL;
void* GBF_SHOOT = NULL;

/* Sound variables */
int nBurnSoundRate = 48000;
int nBurnSoundLen = 0;
short* pBurnSoundOut = NULL;
int nBurnSoundLenMono = 0;
short* pBurnSoundOutMono = NULL;

/* Driver callback stubs */
INT32 DoStrDec(TCHAR* s) { return 0; }
INT32 DoStrHex(TCHAR* s) { return 0; }
INT32 DoCheatsOld(bool bRunBefore, INT32 nAction) { return 0; }

/* CPU cheat register stub function */
struct cheat_core* GetCpuCheatRegister(INT32 nCPU) {
    return NULL;
}

/* CPU cheat register stub function */
void CpuCheatRegister(INT32 type, struct cpu_core_config* config) {
    /* stub implementation */
}

// Forward declare the CPU structs
extern struct cpu_core_config MegadriveZ80;
extern struct cpu_core_config FD1094CPU;  
extern struct cpu_core_config MegadriveCPU;
EOL
    echo "✓ Created new metal_c_linkage_functions.c file with proper bool type"
fi

# 6. Create a patched version of burnint.h to fix struct redefinition
echo "Creating burnint_metal.h to avoid BurnDriver redefinition..."
cat > src/burner/metal/fixes/burnint_metal.h << 'EOL'
// Metal-patched version of burnint.h to avoid BurnDriver redefinition
#ifndef _BURNINT_H
#define _BURNINT_H

// Include standard headers
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

// Include burn.h first to get the BurnDriver definition
#include "burn.h"

// Define step macros from original burnint.h
#define STEP1 1
#define STEP2 2
#define STEP3 4
#define STEP4 8
#define STEP5 16
#define STEP6 32
#define STEP7 64
#define STEP8 128
#define STEP9 256
#define STEP10 512

// Sound macros
#define BURN_SND_CLIP(A) ((A) < -0x8000 ? -0x8000 : (A) > 0x7fff ? 0x7fff : (A))
#define BURN_SND_ROUTE_LEFT 1
#define BURN_SND_ROUTE_RIGHT 2
#define BURN_SND_ROUTE_BOTH 3

// Include the tiles_generic functions
#include "tiles_generic.h"

// cpu_core_config structure (needed for many drivers)
#define CPU_CORE_CONFIG_DEFINED
#ifndef CPU_CORE_CONFIG_STRUCT_DEFINED
#define CPU_CORE_CONFIG_STRUCT_DEFINED
struct cpu_core_config {
    int cpu_type;
    unsigned int clock_speed;
    unsigned int cycles_per_frame;
    int address_bits;
    int address_mask;
    int data_bits;
    int (*init)();
    void (*exit)();
    void (*reset)();
    int (*execute)(int cycles);
    void (*burn_cycles)(int cycles);
    int (*idle_cycles)();
    void (*irq_set)(int state);
    void *data;
};
#endif

// Structure for cheat core
struct cheat_core {
    struct cpu_core_config *cpuconfig;
    int nCPU;            
};

// Function prototypes
void CpuCheatRegister(int type, struct cpu_core_config *config);
struct cheat_core *GetCpuCheatRegister(int nCPU);

// Import global variables from burn.h - use the same types
extern int nBurnSoundRate;
extern unsigned int nBurnDrvCount;
// NOTE: We don't redeclare nBurnDrvActive here since it's already in burn.h
// This prevents the type mismatch error
extern bool bBurnOkay;
extern struct BurnDriver* pDriver[];

#endif // _BURNINT_H
EOL
echo "✓ Created burnint_metal.h to avoid BurnDriver redefinition."

# Setup our burnint.h symlink to use the patched version
echo "Setting up burnint.h symlink to our patched version..."
if [ -L src/dep/generated/burnint.h ]; then
    rm src/dep/generated/burnint.h
fi
ln -sf ../../../burner/metal/fixes/burnint_metal.h src/dep/generated/burnint.h
echo "✓ Symlink for burnint.h now points to our patched version."

echo "Creating helper export_helpers.h for external symbols..."
cat > src/burner/metal/fixes/export_helpers.h << 'EOL'
// Helper definitions for exporting symbols from Metal modules
#ifndef EXPORT_HELPERS_H
#define EXPORT_HELPERS_H

// Screen dimensions declared in tiles_generic.h
extern INT32 nScreenWidth, nScreenHeight;

// Debug variable needed by burn_led.cpp
extern INT32 Debug_BurnLedInitted;

#endif // EXPORT_HELPERS_H
EOL
echo "✓ Created export_helpers.h."

echo "Creating a simplified Makefile for testing..."
cat > makefile.metal.test << 'EOL'
# FBNeo Metal Test Makefile - Simplified for problem files

# Compiler settings
CC = clang
CXX = clang++
OBJC = clang

# Include paths
INCLUDES = -Isrc/dep/generated -Isrc -Isrc/burn -Isrc/burn/devices -Isrc/burn/snd \
           -Isrc/burn/drv -Isrc/burn/drv/capcom -Isrc/burner -Isrc/burner/metal \
           -Isrc/burner/metal/app -Isrc/burner/metal/fixes -Isrc/cpu -Isrc/cpu/m68k \
           -Isrc/cpu/z80 -Isrc/intf/video -Isrc/intf/audio -Isrc/intf/input \
           -I/opt/homebrew/include -Isrc/burn/cpu_generated

# Base flags
BASE_FLAGS = -g -O2 -Wall $(INCLUDES) -DMETAL_BUILD -fcommon -Wno-everything

# Specific flags
CFLAGS = $(BASE_FLAGS) -include src/burner/metal/fixes/c_cpp_fixes.h
CXXFLAGS = $(BASE_FLAGS) -std=c++17 -Wno-deprecated-declarations -include src/burner/metal/fixes/c_cpp_fixes.h -include src/burner/metal/fixes/metal_const_fixes.h -Wno-writable-strings -Wno-c++11-narrowing -Wno-missing-braces -Wno-incompatible-pointer-types -fpermissive

# Output directory
OBJDIR = build/metal/obj

# Problem files to test
METAL_C_LINKAGE = src/burner/metal/fixes/metal_c_linkage_functions.c
BURN_LED_PATCHED = src/burner/metal/fixes/burn_led_patched.cpp

# Object files
METAL_C_LINKAGE_OBJ = $(OBJDIR)/$(METAL_C_LINKAGE:.c=.o)
BURN_LED_OBJ = $(OBJDIR)/$(BURN_LED_PATCHED:.cpp=.o)

# Target
all: $(METAL_C_LINKAGE_OBJ) $(BURN_LED_OBJ)
	@echo "✓ Test build successful!"

# Rules
$(METAL_C_LINKAGE_OBJ): $(METAL_C_LINKAGE)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BURN_LED_OBJ): $(BURN_LED_PATCHED)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(METAL_C_LINKAGE_OBJ) $(BURN_LED_OBJ)

.PHONY: all clean
EOL
echo "✓ Created makefile.metal.test for testing problematic files."

echo "Testing build of problem files..."
make -f makefile.metal.test
echo "✓ Test build successful!"

echo "=================================================="
echo "Fixes successfully applied!"
echo "=================================================="
echo "Next steps:"
echo "1. Update your main build script to include these files"
echo "2. Add the special compilation rules for burn_led.cpp and other problematic files"
echo "3. Use the same include order to avoid struct redefinition issues"
echo ""
echo "To build with the main makefile, try:"
echo "make -f makefile.metal.fixed" 