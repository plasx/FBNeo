#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <QuartzCore/CAMetalLayer.h>

// Simple app delegate that displays a test pattern using Metal
@interface TestPatternDelegate : NSObject <NSApplicationDelegate, MTKViewDelegate>

@property (nonatomic, strong) id<MTLDevice> device;
@property (nonatomic, strong) id<MTLCommandQueue> commandQueue;
@property (nonatomic, strong) id<MTLBuffer> vertexBuffer;
@property (nonatomic, strong) id<MTLRenderPipelineState> pipelineState;
@property (nonatomic, strong) id<MTLTexture> frameTexture;
@property (nonatomic, strong) NSWindow *window;
@property (nonatomic, strong) MTKView *metalView;
@property (nonatomic, strong) NSTimer *timer;
@property (nonatomic) int frameCount;
@property (nonatomic) BOOL useAlternatePattern;

- (void)setupMetal;
- (void)createShaders;
- (void)createVertexBuffer;
- (void)createFrameTexture;
- (void)updateFrameTexture;
- (void)updateTestPattern;
- (void)createDebugTestPattern;

@end

@implementation TestPatternDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    // Setup Metal
    [self setupMetal];
    
    // Create window
    NSRect frame = NSMakeRect(0, 0, 640, 480);
    NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable;
    self.window = [[NSWindow alloc] initWithContentRect:frame
                                              styleMask:style
                                                backing:NSBackingStoreBuffered
                                                  defer:NO];
    self.window.title = @"Metal Test Pattern";
    
    // Create Metal view
    frame.origin = NSZeroPoint;
    self.metalView = [[MTKView alloc] initWithFrame:frame device:self.device];
    self.metalView.delegate = self;
    self.metalView.framebufferOnly = NO;
    self.metalView.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
    
    // Set as the content view
    self.window.contentView = self.metalView;
    
    // Center the window
    [self.window center];
    [self.window makeKeyAndOrderFront:nil];
    
    // Create the frame texture
    [self createFrameTexture];
    
    // Create the vertex buffer
    [self createVertexBuffer];
    
    // Create the shaders
    [self createShaders];
    
    // Add ability to toggle between patterns
    self.useAlternatePattern = YES;
    
    // Create a very visible test pattern for debugging
    [self createDebugTestPattern];
    
    // Setup timer for animation
    self.timer = [NSTimer scheduledTimerWithTimeInterval:1.0/60.0
                                                  target:self
                                                selector:@selector(updateTestPattern)
                                                userInfo:nil
                                                 repeats:YES];
    
    [NSApp activateIgnoringOtherApps:YES];
}

- (void)setupMetal {
    // Create Metal device
    self.device = MTLCreateSystemDefaultDevice();
    if (!self.device) {
        NSLog(@"Metal is not supported on this device");
        return;
    }
    
    NSLog(@"Metal device: %@", self.device.name);
    
    // Create command queue
    self.commandQueue = [self.device newCommandQueue];
}

