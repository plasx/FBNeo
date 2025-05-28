#include <stdio.h>
#include "metal_renderer_c.h"

// Stub implementation of MetalRenderer_Init
int MetalRenderer_Init(void* view) {
    printf("Stub: MetalRenderer_Init called\n");
    return 0;
}

// Stub implementation of MetalRenderer_UpdateFrame
void MetalRenderer_UpdateFrame(const void* frameData, unsigned int width, unsigned int height) {
    printf("Stub: MetalRenderer_UpdateFrame called with %dx%d frame\n", width, height);
}

// Stub implementation of MetalRenderer_GetWidth
int MetalRenderer_GetWidth() {
    return 384;
}

// Stub implementation of MetalRenderer_GetHeight
int MetalRenderer_GetHeight() {
    return 224;
}
