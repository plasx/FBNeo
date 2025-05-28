#pragma once

#ifdef __OBJC__
@protocol MTLDevice;
extern id<MTLDevice> g_metalDevice;
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Initialize Metal device
bool InitMetalDevice();

// Shutdown Metal device
void ShutdownMetalDevice();

#ifdef __cplusplus
}
#endif 