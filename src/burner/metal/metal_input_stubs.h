#ifndef METAL_INPUT_STUBS_H
#define METAL_INPUT_STUBS_H

#ifdef __cplusplus
extern "C" {
#endif

// Basic type definitions if needed
typedef int INT32;

// Function declarations with correct signatures matching metal_compat_layer.h
INT32 Metal_InitInput();
INT32 Metal_ExitInput();
INT32 Metal_HandleKeyDown(int keyCode);
INT32 Metal_HandleKeyUp(int keyCode);
void Metal_ProcessInput();

#ifdef __cplusplus
}
#endif

#endif // METAL_INPUT_STUBS_H 