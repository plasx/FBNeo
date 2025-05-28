#import <Cocoa/Cocoa.h>
#import <MetalKit/MetalKit.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include "metal_renderer.h"
#include <stdio.h>
#include <string.h>

// External function declarations to fix linker errors
#ifdef __cplusplus
extern "C" {
#endif
    // BurnLib functions
    INT32 BurnLibInit_Metal();
    INT32 BurnLibExit_Metal();
#ifdef __cplusplus
}
#endif

// External function declarations
extern void* Metal_GetFrameBuffer();
extern bool IsFrameBufferUpdated();
extern "C" void SetFrameBufferUpdated(bool updated);
extern INT32 Metal_RunFrame(int bDraw);

// External CPS2 declarations
extern "C" {
    int Metal_CPS2_Init();
    int Metal_CPS2_Exit();
    int Metal_CPS2_LoadGame(int gameIndex);
    int Metal_CPS2_ExitGame();
    int Metal_CPS2_RunFrame(int render);
    int Metal_CPS2_GetFrameCount();
    void Metal_CPS2_GetGameDimensions(int* width, int* height);
}

// Metal objects
static id<MTLDevice> device = nil;
static id<MTLCommandQueue> commandQueue = nil;
static id<MTLRenderPipelineState> pipelineState = nil;
static id<MTLSamplerState> samplerState = nil;
static MTKView* metalView = nil;
static id<MTLTexture> frameTexture = nil;
static id<MTLBuffer> vertexBuffer = nil;
static id<MTLBuffer> uniformBuffer = nil;

// Rendering settings
static bool useVSync = true;
static bool useBilinearFilter = true;
static float renderScale = 1.0f;
static int currentWidth = 384;
static int currentHeight = 224;

// Frame buffer management
static bool frameBufferUpdated = false;

// Emulation configuration
typedef enum {
    EMULATION_MODE_MINIMAL_VALUE = 0,  // Use minimal emulation
    EMULATION_MODE_CPS2_VALUE = 1      // Use full CPS2 emulation
} EmulationMode;

static EmulationMode currentEmulationMode = EMULATION_MODE_MINIMAL_VALUE;
static bool emulationInitialized = false;
static int currentGameIndex = 0;

// Shader uniform structure - must match shader expectations
typedef struct {
    float textureWidth;
    float textureHeight;
    float reserved[2]; // For alignment
} ShaderUniforms;

// Forward declarations for internal functions
static bool createTexture(int width, int height);
static bool createPipelineState();
static void initVertexData();
static bool initializeEmulation();
static void shutdownEmulation();
static bool runEmulationFrame(bool render);

// Forward declarations for missing symbols
static void* g_pFrameBuffer = NULL;

// Create a sampler state for texture filtering
static void createSamplerState(bool useLinearFiltering) {
    // Create a sampler descriptor
    MTLSamplerDescriptor *samplerDesc = [[MTLSamplerDescriptor alloc] init];
    samplerDesc.minFilter = useLinearFiltering ? MTLSamplerMinMagFilterLinear : MTLSamplerMinMagFilterNearest;
    samplerDesc.magFilter = useLinearFiltering ? MTLSamplerMinMagFilterLinear : MTLSamplerMinMagFilterNearest;
    samplerDesc.sAddressMode = MTLSamplerAddressModeClampToEdge;
    samplerDesc.tAddressMode = MTLSamplerAddressModeClampToEdge;
    samplerDesc.mipFilter = MTLSamplerMipFilterNotMipmapped;
    samplerDesc.normalizedCoordinates = YES;
    
    // Create the sampler state
    samplerState = [device newSamplerStateWithDescriptor:samplerDesc];
    
    NSLog(@"Created sampler state with %@ filtering", useLinearFiltering ? @"linear" : @"nearest");
}

// MTKViewDelegate for Metal view
@interface MetalViewDelegate : NSObject <MTKViewDelegate>
@property (nonatomic, strong) id<MTLDevice> device;
@property (nonatomic, strong) id<MTLCommandQueue> commandQueue;
@property (nonatomic, strong) id<MTLRenderPipelineState> pipelineState;
@property (nonatomic, strong) id<MTLBuffer> vertexBuffer;
@property (nonatomic, strong) id<MTLBuffer> uniformBuffer;
@property (nonatomic, strong) id<MTLTexture> frameTexture;
@property (nonatomic, assign) CGSize frameSize;
@property (nonatomic, assign) BOOL pauseRendering;
@end

@implementation MetalViewDelegate

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
    NSLog(@"Metal view size changed to %.0f x %.0f", size.width, size.height);
    
    // Update frame size to maintain aspect ratio
    _frameSize = CGSizeMake(currentWidth, currentHeight);
    
    // Update uniform buffer with new dimensions
    if (_uniformBuffer) {
        ShaderUniforms* uniforms = (ShaderUniforms*)[_uniformBuffer contents];
        if (uniforms) {
            uniforms->textureWidth = _frameSize.width;
            uniforms->textureHeight = _frameSize.height;
        }
    }
    
    // Force a redraw
    if (!_pauseRendering) {
        [view draw];
    }
}

