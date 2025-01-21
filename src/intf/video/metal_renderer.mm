// src/intf/video/metal_renderer.mm

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>

static id<MTLDevice>               g_mtlDevice       = nil;
static id<MTLCommandQueue>         g_commandQueue    = nil;
static id<MTLRenderPipelineState>  g_pipelineState   = nil;
static id<MTLTexture>              g_srcTexture      = nil;
static id<MTLBuffer>               g_vertexBuffer    = nil;
static CAMetalLayer*               g_metalLayer      = nil;
static NSView*                     g_contentView     = nil;

struct SimpleVertex {
    float position[2];
    float texCoords[2];
};

static const SimpleVertex g_quadVertices[4] = {
    {{-1.0f, -1.0f}, {0.0f, 1.0f}},
    {{ 1.0f, -1.0f}, {1.0f, 1.0f}},
    {{-1.0f,  1.0f}, {0.0f, 0.0f}},
    {{ 1.0f,  1.0f}, {1.0f, 0.0f}},
};

bool MetalRendererInit(void* nsWindowPtr, int width, int height)
{
    @autoreleasepool {
        g_mtlDevice = MTLCreateSystemDefaultDevice();
        if (!g_mtlDevice) {
            NSLog(@"Metal is not supported on this system.");
            return false;
        }

        g_commandQueue = [g_mtlDevice newCommandQueue];
        if (!g_commandQueue) {
            NSLog(@"Failed to create command queue.");
            return false;
        }

        NSWindow* window = (__bridge NSWindow*)nsWindowPtr;
        g_contentView = [window contentView];

        g_metalLayer = [CAMetalLayer layer];
        g_metalLayer.device = g_mtlDevice;
        g_metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        g_metalLayer.framebufferOnly = YES;
        g_metalLayer.contentsScale = [[NSScreen mainScreen] backingScaleFactor];
        g_metalLayer.drawableSize = g_contentView.bounds.size;

        [g_contentView setWantsLayer:YES];
        [g_contentView setLayer:g_metalLayer];

        NSError* error = nil;
        id<MTLLibrary> defaultLibrary = [g_mtlDevice newDefaultLibrary];
        if (!defaultLibrary) {
            NSLog(@"Could not load default Metal library. Possibly no compiled .metal?");
            return false;
        }

        MTLRenderPipelineDescriptor* pipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
        pipelineDesc.vertexFunction   = [defaultLibrary newFunctionWithName:@"vertex_main"];
        pipelineDesc.fragmentFunction = [defaultLibrary newFunctionWithName:@"fragment_main"];
        pipelineDesc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;

        g_pipelineState = [g_mtlDevice newRenderPipelineStateWithDescriptor:pipelineDesc error:&error];
        if (!g_pipelineState) {
            NSLog(@"Failed to create pipeline state: %@", error);
            return false;
        }

        g_vertexBuffer = [g_mtlDevice newBufferWithBytes:g_quadVertices
                                                  length:sizeof(g_quadVertices)
                                                 options:MTLResourceStorageModeManaged];

        MTLTextureDescriptor* texDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                                           width:width
                                                                                          height:height
                                                                                       mipmapped:NO];
        g_srcTexture = [g_mtlDevice newTextureWithDescriptor:texDesc];
        if (!g_srcTexture) {
            NSLog(@"Failed to create source texture for emulator frames.");
            return false;
        }

        NSLog(@"MetalRendererInit: success");
        return true;
    }
}

void MetalRendererShutdown()
{
    g_srcTexture    = nil;
    g_vertexBuffer  = nil;
    g_pipelineState = nil;
    g_commandQueue  = nil;
    g_mtlDevice     = nil;
    g_metalLayer    = nil;
    g_contentView   = nil;
    NSLog(@"MetalRendererShutdown: done");
}

void MetalRendererDrawFrame(const void* frameBuffer, int width, int height)
{
    @autoreleasepool {
        if (g_contentView) {
            g_metalLayer.drawableSize = g_contentView.bounds.size;
        }

        if (g_srcTexture && frameBuffer) {
            MTLRegion region = { {0,0,0}, { (NSUInteger)width, (NSUInteger)height, 1 } };
            [g_srcTexture replaceRegion:region
                           mipmapLevel:0
                             withBytes:frameBuffer
                           bytesPerRow:(width * 4)];
        }

        id<CAMetalDrawable> drawable = [g_metalLayer nextDrawable];
        if (!drawable) {
            return;
        }

        MTLRenderPassDescriptor* passDesc = [MTLRenderPassDescriptor renderPassDescriptor];
        passDesc.colorAttachments[0].texture = drawable.texture;
        passDesc.colorAttachments[0].loadAction = MTLLoadActionClear;
        passDesc.colorAttachments[0].storeAction = MTLStoreActionStore;
        passDesc.colorAttachments[0].clearColor = MTLClearColorMake(0,0,0,1);

        id<MTLCommandBuffer> cmdBuf = [g_commandQueue commandBuffer];
        id<MTLRenderCommandEncoder> encoder = [cmdBuf renderCommandEncoderWithDescriptor:passDesc];

        [encoder setRenderPipelineState:g_pipelineState];
        [encoder setVertexBuffer:g_vertexBuffer offset:0 atIndex:0];
        [encoder setFragmentTexture:g_srcTexture atIndex:0];

        [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
        [encoder endEncoding];

        [cmdBuf presentDrawable:drawable];
        [cmdBuf commit];
    }
}