#include "vid_metal.h"
#include "metal_types.h"
#include <Metal/Metal.h>
#include <MetalKit/MetalKit.h>
#include <QuartzCore/CAMetalLayer.h>
#include "metal_unified.h"
#include "metal_interface.h"
#include "Shaders.h"
#include "ShaderOptions.h"

// Static variables for Metal resources
static id<MTLDevice> device = nil;
static id<MTLCommandQueue> commandQueue = nil;
static id<MTLBuffer> vertexBuffer = nil;
static id<MTLBuffer> indexBuffer = nil;
static id<MTLTexture> texture = nil;
static id<MTLSamplerState> samplerState = nil;
static id<MTLRenderPipelineState> pipelineState = nil;
static id<MTLRenderPipelineState> postProcessPipeline = nil;
static id<MTLRenderPipelineState> dynamicResolutionPipeline = nil;
static CAMetalLayer* metalLayer = nil;
static id<MTLRenderPipelineState> postProcessingPipeline = nil;
static ShaderParameters shaderParams = kDefaultShaderParams;
static ShaderOptions shaderOptions = kDefaultShaderOptions;

// Default post-processing parameters
static PostProcessParams postProcessParams = {
    .scanlineIntensity = 0.3f,
    .scanlineWidth = 1.0f,
    .scanlineOffset = 0.0f,
    .crtCurvature = 0.1f,
    .vignetteStrength = 0.3f,
    .vignetteSmoothness = 0.5f,
    .resolution = {1920.0f, 1080.0f},
    .screenSize = {1920.0f, 1080.0f},
    .dynamicResolution = true
};

// Vertex data
static const VertexIn vertexData[] = {
    { { -1.0f, -1.0f }, { 0.0f, 1.0f }, 0 },
    { {  1.0f, -1.0f }, { 1.0f, 1.0f }, 0 },
    { { -1.0f,  1.0f }, { 0.0f, 0.0f }, 0 },
    { {  1.0f,  1.0f }, { 1.0f, 0.0f }, 0 }
};

// Game size
static int gameWidth = 0;
static int gameHeight = 0;

// View reference
static NSView* gameView = nil;

// Interface for Metal renderer
extern "C" {
    bool InitMetal(void* view);
    void ShutdownMetal();
    void MetalRenderFrame(unsigned char* buffer, int width, int height, int pitch);
    void MetalSetWindowTitle(const char* title);
    void MetalResizeWindow(int width, int height);
    int InitializeMetal(void* windowHandle, int width, int height);
    void ShutdownMetal();
    void ResizeMetal(int width, int height);
    void RenderFrame(void* buffer, int width, int height, int pitch, int bpp);
    void PresentFrame();
    void ClearFrame();
    void SetVSync(int enabled);
    void ToggleFullscreen();
    void SetScanlines(int enable, float intensity);
    void SetCRTCurvature(int enable, float amount);
    void SetVignette(int enable, float strength, float smoothness);
    void SetDynamicResolution(int enable);
    void ApplyShaderPreset(int presetIndex);
}

// Global variables
static bool bMetalInitialized = false;
static bool bMetalVSync = true;
static NSView* metalView = nil;

// Texture/frame buffer properties
static int nMetalImageWidth = 0;
static int nMetalImageHeight = 0;
static int nMetalImagePitch = 0;
static int nMetalImageBPP = 0;
static unsigned char* pMetalFrameBuffer = NULL;

// Frame buffer properties - shared with the main code
static unsigned char* g_pFrameBuffer = NULL;
static int g_nFrameWidth = 384;  // Default width (will be updated)
static int g_nFrameHeight = 224; // Default height (will be updated)
static int g_nFramePitch = 0;    // Will be calculated

// Metal rendering objects
static id<MTLDevice> device = nil;
static id<MTLCommandQueue> commandQueue = nil;
static id<MTLRenderPipelineState> pipelineState = nil;
static id<MTLBuffer> vertexBuffer = nil;
static id<MTLTexture> texture = nil;
static id<MTLSamplerState> samplerState = nil;
static CAMetalLayer* metalLayer = nil;

// Window/view reference
static NSWindow* metalWindow = nil;
static NSView* metalView = nil;

// Initialization flag
static bool g_bMetalInitialized = false;

