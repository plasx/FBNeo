#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <CoreVideo/CoreVideo.h>
#import <dlfcn.h>

// Frame buffer structure
typedef struct {
    uint8_t* data;
    int width;
    int height;
    int pitch;
    int format; // 0 = RGB565, 1 = RGBA8888
    bool updated;
} FrameBuffer;

// Create global frame buffer
static FrameBuffer gFrameBuffer = {0};

// Emulator frame buffer constants
#define EMULATOR_DEFAULT_WIDTH  384  // Default CPS2 game width
#define EMULATOR_DEFAULT_HEIGHT 224  // Default CPS2 game height

// Forward declaration for emulator frame buffer access functions
extern "C" {
    void* GetFrameBufferPtr();
    int GetFrameBufferWidth();
    int GetFrameBufferHeight();
    int GetFrameBufferPitch();
    bool IsFrameBufferUpdated();
    void SetFrameBufferUpdated(bool updated);
}

// Aspect ratio handling options
typedef enum {
    AspectRatioStretch,    // Stretch to fill window
    AspectRatioOriginal,   // Maintain original aspect ratio with letter/pillarboxing
    AspectRatioPixelPerfect // Integer scaling for pixel perfect display
} AspectRatioMode;

// Add shader effect options
typedef enum {
    ShaderModeNormal,     // Standard bilinear filtering
    ShaderModeCRT,        // CRT effect with curvature, scanlines, etc.
    ShaderModeScanlines,  // Simple scanlines effect
    ShaderModePixelPerfect // Pixel-perfect nearest neighbor filtering
} ShaderMode;

// Generate a test pattern for debugging
void GenerateTestPattern(FrameBuffer* fb, float time) {
    if (!fb || !fb->data) return;
    
    // Fill with a test pattern
    uint32_t* pixels = (uint32_t*)fb->data;
    for (int y = 0; y < fb->height; y++) {
        for (int x = 0; x < fb->width; x++) {
            float r = 0.5 + 0.5 * sin(x * 0.05 + time);
            float g = 0.5 + 0.5 * cos(y * 0.05 + time * 0.7);
            float b = 0.5 + 0.5 * sin((x + y) * 0.05 + time * 1.3);
            
            uint32_t pixel = (0xFF << 24) | // Alpha
                             ((uint8_t)(b * 255) << 16) | // Blue
                             ((uint8_t)(g * 255) << 8) |  // Green
                             ((uint8_t)(r * 255));        // Red
            
            pixels[y * fb->width + x] = pixel;
        }
    }
    
    fb->updated = true;
}

// Create and initialize frame buffer
bool InitFrameBuffer(FrameBuffer* fb, int width, int height) {
    if (!fb) return false;
    
    // Free existing buffer if any
    if (fb->data) {
        free(fb->data);
        fb->data = NULL;
    }
    
    // Allocate new buffer
    fb->width = width;
    fb->height = height;
    fb->pitch = width * 4; // RGBA8888 format
    fb->format = 1; // RGBA8888
    fb->data = (uint8_t*)malloc(width * height * 4);
    fb->updated = false;
    
    if (!fb->data) {
        NSLog(@"Failed to allocate frame buffer");
        return false;
    }
    
    // Clear buffer to black
    memset(fb->data, 0, width * height * 4);
    return true;
}

// Clean up frame buffer
void DestroyFrameBuffer(FrameBuffer* fb) {
    if (!fb) return;
    
    if (fb->data) {
        free(fb->data);
        fb->data = NULL;
    }
    
    fb->width = 0;
    fb->height = 0;
    fb->pitch = 0;
    fb->updated = false;
}

// Copy emulator frame buffer to our frame buffer for rendering
bool CopyEmulatorFrameBuffer(FrameBuffer* fb) {
    // Check if emulator frame buffer exists and has been updated
    void* emulatorFrameBuffer = GetFrameBufferPtr();
    if (!emulatorFrameBuffer || !fb || !fb->data) {
        return false;
    }
    
    // Get emulator frame buffer properties
    int width = GetFrameBufferWidth();
    int height = GetFrameBufferHeight();
    int pitch = GetFrameBufferPitch();
    
    // Check for valid dimensions
    if (width <= 0 || height <= 0 || pitch <= 0) {
        return false;
    }
    
    // Check if dimensions have changed
    if (fb->width != width || fb->height != height) {
        // Resize our frame buffer
        if (!InitFrameBuffer(fb, width, height)) {
            return false;
        }
        NSLog(@"Frame buffer resized to %dx%d", width, height);
    }
    
    // Copy emulator frame buffer data to our frame buffer
    memcpy(fb->data, emulatorFrameBuffer, height * pitch);
    fb->updated = true;
    
    // Mark emulator frame buffer as processed
    SetFrameBufferUpdated(false);
    
    return true;
}