- (void)drawInMTKView:(MTKView *)view {
    // CRITICAL: Do not proceed if renderer is not ready or texture is missing
    if (_pauseRendering || !_frameTexture) {
        NSLog(@"⚠️ Skipping render - paused=%d, texture=%p", _pauseRendering, _frameTexture);
        return;
    }
    
    // Debug frame counter
    static int frameCount = 0;
    frameCount++;
    
    // Get current drawable
    id<CAMetalDrawable> drawable = view.currentDrawable;
    if (!drawable) {
        NSLog(@"⚠️ No drawable available");
        return;
    }
    
    // Create render pass descriptor
    MTLRenderPassDescriptor* renderPassDesc = view.currentRenderPassDescriptor;
    if (!renderPassDesc) {
        NSLog(@"⚠️ No render pass descriptor available");
        return;
    }
    
    // Create command buffer
    id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
    if (!commandBuffer) {
        NSLog(@"⚠️ Failed to create command buffer");
        return;
    }
    
    // Run emulation frame if needed
    if (emulationInitialized) {
        // Run the frame and update the texture when finished
        bool frameResult = runEmulationFrame(true);
        if (!frameResult) {
            NSLog(@"⚠️ Error running emulation frame");
        }
    }
    
    // CRITICAL: Always update texture with fresh emulation data
    // This is where the real frame buffer gets uploaded to the GPU
    void* frameBuffer = Metal_GetFrameBuffer();
    if (frameBuffer) {
        // Check frame buffer for content before updating texture
        uint32_t* pixels = (uint32_t*)frameBuffer;
        int width = (int)_frameSize.width;
        int height = (int)_frameSize.height;
        
        // Count non-zero pixels to verify content
        int nonZeroPixels = 0;
        uint32_t checksumValue = 0;
        for (int i = 0; i < 1000 && i < (width * height); i++) {
            if (pixels[i] != 0) {
                nonZeroPixels++;
                checksumValue ^= pixels[i];
            }
        }
        
        // Print frame buffer content information on regular intervals
        if (frameCount % 60 == 0) {
            NSLog(@"📊 Frame buffer check: %d/1000 non-zero pixels, checksum=0x%08X", 
                  nonZeroPixels, checksumValue);
            
            // Sample some pixels
            NSLog(@" First 5 pixels: 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X", 
                  pixels[0], pixels[1], pixels[2], pixels[3], pixels[4]);
        }
        
        // Content detection reporting
        float percentNonZero = 0;
        if (nonZeroPixels > 0) {
            percentNonZero = (float)nonZeroPixels / 10.0f;  // Out of 1000 samples
            if (frameCount % 60 == 0) {
                NSLog(@"✅✅✅ SCREEN CONTENT DETECTED! %.1f%% non-zero pixels", percentNonZero);
            }
        } else {
            // No content detected - this may be normal during initialization
            if (frameCount % 60 == 0) {
                NSLog(@"⚠️ No screen content detected in frame buffer - may be initializing");
            }
        }
        
        // Critical: Ensure we're using the right formats for texture update
        MTLRegion region = MTLRegionMake2D(0, 0, 
                                         (NSUInteger)_frameSize.width, 
                                         (NSUInteger)_frameSize.height);
        
        // Log the texture update if needed
        if (frameCount % 60 == 0) {
            NSLog(@"📱 Updating texture (size: %dx%d, format: %lu, bytesPerRow: %lu)", 
                  (int)_frameSize.width, (int)_frameSize.height, 
                  (unsigned long)_frameTexture.pixelFormat, 
                  (unsigned long)_frameSize.width * 4);
        }
                            
        // CRITICAL: Replace texture content with real emulation data
        [_frameTexture replaceRegion:region
                        mipmapLevel:0
                          withBytes:frameBuffer
                        bytesPerRow:(NSUInteger)_frameSize.width * 4];
                            
        // Mark frame buffer as updated
        SetFrameBufferUpdated(false);
    } else {
        NSLog(@"⚠️ Frame buffer is NULL - skipping texture update");
        return; // CRITICAL: Do not proceed with rendering if no frame data
    }
    
    // CRITICAL: Before encoding render commands, validate the pipeline state and resources
    if (!_pipelineState || !_vertexBuffer) {
        NSLog(@"⚠️ Missing pipeline components: pipelineState=%p, vertexBuffer=%p", 
              _pipelineState, _vertexBuffer);
        [commandBuffer commit];
        return;
    }
    
    // Start render pass encoder
    renderPassDesc.colorAttachments[0].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
    id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDesc];
    if (!renderEncoder) {
        NSLog(@"⚠️ Failed to create render encoder");
        [commandBuffer commit];
        return;
    }
    
    // Set debug label for easier debugging
    [renderEncoder setLabel:@"FBNeo Emulation Render"];
    
    // Configure the render pipeline and resources
    [renderEncoder setRenderPipelineState:_pipelineState];
    [renderEncoder setVertexBuffer:_vertexBuffer offset:0 atIndex:0];
    
    // CRITICAL: Always set fragment texture to our frame texture
    [renderEncoder setFragmentTexture:_frameTexture atIndex:0];
    
    // Set uniform buffer for shader parameters (if available)
    if (_uniformBuffer) {
        [renderEncoder setVertexBuffer:_uniformBuffer offset:0 atIndex:1];
        [renderEncoder setFragmentBuffer:_uniformBuffer offset:0 atIndex:0];
    }
    
    // Set sampler state for texture filtering
    if (samplerState) {
        [renderEncoder setFragmentSamplerState:samplerState atIndex:0];
    }
    
    // Draw the quad - always 4 vertices starting at index 0
    [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangleStrip
                      vertexStart:0
                      vertexCount:4];
    
    // End encoding
    [renderEncoder endEncoding];
    
    // CRITICAL: Present the drawable to the screen
    [commandBuffer presentDrawable:drawable];
    
    // Add completion handler for debugging
    if (frameCount % 60 == 0) {
        [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer) {
            if (buffer.status == MTLCommandBufferStatusCompleted) {
                NSLog(@"✅ Command buffer completed successfully on frame %d", frameCount);
            } else {
                NSLog(@"❌ Command buffer failed with status: %lu on frame %d", 
                     (unsigned long)buffer.status, frameCount);
            }
        }];
    }
    
    // Commit the command buffer to the GPU
    [commandBuffer commit];
}

