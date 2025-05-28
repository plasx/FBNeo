#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <simd/simd.h>

// Vertex data structure
typedef struct {
    vector_float2 position;
    vector_float4 color;
} Vertex;

@interface MetalRenderView : MTKView

@property (nonatomic, strong) id<MTLRenderPipelineState> pipelineState;
@property (nonatomic, strong) id<MTLCommandQueue> commandQueue;
@property (nonatomic, strong) id<MTLBuffer> vertexBuffer;
@property (nonatomic, assign) NSTimeInterval startTime;

@end

@implementation MetalRenderView

- (instancetype)initWithFrame:(NSRect)frameRect device:(id<MTLDevice>)device {
    self = [super initWithFrame:frameRect device:device];
    if (self) {
        [self commonInit];
    }
    return self;
}

- (void)commonInit {
    self.clearColor = MTLClearColorMake(0.0, 0.0, 0.1, 1.0);
    self.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    self.paused = NO;
    self.enableSetNeedsDisplay = NO;
    
    _startTime = [NSDate timeIntervalSinceReferenceDate];
    
    // Create the Metal command queue
    _commandQueue = [self.device newCommandQueue];
    
    // Create the vertex buffer
    [self setupVertexBuffer];
    
    // Create the render pipeline
    [self setupRenderPipeline];
}

- (void)setupVertexBuffer {
    // Create a square
    static const Vertex vertices[] = {
        // Triangle 1
        { {-0.5, -0.5}, {1.0, 0.0, 0.0, 1.0} }, // Bottom left, red
        { { 0.5, -0.5}, {0.0, 1.0, 0.0, 1.0} }, // Bottom right, green
        { { 0.5,  0.5}, {0.0, 0.0, 1.0, 1.0} }, // Top right, blue
        
        // Triangle 2
        { {-0.5, -0.5}, {1.0, 0.0, 0.0, 1.0} }, // Bottom left, red
        { { 0.5,  0.5}, {0.0, 0.0, 1.0, 1.0} }, // Top right, blue
        { {-0.5,  0.5}, {1.0, 0.0, 1.0, 1.0} }, // Top left, purple
    };
    
    // Create the vertex buffer from the vertices
    _vertexBuffer = [self.device newBufferWithBytes:vertices
                                             length:sizeof(vertices)
                                            options:MTLResourceStorageModeShared];
}

