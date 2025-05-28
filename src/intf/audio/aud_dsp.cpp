// Minimal aud_dsp.cpp for Metal build with no external dependencies

// For INT16 and INT32 types
#include <stdint.h>

// Define any required types for BOOL, etc.
typedef int BOOL;
#define FALSE 0
#define TRUE 1

// We need to avoid including burnint.h which causes issues

// Initialize DSP
void DSPCreate()
{
    // Nothing to do
}

// Clean up DSP
void DSPDestroy()
{
    // Nothing to do
}

// Apply DSP effects
void DSPApply(int16_t* buffer, int32_t samples)
{
    // Nothing to do - pass audio through unchanged
}

// Set DSP parameters
void DSPSetParameters(int32_t param1, int32_t param2)
{
    // Nothing to do
}

// Check if DSP is enabled
BOOL DSPEnabled()
{
    return FALSE; // DSP disabled for simplicity
}
