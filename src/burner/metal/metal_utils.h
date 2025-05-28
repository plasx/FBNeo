#pragma once

#ifdef __OBJC__
@protocol MTLDevice;
@protocol MTLTexture;
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Frame texture update function declared in metal_declarations.h
void UpdateMetalFrameTexture(const void* frameData, unsigned int width, unsigned int height);

#ifdef __OBJC__
// Get the current frame texture
id<MTLTexture> GetCurrentFrameTexture();

// Set the Metal device
void SetMetalDevice(id<MTLDevice> device);
#endif

#ifdef __cplusplus
}
#endif 