@interface FBNeoMetalView : MTKView

@property (nonatomic, strong) id<MTLRenderPipelineState> pipelineState;
@property (nonatomic, strong) id<MTLCommandQueue> commandQueue;
@property (nonatomic, strong) id<MTLTexture> frameTexture;
@property (nonatomic, strong) id<MTLBuffer> vertexBuffer;
@property (nonatomic, assign) NSTimeInterval startTime;
@property (nonatomic, assign) BOOL useTestPattern;
@property (nonatomic, strong) NSTimer *animationTimer;
@property (nonatomic, assign) AspectRatioMode aspectRatioMode;
@property (nonatomic, assign) BOOL emulatorConnected;
@property (nonatomic, assign) float frameScale;
@property (nonatomic, assign) ShaderMode shaderMode;
@property (nonatomic, strong) id<MTLRenderPipelineState> normalPipelineState;
@property (nonatomic, strong) id<MTLRenderPipelineState> crtPipelineState;
@property (nonatomic, strong) id<MTLRenderPipelineState> scanlinePipelineState;
@property (nonatomic, strong) id<MTLRenderPipelineState> pixelPerfectPipelineState;
@property (nonatomic, strong) id<MTLBuffer> uniformBuffer;
@property (nonatomic, assign) float scanlineIntensity;
@property (nonatomic, assign) float crtCurvature;
@property (nonatomic, assign) float vignetteIntensity;
@property (nonatomic, assign) float noiseAmount;

@end

@implementation FBNeoMetalView

- (instancetype)initWithFrame:(NSRect)frameRect device:(id<MTLDevice>)device {
    self = [super initWithFrame:frameRect device:device];
    if (self) {
        [self setupMetal];
        self.startTime = [NSDate timeIntervalSinceReferenceDate];
        self.useTestPattern = YES; // Enable test pattern by default
        self.aspectRatioMode = AspectRatioOriginal; // Default to original aspect ratio
        self.emulatorConnected = NO; // Not connected to emulator by default
        self.frameScale = 1.0f; // Default scale factor
        self.shaderMode = ShaderModeNormal; // Default shader mode
        
        // Setup frame buffer with emulator dimensions or view dimensions if emulator not connected
        if (self.emulatorConnected) {
            int width = GetFrameBufferWidth();
            int height = GetFrameBufferHeight();
            
            if (width > 0 && height > 0) {
                InitFrameBuffer(&gFrameBuffer, width, height);
            } else {
                InitFrameBuffer(&gFrameBuffer, EMULATOR_DEFAULT_WIDTH, EMULATOR_DEFAULT_HEIGHT);
            }
        } else {
            InitFrameBuffer(&gFrameBuffer, frameRect.size.width, frameRect.size.height);
        }
        
        // Start animation timer
        self.animationTimer = [NSTimer scheduledTimerWithTimeInterval:1.0/60.0
                                                             target:self
                                                           selector:@selector(updateAnimation:)
                                                           userInfo:nil
                                                            repeats:YES];
    }
    return self;
}

- (void)dealloc {
    [self.animationTimer invalidate];
    DestroyFrameBuffer(&gFrameBuffer);
}

- (void)updateAnimation:(NSTimer *)timer {
    // Try to get frame buffer data from emulator
    BOOL emulatorFrameUpdated = NO;
    
    // Check if the emulator has an updated frame
    if (IsFrameBufferUpdated()) {
        // Copy from emulator frame buffer to our buffer
        emulatorFrameUpdated = CopyEmulatorFrameBuffer(&gFrameBuffer);
        self.emulatorConnected = emulatorFrameUpdated;
    }
    
    // If we're using test pattern or emulator frame wasn't updated, generate a test pattern
    if (self.useTestPattern && !emulatorFrameUpdated) {
        NSTimeInterval currentTime = [NSDate timeIntervalSinceReferenceDate];
        NSTimeInterval elapsed = currentTime - self.startTime;
        
        // Generate test pattern
        GenerateTestPattern(&gFrameBuffer, elapsed);
    }
    
    // Trigger redraw
    [self setNeedsDisplay:YES];
}

- (void)setupMetal {
    // Create command queue
    self.commandQueue = [self.device newCommandQueue];
    
    // Set pixel format
    self.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    
    // Set default shader parameters
    self.scanlineIntensity = 0.5f;
    self.crtCurvature = 0.1f;
    self.vignetteIntensity = 0.3f;
    self.noiseAmount = 0.05f;
    
    // Try to load shader file
    [self setupShaders];
    
    // Create vertex buffer for quad (will be updated with proper aspect ratio)
    [self updateVertexBufferForAspectRatio];
    
    // Create uniform buffer for shader parameters
    self.uniformBuffer = [self.device newBufferWithLength:sizeof(float) * 16 // Enough space for all uniform values
                                                options:MTLResourceStorageModeShared];
    
    // Create texture for frame buffer
    [self createFrameTexture];
}