// Vertex data for a quad covering the viewport
static const float quadVertices[] = {
    // positions        // texture coords
    -1.0f, -1.0f, 0.0f, 1.0f,  0.0f, 1.0f, // bottom left
     1.0f, -1.0f, 0.0f, 1.0f,  1.0f, 1.0f, // bottom right
    -1.0f,  1.0f, 0.0f, 1.0f,  0.0f, 0.0f, // top left
     1.0f,  1.0f, 0.0f, 1.0f,  1.0f, 0.0f  // top right
};

// Texture dimension helpers
static int RoundUpToPowerOf2(int value) {
    int powerOf2 = 1;
    while (powerOf2 < value) {
        powerOf2 *= 2;
    }
    return powerOf2;
}

// Metal view delegate
@interface MetalViewDelegate : NSObject<NSWindowDelegate, CALayerDelegate>
- (void)drawLayer:(CALayer*)layer inContext:(CGContextRef)ctx;
- (void)windowWillClose:(NSNotification*)notification;
@end

@implementation MetalViewDelegate
- (void)drawLayer:(CALayer*)layer inContext:(CGContextRef)ctx {
    // This is called when the layer needs to be drawn
    if (!g_bMetalInitialized) return;
    
    // Render the current frame
    [self renderFrame];
}

- (void)windowWillClose:(NSNotification*)notification {
    // Handle window closing
    NSLog(@"Metal window will close - performing cleanup");
}