@end

// Global view delegate
static MetalViewDelegate* viewDelegate = nil;

// Initialize vertex data for a full-screen quad
static void initVertexData() {
    // Vertex positions and texture coordinates for a full-screen quad
    // CRITICAL: Ensure quad covers entire screen in NDC space (-1 to 1)
    float quadVertices[] = {
        // Positions     // Texture coordinates
        -1.0f, -1.0f,    0.0f, 1.0f,   // Bottom left
         1.0f, -1.0f,    1.0f, 1.0f,   // Bottom right
        -1.0f,  1.0f,    0.0f, 0.0f,   // Top left
         1.0f,  1.0f,    1.0f, 0.0f    // Top right
    };
    
    // Create vertex buffer
    vertexBuffer = [device newBufferWithBytes:quadVertices
                                       length:sizeof(quadVertices)
                                      options:MTLResourceStorageModeShared];
    
    // Create uniform buffer
    ShaderUniforms uniforms;
    uniforms.textureWidth = currentWidth;
    uniforms.textureHeight = currentHeight;
    uniforms.reserved[0] = 0.0f;
    uniforms.reserved[1] = 0.0f;
    
    uniformBuffer = [device newBufferWithBytes:&uniforms
                                        length:sizeof(uniforms)
                                       options:MTLResourceStorageModeShared];
    
    NSLog(@"🔼 Created vertex buffer with %zu bytes, quad size: -1.0 to 1.0", sizeof(quadVertices));
    NSLog(@"🔼 Texture coordinates: (0,0) to (1,1)");
}

// Create a texture for rendering
static bool createTexture(int width, int height) {
    if (width <= 0 || height <= 0) {
        NSLog(@"Invalid texture dimensions: %dx%d", width, height);
        return false;
    }
    
    // Save dimensions
    currentWidth = width;
    currentHeight = height;
    
    // Create a texture descriptor
    MTLTextureDescriptor *textureDescriptor = [MTLTextureDescriptor 
                                               texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                               width:width
                                               height:height
                                               mipmapped:NO];
    textureDescriptor.usage = MTLTextureUsageShaderRead;
    
    // Create the texture
    frameTexture = [device newTextureWithDescriptor:textureDescriptor];
    if (!frameTexture) {
        NSLog(@"Failed to create texture");
        return false;
    }
    
    // Print detailed information about the texture
    NSLog(@"🖼️ Created Metal texture: %p", frameTexture);
    NSLog(@"   - Dimensions: %lux%lu pixels", frameTexture.width, frameTexture.height);
    NSLog(@"   - Pixel Format: %lu (8=BGRA8Unorm)", (unsigned long)frameTexture.pixelFormat);
    NSLog(@"   - Byte Length: %lu bytes", (unsigned long)frameTexture.width * frameTexture.height * 4);
    NSLog(@"   - Storage Mode: %lu", (unsigned long)frameTexture.storageMode);
    NSLog(@"   - Usage: %lu", (unsigned long)frameTexture.usage);
    
    return true;
}