- (void)setupShaders {
    NSError *error = nil;
    id<MTLLibrary> library = nil;
    
    // Try loading from metal_shaders.metal file
    if ([[NSFileManager defaultManager] fileExistsAtPath:@"metal_shaders.metal"]) {
        NSString *shaderSource = [NSString stringWithContentsOfFile:@"metal_shaders.metal" 
                                                          encoding:NSUTF8StringEncoding 
                                                             error:&error];
        if (shaderSource) {
            library = [self.device newLibraryWithSource:shaderSource options:nil error:&error];
            if (library) {
                NSLog(@"Successfully loaded shaders from metal_shaders.metal");
            }
        }
    }
    
    // If that fails, try to load from compiled metallib
    if (!library) {
        NSArray *metallibPaths = @[
            @"default.metallib",
            @"fbneo_shaders.metallib"
        ];
        
        for (NSString *path in metallibPaths) {
            if ([[NSFileManager defaultManager] fileExistsAtPath:path]) {
                library = [self.device newLibraryWithURL:[NSURL fileURLWithPath:path] error:&error];
                if (library) {
                    NSLog(@"Successfully loaded shaders from %@", path);
                    break;
                }
            }
        }
    }
    
    // If that also fails, use embedded shader
    if (!library) {
        // Define shader source inline
        NSString *shaderSource = @"#include <metal_stdlib>\n"
                                "using namespace metal;\n"
                                "\n"
                                "struct VertexOutput {\n"
                                "    float4 position [[position]];\n"
                                "    float2 texCoord;\n"
                                "};\n"
                                "\n"
                                "struct VertexInput {\n"
                                "    float2 position [[attribute(0)]];\n"
                                "    float2 texCoord [[attribute(1)]];\n"
                                "};\n"
                                "\n"
                                "struct ShaderUniforms {\n"
                                "    float2 screenSize;\n"
                                "    float time;\n"
                                "    float scanlineIntensity;\n"
                                "    float curvature;\n"
                                "    float vignetteIntensity;\n"
                                "    float noiseAmount;\n"
                                "    int shaderMode;\n"
                                "};\n"
                                "\n"
                                "vertex VertexOutput vertexShader(uint vertexID [[vertex_id]],\n"
                                "                              constant VertexInput* vertices [[buffer(0)]]) {\n"
                                "    VertexOutput out;\n"
                                "    out.position = float4(vertices[vertexID].position, 0.0, 1.0);\n"
                                "    out.texCoord = vertices[vertexID].texCoord;\n"
                                "    return out;\n"
                                "}\n"
                                "\n"
                                "fragment float4 fragmentShader(VertexOutput in [[stage_in]],\n"
                                "                            texture2d<float> texture [[texture(0)]]) {\n"
                                "    constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);\n"
                                "    return texture.sample(textureSampler, in.texCoord);\n"
                                "}\n"
                                "\n"
                                "fragment float4 pixelPerfectShader(VertexOutput in [[stage_in]],\n"
                                "                               texture2d<float> texture [[texture(0)]]) {\n"
                                "    constexpr sampler textureSampler(mag_filter::nearest, min_filter::nearest);\n"
                                "    return texture.sample(textureSampler, in.texCoord);\n"
                                "}\n";
        
        library = [self.device newLibraryWithSource:shaderSource options:nil error:&error];
        
        if (!library) {
            NSLog(@"Error creating shader library: %@", error);
            return;
        }
    }
    
    // Create pipeline states for different shader modes
    MTLRenderPipelineDescriptor *pipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDesc.colorAttachments[0].pixelFormat = self.colorPixelFormat;
    
    // Vertex function (same for all modes)
    id<MTLFunction> vertexFunction = [library newFunctionWithName:@"vertexShader"];
    if (!vertexFunction) {
        vertexFunction = [library newFunctionWithName:@"default_vertexShader"];
    }
    
    // Normal mode
    id<MTLFunction> normalFragmentFunction = [library newFunctionWithName:@"fragmentShader"];
    if (!normalFragmentFunction) {
        normalFragmentFunction = [library newFunctionWithName:@"default_fragmentShader"];
    }
    
    pipelineDesc.vertexFunction = vertexFunction;
    pipelineDesc.fragmentFunction = normalFragmentFunction;
    
    self.normalPipelineState = [self.device newRenderPipelineStateWithDescriptor:pipelineDesc error:&error];
    if (!self.normalPipelineState) {
        NSLog(@"Error creating normal pipeline state: %@", error);
    }
    
    // Default to normal pipeline initially
    self.pipelineState = self.normalPipelineState;
    
    // Pixel perfect mode
    id<MTLFunction> pixelPerfectFragmentFunction = [library newFunctionWithName:@"pixelPerfectShader"];
    if (pixelPerfectFragmentFunction) {
        pipelineDesc.fragmentFunction = pixelPerfectFragmentFunction;
        self.pixelPerfectPipelineState = [self.device newRenderPipelineStateWithDescriptor:pipelineDesc error:&error];
        if (!self.pixelPerfectPipelineState) {
            NSLog(@"Error creating pixel perfect pipeline state: %@", error);
        }
    }
    
    // Scanline mode
    id<MTLFunction> scanlineFragmentFunction = [library newFunctionWithName:@"scanlineShader"];
    if (scanlineFragmentFunction) {
        pipelineDesc.fragmentFunction = scanlineFragmentFunction;
        self.scanlinePipelineState = [self.device newRenderPipelineStateWithDescriptor:pipelineDesc error:&error];
        if (!self.scanlinePipelineState) {
            NSLog(@"Error creating scanline pipeline state: %@", error);
        }
    }
    
    // CRT mode
    id<MTLFunction> crtFragmentFunction = [library newFunctionWithName:@"crtShader"];
    if (crtFragmentFunction) {
        pipelineDesc.fragmentFunction = crtFragmentFunction;
        self.crtPipelineState = [self.device newRenderPipelineStateWithDescriptor:pipelineDesc error:&error];
        if (!self.crtPipelineState) {
            NSLog(@"Error creating CRT pipeline state: %@", error);
        }
    }
}

