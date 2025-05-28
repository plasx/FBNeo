#ifndef METAL_WINDOW_H
#define METAL_WINDOW_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Window management
void* Metal_CreateWindow(int width, int height, const char* title);
void Metal_DestroyWindow(void* window);
void Metal_UpdateWindowFrameBuffer(void* window, uint32_t* buffer, int width, int height, int pitch);

// Input handling
void Metal_ProcessKeyEvent(int keyCode, bool isKeyDown);

#ifdef __cplusplus
}
#endif

#endif // METAL_WINDOW_H 