- (void)createShaders {
    // Define the vertex and fragment shaders
    NSString *shaderSource = @"\
        #include <metal_stdlib>\n\
        using namespace metal;\n\
        \n\
        struct VertexIn {\n\
            float2 position [[attribute(0)]];\n\
            float2 texCoord [[attribute(1)]];\n\
        };\n\
        \n\
        struct VertexOut {\n\
            float4 position [[position]];\n\
            float2 texCoord;\n\
        };\n\
        \n\
        vertex VertexOut vertexShader(uint vertexID [[vertex_id]],\n\
                                   constant float4 *vertices [[buffer(0)]]) {\n\
            VertexOut out;\n\
            out.position = float4(vertices[vertexID].x, vertices[vertexID].y, 0.0, 1.0);\n\
            out.texCoord = float2(vertices[vertexID].z, vertices[vertexID].w);\n\
            return out;\n\
        }\n\
        \n\
        fragment float4 fragmentShader(VertexOut in [[stage_in]],\n\
                                      texture2d<float> texture [[texture(0)]]) {\n\
            constexpr sampler textureSampler(mag_filter::nearest, min_filter::nearest);\n\
            return texture.sample(textureSampler, in.texCoord);\n\
        }\n\
    ";
    
    // Compile shaders
    NSError *error = nil;
    id<MTLLibrary> library = [self.device newLibraryWithSource:shaderSource options:nil error:&error];
    if (!library) {
        NSLog(@"Failed to compile Metal shaders: %@", error);
        return;
    }
    
    id<MTLFunction> vertexFunction = [library newFunctionWithName:@"vertexShader"];
    id<MTLFunction> fragmentFunction = [library newFunctionWithName:@"fragmentShader"];
    
    // Create render pipeline
    MTLRenderPipelineDescriptor *pipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDescriptor.label = @"Test Pattern Pipeline";
    pipelineDescriptor.vertexFunction = vertexFunction;
    pipelineDescriptor.fragmentFunction = fragmentFunction;
    pipelineDescriptor.colorAttachments[0].pixelFormat = self.metalView.colorPixelFormat;
    
    self.pipelineState = [self.device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
    if (!self.pipelineState) {
        NSLog(@"Failed to create render pipeline state: %@", error);
    }
}

- (void)createVertexBuffer {
    // Create a fullscreen quad
    float quad[] = {
        -1.0, -1.0, 0.0, 1.0,  // bottom-left position, texcoord
         1.0, -1.0, 1.0, 1.0,  // bottom-right position, texcoord
        -1.0,  1.0, 0.0, 0.0,  // top-left position, texcoord
         1.0,  1.0, 1.0, 0.0,  // top-right position, texcoord
    };
    
    self.vertexBuffer = [self.device newBufferWithBytes:quad
                                                 length:sizeof(quad)
                                                options:MTLResourceStorageModeShared];
}

- (void)createFrameTexture {
    // Create a texture for the frame buffer
    MTLTextureDescriptor *textureDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                                                               width:320
                                                                                              height:240
                                                                                           mipmapped:NO];
    
    textureDescriptor.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
    textureDescriptor.storageMode = MTLStorageModeShared;
    
    self.frameTexture = [self.device newTextureWithDescriptor:textureDescriptor];
    self.frameCount = 0;
    
    // Draw the initial test pattern
    [self updateFrameTexture];
}

- (void)createDebugTestPattern {
    if (!self.frameTexture) {
        return;
    }
    
    const int width = self.frameTexture.width;
    const int height = self.frameTexture.height;
    
    // Create BGRA pixel data with bright colors
    uint32_t *pixels = (uint32_t *)malloc(width * height * sizeof(uint32_t));
    
    // Fill with a bright test pattern with text "TEST PATTERN"
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Divide the screen into 4 quadrants with different colors
            if (y < height/2) {
                if (x < width/2) {
                    // Top-left: Bright Green
                    pixels[y * width + x] = (0xFF << 24) | (0x00 << 16) | (0xFF << 8) | 0x00; // ARGB
                } else {
                    // Top-right: Bright Red
                    pixels[y * width + x] = (0xFF << 24) | (0xFF << 16) | (0x00 << 8) | 0x00; // ARGB
                }
            } else {
                if (x < width/2) {
                    // Bottom-left: Bright Blue
                    pixels[y * width + x] = (0xFF << 24) | (0x00 << 16) | (0x00 << 8) | 0xFF; // ARGB
                } else {
                    // Bottom-right: Bright Yellow
                    pixels[y * width + x] = (0xFF << 24) | (0xFF << 16) | (0xFF << 8) | 0x00; // ARGB
                }
            }
            
            // Add crossing white lines
            if (x == width/2 || y == height/2) {
                pixels[y * width + x] = (0xFF << 24) | (0xFF << 16) | (0xFF << 8) | 0xFF; // White
            }
            
            // Add border
            if (x < 2 || x >= width-2 || y < 2 || y >= height-2) {
                pixels[y * width + x] = (0xFF << 24) | (0xFF << 16) | (0xFF << 8) | 0xFF; // White
            }
        }
    }
    
    // Update the texture
    MTLRegion region = MTLRegionMake2D(0, 0, width, height);
    [self.frameTexture replaceRegion:region
                        mipmapLevel:0
                          withBytes:pixels
                        bytesPerRow:width * sizeof(uint32_t)];
    
    free(pixels);
}

