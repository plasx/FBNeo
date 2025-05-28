#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <simd/simd.h>

// Vertex data
typedef struct {
    vector_float2 position;
    vector_float2 texCoord;
} Vertex;

// Uniforms for shaders
typedef struct {
    float time;
    float aspectRatio;
    float padding[2]; // Padding for alignment
} Uniforms;

@interface MetalRenderer : NSObject
@property (nonatomic, strong) id<MTLDevice> device;
@property (nonatomic, strong) id<MTLCommandQueue> commandQueue;
@property (nonatomic, strong) id<MTLRenderPipelineState> pipelineState;
@property (nonatomic, strong) id<MTLBuffer> vertexBuffer;
@property (nonatomic, strong) id<MTLBuffer> uniformBuffer;
@property (nonatomic, strong) MTKView *mtkView;
@property (nonatomic, assign) float time;

- (instancetype)initWithView:(MTKView *)view;
- (void)render;
@end

@implementation MetalRenderer

- (instancetype)initWithView:(MTKView *)view {
    self = [super init];
    if (self) {
        _mtkView = view;
        _device = view.device;
        _time = 0.0;
        
        [self setupMetal];
        [self setupVertexBuffer];
        [self setupUniformBuffer];
        [self setupPipelineState];
    }
    return self;
}

- (void)setupMetal {
    _commandQueue = [_device newCommandQueue];
}

- (void)setupVertexBuffer {
    static const Vertex vertices[] = {
        // Triangle 1
        { {-1.0, -1.0}, {0.0, 1.0} },
        { { 1.0, -1.0}, {1.0, 1.0} },
        { { 1.0,  1.0}, {1.0, 0.0} },
        
        // Triangle 2
        { {-1.0, -1.0}, {0.0, 1.0} },
        { { 1.0,  1.0}, {1.0, 0.0} },
        { {-1.0,  1.0}, {0.0, 0.0} },
    };
    
    _vertexBuffer = [_device newBufferWithBytes:vertices
                                         length:sizeof(vertices)
                                        options:MTLResourceStorageModeShared];
}

- (void)setupUniformBuffer {
    _uniformBuffer = [_device newBufferWithLength:sizeof(Uniforms)
                                          options:MTLResourceStorageModeShared];
}