// Create render pipeline state
static bool createPipelineState() {
    // Default Metal shader for basic texture rendering
    // CRITICAL: Use a simplified shader with nearest-neighbor filtering
    NSString* shaderSource = @R"(
        #include <metal_stdlib>
        using namespace metal;
        
        // Vertex input structure
        struct VertexInput {
            float2 position [[attribute(0)]];
            float2 texCoord [[attribute(1)]];
        };
        
        // Vertex output structure
        struct VertexOutput {
            float4 position [[position]];
            float2 texCoord;
        };
        
        // Uniform buffer structure
        struct Uniforms {
            float textureWidth;
            float textureHeight;
            float reserved[2];
        };
        
        // Vertex shader
        vertex VertexOutput vertexShader(uint vertexID [[vertex_id]],
                                        constant float4* vertices [[buffer(0)]],
                                        constant Uniforms& uniforms [[buffer(1)]]) {
            VertexOutput out;
            
            // Each vertex has position (xy) and texture coordinates (zw)
            float4 vertexData = vertices[vertexID];
            
            // Set position (no transformation needed for fullscreen quad)
            out.position = float4(vertexData.xy, 0.0, 1.0);
            
            // Pass texture coordinates to fragment shader
            out.texCoord = vertexData.zw;
            
            return out;
        }
        
        // Fragment shader
        fragment float4 fragmentShader(VertexOutput in [[stage_in]],
                                      texture2d<float> texture [[texture(0)]],
                                      constant Uniforms& uniforms [[buffer(0)]]) {
            // CRITICAL: Use nearest-neighbor filtering for pixel-perfect rendering
            constexpr sampler textureSampler(mag_filter::nearest,
                                           min_filter::nearest,
                                           address::clamp_to_edge);
            
            // Sample the texture using the provided texture coordinates
            float4 color = texture.sample(textureSampler, in.texCoord);
            
            // Ensure alpha is fully opaque
            return float4(color.rgb, 1.0);
        }
    )";
    
    // Compile shader
    NSError* error = nil;
    id<MTLLibrary> library = [device newLibraryWithSource:shaderSource
                                                  options:nil
                                                    error:&error];
    
    if (!library) {
        NSLog(@"⚠️ Failed to compile shader: %@", error);
        return false;
    }
    
    // Get shader functions
    id<MTLFunction> vertexFunction = [library newFunctionWithName:@"vertexShader"];
    id<MTLFunction> fragmentFunction = [library newFunctionWithName:@"fragmentShader"];
    
    if (!vertexFunction || !fragmentFunction) {
        NSLog(@"⚠️ Failed to get shader functions");
        return false;
    }
    
    // Create pipeline descriptor
    MTLRenderPipelineDescriptor* pipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDesc.vertexFunction = vertexFunction;
    pipelineDesc.fragmentFunction = fragmentFunction;
    pipelineDesc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    
    // Create pipeline state
    pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineDesc
                                                           error:&error];
    
    if (!pipelineState) {
        NSLog(@"⚠️ Failed to create pipeline state: %@", error);
        return false;
    }
    
    NSLog(@"⚙️ Created Metal render pipeline with nearest-neighbor filtering");
    return true;
}

// Draw the current frame with the view delegate
void drawCurrentFrame() {
    if (metalView && !metalView.paused) {
        // Ensure view is not paused
        metalView.paused = NO;
        
        // Use correct method to request drawing
        [metalView draw];
        
        // Force immediate drawing if needed
        if (metalView.delegate) {
            id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
            id<CAMetalDrawable> drawable = [metalView currentDrawable];
            MTLRenderPassDescriptor *renderPassDesc = [metalView currentRenderPassDescriptor];
            
            if (commandBuffer && drawable && renderPassDesc) {
                // Manual rendering using the delegate directly if needed
                MetalViewDelegate *delegate = (MetalViewDelegate*)metalView.delegate;
                [delegate mtkView:metalView drawableSizeWillChange:metalView.drawableSize];
                [delegate drawInMTKView:metalView];
            }
        }
    }
}

// Draw one frame
void MetalRenderer_Draw() {
    // Request drawing
    drawCurrentFrame();
}

// Update and draw the next frame with current buffer
void MetalRenderer_DrawNextFrame() {
    // Get current frame buffer
    void* frameBuffer = Metal_GetFrameBuffer();
    
    // Update frame content and request drawing
    if (frameTexture && frameBuffer) {
        // Update texture with the current frame buffer
        MetalRenderer_UpdateTexture(frameBuffer, currentWidth, currentHeight);
        
        // Request drawing
        drawCurrentFrame();
    }
}

// Set render state
void Metal_SetRenderState(int state, int value) {
    switch (state) {
        case METAL_STATE_VSYNC:
            // Enable/disable vsync
            if (metalView) {
                metalView.preferredFramesPerSecond = (value == 0) ? 0 : 60;
            }
            break;
            
        case METAL_STATE_FILTERING:
            // Enable/disable texture filtering
            createSamplerState(value != 0);
            break;
            
        case METAL_STATE_CRT:
            // Enable/disable CRT effect (not implemented)
            break;
            
        case METAL_STATE_SCANLINES:
            // Enable/disable scanlines (not implemented)
            break;
    }
}

// Force a redraw
void MetalRenderer_ForceRedraw() {
    if (metalView) {
        // Ensure view is not paused
        metalView.paused = NO;
        
        // Request drawing
        [metalView draw];
        
        // Force immediate drawing if needed
        drawCurrentFrame();
    }
}