// Update vertex buffer for current aspect ratio mode
- (void)updateVertexBufferForAspectRatio {
    // Calculate aspect ratios
    float viewAspect = self.bounds.size.width / self.bounds.size.height;
    float gameAspect = (float)gFrameBuffer.width / (float)gFrameBuffer.height;
    
    // Scale factors
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    
    // Adjust based on aspect ratio mode
    switch (self.aspectRatioMode) {
        case AspectRatioStretch:
            // No adjustments needed - stretch to fill
            break;
            
        case AspectRatioOriginal:
            // Maintain original aspect ratio with letter/pillarboxing
            if (viewAspect > gameAspect) {
                // View is wider than game - add pillarboxing
                scaleX = gameAspect / viewAspect;
            } else {
                // View is taller than game - add letterboxing
                scaleY = viewAspect / gameAspect;
            }
            break;
            
        case AspectRatioPixelPerfect:
            // Integer scaling for pixel perfect display
            float scaleFactorX = floor(self.bounds.size.width / gFrameBuffer.width);
            float scaleFactorY = floor(self.bounds.size.height / gFrameBuffer.height);
            float scaleFactor = MIN(scaleFactorX, scaleFactorY);
            
            // Ensure we have at least 1x scaling
            scaleFactor = MAX(1.0f, scaleFactor);
            
            scaleX = (scaleFactor * gFrameBuffer.width) / self.bounds.size.width;
            scaleY = (scaleFactor * gFrameBuffer.height) / self.bounds.size.height;
            
            // Convert to normalized device coordinates (-1 to 1)
            scaleX = scaleX * 2.0f - 1.0f;
            scaleY = scaleY * 2.0f - 1.0f;
            break;
    }
    
    // Create quad vertices with adjusted aspect ratio
    typedef struct {
        float position[2];
        float texCoord[2];
    } Vertex;
    
    Vertex quadVertices[] = {
        // Position (x,y)     // Texture coord (u,v)
        {{-scaleX, -scaleY},  {0.0f, 1.0f}},  // Bottom left
        {{ scaleX, -scaleY},  {1.0f, 1.0f}},  // Bottom right
        {{-scaleX,  scaleY},  {0.0f, 0.0f}},  // Top left
        {{ scaleX,  scaleY},  {1.0f, 0.0f}},  // Top right
    };
    
    // Create or update vertex buffer
    if (self.vertexBuffer) {
        // If buffer already exists, update it
        memcpy([self.vertexBuffer contents], quadVertices, sizeof(quadVertices));
    } else {
        // Create new buffer
        self.vertexBuffer = [self.device newBufferWithBytes:quadVertices
                                                   length:sizeof(quadVertices)
                                                  options:MTLResourceStorageModeShared];
    }
}

- (void)createFrameTexture {
    if (gFrameBuffer.width <= 0 || gFrameBuffer.height <= 0) {
        NSLog(@"Invalid frame buffer dimensions");
        return;
    }
    
    MTLTextureDescriptor *textureDesc = [MTLTextureDescriptor 
                                         texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                         width:gFrameBuffer.width
                                         height:gFrameBuffer.height
                                         mipmapped:NO];
    
    textureDesc.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
    self.frameTexture = [self.device newTextureWithDescriptor:textureDesc];
    
    if (!self.frameTexture) {
        NSLog(@"Failed to create frame texture");
    }
}

