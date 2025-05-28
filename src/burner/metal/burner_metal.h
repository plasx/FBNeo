#pragma once

// Only include in Metal builds
#ifdef BUILD_METAL

// Standard includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>

// In Objective-C context, import Foundation to get proper BOOL definition
#ifdef __OBJC__
#import <Foundation/Foundation.h>
#endif

// Define types compatible with FBNeo
typedef uint8_t UINT8;
typedef uint16_t UINT16;
typedef uint32_t UINT32;
typedef uint64_t UINT64;

typedef int8_t INT8;
typedef int16_t INT16;
typedef int32_t INT32;
typedef int64_t INT64;

typedef char CHAR;
typedef char* PCHAR;

// Only define BOOL if not in Objective-C context
#ifndef __OBJC__
typedef int BOOL;
#define TRUE  1
#define FALSE 0
#endif

#ifndef NULL
#define NULL 0
#endif

// Definitions for the Metal frontend
#define METAL_BUILD 1

// Directory type definitions - used for consistent paths on macOS
#define DIRNAME_MAX 256
#define DIRS_MAX 10
#define MAX_PATH 260

// Defines for macOS directories
#define DIRTYPE_ROM         0
#define DIRTYPE_PREVIEW     1
#define DIRTYPE_TITLE       2
#define DIRTYPE_SCREENSHOTS 3
#define DIRTYPE_CHEATS      4
#define DIRTYPE_HISCORE     5
#define DIRTYPE_SAMPLES     6
#define DIRTYPE_CONFIG      7
#define DIRTYPE_NVRAM       8
#define DIRTYPE_SAVESTATE   9

// The driver text field definitions
// Using same values as burn.h
#define DRV_NAME        (0)
#define DRV_DATE        (1)
#define DRV_FULLNAME    (2)
#define DRV_COMMENT     (4)  // 4 in burn.h
#define DRV_MANUFACTURER (5)  // 5 in burn.h
#define DRV_SOUND       (5)
#define DRV_INPUT       (6)
#define DRV_WIDTH       (7)
#define DRV_HEIGHT      (8)
#define DRV_COLOR_DEPTH (9)
#define DRV_MAX         (10)
#define DRV_PARENT      (7)  // 7 in burn.h
#define DRV_BOARDROM    (8)  // 8 in burn.h
#define DRV_SAMPLENAME  (9)  // 9 in burn.h

// Input-related defines for Metal frontend
#define MAX_KEYBINDS 16
#define MAX_PLAYERS 4

// Only define FBNeoSettings if it's not already defined
// This prevents conflicts with metal_app.h
#ifndef _FBNEO_SETTINGS_DEFINED
#define _FBNEO_SETTINGS_DEFINED

// Settings to be implemented in Metal frontend
typedef struct {
    int scalingMode;  // 0: Nearest, 1: Linear, 2: CRT
    int aspectRatio;  // 0: Original, 1: Full stretch, 2: 4:3, 3: 16:9
    
    // In ObjC context, use BOOL, in C++ context use int
    #ifdef __OBJC__
    BOOL scanlines;   // Scanline effect on/off
    BOOL smoothing;   // Texture smoothing on/off
    BOOL fullscreen;  // Fullscreen mode on/off
    #else
    int scanlines;    // Scanline effect on/off
    int smoothing;    // Texture smoothing on/off
    int fullscreen;   // Fullscreen mode on/off
    #endif
    
    int frameSkip;    // Number of frames to skip
    
    #ifdef __OBJC__
    BOOL vsync;       // Vertical sync on/off
    BOOL showFPS;     // Show FPS counter on/off
    
    // Audio settings
    BOOL audioEnabled;  // Audio enabled/disabled
    #else
    int vsync;         // Vertical sync on/off
    int showFPS;       // Show FPS counter on/off
    
    // Audio settings
    int audioEnabled;   // Audio enabled/disabled
    #endif
    
    int volume;         // Audio volume (0-100)
    int sampleRate;     // Sample rate (e.g., 44100, 48000)
    
    // Input settings
    #ifdef __OBJC__
    BOOL autoFire;      // Auto-fire enabled/disabled
    #else
    int autoFire;       // Auto-fire enabled/disabled
    #endif
    
    int autoFireRate;   // Auto-fire rate (in frames)
    
    // Debug settings
    #ifdef __OBJC__
    BOOL hitboxViewer;    // Show hitboxes on/off
    BOOL frameCounter;    // Show frame counter on/off
    BOOL inputDisplay;    // Show input display on/off
    
    // AI settings
    BOOL aiEnabled;          // AI features enabled/disabled
    #else
    int hitboxViewer;     // Show hitboxes on/off
    int frameCounter;     // Show frame counter on/off
    int inputDisplay;     // Show input display on/off
    
    // AI settings
    int aiEnabled;           // AI features enabled/disabled
    #endif
    
    int aiControlledPlayer;  // Which player AI controls (1, 2, or 3 for both)
    int aiDifficulty;        // AI difficulty level (1-5)
    
    #ifdef __OBJC__
    BOOL aiTrainingMode;     // AI training mode on/off
    BOOL aiDebugOverlay;     // Show AI debug info on/off
    #else
    int aiTrainingMode;      // AI training mode on/off
    int aiDebugOverlay;      // Show AI debug info on/off
    #endif
    
    // Display mode
    int displayMode;     // 0: Normal, 1: Stretched, 2: Integer scaled
    
    // For backward compatibility with metal_app.h
    #ifdef __OBJC__
    BOOL autoRun;
    BOOL enableSpeedHacks;
    #else
    int autoRun;
    int enableSpeedHacks;
    #endif
    
    int scanlineIntensity;
    int controllerType;
} FBNeoSettings;

#endif // _FBNEO_SETTINGS_DEFINED

// Base ROM paths
extern char szAppRomPaths[DIRS_MAX][MAX_PATH];
extern char szAppDirPath[MAX_PATH];

// Platform-specific path separator
#define PATH_SEPARATOR "/"

// Game driver information
struct BurnDrvMeta {
    char* szShortName;
    char* szFullNameA;
    INT32 nWidth;
    INT32 nHeight;
    INT32 nAspectX;
    INT32 nAspectY;
};

extern BurnDrvMeta BurnDrvInfo;

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration of key functions from burn_stubs.cpp
INT32 BurnLibInit_Metal();
INT32 BurnLibExit_Metal();
INT32 BurnDrvInit_Metal(INT32 nDrvNum);
INT32 BurnDrvExit_Metal();
char* BurnDrvGetTextA_Metal(UINT32 i);
INT32 BurnDrvGetIndexByName(const char* szName);
INT32 SetBurnHighCol(INT32 nDepth);

// CPS2 support functions
void Cps2_SetupMetalLinkage();

#ifdef __cplusplus
}
#endif

// ROM path handling
extern int GetCurrentROMPath(char* szPath, size_t len);
extern int SetCurrentROMPath(const char* szPath);
extern int ValidateROMPath(const char* path);

#endif // BUILD_METAL