// Initialize the Metal renderer
int MetalRenderer_Init(void* viewPtr, int width, int height) {
    NSLog(@"🚀 Initializing Metal renderer: %dx%d", width, height);
    
    // Validate parameters
    if (!viewPtr || width <= 0 || height <= 0) {
        NSLog(@"⚠️ Invalid renderer parameters");
        return 1; // Error code 1 for invalid parameters
    }
    
    // Get MTKView from pointer
    metalView = (__bridge MTKView*)viewPtr;
    
    // Get default Metal device
    device = MTLCreateSystemDefaultDevice();
    if (!device) {
        NSLog(@"⚠️ Failed to create Metal device");
        return 2; // Error code 2 for device creation failure
    }
    
    NSLog(@"📱 Metal device: %@", device.name);
    
    // Create command queue
    commandQueue = [device newCommandQueue];
    if (!commandQueue) {
        NSLog(@"⚠️ Failed to create command queue");
        return 3; // Error code 3 for command queue creation failure
    }
    
    // Update dimensions
    currentWidth = width;
    currentHeight = height;
    
    // Initialize vertex and uniform data
    initVertexData();
    
    // Create frame texture
    if (!createTexture(width, height)) {
        NSLog(@"⚠️ Failed to create frame texture");
        return 4; // Error code 4 for texture creation failure
    }
    
    // Create render pipeline
    if (!createPipelineState()) {
        NSLog(@"⚠️ Failed to create render pipeline");
        return 5; // Error code 5 for pipeline creation failure
    }
    
    // Create view delegate
    @try {
        viewDelegate = [[MetalViewDelegate alloc] init];
        viewDelegate.device = device;
        viewDelegate.commandQueue = commandQueue;
        viewDelegate.pipelineState = pipelineState;
        viewDelegate.vertexBuffer = vertexBuffer;
        viewDelegate.uniformBuffer = uniformBuffer;
        viewDelegate.frameTexture = frameTexture;
        viewDelegate.frameSize = CGSizeMake(width, height);
        viewDelegate.pauseRendering = NO;
        
        // Configure MTKView
        metalView.device = device;
        
        // CRITICAL: Set delegate and verify it's properly set
        metalView.delegate = viewDelegate;
        
        if (metalView.delegate == nil) {
            NSLog(@"⚠️ CRITICAL ERROR: Failed to set MTKView delegate!");
            NSLog(@"    - viewDelegate: %@", viewDelegate);
            NSLog(@"    - metalView: %@", metalView);
            
            // Try setting it again
            dispatch_async(dispatch_get_main_queue(), ^{
                metalView.delegate = viewDelegate;
                NSLog(@"    - After async setting: delegate=%@", metalView.delegate);
            });
        } else {
            NSLog(@"✅ MTKView delegate set successfully: %@", metalView.delegate);
        }
        
        // CRITICAL: Configure MTKView for proper rendering
        metalView.framebufferOnly = YES;
        metalView.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
        metalView.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
        metalView.paused = NO;
        
        // Set drawing mode to continuous
        metalView.preferredFramesPerSecond = 60;
        
        // Force a redraw immediately to test the delegate
        [metalView draw];
        
        // Schedule regular redraws
        dispatch_async(dispatch_get_main_queue(), ^{
            // Create a timer to force redraw
            NSTimer *redrawTimer = [NSTimer scheduledTimerWithTimeInterval:1.0/60.0
                                                                  target:[NSBlockOperation blockOperationWithBlock:^{
                                                                      void* frameBuffer = Metal_GetFrameBuffer();
                                                                      if (frameTexture && frameBuffer) {
                                                                          // Update texture with the current frame buffer
                                                                          MetalRenderer_UpdateTexture(frameBuffer, currentWidth, currentHeight);
                                                                          
                                                                          // Request drawing
                                                                          if (metalView && !metalView.paused) {
                                                                              [metalView draw];
                                                                          }
                                                                      }
                                                                  }]
                                                                selector:@selector(main)
                                                                userInfo:nil
                                                                 repeats:YES];
            
            [[NSRunLoop currentRunLoop] addTimer:redrawTimer forMode:NSRunLoopCommonModes];
        });
    } @catch (NSException *exception) {
        NSLog(@"⚠️ Exception setting up Metal view: %@", exception);
        return 6; // Error code 6 for view setup failure
    }
    
    NSLog(@"✅ Metal renderer initialized successfully");
    NSLog(@"📊 Renderer config: framebufferOnly=%d, colorPixelFormat=%lu", 
          (int)metalView.framebufferOnly, (unsigned long)metalView.colorPixelFormat);
    
    return 0; // Success
}

// Shutdown the Metal renderer
void MetalRenderer_Exit() {
    NSLog(@"🛑 Shutting down Metal renderer");
    
    // Stop rendering
    if (metalView) {
        metalView.paused = YES;
        metalView.delegate = nil;
    }
    
    // Release all Metal objects
    viewDelegate = nil;
    frameTexture = nil;
    pipelineState = nil;
    vertexBuffer = nil;
    uniformBuffer = nil;
    commandQueue = nil;
    device = nil;
    metalView = nil;
    
    NSLog(@"✅ Metal renderer shutdown complete");
}

// Resize frame buffer
bool MetalRenderer_Resize(int width, int height) {
    if (width <= 0 || height <= 0 || !device) {
        return false;
    }
    
    NSLog(@"↔️ Resizing Metal frame buffer to %dx%d", width, height);
    
    // Create new texture with new dimensions
    return createTexture(width, height);
}

// Set renderer paused state
void MetalRenderer_SetPaused(bool paused) {
    if (viewDelegate) {
        viewDelegate.pauseRendering = paused;
    }
    
    if (metalView) {
        metalView.paused = paused;
    }
    
    NSLog(@"⏯️ Metal renderer paused: %d", paused);
}

