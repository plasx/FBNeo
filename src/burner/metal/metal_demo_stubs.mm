#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

// Simple typedefs for the demo
typedef unsigned char UINT8;
typedef int INT32;
typedef unsigned int UINT32;

// Test frame buffer for rendering
static uint32_t *testFrameBuffer = NULL;
static int testFrameWidth = 640;
static int testFrameHeight = 480;

// App delegate for the demo
@interface MetalDemoAppDelegate : NSObject <NSApplicationDelegate, MTKViewDelegate>

@property (nonatomic, strong) NSWindow *window;
@property (nonatomic, strong) MTKView *metalView;
@property (nonatomic, strong) id<MTLCommandQueue> commandQueue;
@property (nonatomic, strong) id<MTLBuffer> vertexBuffer;
@property (nonatomic, strong) id<MTLRenderPipelineState> pipelineState;
@property (nonatomic, strong) id<MTLTexture> texture;

- (void)setupMetal;
- (void)drawTestPattern;

@end

@implementation MetalDemoAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    // Create a window
    NSRect frame = NSMakeRect(0, 0, 800, 600);
    _window = [[NSWindow alloc] initWithContentRect:frame
                                          styleMask:NSWindowStyleMaskTitled |
                                                    NSWindowStyleMaskClosable |
                                                    NSWindowStyleMaskMiniaturizable |
                                                    NSWindowStyleMaskResizable
                                            backing:NSBackingStoreBuffered
                                              defer:NO];
    [_window setTitle:@"FBNeo Metal Demo"];
    [_window center];
    
    // Create a Metal view
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device) {
        NSLog(@"Metal is not supported on this device");
        [NSApp terminate:self];
        return;
    }
    
    _metalView = [[MTKView alloc] initWithFrame:_window.contentView.bounds device:device];
    _metalView.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
    _metalView.delegate = self;
    _metalView.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    _metalView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    
    [_window.contentView addSubview:_metalView];
    [_window makeKeyAndOrderFront:nil];
    
    // Setup Metal
    [self setupMetal];
    
    // Create test frame buffer
    [self drawTestPattern];
    
    // Add a label with the ROM path
    NSTextField *romLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(10, 10, 780, 30)];
    romLabel.bezeled = NO;
    romLabel.editable = NO;
    romLabel.selectable = YES;
    romLabel.drawsBackground = NO;
    [romLabel setStringValue:@"FBNeo Metal Demo - ROM loading demonstration"];
    [romLabel setTextColor:[NSColor whiteColor]];
    [_window.contentView addSubview:romLabel];
}

- (void)setupMetal {
    // Create command queue
    _commandQueue = [_metalView.device newCommandQueue];
    
    // Create vertex buffer for quad
    float quadVertices[] = {
        -1.0,  1.0, 0.0, 0.0,  // top left
        -1.0, -1.0, 0.0, 1.0,  // bottom left
         1.0, -1.0, 1.0, 1.0,  // bottom right
        
         1.0, -1.0, 1.0, 1.0,  // bottom right
         1.0,  1.0, 1.0, 0.0,  // top right
        -1.0,  1.0, 0.0, 0.0,  // top left
    };
    
    _vertexBuffer = [_metalView.device newBufferWithBytes:quadVertices
                                                  length:sizeof(quadVertices)
                                                 options:MTLResourceStorageModeShared];
    
    // Create texture
    MTLTextureDescriptor *textureDesc = [MTLTextureDescriptor
                                        texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                        width:testFrameWidth
                                        height:testFrameHeight
                                        mipmapped:NO];
    
    textureDesc.usage = MTLTextureUsageShaderRead;
    _texture = [_metalView.device newTextureWithDescriptor:textureDesc];
    
    // Load shader library
    NSError *error = nil;
    id<MTLLibrary> library = nil;
    
    // Try loading the compiled metallib first
    NSString *path = [[NSBundle mainBundle] pathForResource:@"default" ofType:@"metallib"];
    if (path) {
        NSURL *url = [NSURL fileURLWithPath:path];
        library = [_metalView.device newLibraryWithURL:url error:&error];
    }
    
    // If that fails, use hardcoded source
    if (!library) {
        NSString *source = @"#include <metal_stdlib>\n"
                          "using namespace metal;\n"
                          "\n"
                          "struct VertexIn {\n"
                          "    float2 position [[attribute(0)]];\n"
                          "    float2 texCoord [[attribute(1)]];\n"
                          "};\n"
                          "\n"
                          "struct VertexOut {\n"
                          "    float4 position [[position]];\n"
                          "    float2 texCoord;\n"
                          "};\n"
                          "\n"
                          "vertex VertexOut basic_vertex(uint vid [[vertex_id]],\n"
                          "                            constant float4* vertices [[buffer(0)]]) {\n"
                          "    VertexOut out;\n"
                          "    out.position = float4(vertices[vid].xy, 0.0, 1.0);\n"
                          "    out.texCoord = vertices[vid].zw;\n"
                          "    return out;\n"
                          "}\n"
                          "\n"
                          "fragment float4 basic_fragment(VertexOut in [[stage_in]],\n"
                          "                             texture2d<float> tex [[texture(0)]],\n"
                          "                             sampler samp [[sampler(0)]]) {\n"
                          "    return tex.sample(samp, in.texCoord);\n"
                          "}\n";
        
        library = [_metalView.device newLibraryWithSource:source options:nil error:&error];
    }
    
    if (!library) {
        NSLog(@"Failed to load Metal library: %@", error);
        return;
    }
    
    // Create pipeline
    MTLRenderPipelineDescriptor *pipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDesc.vertexFunction = [library newFunctionWithName:@"basic_vertex"];
    pipelineDesc.fragmentFunction = [library newFunctionWithName:@"basic_fragment"];
    pipelineDesc.colorAttachments[0].pixelFormat = _metalView.colorPixelFormat;
    
    _pipelineState = [_metalView.device newRenderPipelineStateWithDescriptor:pipelineDesc error:&error];
    if (!_pipelineState) {
        NSLog(@"Failed to create pipeline state: %@", error);
    }
}

