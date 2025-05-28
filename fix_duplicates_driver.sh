#!/bin/bash
# Fix duplicate symbol issues in BurnDrv driver functions

set -e  # Exit on error

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
RESET='\033[0m'

echo -e "${BLUE}Fixing duplicate driver function symbols...${RESET}"

# Create a unified driver functions file that will be the only implementation
mkdir -p build/metal_fixed
cat > build/metal_fixed/driver_functions.c << 'EOL'
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

// Basic type definitions needed for driver functions
typedef int INT32;
typedef unsigned int UINT32;
typedef unsigned char UINT8;

// Define constants needed for driver functions
#define DRV_NAME         0  // Game short name
#define DRV_DATE         1  // Release date
#define DRV_FULLNAME     2  // Game full name
#define DRV_COMMENT      3  // Comment
#define DRV_MANUFACTURER 4  // Manufacturer
#define DRV_SYSTEM       5  // System
#define DRV_PARENT       6  // Parent set name

// Sample driver info for Marvel vs Capcom
static char* s_szShortName = "mvsc";
static char* s_szDate = "1998";
static char* s_szFullNameA = "Marvel vs. Capcom: Clash of Super Heroes (USA 980123)";
static char* s_szCommentA = "Emulated by FBNeo Metal";
static char* s_szManufacturerA = "Capcom";
static char* s_szSystemA = "CPS2";

// Screen dimensions for Marvel vs Capcom
INT32 nScreenWidth = 384;   // CPS2 standard width
INT32 nScreenHeight = 224;  // CPS2 standard height

// Frame buffer variables needed by FBNeo
UINT8* pBurnDraw = NULL;    // Pointer to frame buffer
INT32 nBurnPitch = 0;       // Pitch in bytes
INT32 nBurnBpp = 0;         // Bytes per pixel

// Driver functions needed by FBNeo

// Get driver text information
const char* BurnDrvGetTextA(UINT32 i) {
    switch (i) {
        case DRV_NAME:
            return s_szShortName;
        case DRV_FULLNAME:
            return s_szFullNameA;
        case DRV_MANUFACTURER:
            return s_szManufacturerA;
        case DRV_SYSTEM:
            return s_szSystemA;
        case DRV_COMMENT:
            return s_szCommentA;
        case DRV_DATE:
            return s_szDate;
        default:
            return "";
    }
}

// Get the index of a driver by name
INT32 BurnDrvGetIndex(const char* szName) {
    if (szName && strcmp(szName, "mvsc") == 0) {
        return 0;  // Return index 0 for Marvel vs. Capcom
    }
    return -1;
}

// Select a driver by index
INT32 BurnDrvSelect(INT32 nDrvNum) {
    return 0;  // Always succeed
}

// Initialize the driver
INT32 BurnDrvInit() {
    printf("BurnDrvInit: Initializing CPS2 driver\n");
    return 0;
}

// Exit the driver
INT32 BurnDrvExit() {
    printf("BurnDrvExit: Shutting down CPS2 driver\n");
    return 0;
}

// Run one frame of the driver
INT32 BurnDrvFrame() {
    return 0;  // Just a stub, to be replaced with actual implementation
}

// Get visible size of the game
INT32 BurnDrvGetVisibleSize(INT32* pnWidth, INT32* pnHeight) {
    if (pnWidth) *pnWidth = nScreenWidth;
    if (pnHeight) *pnHeight = nScreenHeight;
    return 0;
}

// Connect the frame buffer
void BurnDrvSetPBurnDraw(UINT8* pImage, INT32 nPitch, INT32 nBpp) {
    pBurnDraw = pImage;
    nBurnPitch = nPitch;
    nBurnBpp = nBpp;
    printf("BurnDrvSetPBurnDraw: Connecting frame buffer (%p, pitch %d, bpp %d)\n", pImage, nPitch, nBpp);
}