- (void)setupRenderPipeline {
    // Create a shader library
    NSString *shaderSource = @"#include <metal_stdlib>\n"
                             "using namespace metal;\n"
                             "\n"
                             "struct VertexIn {\n"
                             "    float2 position [[attribute(0)]];\n"
                             "    float4 color [[attribute(1)]];\n"
                             "};\n"
                             "\n"
                             "struct VertexOut {\n"
                             "    float4 position [[position]];\n"
                             "    float4 color;\n"
                             "};\n"
                             "\n"
                             "vertex VertexOut vertexShader(uint vertexID [[vertex_id]],\n"
                             "                              constant VertexIn *vertices [[buffer(0)]]) {\n"
                             "    VertexOut out;\n"
                             "    \n"
                             "    // Animate position based on vertexID (move in a circle)\n"
                             "    float2 position = vertices[vertexID].position;\n"
                             "    \n"
                             "    out.position = float4(position, 0.0, 1.0);\n"
                             "    out.color = vertices[vertexID].color;\n"
                             "    \n"
                             "    return out;\n"
                             "}\n"
                             "\n"
                             "fragment float4 fragmentShader(VertexOut in [[stage_in]]) {\n"
                             "    return in.color;\n"
                             "}\n";
    
    NSError *error = nil;
    id<MTLLibrary> library = [self.device newLibraryWithSource:shaderSource options:nil error:&error];
    if (!library) {
        NSLog(@"Failed to create shader library: %@", error);
        return;
    }
    
    id<MTLFunction> vertexFunction = [library newFunctionWithName:@"vertexShader"];
    id<MTLFunction> fragmentFunction = [library newFunctionWithName:@"fragmentShader"];
    
    // Create a render pipeline descriptor
    MTLRenderPipelineDescriptor *pipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDescriptor.vertexFunction = vertexFunction;
    pipelineDescriptor.fragmentFunction = fragmentFunction;
    pipelineDescriptor.colorAttachments[0].pixelFormat = self.colorPixelFormat;
    
    // Configure the vertex descriptor
    MTLVertexDescriptor *vertexDescriptor = [MTLVertexDescriptor vertexDescriptor];
    
    // Position attribute
    vertexDescriptor.attributes[0].format = MTLVertexFormatFloat2;
    vertexDescriptor.attributes[0].offset = offsetof(Vertex, position);
    vertexDescriptor.attributes[0].bufferIndex = 0;
    
    // Color attribute
    vertexDescriptor.attributes[1].format = MTLVertexFormatFloat4;
    vertexDescriptor.attributes[1].offset = offsetof(Vertex, color);
    vertexDescriptor.attributes[1].bufferIndex = 0;
    
    // Single buffer layout
    vertexDescriptor.layouts[0].stride = sizeof(Vertex);
    vertexDescriptor.layouts[0].stepRate = 1;
    vertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
    
    pipelineDescriptor.vertexDescriptor = vertexDescriptor;
    
    // Create the pipeline state
    _pipelineState = [self.device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
    if (!_pipelineState) {
        NSLog(@"Failed to create pipeline state: %@", error);
    }
}

- (void)updateAnimation {
    // Calculate elapsed time
    NSTimeInterval elapsedTime = [NSDate timeIntervalSinceReferenceDate] - _startTime;
    
    // Create an animated square by updating vertex positions
    Vertex *vertices = (Vertex*)_vertexBuffer.contents;
    
    // Update the positions for animation (rotate the square)
    float scale = 0.5 + 0.2 * sinf(elapsedTime);
    float angle = elapsedTime;
    
    // First triangle
    vertices[0].position = (vector_float2){-scale * cosf(angle), -scale * sinf(angle)};
    vertices[1].position = (vector_float2){ scale * cosf(angle), -scale * sinf(angle)};
    vertices[2].position = (vector_float2){ scale * cosf(angle),  scale * sinf(angle)};
    
    // Second triangle
    vertices[3].position = (vector_float2){-scale * cosf(angle), -scale * sinf(angle)};
    vertices[4].position = (vector_float2){ scale * cosf(angle),  scale * sinf(angle)};
    vertices[5].position = (vector_float2){-scale * cosf(angle),  scale * sinf(angle)};
    
    // Update colors based on time for a rainbow effect
    for (int i = 0; i < 6; i++) {
        float hue = fmodf(elapsedTime * 0.2 + i * 0.1, 1.0);
        float saturation = 0.8;
        float brightness = 1.0;
        
        // Convert HSV to RGB
        float c = brightness * saturation;
        float x = c * (1 - fabsf(fmodf(hue * 6, 2) - 1));
        float m = brightness - c;
        
        float r, g, b;
        if (hue < 1.0/6.0) { r = c; g = x; b = 0; }
        else if (hue < 2.0/6.0) { r = x; g = c; b = 0; }
        else if (hue < 3.0/6.0) { r = 0; g = c; b = x; }
        else if (hue < 4.0/6.0) { r = 0; g = x; b = c; }
        else if (hue < 5.0/6.0) { r = x; g = 0; b = c; }
        else { r = c; g = 0; b = x; }
        
        vertices[i].color = (vector_float4){r + m, g + m, b + m, 1.0};
    }
}

- (void)drawRect:(NSRect)dirtyRect {
    // Update animation
    [self updateAnimation];
    
    // Create a command buffer
    id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
    
    // Get the current drawable and render pass descriptor
    MTLRenderPassDescriptor *renderPassDescriptor = self.currentRenderPassDescriptor;
    if (renderPassDescriptor && self.currentDrawable) {
        // Create a render encoder
        id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        
        // Set the pipeline state
        [renderEncoder setRenderPipelineState:_pipelineState];
        
        // Set the vertex buffer
        [renderEncoder setVertexBuffer:_vertexBuffer offset:0 atIndex:0];
        
        // Draw triangles
        [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:6];
        
        // End encoding
        [renderEncoder endEncoding];
        
        // Present drawable
        [commandBuffer presentDrawable:self.currentDrawable];
    }
    
    // Commit the command buffer
    [commandBuffer commit];
}

@end

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (strong) NSWindow *window;
@property (strong) MetalRenderView *metalView;
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    // Create a Metal device
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device) {
        NSLog(@"Metal is not supported on this device");
        [NSApp terminate:nil];
        return;
    }
    
    // Create main window
    NSRect frame = NSMakeRect(0, 0, 800, 600);
    _window = [[NSWindow alloc] initWithContentRect:frame
                                          styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                                            backing:NSBackingStoreBuffered
                                              defer:NO];
    [_window setTitle:@"FBNeo Metal Render Test"];
    [_window center];
    
    // Create Metal view
    _metalView = [[MetalRenderView alloc] initWithFrame:frame device:device];
    _window.contentView = _metalView;
    
    // Show window
    [_window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
    
    NSLog(@"Metal device: %@", device.name);
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

@end

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        NSLog(@"FBNeo Metal Render Test");
        
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        
        AppDelegate *delegate = [[AppDelegate alloc] init];
        [NSApp setDelegate:delegate];
        
        [NSApp run];
    }
    return 0;
} 