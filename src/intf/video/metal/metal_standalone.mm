#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <Cocoa/Cocoa.h>

// Frame buffer properties for a typical CPS2 game (384x224)
static uint32_t* g_frameBuffer = NULL;
static int g_frameWidth = 384;
static int g_frameHeight = 224;
static bool g_frameUpdated = false;
static int g_frameCount = 0;

// Post-processing parameters
typedef struct {
    float scanlineIntensity;
    float scanlineCount;
    float curvature;
    float vignetteStrength;
    float screenSize[2];
    float textureSize[2];
} PostProcessParams;

static PostProcessParams g_postProcessParams = {
    .scanlineIntensity = 0.3f,
    .scanlineCount = 224.0f,
    .curvature = 0.1f,
    .vignetteStrength = 0.2f,
    .screenSize = {800.0f, 600.0f},
    .textureSize = {384.0f, 224.0f}
};

// Metal objects
static id<MTLDevice> g_device = nil;
static id<MTLCommandQueue> g_commandQueue = nil;
static id<MTLRenderPipelineState> g_pipelineState = nil;
static id<MTLRenderPipelineState> g_postProcessPipeline = nil;
static id<MTLBuffer> g_vertexBuffer = nil;
static id<MTLTexture> g_texture = nil;
static id<MTLSamplerState> g_samplerState = nil;
static id<MTLBuffer> g_uniformBuffer = nil;

// Forward declarations
@interface MetalView : MTKView
@property (strong) id<MTLRenderPipelineState> pipelineState;
@property (strong) id<MTLRenderPipelineState> postProcessPipeline;
@property (strong) id<MTLBuffer> vertexBuffer;
@property (strong) id<MTLTexture> texture;
@property (strong) id<MTLSamplerState> samplerState;
@property (assign) bool usePostProcessing;
@property (assign) int testMode;
@end

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (strong) NSWindow *window;
@property (strong) MetalView *metalView;
@end

// MetalView implementation
@implementation MetalView

- (instancetype)initWithFrame:(NSRect)frameRect device:(id<MTLDevice>)device {
    self = [super initWithFrame:frameRect device:device];
    if (self) {
        _testMode = 0;
        _usePostProcessing = true;
        [self setupMetal];
    }
    return self;
}