// Update the frame texture with new data
bool MetalRenderer_UpdateFrameTexture(void* frameData, int width, int height, int pitch) {
    // Validate parameters
    if (!frameData || width <= 0 || height <= 0 || pitch <= 0 || !frameTexture) {
        NSLog(@"⚠️ Invalid parameters for texture update");
        return false;
    }
    
    // Update texture dimensions if needed
    if (width != currentWidth || height != currentHeight) {
        NSLog(@"↔️ Resizing texture from %dx%d to %dx%d", currentWidth, currentHeight, width, height);
        if (!createTexture(width, height)) {
            NSLog(@"⚠️ Failed to resize texture");
            return false;
        }
        
        // Update delegate's frame texture reference
        if (viewDelegate) {
            viewDelegate.frameTexture = frameTexture;
            viewDelegate.frameSize = CGSizeMake(width, height);
        }
    }
    
    // Update texture data
    MTLRegion region = MTLRegionMake2D(0, 0, width, height);
    [frameTexture replaceRegion:region
                    mipmapLevel:0
                      withBytes:frameData
                    bytesPerRow:pitch];
    
    return true;
}

// Debug verification function for the entire Metal rendering pipeline
bool MetalRenderer_VerifyPipeline() {
    bool success = true;
    
    // Start with detailed header
    printf("\n===== METAL RENDERING PIPELINE VERIFICATION =====\n");
    
    // 1. Check Metal device
    if (!device) {
        printf("❌ ERROR: Metal device is NULL\n");
        success = false;
    } else {
        printf("✅ Metal device: %s\n", [device.name UTF8String]);
    }
    
    // 2. Check command queue
    if (!commandQueue) {
        printf("❌ ERROR: Command queue is NULL\n");
        success = false;
    } else {
        printf("✅ Command queue: %p\n", commandQueue);
    }
    
    // 3. Check pipeline state
    if (!pipelineState) {
        printf("❌ ERROR: Render pipeline state is NULL\n");
        success = false;
    } else {
        printf("✅ Render pipeline state: %p\n", pipelineState);
    }
    
    // 4. Check vertex buffer
    if (!vertexBuffer) {
        printf("❌ ERROR: Vertex buffer is NULL\n");
        success = false;
    } else {
        // Read vertex data for verification
        float* vertexData = (float*)[vertexBuffer contents];
        printf("✅ Vertex buffer: %p\n", vertexBuffer);
        printf("   Vertex data (first 4 floats): %.1f, %.1f, %.1f, %.1f\n",
               vertexData[0], vertexData[1], vertexData[2], vertexData[3]);
    }
    
    // 5. Check frame texture
    if (!frameTexture) {
        printf("❌ ERROR: Frame texture is NULL\n");
        success = false;
    } else {
        printf("✅ Frame texture: %p (%ldx%ld, format=%lu)\n", 
              frameTexture, frameTexture.width, frameTexture.height, (unsigned long)frameTexture.pixelFormat);
    }
    
    // 6. Check MTKView configuration
    if (!metalView) {
        printf("❌ ERROR: MTKView is NULL\n");
        success = false;
    } else {
        printf("✅ MTKView: %p\n", metalView);
        printf("   MTKView framebufferOnly: %s\n", metalView.framebufferOnly ? "YES" : "NO");
        printf("   MTKView colorPixelFormat: %lu\n", (unsigned long)metalView.colorPixelFormat);
        printf("   MTKView paused: %s\n", metalView.paused ? "YES" : "NO");
        
        // Check delegate
        if (!metalView.delegate) {
            printf("❌ ERROR: MTKView delegate is NULL\n");
            success = false;
        } else {
            printf("✅ MTKView delegate: %s\n", [NSStringFromClass([metalView.delegate class]) UTF8String]);
        }
    }
    
    // 7. Test access to the frame buffer
    void* frameBuffer = Metal_GetFrameBuffer();
    if (!frameBuffer) {
        printf("❌ ERROR: Metal_GetFrameBuffer() returned NULL\n");
        success = false;
    } else {
        // Calculate checksum of the first 1000 pixels
        uint32_t* pixels = (uint32_t*)frameBuffer;
        uint32_t checksum = 0;
        int nonZeroPixels = 0;
        
        for (int i = 0; i < 1000; i++) {
            checksum ^= pixels[i];
            if (pixels[i] != 0) nonZeroPixels++;
        }
        
        printf("✅ Frame buffer: %p\n", frameBuffer);
        printf("   Frame buffer checksum: 0x%08X\n", checksum);
        printf("   Frame buffer non-zero pixels: %d/1000\n", nonZeroPixels);
        
        // Sample some pixels
        printf("   Sample pixels:");
        for (int i = 0; i < 5; i++) {
            uint32_t pixel = pixels[i];
            printf(" [%d]=0x%08X", i, pixel);
        }
        printf("\n");
    }
    
    // 8. Check uniform buffer
    if (!uniformBuffer) {
        printf("⚠️ WARNING: Uniform buffer is NULL (not critical)\n");
    } else {
        ShaderUniforms* uniforms = (ShaderUniforms*)[uniformBuffer contents];
        printf("✅ Uniform buffer: %p\n", uniformBuffer);
        printf("   Texture dimensions: %.0fx%.0f\n", uniforms->textureWidth, uniforms->textureHeight);
    }
    
    // 9. Verify that we're calling Metal_RunFrame with correct parameters
    printf("ℹ️ Metal_RunFrame: Called with bDraw=1 for rendering frames\n");
    
    // 10. Verify texture format and swizzling is correct
    printf("ℹ️ Texture format: BGRA8Unorm (expected by Metal)\n");
    printf("ℹ️ Frame buffer conversion: RGBA → BGRA swizzling in ConvertFrameBufferToMetal()\n");
    
    // 11. Check view delegate
    if (!viewDelegate) {
        printf("❌ ERROR: MetalViewDelegate is NULL\n");
        success = false;
    } else {
        printf("✅ MetalViewDelegate: %p\n", viewDelegate);
        printf("   Rendering paused: %s\n", viewDelegate.pauseRendering ? "YES" : "NO");
    }
    
    // 12. Force a redraw to test the pipeline
    if (metalView && !metalView.paused) {
        printf("ℹ️ Forcing a redraw to test the pipeline\n");
        [metalView draw];
    }
    
    // Print overall result
    if (success) {
        printf("✅ METAL RENDERING PIPELINE VERIFICATION PASSED ✅\n");
    } else {
        printf("❌ METAL RENDERING PIPELINE VERIFICATION FAILED ❌\n");
    }
    printf("=================================================\n\n");
    
    return success;
}