- (void)updateFrameTexture {
    if (!gFrameBuffer.data || !gFrameBuffer.updated || !self.frameTexture) {
        return;
    }
    
    // Copy frame buffer data to texture
    MTLRegion region = MTLRegionMake2D(0, 0, gFrameBuffer.width, gFrameBuffer.height);
    [self.frameTexture replaceRegion:region
                         mipmapLevel:0
                           withBytes:gFrameBuffer.data
                         bytesPerRow:gFrameBuffer.pitch];
    
    gFrameBuffer.updated = false;
}

- (void)drawRect:(NSRect)dirtyRect {
    // Update texture with latest frame buffer data
    [self updateFrameTexture];
    
    // Select appropriate pipeline state based on current shader mode
    id<MTLRenderPipelineState> selectedPipeline = self.normalPipelineState;
    BOOL needsUniforms = NO;
    
    switch (self.shaderMode) {
        case ShaderModeNormal:
            selectedPipeline = self.normalPipelineState;
            break;
            
        case ShaderModeCRT:
            if (self.crtPipelineState) {
                selectedPipeline = self.crtPipelineState;
                needsUniforms = YES;
            }
            break;
            
        case ShaderModeScanlines:
            if (self.scanlinePipelineState) {
                selectedPipeline = self.scanlinePipelineState;
                needsUniforms = YES;
            }
            break;
            
        case ShaderModePixelPerfect:
            if (self.pixelPerfectPipelineState) {
                selectedPipeline = self.pixelPerfectPipelineState;
            }
            break;
    }
    
    // Update uniform buffer if needed
    if (needsUniforms) {
        [self updateUniformBuffer];
    }
    
    id<MTLCommandBuffer> commandBuffer = [self.commandQueue commandBuffer];
    MTLRenderPassDescriptor *renderPassDescriptor = self.currentRenderPassDescriptor;
    
    if (renderPassDescriptor && selectedPipeline) {
        id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        
        [renderEncoder setRenderPipelineState:selectedPipeline];
        [renderEncoder setVertexBuffer:self.vertexBuffer offset:0 atIndex:0];
        [renderEncoder setFragmentTexture:self.frameTexture atIndex:0];
        
        // Set uniform buffer if needed for this shader
        if (needsUniforms) {
            [renderEncoder setFragmentBuffer:self.uniformBuffer offset:0 atIndex:1];
        }
        
        // Draw quad as two triangles (4 vertices)
        [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
        [renderEncoder endEncoding];
        
        [commandBuffer presentDrawable:self.currentDrawable];
    }
    
    [commandBuffer commit];
}

- (void)updateUniformBuffer {
    if (!self.uniformBuffer) return;
    
    // Get current time for time-based effects
    NSTimeInterval currentTime = [NSDate timeIntervalSinceReferenceDate];
    NSTimeInterval elapsed = currentTime - self.startTime;
    
    // Uniform structure that matches ShaderUniforms in metal shader
    typedef struct {
        float screenSize[2];
        float time;
        float scanlineIntensity;
        float curvature;
        float vignetteIntensity;
        float noiseAmount;
        int shaderMode;
        float padding[1]; // For alignment
    } ShaderUniforms;
    
    // Set uniform values
    ShaderUniforms uniforms;
    uniforms.screenSize[0] = self.bounds.size.width;
    uniforms.screenSize[1] = self.bounds.size.height;
    uniforms.time = elapsed;
    uniforms.scanlineIntensity = self.scanlineIntensity;
    uniforms.curvature = self.crtCurvature;
    uniforms.vignetteIntensity = self.vignetteIntensity;
    uniforms.noiseAmount = self.noiseAmount;
    uniforms.shaderMode = (int)self.shaderMode;
    
    // Copy to uniform buffer
    memcpy([self.uniformBuffer contents], &uniforms, sizeof(uniforms));
}

// Method to set the shader mode
- (void)setShaderMode:(ShaderMode)mode {
    if (_shaderMode != mode) {
        _shaderMode = mode;
        [self setNeedsDisplay:YES];
    }
}

- (void)resizeWithOldSuperviewSize:(NSSize)oldSize {
    [super resizeWithOldSuperviewSize:oldSize];
    
    // Update vertex buffer for new size/aspect ratio
    [self updateVertexBufferForAspectRatio];
    
    // If we're not connected to the emulator, also update the frame buffer size
    if (!self.emulatorConnected) {
        NSSize newSize = self.bounds.size;
        
        // Update frame buffer
        if (newSize.width > 0 && newSize.height > 0) {
            InitFrameBuffer(&gFrameBuffer, newSize.width, newSize.height);
            [self createFrameTexture];
        }
    }
}

// Change aspect ratio mode
- (void)setAspectRatioMode:(AspectRatioMode)mode {
    if (_aspectRatioMode != mode) {
        _aspectRatioMode = mode;
        [self updateVertexBufferForAspectRatio];
        [self setNeedsDisplay:YES];
    }
}

// Connect to emulator frame buffer
- (void)connectToEmulator:(BOOL)connect {
    self.emulatorConnected = connect;
    self.useTestPattern = !connect;
    
    if (connect) {
        // Get emulator frame buffer dimensions
        int width = GetFrameBufferWidth();
        int height = GetFrameBufferHeight();
        
        if (width > 0 && height > 0) {
            // Initialize frame buffer with emulator dimensions
            InitFrameBuffer(&gFrameBuffer, width, height);
            [self createFrameTexture];
            [self updateVertexBufferForAspectRatio];
        }
    }
}

@end

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (nonatomic, strong) NSWindow *window;
@property (nonatomic, strong) FBNeoMetalView *metalView;
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    NSRect frame = NSMakeRect(0, 0, 800, 600);
    self.window = [[NSWindow alloc] initWithContentRect:frame
                                             styleMask:NSWindowStyleMaskTitled | 
                                                       NSWindowStyleMaskClosable | 
                                                       NSWindowStyleMaskMiniaturizable |
                                                       NSWindowStyleMaskResizable
                                               backing:NSBackingStoreBuffered
                                                 defer:NO];
    
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device) {
        NSLog(@"Metal is not supported on this device");
        return;
    }
    
    self.metalView = [[FBNeoMetalView alloc] initWithFrame:self.window.contentView.bounds device:device];
    self.metalView.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    self.metalView.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);  // Black background
    self.metalView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    
    // Enable variable refresh rate if available
    if (@available(macOS 10.15, *)) {
        self.metalView.preferredFramesPerSecond = 60;
    }
    
    // Try to connect to emulator
    [self.metalView connectToEmulator:YES];
    
    [self.window.contentView addSubview:self.metalView];
    
    self.window.title = @"FBNeo Metal Renderer";
    [self.window center];
    [self.window makeKeyAndOrderFront:nil];
    
    // Create menu for aspect ratio selection
    [self setupMenu];
}