- (void)setupMetal {
    // Create command queue
    g_commandQueue = [self.device newCommandQueue];
    
    // Create vertex buffer with quad vertices
    float quadVertices[] = {
        // positions           // texture coordinates
        -1.0f, -1.0f, 0.0f, 1.0f,  0.0f, 1.0f, // bottom left
         1.0f, -1.0f, 0.0f, 1.0f,  1.0f, 1.0f, // bottom right
        -1.0f,  1.0f, 0.0f, 1.0f,  0.0f, 0.0f, // top left
         1.0f,  1.0f, 0.0f, 1.0f,  1.0f, 0.0f  // top right
    };
    
    g_vertexBuffer = [self.device newBufferWithBytes:quadVertices 
                                            length:sizeof(quadVertices) 
                                           options:MTLResourceStorageModeShared];
    self.vertexBuffer = g_vertexBuffer;
    
    // Create uniform buffer for post-processing parameters
    g_uniformBuffer = [self.device newBufferWithBytes:&g_postProcessParams
                                             length:sizeof(PostProcessParams)
                                            options:MTLResourceStorageModeShared];
    
    // Create initial texture
    [self createTexture];
    
    // Create sampler state
    MTLSamplerDescriptor *samplerDescriptor = [[MTLSamplerDescriptor alloc] init];
    samplerDescriptor.minFilter = MTLSamplerMinMagFilterLinear;
    samplerDescriptor.magFilter = MTLSamplerMinMagFilterLinear;
    samplerDescriptor.sAddressMode = MTLSamplerAddressModeClampToEdge;
    samplerDescriptor.tAddressMode = MTLSamplerAddressModeClampToEdge;
    
    g_samplerState = [self.device newSamplerStateWithDescriptor:samplerDescriptor];
    self.samplerState = g_samplerState;
    
    // Create shader library
    NSError *error = nil;
    id<MTLLibrary> library = nil;
    
    // Try to load compiled shader library file paths (try multiple locations)
    NSArray *libraryPaths = @[
        @"metal_standalone.metallib",
        @"build/metal/shaders/metal_standalone.metallib",
        @"fbneo_shaders.metallib",
        @"default.metallib",
        @"Shaders.metallib"
    ];
    
    for (NSString *path in libraryPaths) {
        if ([[NSFileManager defaultManager] fileExistsAtPath:path]) {
            NSURL *url = [NSURL fileURLWithPath:path];
            library = [self.device newLibraryWithURL:url error:&error];
            if (library) {
                NSLog(@"Loaded Metal shader library from file: %@", path);
                break;
            }
        }
    }
    
    // Fall back to inline shader source if library file not found
    if (!library) {
        NSLog(@"Creating shader library from embedded source");
        NSString *shaderSource = @"#include <metal_stdlib>\n"
                                "using namespace metal;\n"
                                "\n"
                                "struct VertexIn {\n"
                                "    float4 position [[attribute(0)]];\n"
                                "    float2 texCoord [[attribute(1)]];\n"
                                "};\n"
                                "\n"
                                "struct VertexOut {\n"
                                "    float4 position [[position]];\n"
                                "    float2 texCoord;\n"
                                "};\n"
                                "\n"
                                "struct PostProcessUniforms {\n"
                                "    float scanlineIntensity;\n"
                                "    float scanlineCount;\n"
                                "    float curvature;\n"
                                "    float vignetteStrength;\n"
                                "    float2 screenSize;\n"
                                "    float2 textureSize;\n"
                                "};\n"
                                "\n"
                                "vertex VertexOut vertex_main(uint vertexID [[vertex_id]],\n"
                                "                           constant float* vertices [[buffer(0)]]) {\n"
                                "    VertexOut out;\n"
                                "    out.position = float4(vertices[vertexID * 6], vertices[vertexID * 6 + 1], vertices[vertexID * 6 + 2], vertices[vertexID * 6 + 3]);\n"
                                "    out.texCoord = float2(vertices[vertexID * 6 + 4], vertices[vertexID * 6 + 5]);\n"
                                "    return out;\n"
                                "}\n"
                                "\n"
                                "fragment float4 fragment_main(VertexOut in [[stage_in]],\n"
                                "                            texture2d<float> texture [[texture(0)]],\n"
                                "                            sampler textureSampler [[sampler(0)]]) {\n"
                                "    return texture.sample(textureSampler, in.texCoord);\n"
                                "}\n"
                                "\n"
                                "fragment float4 crt_postprocess(VertexOut in [[stage_in]],\n"
                                "                             texture2d<float> texture [[texture(0)]],\n"
                                "                             sampler textureSampler [[sampler(0)]],\n"
                                "                             constant PostProcessUniforms &uniforms [[buffer(0)]]) {\n"
                                "    float2 centeredCoord = in.texCoord * 2.0 - 1.0;\n"
                                "    float distort = dot(centeredCoord, centeredCoord) * uniforms.curvature;\n"
                                "    centeredCoord = centeredCoord * (1.0 + distort);\n"
                                "    float2 finalCoord = (centeredCoord * 0.5 + 0.5);\n"
                                "    \n"
                                "    if (finalCoord.x < 0.0 || finalCoord.x > 1.0 || finalCoord.y < 0.0 || finalCoord.y > 1.0) {\n"
                                "        return float4(0.0, 0.0, 0.0, 1.0);\n"
                                "    }\n"
                                "    \n"
                                "    float4 baseColor = texture.sample(textureSampler, finalCoord);\n"
                                "    \n"
                                "    float scanlinePosition = finalCoord.y * uniforms.scanlineCount;\n"
                                "    float scanlineFactor = 1.0 - abs(sin(scanlinePosition * 3.14159)) * uniforms.scanlineIntensity;\n"
                                "    \n"
                                "    float vignette = 1.0 - length(centeredCoord * 0.5) * uniforms.vignetteStrength;\n"
                                "    vignette = clamp(vignette, 0.0, 1.0);\n"
                                "    \n"
                                "    return baseColor * scanlineFactor * vignette;\n"
                                "}\n";
        
        library = [self.device newLibraryWithSource:shaderSource options:nil error:&error];
        if (!library) {
            NSLog(@"Failed to create shader library from source: %@", error);
            return;
        }
    }
    
    // Get shader functions
    id<MTLFunction> vertexFunction = [library newFunctionWithName:@"vertex_main"];
    id<MTLFunction> fragmentFunction = [library newFunctionWithName:@"fragment_main"];
    id<MTLFunction> postProcessFunction = [library newFunctionWithName:@"crt_postprocess"];
    
    if (!vertexFunction || !fragmentFunction) {
        NSLog(@"Failed to get shader functions");
        return;
    }
    
    // Create basic render pipeline
    MTLRenderPipelineDescriptor *pipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDescriptor.vertexFunction = vertexFunction;
    pipelineDescriptor.fragmentFunction = fragmentFunction;
    pipelineDescriptor.colorAttachments[0].pixelFormat = self.colorPixelFormat;
    
    g_pipelineState = [self.device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
    if (!g_pipelineState) {
        NSLog(@"Failed to create pipeline state: %@", error);
        return;
    }
    self.pipelineState = g_pipelineState;
    
    // Create post-processing pipeline if function is available
    if (postProcessFunction) {
        pipelineDescriptor.fragmentFunction = postProcessFunction;
        g_postProcessPipeline = [self.device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
        if (!g_postProcessPipeline) {
            NSLog(@"Failed to create post-processing pipeline: %@", error);
        } else {
            self.postProcessPipeline = g_postProcessPipeline;
        }
    }
    
    NSLog(@"Metal setup complete (post-processing: %@)", self.usePostProcessing ? @"enabled" : @"disabled");
}

- (void)createTexture {
    if (g_frameWidth <= 0 || g_frameHeight <= 0) {
        NSLog(@"Invalid frame buffer dimensions: %dx%d", g_frameWidth, g_frameHeight);
        return;
    }
    
    MTLTextureDescriptor *textureDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                                                              width:g_frameWidth
                                                                                             height:g_frameHeight
                                                                                          mipmapped:NO];
    
    g_texture = [self.device newTextureWithDescriptor:textureDescriptor];
    self.texture = g_texture;
    
    // Create initial frame buffer if needed
    if (!g_frameBuffer) {
        g_frameBuffer = (uint32_t*)malloc(g_frameWidth * g_frameHeight * sizeof(uint32_t));
        if (g_frameBuffer) {
            // Fill with initial test pattern
            [self updateTestPattern];
            g_frameUpdated = true;
            NSLog(@"Created frame buffer: %dx%d", g_frameWidth, g_frameHeight);
        }
    }
}

- (void)updateTestPattern {
    if (!g_frameBuffer) return;
    
    // Choose pattern based on test mode
    switch (self.testMode) {
        case 0: { // Gradient pattern
            for (int y = 0; y < g_frameHeight; y++) {
                for (int x = 0; x < g_frameWidth; x++) {
                    uint8_t r = (uint8_t)((float)x / g_frameWidth * 255);
                    uint8_t g = (uint8_t)((float)y / g_frameHeight * 255);
                    uint8_t b = (uint8_t)((float)(x + y) / (g_frameWidth + g_frameHeight) * 255);
                    g_frameBuffer[y * g_frameWidth + x] = (255 << 24) | (r << 16) | (g << 8) | b;
                }
            }
            break;
        }
        case 1: { // Animated pattern
            float time = g_frameCount / 60.0f;
            for (int y = 0; y < g_frameHeight; y++) {
                for (int x = 0; x < g_frameWidth; x++) {
                    float r = (sin(x * 0.05f + time) * 0.5f + 0.5f) * 255;
                    float g = (cos(y * 0.05f + time * 0.7f) * 0.5f + 0.5f) * 255;
                    float b = (sin((x + y) * 0.05f + time * 1.3f) * 0.5f + 0.5f) * 255;
                    g_frameBuffer[y * g_frameWidth + x] = 0xFF000000 | ((uint8_t)r << 16) | ((uint8_t)g << 8) | (uint8_t)b;
                }
            }
            break;
        }
        case 2: { // Checkerboard
            for (int y = 0; y < g_frameHeight; y++) {
                for (int x = 0; x < g_frameWidth; x++) {
                    bool isWhite = ((x / 16) + (y / 16)) % 2 == 0;
                    g_frameBuffer[y * g_frameWidth + x] = isWhite ? 0xFFFFFFFF : 0xFF000000;
                }
            }
            break;
        }
        case 3: { // Grid
            for (int y = 0; y < g_frameHeight; y++) {
                for (int x = 0; x < g_frameWidth; x++) {
                    bool isGridLine = (x % 32 == 0) || (y % 32 == 0);
                    if (isGridLine) {
                        g_frameBuffer[y * g_frameWidth + x] = 0xFFFFFFFF; // White grid lines
                    } else {
                        // Color based on grid cell
                        int gridX = x / 32;
                        int gridY = y / 32;
                        
                        if ((gridX + gridY) % 2 == 0) {
                            g_frameBuffer[y * g_frameWidth + x] = 0xFF800040; // Purple
                        } else {
                            g_frameBuffer[y * g_frameWidth + x] = 0xFF408000; // Green
                        }
                    }
                }
            }
            break;
        }
        default:
            break;
    }
    
    // Draw frame counter text
    char frameText[32];
    snprintf(frameText, sizeof(frameText), "Frame: %d", g_frameCount);
    
    // Draw frame text
    int textX = 10;
    int textY = 10;
    for (int i = 0; frameText[i] != '\0'; i++) {
        char c = frameText[i];
        
        // Draw a simple 5x7 character
        for (int cy = 0; cy < 7; cy++) {
            for (int cx = 0; cx < 5; cx++) {
                int x = textX + i * 6 + cx;
                int y = textY + cy;
                
                if (x < 0 || x >= g_frameWidth || y < 0 || y >= g_frameHeight) {
                    continue;
                }
                
                // Simple font rendering
                bool isPixelOn = false;
                
                // Implement a basic font for numbers and a few characters
                switch (c) {
                    case 'F': isPixelOn = (cx == 0) || (cy == 0) || (cy == 3 && cx < 4); break;
                    case 'r': isPixelOn = (cx == 0) || (cy == 1 && cx < 3); break;
                    case 'a': isPixelOn = (cy == 3) || (cy > 3 && (cx == 0 || cx == 4)) || (cy == 0 && cx > 0 && cx < 4); break;
                    case 'm': isPixelOn = (cx == 0 || cx == 4) || (cy == 1 && cx > 0 && cx < 4); break;
                    case 'e': isPixelOn = (cy == 0 || cy == 3 || cy == 6 || (cx == 0 && cy > 0)); break;
                    case ':': isPixelOn = (cx == 2 && (cy == 1 || cy == 5)); break;
                    case ' ': isPixelOn = false; break;
                    default:
                        if (c >= '0' && c <= '9') {
                            // Simple digit rendering
                            isPixelOn = (cx == 0 || cx == 4) || (cy == 0 || cy == 6);
                            if (c == '0' || c == '4' || c == '8' || c == '9') isPixelOn |= (cy == 3);
                            if (c == '2') isPixelOn |= (cy == 3 || (cy < 3 && cx == 4) || (cy > 3 && cx == 0));
                            if (c == '5') isPixelOn |= (cy == 3 || (cy < 3 && cx == 0) || (cy > 3 && cx == 4));
                        }
                        break;
                }
                
                if (isPixelOn) {
                    g_frameBuffer[y * g_frameWidth + x] = 0xFFFFFFFF; // White text
                }
            }
        }
    }
    
    g_frameUpdated = true;
}

- (void)drawRect:(NSRect)dirtyRect {
    // Update texture if needed
    if (g_frameUpdated && g_frameBuffer && g_texture) {
        MTLRegion region = MTLRegionMake2D(0, 0, g_frameWidth, g_frameHeight);
        [g_texture replaceRegion:region
                    mipmapLevel:0
                      withBytes:g_frameBuffer
                    bytesPerRow:g_frameWidth * sizeof(uint32_t)];
        g_frameUpdated = false;
    }
    
    // Get next drawable
    id<CAMetalDrawable> drawable = [self currentDrawable];
    if (!drawable) {
        return;
    }
    
    // Create render pass descriptor
    MTLRenderPassDescriptor *renderPassDescriptor = [self currentRenderPassDescriptor];
    if (!renderPassDescriptor) {
        return;
    }
    
    // Create command buffer
    id<MTLCommandBuffer> commandBuffer = [g_commandQueue commandBuffer];
    
    // Create render encoder
    id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
    
    // Choose pipeline based on post-processing setting
    if (self.usePostProcessing && self.postProcessPipeline) {
        // Update uniform buffer with current parameters
        g_postProcessParams.screenSize[0] = self.drawableSize.width;
        g_postProcessParams.screenSize[1] = self.drawableSize.height;
        g_postProcessParams.textureSize[0] = g_frameWidth;
        g_postProcessParams.textureSize[1] = g_frameHeight;
        memcpy([g_uniformBuffer contents], &g_postProcessParams, sizeof(PostProcessParams));
        
        [renderEncoder setRenderPipelineState:self.postProcessPipeline];
        [renderEncoder setFragmentBuffer:g_uniformBuffer offset:0 atIndex:0];
    } else {
        [renderEncoder setRenderPipelineState:self.pipelineState];
    }
    
    // Set other resources
    [renderEncoder setVertexBuffer:g_vertexBuffer offset:0 atIndex:0];
    [renderEncoder setFragmentTexture:g_texture atIndex:0];
    [renderEncoder setFragmentSamplerState:g_samplerState atIndex:0];
    
    // Draw quad
    [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
    
    // End encoding
    [renderEncoder endEncoding];
    
    // Present drawable
    [commandBuffer presentDrawable:drawable];
    [commandBuffer commit];
}

@end

// App delegate implementation
@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    // Create Metal device
    g_device = MTLCreateSystemDefaultDevice();
    if (!g_device) {
        NSLog(@"Metal is not supported on this device");
        return;
    }
    
    // Create window
    NSRect frame = NSMakeRect(0, 0, 800, 600);
    self.window = [[NSWindow alloc] initWithContentRect:frame
                                             styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable
                                               backing:NSBackingStoreBuffered
                                                 defer:NO];
    
    [self.window setTitle:@"FBNeo Metal Renderer"];
    [self.window center];
    
    // Create Metal view
    self.metalView = [[MetalView alloc] initWithFrame:self.window.contentView.bounds device:g_device];
    self.metalView.clearColor = MTLClearColorMake(0.0, 0.0, 0.2, 1.0);
    self.metalView.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    self.metalView.depthStencilPixelFormat = MTLPixelFormatInvalid;
    self.metalView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    self.metalView.enableSetNeedsDisplay = YES;
    
    [self.window.contentView addSubview:self.metalView];
    
    // Create menus
    NSMenu *mainMenu = [[NSMenu alloc] init];
    
    // Application menu
    NSMenuItem *appMenuItem = [[NSMenuItem alloc] init];
    NSMenu *appMenu = [[NSMenu alloc] init];
    [appMenu addItemWithTitle:@"Quit" action:@selector(terminate:) keyEquivalent:@"q"];
    [appMenuItem setSubmenu:appMenu];
    [mainMenu addItem:appMenuItem];
    
    // View menu
    NSMenuItem *viewMenuItem = [[NSMenuItem alloc] init];
    viewMenuItem.title = @"View";
    NSMenu *viewMenu = [[NSMenu alloc] init];
    viewMenu.title = @"View";
    
    // Add toggle for post-processing
    NSMenuItem *togglePostProcessItem = [[NSMenuItem alloc] initWithTitle:@"Toggle Post-Processing" 
                                                                action:@selector(togglePostProcessing:)
                                                         keyEquivalent:@"p"];
    [viewMenu addItem:togglePostProcessItem];
    
    // Add test pattern selector submenu
    NSMenuItem *testPatternItem = [[NSMenuItem alloc] init];
    testPatternItem.title = @"Test Pattern";
    NSMenu *testPatternMenu = [[NSMenu alloc] init];
    
    [testPatternMenu addItemWithTitle:@"Gradient" action:@selector(selectTestPattern:) keyEquivalent:@"1"];
    [[testPatternMenu itemAtIndex:0] setTag:0];
    
    [testPatternMenu addItemWithTitle:@"Animated" action:@selector(selectTestPattern:) keyEquivalent:@"2"];
    [[testPatternMenu itemAtIndex:1] setTag:1];
    
    [testPatternMenu addItemWithTitle:@"Checkerboard" action:@selector(selectTestPattern:) keyEquivalent:@"3"];
    [[testPatternMenu itemAtIndex:2] setTag:2];
    
    [testPatternMenu addItemWithTitle:@"Grid" action:@selector(selectTestPattern:) keyEquivalent:@"4"];
    [[testPatternMenu itemAtIndex:3] setTag:3];
    
    testPatternItem.submenu = testPatternMenu;
    [viewMenu addItem:testPatternItem];
    
    [viewMenuItem setSubmenu:viewMenu];
    [mainMenu addItem:viewMenuItem];
    
    [NSApp setMainMenu:mainMenu];
    
    // Show window
    [self.window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
    
    // Set up animation timer
    [NSTimer scheduledTimerWithTimeInterval:1.0/60.0
                                    target:self
                                  selector:@selector(updateAnimation:)
                                  userInfo:nil
                                   repeats:YES];
}

- (void)togglePostProcessing:(id)sender {
    self.metalView.usePostProcessing = !self.metalView.usePostProcessing;
    NSLog(@"Post-processing: %@", self.metalView.usePostProcessing ? @"enabled" : @"disabled");
}

- (void)selectTestPattern:(id)sender {
    NSMenuItem *menuItem = (NSMenuItem *)sender;
    self.metalView.testMode = (int)menuItem.tag;
    NSLog(@"Selected test pattern: %d", self.metalView.testMode);
}

- (void)updateAnimation:(NSTimer *)timer {
    g_frameCount++;
    
    // Update frame buffer with selected test pattern
    [self.metalView updateTestPattern];
    
    // Make the view redraw
    [self.metalView setNeedsDisplay:YES];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

- (void)applicationWillTerminate:(NSNotification *)notification {
    // Clean up
    if (g_frameBuffer) {
        free(g_frameBuffer);
        g_frameBuffer = NULL;
    }
}

@end

// Main entry point
int main(int argc, const char * argv[]) {
    @autoreleasepool {
        NSLog(@"Starting FBNeo Metal Renderer");
        
        // Create application
        [NSApplication sharedApplication];
        
        // Create app delegate
        AppDelegate *delegate = [[AppDelegate alloc] init];
        [NSApp setDelegate:delegate];
        
        // Run the application
        [NSApp activateIgnoringOtherApps:YES];
        [NSApp run];
    }
    return 0;
} 