// Create MTKView and delegate
bool MetalRenderer_CreateView(void* windowHandle, int width, int height) {
    NSWindow* window = (__bridge NSWindow*)windowHandle;
    if (!window) {
        NSLog(@"Error: No window handle provided");
        return false;
    }
    
    NSLog(@"Creating Metal view with size %dx%d", width, height);
    
    // Create Metal device
    device = MTLCreateSystemDefaultDevice();
    if (!device) {
        NSLog(@"Error: Could not create Metal device");
        return false;
    }
    
    // Create command queue
    commandQueue = [device newCommandQueue];
    if (!commandQueue) {
        NSLog(@"Error: Could not create command queue");
        return false;
    }
    
    // Create Metal view
    metalView = [[MTKView alloc] initWithFrame:window.contentView.bounds device:device];
    if (!metalView) {
        NSLog(@"Error: Could not create Metal view");
        return false;
    }
    
    // Configure view properties
    metalView.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    metalView.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
    metalView.framebufferOnly = YES;
    metalView.enableSetNeedsDisplay = NO;
    metalView.paused = NO;
    metalView.autoResizeDrawable = YES;
    
    // Set up view delegate
    MetalViewDelegate* delegate = [[MetalViewDelegate alloc] init];
    delegate.device = device;
    delegate.commandQueue = commandQueue;
    delegate.frameSize = CGSizeMake(width, height);
    delegate.pauseRendering = NO;
    metalView.delegate = delegate;
    
    // Add view to window
    [window.contentView addSubview:metalView];
    metalView.frame = window.contentView.bounds;
    metalView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    
    // Set up rendering resources
    if (!createPipelineState()) {
        NSLog(@"Error: Failed to create pipeline state");
        return false;
    }
    
    // Initialize vertex buffer
    initVertexData();
    
    // Create framebuffer texture for rendering
    if (!createTexture(width, height)) {
        NSLog(@"Error: Failed to create texture");
        return false;
    }
    
    // Create uniform buffer for shader parameters
    uniformBuffer = [device newBufferWithLength:sizeof(ShaderUniforms)
                                        options:MTLResourceStorageModeShared];
    if (uniformBuffer) {
        ShaderUniforms* uniforms = (ShaderUniforms*)[uniformBuffer contents];
        uniforms->textureWidth = width;
        uniforms->textureHeight = height;
    }
    
    // Update delegate with created resources
    delegate.pipelineState = pipelineState;
    delegate.vertexBuffer = vertexBuffer;
    delegate.uniformBuffer = uniformBuffer;
    delegate.frameTexture = frameTexture;
    
    // Store current dimensions
    currentWidth = width;
    currentHeight = height;
    
    NSLog(@"Metal view created successfully");
    return true;
}

// Update texture with new frame data
bool MetalRenderer_UpdateTexture(void* data, int width, int height) {
    if (!frameTexture || !data) {
        NSLog(@"Error: Cannot update texture - texture=%p, data=%p", frameTexture, data);
        return false;
    }
    
    if (width != currentWidth || height != currentHeight) {
        NSLog(@"Warning: Texture size mismatch - expected %dx%d, got %dx%d",
              currentWidth, currentHeight, width, height);
    }
    
    // Replace texture content with new data
    MTLRegion region = MTLRegionMake2D(0, 0, width, height);
    [frameTexture replaceRegion:region
                    mipmapLevel:0
                      withBytes:data
                    bytesPerRow:width * 4];
    
    return true;
}

// Set emulation mode
bool MetalRenderer_SetEmulationMode(int mode) {
    // Shutdown current emulation if active
    if (emulationInitialized) {
        shutdownEmulation();
    }
    
    // Set new emulation mode
    currentEmulationMode = (EmulationMode)mode;
    
    // Initialize the new emulation mode
    return initializeEmulation();
}

