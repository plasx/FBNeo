#!/bin/bash
# Script to fix issues in burn.cpp specifically
set -e

echo "FBNeo Metal Build - burn.cpp Fix"
echo "================================"

# Create necessary directories
mkdir -p src/burner/metal/fixes

# 1. Create a fix specifically for burn.cpp that includes complete BurnDriver definition
echo "Creating burn_cpp_fix.h for burn.cpp"
cat > src/burner/metal/fixes/burn_cpp_fix.h << 'EOL'
// Fix for burn.cpp specific issues
#ifndef BURN_CPP_FIX_H
#define BURN_CPP_FIX_H

// First include needed headers
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

// Define our own UINT32 type and INT32 type to match burn.h
typedef unsigned int UINT32;
typedef int INT32;

// Make sure this BurnDriver struct matches exactly burn.h
struct BurnDriver {
	const char* szShortName;			// 8.3 name of game driver
	const char* szParent;				// Parent of the driver
	const char* szBoardROM;				// Board ROMs
	const char* szAllRomsAllSoftwareRegionAllDisks;
	const char* szDate;					// Date of game driver release

	const char* szFullNameA;			// Full ASCII name of the game driver
	const char *szGlueTitle;			// Help glue name to date
	const char* szCommentA;				// Angle brackets - Comment about the game driver
	const char* szManufacturerA;		// ASCII name of the manufacturer
	const char* szSystemA;				// ASCII name of the system

	const char* szFullNameW;			// Full WCHAR name of the game driver
	const char* szCommentW;				// Angle brackets - Comment about the game driver
	const char* szManufacturerW;		// WCHAR name of the manufacturer
	const char* szSystemW;				// WCHAR name of the system

	int nGenre;							// Genre of the game
	int nFamily;						// Bitfield of 32 flags, hardware platform
	int nFlags;							// Bitfield of 32 flags, general driver flags

	int nMaxPlayers;					// The maximum number of players the game supports (1-4)
	int nWidth;							// Screen width
	int nHeight;						// Screen height
	int nXAspect;						// Aspect ratio, X axis
	int nYAspect;						// Aspect ratio, Y axis

	int nScrnFlags;						// Scrn flags
	void* pDriverCallback;				// Driver callback
	void* pGetZipName;					// Get .ZIP name callback
	void* pGetRomInfo;					// Get ROM info callback
	void* pGetRomName;					// Get ROM name callback
	void* pGetSampleInfo;				// Get samples info callback
	void* pGetSampleName;				// Get samples name callback
	void* pGetInputInfo;				// Get input info callback
	void* pGetDIPInfo;					// Get DIP switch info callback
	void* pInit;						// Initialisation callback
	void* pExit;						// Exit callback
	void* pFrame;						// Frame callback
	void* pDraw;						// Draw callback
	void* pScan;						// Scan Callback
	void* pSetColorTable;				// Set color table callback
};

// Define the type of nBurnDrvActive as int to match the modified burn.h
extern int nBurnDrvActive;
extern struct BurnDriver* pDriver[];

// Add function prototypes to avoid the BurnSoundInit undefined error
void BurnSoundInit();

#endif // BURN_CPP_FIX_H
EOL

# 2. Create special makefile rule for burn.cpp
echo "Creating a special makefile rule for burn.cpp"
cat > makefile.burn_cpp << 'EOL'
# Compile the burn.cpp file specifically
obj/metal/burner/burn/burn.o: src/burn/burn.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -include src/burner/metal/fixes/burn_cpp_fix.h -c $< -o $@
EOL

# 3. Edit the burn.cpp file to use int instead of UINT32 for nBurnDrvActive
echo "Modifying burn.cpp to use int for nBurnDrvActive"
BURN_CPP="src/burn/burn.cpp"
BURN_CPP_BAK="src/burn/burn.cpp.bak"

if [ ! -f "$BURN_CPP_BAK" ]; then
    echo "Backing up original burn.cpp..."
    cp "$BURN_CPP" "$BURN_CPP_BAK"
fi

# Use sed to replace UINT32 nBurnDrvActive with int nBurnDrvActive
sed -i '' 's/UINT32 nBurnDrvActive/int nBurnDrvActive/' "$BURN_CPP"

# 4. Create a simplified README with instructions
echo "Creating README.md..."
cat > README.burn_cpp.md << 'EOL'
# FBNeo Burn.cpp Fix

This script addresses specific issues in burn.cpp:

1. **BurnDriver struct redefinition** - Provides a complete definition in a special header
2. **nBurnDrvActive type mismatch** - Ensures the type is consistently 'int' across all files
3. **Undefined BurnSoundInit** - Adds a function prototype to avoid undefined errors

## How to use

1. Run the script:
   ```
   ./fix_burn_cpp.sh
   ```

2. When building, include the special rule for burn.cpp:
   ```
   make -f makefile.metal.fixed -f makefile.burn_cpp
   ```

This fix is specifically targeted at issues within burn.cpp and complements the more general fixes for the entire codebase.
EOL

echo "Fix completed. Use 'make -f makefile.metal.fixed -f makefile.burn_cpp' to build with this fix." 