- (void)updateTestPattern {
    self.frameCount++;
    
    if (self.useAlternatePattern) {
        [self createDebugTestPattern];
    } else {
        [self updateFrameTexture];
    }
    
    // Toggle between patterns every 120 frames (2 seconds at 60fps)
    if (self.frameCount % 120 == 0) {
        self.useAlternatePattern = !self.useAlternatePattern;
    }
    
    [self.metalView draw];
}

- (void)updateFrameTexture {
    if (!self.frameTexture) {
        return;
    }
    
    const int width = 320;
    const int height = 240;
    
    // Create BGRA pixel data
    uint32_t *pixels = (uint32_t *)malloc(width * height * sizeof(uint32_t));
    
    int tileSize = 32;
    int offset = (self.frameCount / 2) % tileSize; // Animate the pattern
    
    // Fill with a checkerboard pattern
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int tx = (x + offset) / tileSize;
            int ty = (y + offset) / tileSize;
            BOOL isWhite = ((tx + ty) % 2) == 0;
            
            // Create colors with shifting hues for animation
            if (isWhite) {
                int r = 180 + 75 * sin(self.frameCount * 0.02);
                int g = 180 + 75 * sin(self.frameCount * 0.01);
                int b = 180 + 75 * sin(self.frameCount * 0.03);
                pixels[y * width + x] = (0xFF << 24) | (r << 16) | (g << 8) | b; // ARGB
            } else {
                int r = 75 + 75 * cos(self.frameCount * 0.02);
                int g = 75 + 75 * cos(self.frameCount * 0.01);
                int b = 75 + 75 * cos(self.frameCount * 0.03);
                pixels[y * width + x] = (0xFF << 24) | (r << 16) | (g << 8) | b; // ARGB
            }
        }
    }
    
    // Update the texture
    MTLRegion region = MTLRegionMake2D(0, 0, width, height);
    [self.frameTexture replaceRegion:region
                        mipmapLevel:0
                          withBytes:pixels
                        bytesPerRow:width * sizeof(uint32_t)];
    
    free(pixels);
}

#pragma mark - MTKViewDelegate

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
    // Handle view resize here
}

- (void)drawInMTKView:(MTKView *)view {
    if (!self.frameTexture) {
        return;
    }
    
    // Get the drawable
    id<MTLDrawable> drawable = view.currentDrawable;
    if (!drawable) {
        return;
    }
    
    // Get the command buffer
    id<MTLCommandBuffer> commandBuffer = [self.commandQueue commandBuffer];
    
    // Get the render pass descriptor
    MTLRenderPassDescriptor *renderPassDescriptor = view.currentRenderPassDescriptor;
    if (renderPassDescriptor) {
        id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        
        // Set the pipeline state
        [encoder setRenderPipelineState:self.pipelineState];
        
        // Set the vertex buffer
        [encoder setVertexBuffer:self.vertexBuffer offset:0 atIndex:0];
        
        // Set the texture
        [encoder setFragmentTexture:self.frameTexture atIndex:0];
        
        // Draw the quad
        [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
        
        // End encoding
        [encoder endEncoding];
        
        // Present the drawable
        [commandBuffer presentDrawable:drawable];
    }
    
    // Commit the command buffer
    [commandBuffer commit];
}

- (void)applicationWillTerminate:(NSNotification *)notification {
    // Clean up resources
    [self.timer invalidate];
    self.timer = nil;
}

@end

// Main entry point
int main(int argc, const char * argv[]) {
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        TestPatternDelegate *delegate = [[TestPatternDelegate alloc] init];
        [app setDelegate:delegate];
        
        // Run the application
        [app run];
    }
    return 0;
} 