#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <QuartzCore/CAMetalLayer.h>

// Simple Metal app that shows a test pattern
@interface TestPatternApp : NSObject <NSApplicationDelegate, MTKViewDelegate>

@property (nonatomic, strong) id<MTLDevice> device;
@property (nonatomic, strong) id<MTLCommandQueue> commandQueue;
@property (nonatomic, strong) id<MTLRenderPipelineState> pipelineState;
@property (nonatomic, strong) id<MTLTexture> texture;
@property (nonatomic, strong) NSWindow *window;
@property (nonatomic, strong) MTKView *metalView;
@property (nonatomic, assign) int frameCount;
@property (nonatomic, assign) BOOL useAnimation;

@end

@implementation TestPatternApp

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    // Create a Metal device
    self.device = MTLCreateSystemDefaultDevice();
    if (!self.device) {
        NSLog(@"Metal is not supported on this device");
        exit(1);
    }
    
    // Create a command queue
    self.commandQueue = [self.device newCommandQueue];
    
    // Create a window
    NSRect frame = NSMakeRect(0, 0, 800, 600);
    self.window = [[NSWindow alloc] initWithContentRect:frame
                                              styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                                                backing:NSBackingStoreBuffered
                                                  defer:NO];
    [self.window setTitle:@"FBNeo Metal Test Pattern"];
    [self.window center];
    [self.window makeKeyAndOrderFront:nil];
    
    // Create a Metal view
    self.metalView = [[MTKView alloc] initWithFrame:frame device:self.device];
    self.metalView.delegate = self;
    self.metalView.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
    
    // Set as the content view
    self.window.contentView = self.metalView;
    
    // Create texture for test pattern
    [self createTextureWithWidth:384 height:224];
    
    // Create pipeline state
    [self createRenderPipeline];
    
    // Generate the test pattern
    [self generateTestPattern];
    
    // Start animation timer
    self.useAnimation = YES;
    if (self.useAnimation) {
        [NSTimer scheduledTimerWithTimeInterval:1.0/60.0 
                                         target:self
                                       selector:@selector(updateAnimation:)
                                       userInfo:nil
                                        repeats:YES];
    }
}

- (void)updateAnimation:(NSTimer *)timer {
    self.frameCount++;
    [self.metalView setNeedsDisplay:YES];
}

- (void)createTextureWithWidth:(int)width height:(int)height {
    MTLTextureDescriptor *textureDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                                                                 width:width
                                                                                                height:height
                                                                                             mipmapped:NO];
    textureDescriptor.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
    self.texture = [self.device newTextureWithDescriptor:textureDescriptor];
}

