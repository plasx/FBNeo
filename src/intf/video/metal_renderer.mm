/**
 * metal_renderer.mm
 *
 * Minimal demonstration of how to set up a CAMetalLayer, create a pipeline, and upload texture data.
 */
#import <AppKit/AppKit.h>
#include <Metal/Metal.h>
#include <QuartzCore/CAMetalLayer.h>
#include <Foundation/Foundation.h>
#include <vector>
#include <cstring>   // For memcpy, etc.

// Forward declarations
bool MetalRendererInit(void* nsWindowPtr, int width, int height);
void MetalRendererShutdown();
void MetalRendererDrawFrame(const void* frameBuffer, int width, int height);

// Some global or static variables to hold Metal objects
static id<MTLDevice> g_mtlDevice = nil;
static id<MTLCommandQueue> g_commandQueue = nil;
static id<MTLRenderPipelineState> g_pipelineState = nil;
static CAMetalLayer* g_metalLayer = nil;
static id<MTLTexture> g_srcTexture = nil;

// Store the NSView globally
static NSView* g_contentView = nil;

// You might have a small Vertex struct (position, texCoords)
struct SimpleVertex {
    float position[2];
    float texCoords[2];
};

// A simple fullscreen quad
static const SimpleVertex g_quadVertices[4] = {
    {{-1.0f, -1.0f}, {0.0f, 1.0f}},
    {{ 1.0f, -1.0f}, {1.0f, 1.0f}},
    {{-1.0f,  1.0f}, {0.0f, 0.0f}},
    {{ 1.0f,  1.0f}, {1.0f, 0.0f}}
};

// This buffer holds our vertex data
static id<MTLBuffer> g_vertexBuffer = nil;

bool MetalRendererInit(void* nsWindowPtr, int width, int height)
{
    @autoreleasepool {
        // 1. Create the MTLDevice
        g_mtlDevice = MTLCreateSystemDefaultDevice();
        if (!g_mtlDevice) {
            printf("Metal is not supported on this system.\n");
            return false;
        }

        // 2. Create the command queue
        g_commandQueue = [g_mtlDevice newCommandQueue];
        if (!g_commandQueue) {
            printf("Failed to create Metal command queue.\n");
            return false;
        }

        // 3. Create a CAMetalLayer and attach it to the NSWindow’s contentView
        NSWindow* window = (__bridge NSWindow*)nsWindowPtr; // Use (__bridge void*) for casting
        g_contentView = [window contentView]; // Store contentView globally

        g_metalLayer = [CAMetalLayer layer];
        g_metalLayer.device = g_mtlDevice;
        g_metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        g_metalLayer.framebufferOnly = YES;
        g_metalLayer.contentsScale = [[NSScreen mainScreen] backingScaleFactor];
        g_metalLayer.drawableSize = g_contentView.bounds.size;

        // Make sure contentView wants layer-backed rendering
        [g_contentView setWantsLayer:YES];
        [g_contentView setLayer:g_metalLayer];

        // 4. Load/Compile the Metal shader library
        NSError* error = nil;
        id<MTLLibrary> defaultLibrary = [g_mtlDevice newDefaultLibrary];
        if (!defaultLibrary) {
            printf("Could not load default Metal library.\n");
            return false;
        }

        // 5. Create a pipeline descriptor
        MTLRenderPipelineDescriptor* pipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
        pipelineDesc.vertexFunction   = [defaultLibrary newFunctionWithName:@"vertex_main"];
        pipelineDesc.fragmentFunction = [defaultLibrary newFunctionWithName:@"fragment_main"];
        pipelineDesc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;

        g_pipelineState = [g_mtlDevice newRenderPipelineStateWithDescriptor:pipelineDesc error:&error];
        if (!g_pipelineState) {
            NSLog(@"Error occurred when creating render pipeline state: %@", error);
            return false;
        }

        // 6. Create our vertex buffer
        g_vertexBuffer = [g_mtlDevice newBufferWithBytes:g_quadVertices
                                                  length:sizeof(g_quadVertices)
                                                 options:MTLResourceStorageModeManaged];
        if (!g_vertexBuffer) {
            printf("Failed to create vertex buffer.\n");
            return false;
        }

        // 7. Create a texture to hold the emulator’s frame data
        MTLTextureDescriptor* texDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                                           width:width
                                                                                          height:height
                                                                                       mipmapped:NO];
        if (!texDesc) {
            printf("Failed to create texture descriptor.\n");
            return false;
        }
        g_srcTexture = [g_mtlDevice newTextureWithDescriptor:texDesc];
        if (!g_srcTexture) {
            printf("Failed to create source texture.\n");
            return false;
        }

        printf("MetalRendererInit: success.\n");
        return true;
    }
}

void MetalRendererShutdown()
{
    // Clean up any allocated Metal resources here
    g_srcTexture    = nil;
    g_vertexBuffer  = nil;
    g_pipelineState = nil;
    g_commandQueue  = nil;
    g_mtlDevice     = nil;
    g_metalLayer    = nil;
    g_contentView   = nil;

    printf("MetalRendererShutdown: done.\n");
}

void MetalRendererDrawFrame(const void* frameBuffer, int width, int height)
{
    @autoreleasepool {
        if (!g_metalLayer) return;

        // 1. Update the size of the drawable if needed
        if (g_contentView) {
            g_metalLayer.drawableSize = g_contentView.bounds.size;
        }

        // 2. Update the emulator texture with new frame data
        if (g_srcTexture && frameBuffer) {
            MTLRegion region = {
                {0, 0, 0},
                {(NSUInteger)width, (NSUInteger)height, 1}
            };
            [g_srcTexture replaceRegion:region
                           mipmapLevel:0
                             withBytes:frameBuffer
                           bytesPerRow:width * 4];
        }

        // 3. Get a drawable from the layer
        id<CAMetalDrawable> drawable = [g_metalLayer nextDrawable];
        if (!drawable) {
            // Could happen if the layer is not ready
            return;
        }

        // 4. Create a render pass descriptor
        MTLRenderPassDescriptor* passDesc = [MTLRenderPassDescriptor renderPassDescriptor];
        passDesc.colorAttachments[0].texture = drawable.texture;
        passDesc.colorAttachments[0].loadAction = MTLLoadActionClear;
        passDesc.colorAttachments[0].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
        passDesc.colorAttachments[0].storeAction = MTLStoreActionStore;

        // 5. Create a command buffer + encoder
        id<MTLCommandBuffer> cmdBuf = [g_commandQueue commandBuffer];
        id<MTLRenderCommandEncoder> encoder = [cmdBuf renderCommandEncoderWithDescriptor:passDesc];

        // 6. Set pipeline, buffers, texture
        [encoder setRenderPipelineState:g_pipelineState];
        [encoder setVertexBuffer:g_vertexBuffer offset:0 atIndex:0];
        [encoder setFragmentTexture:g_srcTexture atIndex:0];

        // 7. Draw our fullscreen quad (4 vertices, 2 triangles)
        [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];

        [encoder endEncoding];

        // 8. Present and commit
        [cmdBuf presentDrawable:drawable];
        [cmdBuf commit];
    }
}