- (void)renderFrame {
    if (!metalLayer || !commandQueue || !pipelineState || !texture) {
        return;
    }
    
    // Get next drawable from layer
    id<CAMetalDrawable> drawable = [metalLayer nextDrawable];
    if (!drawable) {
        return;
    }
    
    // Create a render pass descriptor
    MTLRenderPassDescriptor *renderPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
    renderPassDescriptor.colorAttachments[0].texture = drawable.texture;
    renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
    renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
    renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
    
    // Create a command buffer
    id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
    
    // Create a render encoder
    id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
    
    // Set the render pipeline state
    [renderEncoder setRenderPipelineState:pipelineState];
    
    // Set the vertex buffer
    [renderEncoder setVertexBuffer:vertexBuffer offset:0 atIndex:0];
    
    // Set the texture and sampler
    [renderEncoder setFragmentTexture:texture atIndex:0];
    [renderEncoder setFragmentSamplerState:samplerState atIndex:0];
    
    // Draw quad
    [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
    
    // End encoding and present
    [renderEncoder endEncoding];
    [commandBuffer presentDrawable:drawable];
    [commandBuffer commit];
}
@end

// Global delegate
static MetalViewDelegate* viewDelegate = nil;

// Timer for frame rendering
static NSTimer* renderTimer = nil;

// Timer callback
void renderCallback(CFRunLoopTimerRef timer, void* info) {
    if (viewDelegate) {
        [viewDelegate renderFrame];
    }
}

#pragma mark - Metal Setup and Resource Management

// Create a basic pipeline state
static bool createShaderPipelines() {
    id<MTLLibrary> defaultLibrary = [device newDefaultLibrary];
    if (!defaultLibrary) {
        NSLog(@"Failed to load default Metal shader library");
        return false;
    }
    
    // Create basic pipeline
    id<MTLFunction> vertexFunction = [defaultLibrary newFunctionWithName:@"vertexShader"];
    id<MTLFunction> fragmentFunction = [defaultLibrary newFunctionWithName:@"fragmentShader"];
    
    if (!vertexFunction || !fragmentFunction) {
        NSLog(@"Failed to load basic Metal shader functions");
        return false;
    }
    
    // Configure a pipeline descriptor for basic rendering
    MTLRenderPipelineDescriptor *pipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDescriptor.vertexFunction = vertexFunction;
    pipelineDescriptor.fragmentFunction = fragmentFunction;
    pipelineDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    
    // Create basic pipeline state
    NSError *error = nil;
    pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
    if (!pipelineState) {
        NSLog(@"Failed to create basic pipeline state: %@", error);
        return false;
    }
    
    // Create post-processing pipeline
    id<MTLFunction> postProcessFragment = [defaultLibrary newFunctionWithName:@"postProcessingShader"];
    if (!postProcessFragment) {
        NSLog(@"Failed to load post-processing shader function");
        return false;
    }
    
    pipelineDescriptor.fragmentFunction = postProcessFragment;
    postProcessingPipeline = [device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
    if (!postProcessingPipeline) {
        NSLog(@"Failed to create post-processing pipeline state: %@", error);
        return false;
    }
    
    // Create dynamic resolution pipeline
    id<MTLFunction> dynamicResFragment = [defaultLibrary newFunctionWithName:@"dynamicResolutionShader"];
    if (!dynamicResFragment) {
        NSLog(@"Failed to load dynamic resolution shader function");
        return false;
    }
    
    pipelineDescriptor.fragmentFunction = dynamicResFragment;
    dynamicResolutionPipeline = [device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
    if (!dynamicResolutionPipeline) {
        NSLog(@"Failed to create dynamic resolution pipeline state: %@", error);
        return false;
    }
    
    NSLog(@"All shader pipelines created successfully");
    return true;
}

// Create a texture for the game framebuffer
static id<MTLTexture> createTexture(int width, int height) {
    MTLTextureDescriptor *textureDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                                                                 width:width
                                                                                                height:height
                                                                                             mipmapped:NO];
    textureDescriptor.usage = MTLTextureUsageShaderRead;
    id<MTLTexture> newTexture = [device newTextureWithDescriptor:textureDescriptor];
    if (!newTexture) {
        NSLog(@"Failed to create Metal texture");
    }
    return newTexture;
}

// Create a sampler state for the texture
static id<MTLSamplerState> createSamplerState() {
    MTLSamplerDescriptor *samplerDescriptor = [[MTLSamplerDescriptor alloc] init];
    samplerDescriptor.minFilter = MTLSamplerMinMagFilterLinear;
    samplerDescriptor.magFilter = MTLSamplerMinMagFilterLinear;
    samplerDescriptor.sAddressMode = MTLSamplerAddressModeClampToEdge;
    samplerDescriptor.tAddressMode = MTLSamplerAddressModeClampToEdge;
    
    id<MTLSamplerState> sampler = [device newSamplerStateWithDescriptor:samplerDescriptor];
    if (!sampler) {
        NSLog(@"Failed to create Metal sampler state");
    }
    return sampler;
}

#pragma mark - C Interface Functions

extern "C" {

// Initialize Metal rendering system
bool InitMetal(void* view) {
    @autoreleasepool {
        // Store the view reference
        gameView = (__bridge NSView*)view;
        
        // Create the Metal device
        device = MTLCreateSystemDefaultDevice();
        if (!device) {
            NSLog(@"Metal is not supported on this device");
            return false;
        }
        
        // Create command queue
        commandQueue = [device newCommandQueue];
        if (!commandQueue) {
            NSLog(@"Failed to create Metal command queue");
            return false;
        }
        
        // Create vertex buffer
        vertexBuffer = [device newBufferWithBytes:vertexData
                                          length:sizeof(vertexData)
                                         options:MTLResourceStorageModeShared];
        if (!vertexBuffer) {
            NSLog(@"Failed to create vertex buffer");
            return false;
        }
        
        // Create the Metal layer if we have a view
        if (gameView) {
            metalLayer = [CAMetalLayer layer];
            metalLayer.device = device;
            metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
            metalLayer.framebufferOnly = YES;
            
            NSRect bounds = [gameView bounds];
            metalLayer.frame = CGRectMake(0, 0, bounds.size.width, bounds.size.height);
            
            // Ensure the view is layer-backed
            [gameView setWantsLayer:YES];
            
            // Add the Metal layer to the view's layer
            [gameView setLayer:metalLayer];
        }
        
        // Create the render pipeline
        if (!createShaderPipelines()) {
            return false;
        }
        
        // Create sampler state
        samplerState = createSamplerState();
        if (!samplerState) {
            return false;
        }
        
        NSLog(@"Metal renderer initialized successfully");
        return true;
    }
}

// Shut down Metal rendering system
void ShutdownMetal() {
    @autoreleasepool {
        // With ARC, we release references by setting to nil
        device = nil;
        commandQueue = nil;
        vertexBuffer = nil;
        indexBuffer = nil;
        texture = nil;
        samplerState = nil;
        pipelineState = nil;
        postProcessingPipeline = nil;
        dynamicResolutionPipeline = nil;
        
        if (metalLayer) {
            [metalLayer removeFromSuperlayer];
            metalLayer = nil;
        }
        
        gameView = nil;
        
        NSLog(@"Metal renderer shut down");
    }
}

// Set the game screen size and recreate texture if needed
void MetalSetScreenSize(int width, int height) {
    @autoreleasepool {
        if (width <= 0 || height <= 0) {
            NSLog(@"Invalid screen dimensions: %d x %d", width, height);
            return;
        }
        
        gameWidth = width;
        gameHeight = height;
        
        // Update post-processing parameters
        postProcessParams.resolution = simd_make_float2(width, height);
        
        // Create a new texture with the right dimensions
        if (device) {
            texture = createTexture(width, height);
            NSLog(@"Metal texture created for game: %d x %d", width, height);
        }
    }
}

// Render a frame using Metal
void MetalRenderFrame(unsigned char* buffer, int width, int height, int pitch) {
    @autoreleasepool {
        if (!device || !commandQueue || !pipelineState || !texture || !vertexBuffer || !buffer) {
            return;
        }
        
        // Make sure our dimensions match what we expect
        if (width != gameWidth || height != gameHeight) {
            MetalSetScreenSize(width, height);
        }
        
        // Create a region for the entire texture
        MTLRegion region = {
            { 0, 0, 0 },
            { static_cast<NSUInteger>(width), static_cast<NSUInteger>(height), 1 }
        };
        
        // Copy the pixel data into the texture
        [texture replaceRegion:region
                  mipmapLevel:0
                    withBytes:buffer
                  bytesPerRow:pitch];
        
        // If we have a layer, render to it
        if (metalLayer) {
            // Get the next drawable
            id<CAMetalDrawable> drawable = [metalLayer nextDrawable];
            if (!drawable) {
                return;
            }
            
            // Create a render pass descriptor
            MTLRenderPassDescriptor *renderPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
            renderPassDescriptor.colorAttachments[0].texture = drawable.texture;
            renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
            renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
            renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
            
            // Create command buffer and encoder
            id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
            id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
            
            // Choose which pipeline to use based on shader options
            id<MTLRenderPipelineState> activePipeline = pipelineState; // Default basic pipeline
            
            if (shaderOptions.enableDynamicResolution) {
                activePipeline = dynamicResolutionPipeline;
            } else if (shaderOptions.enableScanlines || 
                      shaderOptions.enableCrtCurvature || 
                      shaderOptions.enableVignette) {
                activePipeline = postProcessingPipeline;
            }
            
            // Set the active render pipeline state
            [renderEncoder setRenderPipelineState:activePipeline];
            
            // Set the vertex buffer
            [renderEncoder setVertexBuffer:vertexBuffer offset:0 atIndex:0];
            
            // Set the texture and sampler
            [renderEncoder setFragmentTexture:texture atIndex:0];
            [renderEncoder setFragmentSamplerState:samplerState atIndex:0];
            
            // Set shader parameters if using advanced shaders
            if (activePipeline != pipelineState) {
                [renderEncoder setFragmentBytes:&postProcessParams 
                                         length:sizeof(PostProcessParams) 
                                        atIndex:0];
            }
            
            // Draw the quad
            [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangleStrip
                              vertexStart:0
                              vertexCount:4];
            
            // End encoding and present
            [renderEncoder endEncoding];
            [commandBuffer presentDrawable:drawable];
            [commandBuffer commit];
        }
    }
}

// Callback for Metal game loop - used by external code (like SDL)
int RunMetalGame() {
    // This function would contain a game loop that calls MetalFrame() and other functions
    // For now, it's just a stub returning success
    NSLog(@"Metal game loop started");
    return 0;
}

// Handle input from the Metal UI
void MetalHandleInput(bool bCopy) {
    // This function would handle input from the Metal UI
    // For now, it's just a stub
    // bCopy indicates whether to just copy the input state or process it
}

// Add C-linkage wrappers for C++
int InitializeMetal(void* windowHandle, int width, int height) {
    return InitMetal(windowHandle) ? 1 : 0;
}

void ShutdownMetal() {
    ShutdownMetal();
}

void ResizeMetal(int width, int height) {
    MetalSetScreenSize(width, height);
}

void RenderFrame(void* buffer, int width, int height, int pitch, int bpp) {
    MetalRenderFrame((unsigned char*)buffer, width, height, pitch);
}

void PresentFrame() {
    // Present is handled in MetalRenderFrame after drawing
}

void ClearFrame() {
    // Optionally clear the frame buffer if needed
}

void SetVSync(int enabled) {
    // Not implemented: Metal VSync is handled by display link
}

// Toggle fullscreen state
void ToggleFullscreen() {
    // Get current fullscreen state
    if (metalView) {
        NSWindow* window = [metalView window];
        if (window) {
            bool isFullscreen = ([window styleMask] & NSWindowStyleMaskFullScreen) != 0;
            // Toggle to opposite state
            VidMetalSetFullscreen(!isFullscreen);
        }
    }
}

// Enable or disable scanlines
void SetScanlines(int enable, float intensity) {
    shaderOptions.enableScanlines = (enable != 0);
    shaderParams.scanlineIntensity = intensity;
    MetalSetShaderParameters(shaderParams);
    NSLog(@"Scanlines %s, intensity %.2f", enable ? "enabled" : "disabled", intensity);
}

// Enable or disable CRT curvature
void SetCRTCurvature(int enable, float amount) {
    shaderOptions.enableCrtCurvature = (enable != 0);
    shaderParams.crtCurvature = amount;
    MetalSetShaderParameters(shaderParams);
    NSLog(@"CRT curvature %s, amount %.2f", enable ? "enabled" : "disabled", amount);
}

// Enable or disable vignette effect
void SetVignette(int enable, float strength, float smoothness) {
    shaderOptions.enableVignette = (enable != 0);
    shaderParams.vignetteStrength = strength;
    shaderParams.vignetteSmoothness = smoothness;
    MetalSetShaderParameters(shaderParams);
    NSLog(@"Vignette %s, strength %.2f, smoothness %.2f", 
          enable ? "enabled" : "disabled", strength, smoothness);
}

// Enable or disable dynamic resolution scaling
void SetDynamicResolution(int enable) {
    shaderOptions.enableDynamicResolution = (enable != 0);
    shaderParams.dynamicResolution = enable;
    MetalSetShaderParameters(shaderParams);
    NSLog(@"Dynamic resolution %s", enable ? "enabled" : "disabled");
}

// Apply a shader preset
void ApplyShaderPreset(int presetIndex) {
    switch (presetIndex) {
        case 0: // Default - no effects
            shaderOptions.enableScanlines = false;
            shaderOptions.enableCrtCurvature = false;
            shaderOptions.enableVignette = false;
            shaderOptions.enableBloom = false;
            shaderOptions.enablePixelation = false;
            shaderOptions.enableDynamicResolution = true;
            
            shaderParams = kDefaultShaderParams;
            
            NSLog(@"Applied default shader preset");
            break;
            
        case 1: // Arcade CRT
            shaderOptions.enableScanlines = true;
            shaderOptions.enableCrtCurvature = true;
            shaderOptions.enableVignette = true;
            shaderOptions.enableBloom = false;
            shaderOptions.enablePixelation = false;
            shaderOptions.enableDynamicResolution = true;
            
            shaderParams = kDefaultShaderParams;
            shaderParams.scanlineIntensity = 0.3f;
            shaderParams.scanlineWidth = 1.0f;
            shaderParams.crtCurvature = 0.1f;
            shaderParams.vignetteStrength = 0.3f;
            shaderParams.vignetteSmoothness = 0.5f;
            
            NSLog(@"Applied arcade CRT shader preset");
            break;
            
        case 2: // Sharp Pixel
            shaderOptions.enableScanlines = false;
            shaderOptions.enableCrtCurvature = false;
            shaderOptions.enableVignette = false;
            shaderOptions.enableBloom = false;
            shaderOptions.enablePixelation = true;
            shaderOptions.enableDynamicResolution = false;
            
            shaderParams = kDefaultShaderParams;
            
            NSLog(@"Applied sharp pixel shader preset");
            break;
            
        case 3: // Heavy CRT
            shaderOptions.enableScanlines = true;
            shaderOptions.enableCrtCurvature = true;
            shaderOptions.enableVignette = true;
            shaderOptions.enableBloom = true;
            shaderOptions.enablePixelation = false;
            shaderOptions.enableDynamicResolution = true;
            
            shaderParams = kDefaultShaderParams;
            shaderParams.scanlineIntensity = 0.5f;
            shaderParams.scanlineWidth = 0.7f;
            shaderParams.crtCurvature = 0.15f;
            shaderParams.vignetteStrength = 0.4f;
            shaderParams.vignetteSmoothness = 0.3f;
            
            NSLog(@"Applied heavy CRT shader preset");
            break;
    }
    
    // Apply the updated parameters
    MetalSetShaderOptions(shaderOptions);
    MetalSetShaderParameters(shaderParams);
}

// Add a function to set shader options
void MetalSetShaderOptions(ShaderOptions options) {
    shaderOptions = options;
    NSLog(@"Shader options updated");
}

// Add a function to set shader parameters
void MetalSetShaderParameters(ShaderParameters params) {
    shaderParams = params;
    NSLog(@"Shader parameters updated");
    
    // Update post-processing parameters
    postProcessParams.scanlineIntensity = params.scanlineIntensity;
    postProcessParams.scanlineWidth = params.scanlineWidth;
    postProcessParams.scanlineOffset = params.scanlineOffset;
    postProcessParams.crtCurvature = params.crtCurvature;
    postProcessParams.vignetteStrength = params.vignetteStrength;
    postProcessParams.vignetteSmoothness = params.vignetteSmoothness;
    postProcessParams.resolution = params.resolution;
    postProcessParams.screenSize = params.screenSize;
    postProcessParams.dynamicResolution = params.dynamicResolution;
}

} // extern "C"

void MetalInit() {
    NSLog(@"Metal video interface initializing");
    
    // Already initialized
    if (bMetalInitialized) {
        return;
    }
    
    // Get the current view
    metalView = (__bridge NSView*)VidGetWindow();
    
    // Initialize Metal renderer
    if (!InitMetal(metalView)) {
        NSLog(@"Failed to initialize Metal renderer");
        return;
    }
    
    // Get current game size
    INT32 width = 0, height = 0;
    if (bDrvOkay) {
        BurnDrvGetVisibleSize(&width, &height);
    } else {
        width = 320;
        height = 240;
    }
    
    // Set image properties
    nMetalImageWidth = width;
    nMetalImageHeight = height;
    nMetalImageBPP = (nVidImageDepth == 16) ? 2 : 4;
    nMetalImagePitch = nMetalImageWidth * nMetalImageBPP;
    
    // Allocate frame buffer if needed (usually not needed as pBurnDraw is used directly)
    if (pMetalFrameBuffer) {
        free(pMetalFrameBuffer);
    }
    pMetalFrameBuffer = (unsigned char*)malloc(nMetalImageHeight * nMetalImagePitch);
    if (!pMetalFrameBuffer) {
        NSLog(@"Failed to allocate Metal frame buffer");
        return;
    }
    
    // Resize window/view to match game size
    MetalResizeWindow(width, height);
    
    // Set window title
    if (bDrvOkay) {
        const char* title = BurnDrvGetTextA(DRV_FULLNAME);
        if (title) {
            MetalSetWindowTitle(title);
        } else {
            MetalSetWindowTitle("FBNeo Metal");
        }
    } else {
        MetalSetWindowTitle("FBNeo Metal");
    }
    
    bMetalInitialized = true;
    NSLog(@"Metal video interface initialized successfully");
}

void MetalExit() {
    NSLog(@"Metal video interface shutting down");
    
    // Free frame buffer
    if (pMetalFrameBuffer) {
        free(pMetalFrameBuffer);
        pMetalFrameBuffer = NULL;
    }
    
    // Shutdown Metal renderer
    if (bMetalInitialized) {
        ShutdownMetal();
    }
    
    bMetalInitialized = false;
}

void MetalFrame() {
    // Update frame
    // ... (implementation details)
}

void MetalPaint() {
    // Paint frame
    // ... (implementation details)
}

// Implementation of fullscreen toggle
int VidMetalSetFullscreen(bool bFullscreen) {
    @autoreleasepool {
        NSLog(@"Toggling fullscreen: %s", bFullscreen ? "true" : "false");
        
        if (!metalView || !metalLayer) {
            NSLog(@"Error: Cannot toggle fullscreen, Metal view or layer not initialized");
            return 1; // Error
        }
        
        NSWindow* window = [metalView window];
        if (!window) {
            NSLog(@"Error: Cannot toggle fullscreen, window not found");
            return 1; // Error
        }
        
        // Check if we need to change the fullscreen state
        bool isCurrentlyFullscreen = ([window styleMask] & NSWindowStyleMaskFullScreen) != 0;
        if (isCurrentlyFullscreen == bFullscreen) {
            NSLog(@"Window is already in the requested fullscreen state");
            return 0; // Success - already in requested state
        }
        
        // Store original window properties before transition
        NSRect originalFrame = [window frame];
        NSRect originalViewFrame = [metalView frame];
        
        // Toggle fullscreen state using native macOS fullscreen API
        [window toggleFullScreen:nil];
        
        // Post-toggle adjustments
        // These will be applied after the animation completes
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.5 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
            // Get updated dimensions
            NSRect newFrame = [metalView frame];
            
            // Update Metal layer to match the new view size
            metalLayer.frame = NSMakeRect(0, 0, newFrame.size.width, newFrame.size.height);
            metalLayer.drawableSize = CGSizeMake(newFrame.size.width, newFrame.size.height);
            
            // Update post-processing parameters for the new screen size
            postProcessParams.screenSize = simd_make_float2(newFrame.size.width, newFrame.size.height);
            
            // Calculate and maintain proper aspect ratio if entering fullscreen
            if (bFullscreen) {
                // Calculate aspect ratio of game
                float gameAspect = (float)gameWidth / (float)gameHeight;
                
                // Calculate aspect ratio of screen
                float screenAspect = newFrame.size.width / newFrame.size.height;
                
                // Adjust parameters if needed
                if (fabs(gameAspect - screenAspect) > 0.01f) {
                    NSLog(@"Adjusting aspect ratio: Game = %.3f, Screen = %.3f", 
                          gameAspect, screenAspect);
                          
                    // Update shader parameters to maintain proper scaling
                    if (gameAspect > screenAspect) {
                        // Game is wider than screen - letterbox (black bars top/bottom)
                        float scale = newFrame.size.width / gameWidth;
                        float scaledHeight = gameHeight * scale;
                        float yOffset = (newFrame.size.height - scaledHeight) / 2.0f;
                        
                        NSLog(@"Letterbox mode: Scale = %.3f, Offset Y = %.1f", scale, yOffset);
                        
                        // In a real implementation, you would update shader uniforms
                        // to handle letterboxing - here we're just logging for now
                    } else {
                        // Game is narrower than screen - pillarbox (black bars left/right)
                        float scale = newFrame.size.height / gameHeight;
                        float scaledWidth = gameWidth * scale;
                        float xOffset = (newFrame.size.width - scaledWidth) / 2.0f;
                        
                        NSLog(@"Pillarbox mode: Scale = %.3f, Offset X = %.1f", scale, xOffset);
                        
                        // In a real implementation, you would update shader uniforms
                        // to handle pillarboxing - here we're just logging for now
                    }
                }
            }
            
            NSLog(@"Fullscreen toggled successfully. New size: %.0f x %.0f", 
                  newFrame.size.width, newFrame.size.height);
        });
        
        return 0; // Success
    }
}

// Add proper fullscreen support to a metal window
void Metal_VideoToggleFullscreen() {
    // This is the main entry point for fullscreen toggling that can be called from external code
    // Get current window fullscreen state
    if (metalView) {
        NSWindow* window = [metalView window];
        if (window) {
            bool isFullscreen = ([window styleMask] & NSWindowStyleMaskFullScreen) != 0;
            // Toggle to opposite state
            VidMetalSetFullscreen(!isFullscreen);
        } else {
            NSLog(@"Cannot toggle fullscreen: No window found");
        }
    } else {
        NSLog(@"Cannot toggle fullscreen: No Metal view found");
    }
}

// Interface info implementation
void InterfaceInfo::AddSlider(const char* name, float* value, float min, float max, float step) {
    // Implementation for adding sliders
}

void InterfaceInfo::AddToggle(const char* name, bool* value) {
    // Implementation for adding toggles
}

// Export the Metal interface
struct VidOutMetal VidOutMetal = {
    MetalInit,
    MetalExit,
    MetalFrame,
    MetalPaint
}; 