- (void)drawTestPattern {
    // Create test frame buffer if needed
    if (!testFrameBuffer) {
        testFrameBuffer = (uint32_t*)malloc(testFrameWidth * testFrameHeight * sizeof(uint32_t));
    }
    
    // Draw a colorful test pattern
    for (int y = 0; y < testFrameHeight; y++) {
        for (int x = 0; x < testFrameWidth; x++) {
            uint8_t r = (uint8_t)((x * 255) / testFrameWidth);
            uint8_t g = (uint8_t)((y * 255) / testFrameHeight);
            uint8_t b = (uint8_t)(((x + y) * 127) / (testFrameWidth + testFrameHeight));
            
            // RGBA format for Metal
            testFrameBuffer[y * testFrameWidth + x] = (r << 24) | (g << 16) | (b << 8) | 0xFF;
        }
    }
    
    // Update the texture
    if (_texture) {
        MTLRegion region = MTLRegionMake2D(0, 0, testFrameWidth, testFrameHeight);
        [_texture replaceRegion:region
                    mipmapLevel:0
                      withBytes:testFrameBuffer
                    bytesPerRow:testFrameWidth * sizeof(uint32_t)];
    }
}

#pragma mark - MTKViewDelegate

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
    // Handle resize
}

- (void)drawInMTKView:(MTKView *)view {
    // Get a drawable
    id<CAMetalDrawable> drawable = view.currentDrawable;
    if (!drawable) return;
    
    // Create a render pass descriptor
    MTLRenderPassDescriptor *renderPassDesc = view.currentRenderPassDescriptor;
    if (!renderPassDesc) return;
    
    // Create a command buffer
    id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
    
    // Create a render command encoder
    id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDesc];
    
    // Set render state
    [encoder setRenderPipelineState:_pipelineState];
    [encoder setVertexBuffer:_vertexBuffer offset:0 atIndex:0];
    [encoder setFragmentTexture:_texture atIndex:0];
    
    // Create a sampler state
    MTLSamplerDescriptor *samplerDesc = [[MTLSamplerDescriptor alloc] init];
    samplerDesc.minFilter = MTLSamplerMinMagFilterLinear;
    samplerDesc.magFilter = MTLSamplerMinMagFilterLinear;
    id<MTLSamplerState> samplerState = [_metalView.device newSamplerStateWithDescriptor:samplerDesc];
    [encoder setFragmentSamplerState:samplerState atIndex:0];
    
    // Draw quad
    [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:6];
    [encoder endEncoding];
    
    // Present
    [commandBuffer presentDrawable:drawable];
    [commandBuffer commit];
}

- (void)applicationWillTerminate:(NSNotification *)notification {
    // Free resources
    if (testFrameBuffer) {
        free(testFrameBuffer);
        testFrameBuffer = NULL;
    }
}

@end

// Main entry point
int main(int argc, const char * argv[]) {
    @autoreleasepool {
        // Create application
        NSApplication *app = [NSApplication sharedApplication];
        MetalDemoAppDelegate *delegate = [[MetalDemoAppDelegate alloc] init];
        [app setDelegate:delegate];
        
        // Process command line arguments
        if (argc > 1) {
            const char *romPath = argv[1];
            NSLog(@"ROM path: %s (ignored in demo)", romPath);
        }
        
        // Run the application
        [app run];
    }
    return 0;
} 