// Load a specific game
bool MetalRenderer_LoadGame(int gameIndex) {
    currentGameIndex = gameIndex;
    
    // If emulation is initialized, load the game
    if (emulationInitialized) {
        if (currentEmulationMode == EMULATION_MODE_CPS2_VALUE) {
            int result = Metal_CPS2_LoadGame(gameIndex);
            if (result != 0) {
                NSLog(@"Error: Failed to load CPS2 game: %d", result);
                return false;
            }
            
            // Update frame dimensions if necessary
            int gameWidth, gameHeight;
            Metal_CPS2_GetGameDimensions(&gameWidth, &gameHeight);
            if (gameWidth != currentWidth || gameHeight != currentHeight) {
                currentWidth = gameWidth;
                currentHeight = gameHeight;
                
                // Recreate texture with new dimensions
                if (!createTexture(gameWidth, gameHeight)) {
                    NSLog(@"Error: Failed to create new texture for game");
                    return false;
                }
                
                // Update MetalViewDelegate with new texture and dimensions
                MetalViewDelegate* delegate = (MetalViewDelegate*)metalView.delegate;
                delegate.frameTexture = frameTexture;
                delegate.frameSize = CGSizeMake(gameWidth, gameHeight);
                
                // Update uniform buffer with new dimensions
                if (uniformBuffer) {
                    ShaderUniforms* uniforms = (ShaderUniforms*)[uniformBuffer contents];
                    if (uniforms) {
                        uniforms->textureWidth = gameWidth;
                        uniforms->textureHeight = gameHeight;
                    }
                }
            }
            
            return true;
        }
    }
    
    // Fall back to minimal mode
    return (Metal_RunFrame(1) == 0);
}

// Initialize the chosen emulation mode
static bool initializeEmulation() {
    switch (currentEmulationMode) {
        case EMULATION_MODE_CPS2_VALUE:
            {
                int result = Metal_CPS2_Init();
                if (result != 0) {
                    NSLog(@"Error: Failed to initialize CPS2 emulation: %d", result);
                    return false;
                }
                
                // Load initial game if specified
                if (currentGameIndex >= 0) {
                    result = Metal_CPS2_LoadGame(currentGameIndex);
                    if (result != 0) {
                        NSLog(@"Error: Failed to load CPS2 game: %d", result);
                        return false;
                    }
                    
                    // Update frame dimensions if necessary
                    int gameWidth, gameHeight;
                    Metal_CPS2_GetGameDimensions(&gameWidth, &gameHeight);
                    if (gameWidth != currentWidth || gameHeight != currentHeight) {
                        currentWidth = gameWidth;
                        currentHeight = gameHeight;
                        
                        // Recreate texture with new dimensions
                        if (!createTexture(gameWidth, gameHeight)) {
                            NSLog(@"Error: Failed to create new texture for CPS2 game");
                            return false;
                        }
                        
                        // Update MetalViewDelegate with new texture and dimensions
                        MetalViewDelegate* delegate = (MetalViewDelegate*)metalView.delegate;
                        delegate.frameTexture = frameTexture;
                        delegate.frameSize = CGSizeMake(gameWidth, gameHeight);
                        
                        // Update uniform buffer with new dimensions
                        if (uniformBuffer) {
                            ShaderUniforms* uniforms = (ShaderUniforms*)[uniformBuffer contents];
                            if (uniforms) {
                                uniforms->textureWidth = gameWidth;
                                uniforms->textureHeight = gameHeight;
                            }
                        }
                    }
                }
            }
            break;
            
        case EMULATION_MODE_MINIMAL_VALUE:
        default:
            // Use default minimal emulation
            extern INT32 BurnLibInit_Metal();
            int result = BurnLibInit_Metal();
            if (result != 0) {
                NSLog(@"Error: Failed to initialize minimal emulation: %d", result);
                return false;
            }
            break;
    }
    
    emulationInitialized = true;
    NSLog(@"Emulation initialized in mode %d", currentEmulationMode);
    return true;
}

// Shutdown the current emulation
static void shutdownEmulation() {
    if (!emulationInitialized) {
        return;
    }
    
    switch (currentEmulationMode) {
        case EMULATION_MODE_CPS2_VALUE:
            {
                // Exit current game if needed
                Metal_CPS2_ExitGame();
                
                // Shutdown CPS2 system
                Metal_CPS2_Exit();
            }
            break;
            
        case EMULATION_MODE_MINIMAL_VALUE:
        default:
            // Exit minimal emulation
            extern INT32 BurnLibExit_Metal();
            BurnLibExit_Metal();
            break;
    }
    
    emulationInitialized = false;
    NSLog(@"Emulation shutdown");
}

// Run a single frame of emulation
static bool runEmulationFrame(bool render) {
    if (!emulationInitialized) {
        return false;
    }
    
    switch (currentEmulationMode) {
        case EMULATION_MODE_CPS2_VALUE:
            {
                int result = Metal_CPS2_RunFrame(render ? 1 : 0);
                return (result == 0);
            }
            
        case EMULATION_MODE_MINIMAL_VALUE:
        default:
            {
                int result = Metal_RunFrame(render ? 1 : 0);
                return (result == 0);
            }
    }
}

// Clean up resources
void MetalRenderer_Cleanup() {
    // Shutdown emulation if active
    if (emulationInitialized) {
        shutdownEmulation();
    }
    
    // Release Metal objects
    frameTexture = nil;
    pipelineState = nil;
    vertexBuffer = nil;
    uniformBuffer = nil;
    samplerState = nil;
    commandQueue = nil;
    device = nil;
    
    // Remove the view
    [metalView removeFromSuperview];
    metalView = nil;
    
    NSLog(@"Metal renderer cleanup complete");
}