// Convert RGB values to high color format for the platform
UINT32 BurnHighCol(INT32 r, INT32 g, INT32 b, INT32 i) {
    // Ensure 0-255 range
    r = r < 0 ? 0 : (r > 255 ? 255 : r);
    g = g < 0 ? 0 : (g > 255 ? 255 : g);
    b = b < 0 ? 0 : (b > 255 ? 255 : b);
    
    // Return ARGB (standard Metal pixel format)
    return (0xFF << 24) | (r << 16) | (g << 8) | b;
}
EOL

# Comment out duplicate implementations in metal_stubs.c
if [ -f src/burner/metal/fixes/metal_stubs.c ]; then
    echo -e "${BLUE}Patching metal_stubs.c for driver functions...${RESET}"
    
    # Comment out BurnDrvGetTextA
    sed -i '' '/^const char\* BurnDrvGetTextA/,/^}/c\
// REMOVED: Duplicate implementation of BurnDrvGetTextA\
// Using the implementation from driver_functions.c\
const char* BurnDrvGetTextA_UNUSED(UINT32 i) {\
    // This function has been removed to avoid duplicates\
    return "Unused";\
}' src/burner/metal/fixes/metal_stubs.c
    
    # Comment out BurnDrvGetIndex
    sed -i '' '/^INT32 BurnDrvGetIndex/,/^}/c\
// REMOVED: Duplicate implementation of BurnDrvGetIndex\
// Using the implementation from driver_functions.c\
INT32 BurnDrvGetIndex_UNUSED(const char* name) {\
    // This function has been removed to avoid duplicates\
    return -1;\
}' src/burner/metal/fixes/metal_stubs.c
    
    # Comment out BurnDrvSelect
    sed -i '' '/^INT32 BurnDrvSelect/,/^}/c\
// REMOVED: Duplicate implementation of BurnDrvSelect\
// Using the implementation from driver_functions.c\
INT32 BurnDrvSelect_UNUSED(INT32 nDriver) {\
    // This function has been removed to avoid duplicates\
    return 0;\
}' src/burner/metal/fixes/metal_stubs.c
    
    # Comment out BurnDrvGetVisibleSize
    sed -i '' '/^INT32 BurnDrvGetVisibleSize/,/^}/c\
// REMOVED: Duplicate implementation of BurnDrvGetVisibleSize\
// Using the implementation from driver_functions.c\
INT32 BurnDrvGetVisibleSize_UNUSED(INT32* pnWidth, INT32* pnHeight) {\
    // This function has been removed to avoid duplicates\
    return 0;\
}' src/burner/metal/fixes/metal_stubs.c
    
    # Comment out BurnDrvInit
    sed -i '' '/^INT32 BurnDrvInit/,/^}/c\
// REMOVED: Duplicate implementation of BurnDrvInit\
// Using the implementation from driver_functions.c\
INT32 BurnDrvInit_UNUSED(void) {\
    // This function has been removed to avoid duplicates\
    return 0;\
}' src/burner/metal/fixes/metal_stubs.c
    
    # Comment out BurnDrvExit
    sed -i '' '/^INT32 BurnDrvExit/,/^}/c\
// REMOVED: Duplicate implementation of BurnDrvExit\
// Using the implementation from driver_functions.c\
INT32 BurnDrvExit_UNUSED(void) {\
    // This function has been removed to avoid duplicates\
    return 0;\
}' src/burner/metal/fixes/metal_stubs.c
    
    # Comment out BurnDrvFrame
    sed -i '' '/^INT32 BurnDrvFrame/,/^}/c\
// REMOVED: Duplicate implementation of BurnDrvFrame\
// Using the implementation from driver_functions.c\
INT32 BurnDrvFrame_UNUSED(void) {\
    // This function has been removed to avoid duplicates\
    return 0;\
}' src/burner/metal/fixes/metal_stubs.c
    
    # Comment out BurnHighCol
    sed -i '' '/^UINT32 BurnHighCol/,/^}/c\
// REMOVED: Duplicate implementation of BurnHighCol\
// Using the implementation from driver_functions.c\
UINT32 BurnHighCol_UNUSED(INT32 r, INT32 g, INT32 b, INT32 i) {\
    // This function has been removed to avoid duplicates\
    return 0;\
}' src/burner/metal/fixes/metal_stubs.c
fi

