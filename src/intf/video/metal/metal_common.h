#pragma once

// Common includes for both C++ and Objective-C++
#include <stdint.h>

// Constants that don't depend on Metal
#define MAX_TEXTURE_SIZE 4096
#define MAX_VERTICES 6
#define MAX_PATH_LENGTH 260

#ifdef __OBJC__
// Metal-specific includes
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <simd/simd.h>
#import <QuartzCore/CAMetalLayer.h>
#endif

// Structure definitions for both Objective-C++ and C/C++
#ifdef __OBJC__
// Metal-specific types when in Objective-C++
typedef id<MTLBuffer> MetalBuffer;
typedef id<MTLTexture> MetalTexture;
typedef id<MTLSamplerState> MetalSamplerState;
typedef id<MTLDevice> MetalDevice;
typedef id<MTLCommandQueue> MetalCommandQueue;
typedef id<MTLRenderPipelineState> MetalRenderPipelineState;
typedef CAMetalLayer* MetalLayer;
#else
// Opaque types for C/C++
typedef void* MetalBuffer;
typedef void* MetalTexture;
typedef void* MetalSamplerState;
typedef void* MetalDevice;
typedef void* MetalCommandQueue;
typedef void* MetalRenderPipelineState;
typedef void* MetalLayer;
#endif

// Vertex structure (compatible with both languages)
typedef struct {
    float position[2];
    float texCoord[2];
} VertexIn;

// Post-processing parameters (compatible with both languages)
typedef struct {
    float screenSize[2];
    float textureSize[2];
    float scanlineOpacity;
    float brightness;
    float contrast;
    float saturation;
    float noiseLevel;
    float chromaSize;
    float vignette;
    float padding[3];  // For 16-byte alignment
} PostProcessParams;

// Metal interface structure
typedef struct {
    // Initialization and cleanup
    INT32 (*Init)(INT32 nWidth, INT32 nHeight, INT32 nBpp);
    INT32 (*Exit)();
    
    // Frame handling
    INT32 (*Frame)(bool bRedraw);
    INT32 (*Paint)(int bValidate);
    
    // Image size
    INT32 (*ImageSize)(RECT* pSize, INT32* pnWidth, INT32* pnHeight);
    
    // Settings
    INT32 (*SetupSettings)();
} VidOutMetal;

// The global Metal video output interface
extern VidOutMetal VidOut;

#endif // _METAL_COMMON_H_ 