- (void)setupMenu {
    // Create main menu
    NSMenu *mainMenu = [[NSMenu alloc] init];
    
    // Application menu
    NSMenuItem *appMenuItem = [[NSMenuItem alloc] init];
    NSMenu *appMenu = [[NSMenu alloc] initWithTitle:@"FBNeo"];
    [appMenu addItemWithTitle:@"Quit" action:@selector(terminate:) keyEquivalent:@"q"];
    [appMenuItem setSubmenu:appMenu];
    [mainMenu addItem:appMenuItem];
    
    // View menu
    NSMenuItem *viewMenuItem = [[NSMenuItem alloc] init];
    NSMenu *viewMenu = [[NSMenu alloc] initWithTitle:@"View"];
    
    // Aspect ratio submenu
    NSMenuItem *aspectMenuItem = [[NSMenuItem alloc] initWithTitle:@"Aspect Ratio" action:nil keyEquivalent:@""];
    NSMenu *aspectMenu = [[NSMenu alloc] init];
    
    [aspectMenu addItemWithTitle:@"Stretch" action:@selector(setAspectRatioStretch:) keyEquivalent:@"1"];
    [aspectMenu addItemWithTitle:@"Original" action:@selector(setAspectRatioOriginal:) keyEquivalent:@"2"];
    [aspectMenu addItemWithTitle:@"Pixel Perfect" action:@selector(setAspectRatioPixelPerfect:) keyEquivalent:@"3"];
    
    [aspectMenuItem setSubmenu:aspectMenu];
    [viewMenu addItem:aspectMenuItem];
    
    // Shader effects submenu
    NSMenuItem *shaderMenuItem = [[NSMenuItem alloc] initWithTitle:@"Shader Effects" action:nil keyEquivalent:@""];
    NSMenu *shaderMenu = [[NSMenu alloc] init];
    
    [shaderMenu addItemWithTitle:@"Normal" action:@selector(setShaderModeNormal:) keyEquivalent:@"0"];
    [shaderMenu addItemWithTitle:@"CRT Effect" action:@selector(setShaderModeCRT:) keyEquivalent:@"4"];
    [shaderMenu addItemWithTitle:@"Scanlines" action:@selector(setShaderModeScanlines:) keyEquivalent:@"5"];
    [shaderMenu addItemWithTitle:@"Pixel Perfect" action:@selector(setShaderModePixelPerfect:) keyEquivalent:@"6"];
    
    // Add separator
    [shaderMenu addItem:[NSMenuItem separatorItem]];
    
    // Add shader settings
    [shaderMenu addItemWithTitle:@"Adjust CRT Settings..." action:@selector(showShaderSettings:) keyEquivalent:@"7"];
    
    [shaderMenuItem setSubmenu:shaderMenu];
    [viewMenu addItem:shaderMenuItem];
    
    // Add separator
    [viewMenu addItem:[NSMenuItem separatorItem]];
    
    // Toggle test pattern
    [viewMenu addItemWithTitle:@"Toggle Test Pattern" action:@selector(toggleTestPattern:) keyEquivalent:@"t"];
    
    [viewMenuItem setSubmenu:viewMenu];
    [mainMenu addItem:viewMenuItem];
    
    // Set as application menu
    [NSApp setMainMenu:mainMenu];
}