- (void)setupPipelineState {
    // Create shader library with inline Metal shader code
    NSString *shaderSource = @"#include <metal_stdlib>\n"
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
                              "struct Uniforms {\n"
                              "    float time;\n"
                              "    float aspectRatio;\n"
                              "    float2 padding;\n"
                              "};\n"
                              "\n"
                              "vertex VertexOut vertexShader(uint vertexID [[vertex_id]],\n"
                              "                              constant VertexIn *vertices [[buffer(0)]]) {\n"
                              "    VertexOut out;\n"
                              "    out.position = float4(vertices[vertexID].position, 0.0, 1.0);\n"
                              "    out.texCoord = vertices[vertexID].texCoord;\n"
                              "    return out;\n"
                              "}\n"
                              "\n"
                              "fragment float4 fragmentShader(VertexOut in [[stage_in]],\n"
                              "                              constant Uniforms &uniforms [[buffer(0)]]) {\n"
                              "    // Create a colorful animated gradient\n"
                              "    float2 uv = in.texCoord;\n"
                              "    float3 color;\n"
                              "    \n"
                              "    // Animated gradient\n"
                              "    color.r = 0.5 + 0.5 * sin(uv.x * 6.28 + uniforms.time * 0.5);\n"
                              "    color.g = 0.5 + 0.5 * sin(uv.y * 6.28 + uniforms.time * 0.7);\n"
                              "    color.b = 0.5 + 0.5 * sin((uv.x + uv.y) * 3.14 + uniforms.time);\n"
                              "    \n"
                              "    return float4(color, 1.0);\n"
                              "}\n";
    
    NSError *error = nil;
    id<MTLLibrary> library = [_device newLibraryWithSource:shaderSource options:nil error:&error];
    if (!library) {
        NSLog(@"Failed to create shader library: %@", error);
        return;
    }
    
    id<MTLFunction> vertexFunction = [library newFunctionWithName:@"vertexShader"];
    id<MTLFunction> fragmentFunction = [library newFunctionWithName:@"fragmentShader"];
    
    // Create render pipeline descriptor
    MTLRenderPipelineDescriptor *pipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDescriptor.vertexFunction = vertexFunction;
    pipelineDescriptor.fragmentFunction = fragmentFunction;
    pipelineDescriptor.colorAttachments[0].pixelFormat = _mtkView.colorPixelFormat;
    
    // Setup vertex descriptor
    MTLVertexDescriptor *vertexDescriptor = [MTLVertexDescriptor vertexDescriptor];
    
    // Position attribute
    vertexDescriptor.attributes[0].format = MTLVertexFormatFloat2;
    vertexDescriptor.attributes[0].offset = offsetof(Vertex, position);
    vertexDescriptor.attributes[0].bufferIndex = 0;
    
    // Texture coordinate attribute
    vertexDescriptor.attributes[1].format = MTLVertexFormatFloat2;
    vertexDescriptor.attributes[1].offset = offsetof(Vertex, texCoord);
    vertexDescriptor.attributes[1].bufferIndex = 0;
    
    // Single buffer layout
    vertexDescriptor.layouts[0].stride = sizeof(Vertex);
    vertexDescriptor.layouts[0].stepRate = 1;
    vertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
    
    pipelineDescriptor.vertexDescriptor = vertexDescriptor;
    
    // Create pipeline state
    _pipelineState = [_device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
    if (!_pipelineState) {
        NSLog(@"Failed to create pipeline state: %@", error);
    }
}

- (void)render {
    // Update time and uniforms
    _time += 0.016; // Approximate for 60 FPS
    
    Uniforms uniforms;
    uniforms.time = _time;
    uniforms.aspectRatio = (float)_mtkView.drawableSize.width / (float)_mtkView.drawableSize.height;
    
    // Copy uniforms data to the buffer
    memcpy([_uniformBuffer contents], &uniforms, sizeof(Uniforms));
    
    // Get the command buffer for this frame
    id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
    
    // If the drawable isn't available, skip rendering this frame
    if (!_mtkView.currentDrawable) {
        return;
    }
    
    // Create a render pass descriptor
    MTLRenderPassDescriptor *renderPassDescriptor = _mtkView.currentRenderPassDescriptor;
    if (renderPassDescriptor) {
        // Configure render encoder
        id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        [renderEncoder setRenderPipelineState:_pipelineState];
        
        // Set vertex buffer
        [renderEncoder setVertexBuffer:_vertexBuffer offset:0 atIndex:0];
        
        // Set fragment uniforms
        [renderEncoder setFragmentBuffer:_uniformBuffer offset:0 atIndex:0];
        
        // Draw triangles
        [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:6];
        
        // Finish encoding
        [renderEncoder endEncoding];
        
        // Present drawable
        [commandBuffer presentDrawable:_mtkView.currentDrawable];
    }
    
    // Commit command buffer
    [commandBuffer commit];
}

@end

@interface MetalViewDelegate : NSObject <MTKViewDelegate>
@property (nonatomic, strong) MetalRenderer *renderer;
@end

@implementation MetalViewDelegate

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
    // Handle window resize if needed
}

- (void)drawInMTKView:(MTKView *)view {
    [_renderer render];
}

@end

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (nonatomic, strong) NSWindow *window;
@property (nonatomic, strong) MTKView *metalView;
@property (nonatomic, strong) MetalRenderer *renderer;
@property (nonatomic, strong) MetalViewDelegate *viewDelegate;
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    // Create the window
    NSRect frame = NSMakeRect(0, 0, 800, 600);
    _window = [[NSWindow alloc] initWithContentRect:frame
                                          styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                                            backing:NSBackingStoreBuffered
                                              defer:NO];
    [_window setTitle:@"FBNeo Metal Demo"];
    [_window center];
    
    // Create Metal view
    _metalView = [[MTKView alloc] initWithFrame:frame device:MTLCreateSystemDefaultDevice()];
    if (!_metalView.device) {
        NSLog(@"Metal is not supported on this device");
        [NSApp terminate:nil];
        return;
    }
    
    // Configure Metal view
    _metalView.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    _metalView.clearColor = MTLClearColorMake(0.0, 0.0, 0.2, 1.0);
    _metalView.enableSetNeedsDisplay = NO;
    _metalView.paused = NO;
    _metalView.framebufferOnly = YES;
    
    // Create renderer and view delegate
    _renderer = [[MetalRenderer alloc] initWithView:_metalView];
    _viewDelegate = [[MetalViewDelegate alloc] init];
    _viewDelegate.renderer = _renderer;
    _metalView.delegate = _viewDelegate;
    
    // Set the view as the window's content view
    _window.contentView = _metalView;
    
    // Show the window
    [_window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

@end

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        NSLog(@"FBNeo Metal Demo - Simple Test App");
        NSLog(@"Metal Device: %@", MTLCreateSystemDefaultDevice().name);
        
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        
        AppDelegate *delegate = [[AppDelegate alloc] init];
        [NSApp setDelegate:delegate];
        
        [NSApp run];
    }
    return 0;
} 