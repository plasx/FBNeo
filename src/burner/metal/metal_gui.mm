// src/burner/metal/metal_gui.mm

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

// FBNeo includes in C mode
extern "C" {
   #include "burnint.h"   // Has pBurnDraw, nBurnPitch, etc.
   // If your code is in burn.h or wherever, include them as needed
}

//-------------------------------------------------------
// FBNeo global variables typically declared in burnint.h
//-------------------------------------------------------
extern UINT8* pBurnDraw;    // Final image buffer
extern INT32  nBurnPitch;   // Bytes per row
extern INT32  nBurnBpp;     // Bytes per pixel (often 4)
extern INT32  nBurnWidth;   // width
extern INT32  nBurnHeight;  // height

//-------------------------------------------------------
// MetalRunFrame: runs one frame & copies the buffer
//-------------------------------------------------------
void MetalRunFrame(CAMetalLayer* layer) {
    if (!layer) return;

    //---------------------------------------------------
    // 1. Run the emulator frame
    //---------------------------------------------------
    BurnDrvFrame(); // typical function that updates pBurnDraw

    if (!pBurnDraw) {
        return; // no valid frame
    }

    //---------------------------------------------------
    // 2. Prepare Metal objects
    //---------------------------------------------------
    id<MTLDevice> device = layer.device;
    if (!device) return;

    static id<MTLCommandQueue> commandQueue = nil;
    if (!commandQueue) {
        commandQueue = [device newCommandQueue];
    }

    // Acquire a drawable
    id<CAMetalDrawable> drawable = [layer nextDrawable];
    if (!drawable) return;

    id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
    if (!commandBuffer) return;

    //---------------------------------------------------
    // 3. Create a staging buffer from pBurnDraw
    //---------------------------------------------------
    // For a 2D image, total bytes = nBurnPitch * nBurnHeight
    size_t dataSize = nBurnPitch * nBurnHeight;
    id<MTLBuffer> stagingBuffer = [device newBufferWithBytes:pBurnDraw
                                                      length:dataSize
                                                     options:MTLResourceStorageModeShared];
    if (!stagingBuffer) return;

    //---------------------------------------------------
    // 4. Blit from CPU buffer -> drawable texture
    //---------------------------------------------------
    id<MTLBlitCommandEncoder> blit = [commandBuffer blitCommandEncoder];
    if (blit) {
        // We assume nBurnWidth, nBurnHeight is the real image size
        MTLSize copySize = MTLSizeMake(nBurnWidth, nBurnHeight, 1);

        [blit copyFromBuffer:stagingBuffer
                sourceOffset:0
           sourceBytesPerRow:nBurnPitch
         sourceBytesPerImage:dataSize   // for 2D only
                  sourceSize:copySize
                   toTexture:drawable.texture
            destinationSlice:0
            destinationLevel:0
           destinationOrigin:MTLOriginMake(0,0,0)];
        [blit endEncoding];
    }

    //---------------------------------------------------
    // 5. Present
    //---------------------------------------------------
    [commandBuffer presentDrawable:drawable];
    [commandBuffer commit];
}