- (void)setAspectRatioStretch:(id)sender {
    self.metalView.aspectRatioMode = AspectRatioStretch;
}

- (void)setAspectRatioOriginal:(id)sender {
    self.metalView.aspectRatioMode = AspectRatioOriginal;
}

- (void)setAspectRatioPixelPerfect:(id)sender {
    self.metalView.aspectRatioMode = AspectRatioPixelPerfect;
}

- (void)toggleTestPattern:(id)sender {
    self.metalView.useTestPattern = !self.metalView.useTestPattern;
}

- (void)setShaderModeNormal:(id)sender {
    self.metalView.shaderMode = ShaderModeNormal;
}

- (void)setShaderModeCRT:(id)sender {
    self.metalView.shaderMode = ShaderModeCRT;
}

- (void)setShaderModeScanlines:(id)sender {
    self.metalView.shaderMode = ShaderModeScanlines;
}

- (void)setShaderModePixelPerfect:(id)sender {
    self.metalView.shaderMode = ShaderModePixelPerfect;
}

- (void)showShaderSettings:(id)sender {
    // Create an alert with sliders for shader settings
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:@"CRT Shader Settings"];
    [alert setInformativeText:@"Adjust the settings for the CRT shader effect:"];
    [alert addButtonWithTitle:@"OK"];
    [alert addButtonWithTitle:@"Cancel"];
    
    // Create a custom view for the sliders
    NSView *accessoryView = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 300, 140)];
    
    // Scanline intensity slider
    NSSlider *scanlineSlider = [[NSSlider alloc] initWithFrame:NSMakeRect(100, 100, 180, 24)];
    [scanlineSlider setMinValue:0.0];
    [scanlineSlider setMaxValue:1.0];
    [scanlineSlider setDoubleValue:self.metalView.scanlineIntensity];
    NSTextField *scanlineLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(10, 100, 90, 24)];
    [scanlineLabel setStringValue:@"Scanlines:"];
    [scanlineLabel setBezeled:NO];
    [scanlineLabel setDrawsBackground:NO];
    [scanlineLabel setEditable:NO];
    [scanlineLabel setSelectable:NO];
    
    // Curvature slider
    NSSlider *curvatureSlider = [[NSSlider alloc] initWithFrame:NSMakeRect(100, 70, 180, 24)];
    [curvatureSlider setMinValue:0.0];
    [curvatureSlider setMaxValue:0.3];
    [curvatureSlider setDoubleValue:self.metalView.crtCurvature];
    NSTextField *curvatureLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(10, 70, 90, 24)];
    [curvatureLabel setStringValue:@"Curvature:"];
    [curvatureLabel setBezeled:NO];
    [curvatureLabel setDrawsBackground:NO];
    [curvatureLabel setEditable:NO];
    [curvatureLabel setSelectable:NO];
    
    // Vignette slider
    NSSlider *vignetteSlider = [[NSSlider alloc] initWithFrame:NSMakeRect(100, 40, 180, 24)];
    [vignetteSlider setMinValue:0.0];
    [vignetteSlider setMaxValue:1.0];
    [vignetteSlider setDoubleValue:self.metalView.vignetteIntensity];
    NSTextField *vignetteLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(10, 40, 90, 24)];
    [vignetteLabel setStringValue:@"Vignette:"];
    [vignetteLabel setBezeled:NO];
    [vignetteLabel setDrawsBackground:NO];
    [vignetteLabel setEditable:NO];
    [vignetteLabel setSelectable:NO];
    
    // Noise slider
    NSSlider *noiseSlider = [[NSSlider alloc] initWithFrame:NSMakeRect(100, 10, 180, 24)];
    [noiseSlider setMinValue:0.0];
    [noiseSlider setMaxValue:0.2];
    [noiseSlider setDoubleValue:self.metalView.noiseAmount];
    NSTextField *noiseLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(10, 10, 90, 24)];
    [noiseLabel setStringValue:@"Noise:"];
    [noiseLabel setBezeled:NO];
    [noiseLabel setDrawsBackground:NO];
    [noiseLabel setEditable:NO];
    [noiseLabel setSelectable:NO];
    
    // Add all controls to the view
    [accessoryView addSubview:scanlineSlider];
    [accessoryView addSubview:scanlineLabel];
    [accessoryView addSubview:curvatureSlider];
    [accessoryView addSubview:curvatureLabel];
    [accessoryView addSubview:vignetteSlider];
    [accessoryView addSubview:vignetteLabel];
    [accessoryView addSubview:noiseSlider];
    [accessoryView addSubview:noiseLabel];
    
    [alert setAccessoryView:accessoryView];
    
    // Show the alert and handle response
    NSModalResponse response = [alert runModal];
    if (response == NSAlertFirstButtonReturn) { // OK
        // Apply the new settings
        self.metalView.scanlineIntensity = scanlineSlider.doubleValue;
        self.metalView.crtCurvature = curvatureSlider.doubleValue;
        self.metalView.vignetteIntensity = vignetteSlider.doubleValue;
        self.metalView.noiseAmount = noiseSlider.doubleValue;
        
        // Force redraw with new settings
        [self.metalView setNeedsDisplay:YES];
    }
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

