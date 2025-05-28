#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "burnint_metal.h"
#include "c_cpp_fixes.h"

/*
 * This file provides minimal C stubs for FBNeo core functions needed by the Metal port.
 * Instead of including the original C++ headers which cause compatibility issues,
 * we use our C-compatible burnint_metal.h header.
 */

// Game dimensions
static int g_nGameWidth = 320;
static int g_nGameHeight = 240;

// Define constants for game genres as regular variables, 
// not using the macros from burn.h
const unsigned int GAME_TYPE_HORSHOOT = GENRE_HORSHOOT;
const unsigned int GAME_TYPE_VERSHOOT = GENRE_VERSHOOT;
const unsigned int GAME_TYPE_SCRFIGHT = GENRE_SCRFIGHT;
const unsigned int GAME_TYPE_VSFIGHT = GENRE_VSFIGHT;
const unsigned int GAME_TYPE_BIOS = GENRE_BIOS;
const unsigned int GAME_TYPE_PUZZLE = GENRE_PUZZLE;
const unsigned int GAME_TYPE_PLATFORM = GENRE_PLATFORM;

// Basic implementation of common functions needed by the Metal port

// Get game dimensions
void GetGameDimensions(int* width, int* height) {
    if (width) *width = g_nGameWidth;
    if (height) *height = g_nGameHeight;
}

// Set game dimensions
void SetGameDimensions(int width, int height) {
    if (width > 0 && height > 0) {
        g_nGameWidth = width;
        g_nGameHeight = height;
    }
}

// Get ROM name from path
const char* GetROMNameFromPath(const char* path) {
    if (!path) return NULL;
    
    // Extract file name from path
    const char* fileName = strrchr(path, '/');
    if (fileName) {
        fileName++; // Skip the slash
    } else {
        fileName = path; // No slash, use whole path
    }
    
    return fileName;
}

// Strip file extension
void StripFileExtension(char* fileName) {
    if (!fileName) return;
    
    char* dot = strrchr(fileName, '.');
    if (dot) {
        *dot = '\0';
    }
}

// Check if a ROM exists
int ROMExists(const char* romName) {
    if (!romName) return 0;
    
    // Simple implementation: just check if the driver exists
    INT32 drvIndex = BurnDrvGetIndex((char*)romName);
    return (drvIndex >= 0) ? 1 : 0;
}

// Get ROM info
int GetROMInfo(const char* romName, char* fullName, int fullNameLen, 
               int* width, int* height, int* genre) {
    if (!romName) return 0;
    
    // Try to find the driver
    INT32 drvIndex = BurnDrvGetIndex((char*)romName);
    if (drvIndex < 0) return 0;
    
    // Select the driver
    BurnDrvSelect(drvIndex);
    
    // Get full name if requested
    if (fullName && fullNameLen > 0) {
        char* name = BurnDrvGetTextA(DRV_FULLNAME);
        if (name) {
            strncpy(fullName, name, fullNameLen - 1);
            fullName[fullNameLen - 1] = '\0';
        } else {
            strncpy(fullName, "Unknown", fullNameLen - 1);
            fullName[fullNameLen - 1] = '\0';
        }
    }
    
    // Get dimensions if requested
    if (width && height) {
        BurnDrvGetVisibleSize(width, height);
    }
    
    // In a real implementation, this would get the genre
    if (genre) {
        *genre = GENRE_HORSHOOT; // Default to horizontal shooter
    }
    
    return 1;
}

// Run an emulation frame
int RunFrame(int bDraw) {
    return Metal_RunFrame(bDraw);
}

// Initialize the driver
int InitDriver(int driverIndex) {
    return BurnDrvInit_Metal(driverIndex);
}

// Exit the driver
int ExitDriver() {
    return BurnDrvExit_Metal();
}

// Initialize the library
int InitLibrary() {
    return BurnLibInit_Metal();
}

// Exit the library
int ExitLibrary() {
    return BurnLibExit_Metal();
}

// Get the current driver name
const char* GetDriverName() {
    return BurnDrvGetTextA(DRV_NAME);
}

// Get the driver date
const char* GetDriverDate() {
    return BurnDrvGetTextA(DRV_DATE);
}

// Generate a test pattern
int GenerateTestPattern(int width, int height, int patternType) {
    return Metal_GenerateTestPattern(patternType);
}

// Simple function to check if a memory region is valid
int IsMemoryRegionValid(unsigned int address, unsigned int size) {
    // Simple stub implementation
    // In a real implementation, this would check if the address range is valid
    return 1;
}

// Read a byte from memory
unsigned char ReadByte(unsigned int address) {
    // Simple stub implementation
    // In a real implementation, this would read from emulated memory
    return 0;
}

// Write a byte to memory
void WriteByte(unsigned int address, unsigned char value) {
    // Simple stub implementation
    // In a real implementation, this would write to emulated memory
}

// Read a word from memory
unsigned short ReadWord(unsigned int address) {
    // Simple stub implementation
    return 0;
}

// Write a word to memory
void WriteWord(unsigned int address, unsigned short value) {
    // Simple stub implementation
}

// Reset the driver
int ResetDriver() {
    return BurnDrvReset();
}

// Find a driver by name
int FindDriver(const char* name) {
    if (!name) return -1;
    return BurnDrvGetIndex((char*)name);
}

// Get the aspect ratio
void GetAspectRatio(int* x, int* y) {
    BurnDrvGetAspect(x, y);
}

// Convert RGB to high color
unsigned int ConvertRGB(int r, int g, int b) {
    return BurnHighCol32(r, g, b, 0);
} 