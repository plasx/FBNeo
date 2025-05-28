#ifndef _METAL_FIXES_H_
#define _METAL_FIXES_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

// Forward declarations for struct types used in the codebase
struct CheatInfo;
struct RomDataInfo;
struct BurnRomInfo;
struct tm;

// Fix for missing enum forward declarations
enum BurnCartrigeCommand;

// Define constants that will not conflict with FBNeo core
#define MAX_AI_ACTIONS 16


// AI Structures
typedef struct {
    bool active;
    float confidence;
    char name[32];
} AIAction;

typedef struct {
    char name[64];
    char version[16];
    int input_width;
    int input_height;
    int input_channels;
    int action_count;
    int model_type;
    int compute_backend;
    int precision;
    unsigned int features;
} AIModelInfo;

// Function declarations for frame dimensions
int Metal_GetFrameWidth(void);
int Metal_GetFrameHeight(void);


// Audio function declarations to fix order dependencies
void Audio_Exit_C(void);
void Audio_Stop_C(void);
void Audio_ExitMetal_C(void);


// Game genre variables defined as integers
extern int GENRE_HORSHOOT;
extern int GENRE_VERSHOOT;
extern int GENRE_SCRFIGHT;
extern int GENRE_VSFIGHT;
extern int GENRE_PLATFORM;


// CoreML and AI integration functions
bool CoreML_FindDefaultModels(void);
bool CoreML_GetModelInfo(AIModelInfo* info);
bool CoreML_Predict(void* data, void* actions);


// Fix for AI_ProcessFrame
typedef struct {
    int flags;
    float confidence;
} C_AIOutputAction;

C_AIOutputAction AI_ProcessFrame(void* gameState, int frameNumber);


#endif // _METAL_FIXES_H_