@end

// Stub implementations for emulator frame buffer access (replace with real implementations)
extern "C" {
    void* GetFrameBufferPtr() {
        // Check if there is a real emulator frame buffer available
        void* emulatorBufferPtr = NULL;
        
        // Try to dynamically resolve symbols from the main executable
        static void* (*realGetFrameBufferPtr)() = NULL;
        
        if (!realGetFrameBufferPtr) {
            realGetFrameBufferPtr = (void* (*)())dlsym(RTLD_DEFAULT, "GetFrameBufferPtr");
        }
        
        // Call the real function if available
        if (realGetFrameBufferPtr) {
            emulatorBufferPtr = realGetFrameBufferPtr();
        }
        
        return emulatorBufferPtr;
    }
    
    int GetFrameBufferWidth() {
        // Try to dynamically resolve symbols from the main executable
        static int (*realGetFrameBufferWidth)() = NULL;
        
        if (!realGetFrameBufferWidth) {
            realGetFrameBufferWidth = (int (*)())dlsym(RTLD_DEFAULT, "GetFrameBufferWidth");
        }
        
        // Call the real function if available
        if (realGetFrameBufferWidth) {
            return realGetFrameBufferWidth();
        }
        
        return EMULATOR_DEFAULT_WIDTH;
    }
    
    int GetFrameBufferHeight() {
        // Try to dynamically resolve symbols from the main executable
        static int (*realGetFrameBufferHeight)() = NULL;
        
        if (!realGetFrameBufferHeight) {
            realGetFrameBufferHeight = (int (*)())dlsym(RTLD_DEFAULT, "GetFrameBufferHeight");
        }
        
        // Call the real function if available
        if (realGetFrameBufferHeight) {
            return realGetFrameBufferHeight();
        }
        
        return EMULATOR_DEFAULT_HEIGHT;
    }
    
    int GetFrameBufferPitch() {
        // Try to dynamically resolve symbols from the main executable
        static int (*realGetFrameBufferPitch)() = NULL;
        
        if (!realGetFrameBufferPitch) {
            realGetFrameBufferPitch = (int (*)())dlsym(RTLD_DEFAULT, "GetFrameBufferPitch");
        }
        
        // Call the real function if available
        if (realGetFrameBufferPitch) {
            return realGetFrameBufferPitch();
        }
        
        return EMULATOR_DEFAULT_WIDTH * 4; // Assume RGBA8888
    }
    
    bool IsFrameBufferUpdated() {
        // Try to dynamically resolve symbols from the main executable
        static bool (*realIsFrameBufferUpdated)() = NULL;
        
        if (!realIsFrameBufferUpdated) {
            realIsFrameBufferUpdated = (bool (*)())dlsym(RTLD_DEFAULT, "IsFrameBufferUpdated");
        }
        
        // Call the real function if available
        if (realIsFrameBufferUpdated) {
            return realIsFrameBufferUpdated();
        }
        
        return false;
    }
    
    void SetFrameBufferUpdated(bool updated) {
        // Try to dynamically resolve symbols from the main executable
        static void (*realSetFrameBufferUpdated)(bool) = NULL;
        
        if (!realSetFrameBufferUpdated) {
            realSetFrameBufferUpdated = (void (*)(bool))dlsym(RTLD_DEFAULT, "SetFrameBufferUpdated");
        }
        
        // Call the real function if available
        if (realSetFrameBufferUpdated) {
            realSetFrameBufferUpdated(updated);
        }
    }
}

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        AppDelegate *delegate = [[AppDelegate alloc] init];
        app.delegate = delegate;
        [app run];
    }
    return 0;
} 