# Comment out duplicate implementations in metal_cps_stubs.c
if [ -f src/burner/metal/fixes/metal_cps_stubs.c ]; then
    echo -e "${BLUE}Patching metal_cps_stubs.c for driver functions...${RESET}"
    
    # Comment out BurnDrvGetTextA
    sed -i '' '/^char\* BurnDrvGetTextA/,/^}/c\
// REMOVED: Duplicate implementation of BurnDrvGetTextA\
// Using the implementation from driver_functions.c\
char* BurnDrvGetTextA_UNUSED(UINT32 i) {\
    // This function has been removed to avoid duplicates\
    return "Unused";\
}' src/burner/metal/fixes/metal_cps_stubs.c
    
    # Comment out BurnDrvGetIndex
    sed -i '' '/^INT32 BurnDrvGetIndex/,/^}/c\
// REMOVED: Duplicate implementation of BurnDrvGetIndex\
// Using the implementation from driver_functions.c\
INT32 BurnDrvGetIndex_UNUSED(char* szName) {\
    // This function has been removed to avoid duplicates\
    return -1;\
}' src/burner/metal/fixes/metal_cps_stubs.c
    
    # Comment out BurnDrvSelect
    sed -i '' '/^INT32 BurnDrvSelect/,/^}/c\
// REMOVED: Duplicate implementation of BurnDrvSelect\
// Using the implementation from driver_functions.c\
INT32 BurnDrvSelect_UNUSED(INT32 nDrvNum) {\
    // This function has been removed to avoid duplicates\
    return 0;\
}' src/burner/metal/fixes/metal_cps_stubs.c
    
    # Comment out BurnDrvInit
    sed -i '' '/^INT32 BurnDrvInit/,/^}/c\
// REMOVED: Duplicate implementation of BurnDrvInit\
// Using the implementation from driver_functions.c\
INT32 BurnDrvInit_UNUSED(void) {\
    // This function has been removed to avoid duplicates\
    return 0;\
}' src/burner/metal/fixes/metal_cps_stubs.c
    
    # Comment out BurnDrvExit
    sed -i '' '/^INT32 BurnDrvExit/,/^}/c\
// REMOVED: Duplicate implementation of BurnDrvExit\
// Using the implementation from driver_functions.c\
INT32 BurnDrvExit_UNUSED(void) {\
    // This function has been removed to avoid duplicates\
    return 0;\
}' src/burner/metal/fixes/metal_cps_stubs.c
    
    # Comment out BurnDrvFrame
    sed -i '' '/^INT32 BurnDrvFrame/,/^}/c\
// REMOVED: Duplicate implementation of BurnDrvFrame\
// Using the implementation from driver_functions.c\
INT32 BurnDrvFrame_UNUSED(void) {\
    // This function has been removed to avoid duplicates\
    return 0;\
}' src/burner/metal/fixes/metal_cps_stubs.c
    
    # Comment out BurnDrvGetVisibleSize
    sed -i '' '/^INT32 BurnDrvGetVisibleSize/,/^}/c\
// REMOVED: Duplicate implementation of BurnDrvGetVisibleSize\
// Using the implementation from driver_functions.c\
INT32 BurnDrvGetVisibleSize_UNUSED(INT32* pnWidth, INT32* pnHeight) {\
    // This function has been removed to avoid duplicates\
    return 0;\
}' src/burner/metal/fixes/metal_cps_stubs.c
    
    # Comment out BurnDrvSetPBurnDraw
    sed -i '' '/^void BurnDrvSetPBurnDraw/,/^}/c\
// REMOVED: Duplicate implementation of BurnDrvSetPBurnDraw\
// Using the implementation from driver_functions.c\
void BurnDrvSetPBurnDraw_UNUSED(UINT8* pImage, INT32 nPitch, INT32 nBpp) {\
    // This function has been removed to avoid duplicates\
}' src/burner/metal/fixes/metal_cps_stubs.c
fi

echo -e "${GREEN}Driver function duplicates fixed successfully!${RESET}" 