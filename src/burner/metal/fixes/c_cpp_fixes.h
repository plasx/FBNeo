#ifndef _C_CPP_FIXES_H_
#define _C_CPP_FIXES_H_

/**
 * C/C++ Compatibility Layer for FBNeo Metal
 * This header provides fixes for common issues between C and C++ code
 */

// Basic C/C++ compatibility
#ifdef __cplusplus
// C++ code
#define EXTERN_C extern "C"
#define EXTERN_C_BEGIN extern "C" {
#define EXTERN_C_END }
#else
// C code
#define EXTERN_C
#define EXTERN_C_BEGIN
#define EXTERN_C_END
#include <stdbool.h>  // For bool type in C code
#endif

// Define missing types if needed
#ifndef UINT8
typedef unsigned char UINT8;
#endif

#ifndef INT8
typedef signed char INT8;
#endif

#ifndef UINT16
typedef unsigned short UINT16;
#endif

#ifndef INT16
typedef signed short INT16;
#endif

#ifndef UINT32
typedef unsigned int UINT32;
#endif

#ifndef INT32
typedef signed int INT32;
#endif

// Fix struct handling between C and C++
#ifdef __cplusplus
#define STRUCT_TYPE(name) name
#define ENUM_TYPE(name) name
#else
#define STRUCT_TYPE(name) struct name
#define ENUM_TYPE(name) enum name
#endif

// AI compatibility - these structs need special handling between C/C++
#ifndef _AI_STUB_TYPES_H_
// Forward declarations for AI types
struct AIAction;
struct AIActions;
struct AIModelInfo;
struct AIFrameData;
struct AIConfig;
#endif

// Game genre constants as variable names (not macros)
// These can be used in C code without macro expansion issues
static const unsigned int C_GENRE_HORSHOOT = 1 << 0;
static const unsigned int C_GENRE_VERSHOOT = 1 << 1;
static const unsigned int C_GENRE_SCRFIGHT = 1 << 2;
static const unsigned int C_GENRE_VSFIGHT = 1 << 3;
static const unsigned int C_GENRE_BIOS = 1 << 4;
static const unsigned int C_GENRE_PUZZLE = 1 << 5;
static const unsigned int C_GENRE_PLATFORM = 1 << 11;

// Function declarations for C compatibility
EXTERN_C_BEGIN

// Metal core functions
INT32 Metal_RunFrame(int bDraw);
int Metal_GenerateTestPattern(int patternType);
int Metal_VerifyFramePipeline(int width, int height);
const char* GetROMPathString(void);
int SetCurrentROMPath(const char* szPath);
int Metal_GetFrameWidth(void);
int Metal_GetFrameHeight(void);

// CoreML functions
bool CoreML_Initialize(void);
void CoreML_Shutdown(void);
bool CoreML_FindDefaultModels(void);
bool CoreML_LoadModel(const char* path);
bool CoreML_GetModelInfo(struct AIModelInfo* info);
bool CoreML_ProcessFrame(const void* frameData, int width, int height, int pitch, float* results, int resultSize);

// AI-related functions
void AI_SetControlledPlayer(int playerIndex);
void AI_SetDifficulty(int level);
void AI_EnableTrainingMode(int enable);
void AI_EnableDebugOverlay(int enable);
void AI_SaveFrameData(const char* filename);
void AI_ConfigureGameMemoryMapping(int gameType, const char* gameId);
void* AI_GetGameObservation(void);

EXTERN_C_END

#endif // _C_CPP_FIXES_H_
