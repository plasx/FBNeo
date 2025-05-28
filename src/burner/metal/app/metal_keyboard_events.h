#ifndef METAL_KEYBOARD_EVENTS_H
#define METAL_KEYBOARD_EVENTS_H

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration for Objective-C types
#ifdef __OBJC__
@class NSEvent;
@class NSRect;
#else
typedef void NSEvent;
typedef struct { float x, y, width, height; } NSRect;
#endif

// Function prototypes for keyboard event handling
int MetalApp_HandleKeyDown(NSEvent *event);
int MetalApp_HandleKeyUp(NSEvent *event);

// Function prototypes for keyboard view creation/management
void* MetalApp_CreateKeyboardView(NSRect frame);
void MetalApp_ReleaseKeyboardView(void* viewPtr);

#ifdef __cplusplus
}
#endif

#endif // METAL_KEYBOARD_EVENTS_H 