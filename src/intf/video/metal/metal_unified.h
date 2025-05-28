#pragma once

// This header provides common definitions for Metal implementations
// to avoid duplicate code and ensure compatibility between modules

#ifdef __OBJC__
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <simd/simd.h>
#endif

// Include standard headers from the codebase to avoid duplicate declarations
#include "../../../burn/burn.h"
#include "../../interface.h"

// Metal-specific vertex structure 
typedef struct {
    float position[2];
    float texcoord[2];
} MetalVertex;

// Metal-specific vertex output structure
typedef struct {
    float position[4]; // position (x, y, z, w)
    float texcoord[2]; // texture coordinates (u, v)
} MetalVertexOutput;

// Define Metal-specific constants
#define METAL_MAX_TEXTURES 16
#define METAL_MAX_BUFFERS 8
#define METAL_VERTEX_BUFFER_INDEX 0
#define METAL_UNIFORM_BUFFER_INDEX 1
#define METAL_TEXTURE_INDEX 0

// Metal rendering functions
#ifdef __cplusplus
extern "C" {
#endif

// Function prototypes
int  MetalInit(void* view);
void MetalShutdown();
void MetalDrawFrame(unsigned char* buffer, int width, int height, int pitch);
void MetalResize(int width, int height);
void MetalSetVSync(bool enabled);

#ifdef __cplusplus
}
#endif 