- (void)createRenderPipeline {
    // Create a basic shader to display a textured quad
    id<MTLLibrary> defaultLibrary = [self.device newDefaultLibrary];
    
    // If the default library failed to load, we need to create it from source
    if (!defaultLibrary) {
        NSError *error = nil;
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
                                  "vertex VertexOut vertex_main(uint vertexID [[vertex_id]]) {\n"
                                  "    // Declare our quad positions\n"
                                  "    const float2 positions[6] = {\n"
                                  "        float2(-1.0, -1.0),\n"
                                  "        float2( 1.0, -1.0),\n"
                                  "        float2(-1.0,  1.0),\n"
                                  "        float2( 1.0, -1.0),\n"
                                  "        float2( 1.0,  1.0),\n"
                                  "        float2(-1.0,  1.0)\n"
                                  "    };\n"
                                  "    // Declare our texture coordinates\n"
                                  "    const float2 texCoords[6] = {\n"
                                  "        float2(0.0, 1.0),\n"
                                  "        float2(1.0, 1.0),\n"
                                  "        float2(0.0, 0.0),\n"
                                  "        float2(1.0, 1.0),\n"
                                  "        float2(1.0, 0.0),\n"
                                  "        float2(0.0, 0.0)\n"
                                  "    };\n"
                                  "    \n"
                                  "    VertexOut out;\n"
                                  "    out.position = float4(positions[vertexID], 0.0, 1.0);\n"
                                  "    out.texCoord = texCoords[vertexID];\n"
                                  "    return out;\n"
                                  "}\n"
                                  "\n"
                                  "fragment float4 fragment_main(VertexOut in [[stage_in]],\n"
                                  "                             texture2d<float> texture [[texture(0)]]) {\n"
                                  "    constexpr sampler textureSampler(mag_filter::nearest, min_filter::nearest);\n"
                                  "    return texture.sample(textureSampler, in.texCoord);\n"
                                  "}\n";
        
        defaultLibrary = [self.device newLibraryWithSource:shaderSource options:nil error:&error];
        if (!defaultLibrary) {
            NSLog(@"Failed to create Metal library: %@", error);
            exit(1);
        }
    }
    
    id<MTLFunction> vertexFunction = [defaultLibrary newFunctionWithName:@"vertex_main"];
    id<MTLFunction> fragmentFunction = [defaultLibrary newFunctionWithName:@"fragment_main"];
    
    if (!vertexFunction || !fragmentFunction) {
        NSLog(@"Failed to load shader functions from default library");
        exit(1);
    }
    
    // Create a render pipeline state
    MTLRenderPipelineDescriptor *pipelineStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineStateDescriptor.label = @"Texture Pipeline";
    pipelineStateDescriptor.vertexFunction = vertexFunction;
    pipelineStateDescriptor.fragmentFunction = fragmentFunction;
    pipelineStateDescriptor.colorAttachments[0].pixelFormat = self.metalView.colorPixelFormat;
    
    NSError *error = nil;
    self.pipelineState = [self.device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
    if (!self.pipelineState) {
        NSLog(@"Failed to create pipeline state: %@", error);
        exit(1);
    }
}

- (void)generateTestPattern {
    int width = (int)self.texture.width;
    int height = (int)self.texture.height;
    
    // Create a buffer to hold the test pattern
    uint32_t *pixels = (uint32_t*)malloc(width * height * sizeof(uint32_t));
    
    // Generate a test pattern
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint32_t color;
            
            // Create a colorful test pattern
            if (y < height/2) {
                if (x < width/2) {
                    // Top-left: Red
                    color = 0xFF0000FF; // BGRA format
                } else {
                    // Top-right: Green
                    color = 0xFF00FF00; // BGRA format
                }
            } else {
                if (x < width/2) {
                    // Bottom-left: Blue
                    color = 0xFFFF0000; // BGRA format
                } else {
                    // Bottom-right: Yellow
                    color = 0xFF00FFFF; // BGRA format
                }
            }
            
            // Add grid lines
            if (x % 32 == 0 || y % 32 == 0) {
                color = 0xFFFFFFFF; // White
            }
            
            pixels[y * width + x] = color;
        }
    }
    
    // Add a "TEST PATTERN" text (simplistic)
    int textX = width / 2 - 100;
    int textY = height / 2;
    
    // Draw a black background for the text
    for (int y = textY - 15; y < textY + 15; y++) {
        for (int x = textX - 5; x < textX + 210; x++) {
            if (x >= 0 && x < width && y >= 0 && y < height) {
                pixels[y * width + x] = 0xFF000000; // Black
            }
        }
    }
    
    // Very basic "TEST PATTERN" text - just for visibility
    // In a real app, you'd use proper text rendering
    
    // Update the texture with the test pattern
    MTLRegion region = MTLRegionMake2D(0, 0, width, height);
    [self.texture replaceRegion:region mipmapLevel:0 withBytes:pixels bytesPerRow:width * sizeof(uint32_t)];
    
    free(pixels);
}

#pragma mark - MTKViewDelegate

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
    // React to window size changes if needed
}

- (void)drawInMTKView:(MTKView *)view {
    // Create a command buffer for this frame
    id<MTLCommandBuffer> commandBuffer = [self.commandQueue commandBuffer];
    commandBuffer.label = @"Frame Command Buffer";
    
    // If we have a drawable (i.e., the view is visible and ready to display)
    if (view.currentDrawable) {
        // Create a render pass descriptor
        MTLRenderPassDescriptor *renderPassDescriptor = view.currentRenderPassDescriptor;
        if (renderPassDescriptor) {
            // Create an encoder for rendering
            id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
            renderEncoder.label = @"Render Encoder";
            
            // Set the render pipeline state
            [renderEncoder setRenderPipelineState:self.pipelineState];
            
            // Set the texture
            [renderEncoder setFragmentTexture:self.texture atIndex:0];
            
            // Draw the quad (6 vertices)
            [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:6];
            
            // Finish encoding
            [renderEncoder endEncoding];
            
            // Schedule presentation
            [commandBuffer presentDrawable:view.currentDrawable];
        }
    }
    
    // Commit the command buffer
    [commandBuffer commit];
}

@end

// Main entry point 
int main(int argc, const char * argv[]) {
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        TestPatternApp *delegate = [[TestPatternApp alloc] init];
        [app setDelegate:delegate];
        [app run];
    }
    return 0;
} 