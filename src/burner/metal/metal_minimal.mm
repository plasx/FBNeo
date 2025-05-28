#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <simd/simd.h>
#include "metal_renderer.h"
#include "metal_declarations.h"
#include "metal_bridge.h"

// Metal renderer globals
static id<MTLDevice> device = nil;
static id<MTLCommandQueue> commandQueue = nil;
static id<MTLLibrary> defaultLibrary = nil;
static id<MTLRenderPipelineState> pipelineState = nil;
static id<MTLSamplerState> samplerState = nil;
static MTKView* metalView = nil;
static id<MTLTexture> frameTexture = nil;
static id<MTLBuffer> vertexBuffer = nil;
static id<MTLBuffer> uniformBuffer = nil;

// Rendering settings
static bool useVSync = true;
static bool useBilinearFilter = true;
static bool useScanlines = false;
static float scanlineIntensity = 0.0f;
static bool useCRTEffect = false;
static float crtCurvature = 0.1f;

// Current texture dimensions
static int currentWidth = 640;
static int currentHeight = 480;

// Vertex structure
typedef struct {
    vector_float2 position;
    vector_float2 texCoord;
} Vertex;

// Shader uniform data
typedef struct {
    float scanlineIntensity;
    float crtCurvature;
    float textureWidth;
    float textureHeight;
    float time;
    float padding[3]; // For alignment
} ShaderUniforms;

// Create vertex buffer with a quad covering the full screen
static bool createVertexBuffer() {
    const Vertex vertices[] = {
        // Position                  Texture Coords
        { {-1.0f, -1.0f},            {0.0f, 1.0f} },    // Bottom left
        { { 1.0f, -1.0f},            {1.0f, 1.0f} },    // Bottom right
        { { 1.0f,  1.0f},            {1.0f, 0.0f} },    // Top right
        
        { {-1.0f, -1.0f},            {0.0f, 1.0f} },    // Bottom left
        { { 1.0f,  1.0f},            {1.0f, 0.0f} },    // Top right
        { {-1.0f,  1.0f},            {0.0f, 0.0f} }     // Top left
    };
    
    vertexBuffer = [device newBufferWithBytes:vertices
                                       length:sizeof(vertices)
                                      options:MTLResourceStorageModeShared];
    
    return vertexBuffer != nil;
}

// Create uniform buffer for shader parameters
static bool createUniformBuffer() {
    uniformBuffer = [device newBufferWithLength:sizeof(ShaderUniforms)
                                        options:MTLResourceStorageModeShared];
    
    // Initialize uniform values
    ShaderUniforms* uniforms = (ShaderUniforms*)[uniformBuffer contents];
    uniforms->scanlineIntensity = useScanlines ? scanlineIntensity : 0.0f;
    uniforms->crtCurvature = useCRTEffect ? crtCurvature : 0.0f;
    uniforms->textureWidth = (float)currentWidth;
    uniforms->textureHeight = (float)currentHeight;
    uniforms->time = 0.0f;
    
    return uniformBuffer != nil;
}

// Create frame texture with given dimensions
static bool createTexture(int width, int height) {
    if (width <= 0 || height <= 0) {
        return false;
    }
    
    // Store current texture dimensions
    currentWidth = width;
    currentHeight = height;
    
    // Create texture descriptor
    MTLTextureDescriptor* textureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                                                          width:width
                                                                                         height:height
                                                                                      mipmapped:NO];
    textureDesc.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
    
    // Use the most efficient storage mode for the current device
    if ([device hasUnifiedMemory]) {
        textureDesc.storageMode = MTLStorageModeShared;
    } else {
        textureDesc.storageMode = MTLStorageModePrivate;
    }
    
    // Create the texture
    frameTexture = [device newTextureWithDescriptor:textureDesc];
    
    // If creation failed with private storage, try shared
    if (!frameTexture && textureDesc.storageMode == MTLStorageModePrivate) {
        textureDesc.storageMode = MTLStorageModeShared;
        frameTexture = [device newTextureWithDescriptor:textureDesc];
    }
    
    // Update uniform buffer with new dimensions
    if (uniformBuffer) {
        ShaderUniforms* uniforms = (ShaderUniforms*)[uniformBuffer contents];
        uniforms->textureWidth = (float)width;
        uniforms->textureHeight = (float)height;
    }
    
    return frameTexture != nil;
}

// Create texture sampler
static bool createSamplerState() {
    MTLSamplerDescriptor* samplerDesc = [[MTLSamplerDescriptor alloc] init];
    samplerDesc.minFilter = useBilinearFilter ? MTLSamplerMinMagFilterLinear : MTLSamplerMinMagFilterNearest;
    samplerDesc.magFilter = useBilinearFilter ? MTLSamplerMinMagFilterLinear : MTLSamplerMinMagFilterNearest;
    samplerDesc.sAddressMode = MTLSamplerAddressModeClampToEdge;
    samplerDesc.tAddressMode = MTLSamplerAddressModeClampToEdge;
    
    samplerState = [device newSamplerStateWithDescriptor:samplerDesc];
    
    return samplerState != nil;
}

// Create render pipeline state
static bool createPipelineState() {
    // If no shader library yet, load defaults from Metal framework
    if (!defaultLibrary) {
        NSError* libraryError = nil;
        
        // Try to load compiled metallib first
        NSString* metalLibPath = [[NSBundle mainBundle] pathForResource:@"default" ofType:@"metallib"];
        if (metalLibPath) {
            defaultLibrary = [device newLibraryWithFile:metalLibPath error:&libraryError];
        }
        
        // If that fails, try bundled shaders
        if (!defaultLibrary) {
            defaultLibrary = [device newDefaultLibrary];
        }
        
        // If still no library, create a basic library with inline shaders
        if (!defaultLibrary) {
            NSString* shaderSource = @"#include <metal_stdlib>\n"
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
                                     "    float scanlineIntensity;\n"
                                     "    float crtCurvature;\n"
                                     "    float textureWidth;\n"
                                     "    float textureHeight;\n"
                                     "    float time;\n"
                                     "    float3 padding;\n"
                                     "};\n"
                                     "\n"
                                     "vertex VertexOut vertexShader(uint vertexID [[vertex_id]],\n"
                                     "                              constant VertexIn* vertices [[buffer(0)]]) {\n"
                                     "    VertexOut out;\n"
                                     "    out.position = float4(vertices[vertexID].position, 0.0, 1.0);\n"
                                     "    out.texCoord = vertices[vertexID].texCoord;\n"
                                     "    return out;\n"
                                     "}\n"
                                     "\n"
                                     "fragment float4 fragmentShader(VertexOut in [[stage_in]],\n"
                                     "                              texture2d<float> tex [[texture(0)]],\n"
                                     "                              constant Uniforms& uniforms [[buffer(0)]],\n"
                                     "                              sampler texSampler [[sampler(0)]]) {\n"
                                     "    float2 uv = in.texCoord;\n"
                                     "    float4 color = tex.sample(texSampler, uv);\n"
                                     "    \n"
                                     "    // Apply scanlines if enabled\n"
                                     "    if (uniforms.scanlineIntensity > 0.0) {\n"
                                     "        float scanline = sin(uv.y * uniforms.textureHeight * 2.0) * 0.5 + 0.5;\n"
                                     "        color.rgb *= mix(1.0, scanline, uniforms.scanlineIntensity);\n"
                                     "    }\n"
                                     "    \n"
                                     "    return color;\n"
                                     "}";
            
            defaultLibrary = [device newLibraryWithSource:shaderSource options:nil error:&libraryError];
            
            if (!defaultLibrary) {
                NSLog(@"Failed to create shader library: %@", libraryError);
                return false;
            }
        }
    }
    
    // Get shader functions
    id<MTLFunction> vertexFunction = [defaultLibrary newFunctionWithName:@"vertexShader"];
    id<MTLFunction> fragmentFunction = [defaultLibrary newFunctionWithName:@"fragmentShader"];
    
    if (!vertexFunction || !fragmentFunction) {
        NSLog(@"Failed to find shader functions");
        return false;
    }
    
    // Create render pipeline descriptor
    MTLRenderPipelineDescriptor* pipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDesc.vertexFunction = vertexFunction;
    pipelineDesc.fragmentFunction = fragmentFunction;
    pipelineDesc.colorAttachments[0].pixelFormat = metalView.colorPixelFormat;
    
    // Create pipeline state
    NSError* pipelineError = nil;
    pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineDesc error:&pipelineError];
    
    if (!pipelineState) {
        NSLog(@"Failed to create pipeline state: %@", pipelineError);
        return false;
    }
    
    return true;
}

// Initialize Metal renderer with view
int MetalRenderer_Init(void* view) {
    // Make sure we have a valid view
    if (!view) {
        NSLog(@"Metal: Invalid view pointer");
        return 1;
    }
    
    // Cast to MTKView
    metalView = (__bridge MTKView*)view;
    
    // Get Metal device from view
    device = metalView.device;
    if (!device) {
        NSLog(@"Metal: No Metal device available");
        return 2;
    }
    
    // Create command queue
    commandQueue = [device newCommandQueue];
    if (!commandQueue) {
        NSLog(@"Metal: Failed to create command queue");
        return 3;
    }
    
    // Configure view
    metalView.framebufferOnly = YES;
    metalView.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
    
    // Set vsync preference on the Metal layer
    if ([metalView.layer isKindOfClass:[CAMetalLayer class]]) {
        CAMetalLayer* layer = (CAMetalLayer*)metalView.layer;
        layer.displaySyncEnabled = useVSync;
    }
    
    // Create resources
    if (!createVertexBuffer()) {
        NSLog(@"Metal: Failed to create vertex buffer");
        return 4;
    }
    
    if (!createUniformBuffer()) {
        NSLog(@"Metal: Failed to create uniform buffer");
        return 5;
    }
    
    // Create frame texture with initial dimensions
    if (!createTexture(currentWidth, currentHeight)) {
        NSLog(@"Metal: Failed to create texture");
        return 6;
    }
    
    // Create sampler state
    if (!createSamplerState()) {
        NSLog(@"Metal: Failed to create sampler state");
        return 7;
    }
    
    // Create pipeline state
    if (!createPipelineState()) {
        NSLog(@"Metal: Failed to create pipeline state");
        return 8;
    }
    
    NSLog(@"Metal: Renderer initialized successfully on %@", device.name);
    return 0;
}

// Shut down Metal renderer
void MetalRenderer_Shutdown(void) {
    NSLog(@"Metal: Shutting down renderer");
    
    // Release all Metal resources
    vertexBuffer = nil;
    uniformBuffer = nil;
    frameTexture = nil;
    samplerState = nil;
    pipelineState = nil;
    defaultLibrary = nil;
    commandQueue = nil;
    
    // Reset view delegate and view
    metalView = nil;
    
    // Reset device reference
    device = nil;
}

// Update frame with new data
int MetalRenderer_UpdateFrame(const void* frameData, int width, int height, int pitch) {
    if (!frameData || width <= 0 || height <= 0 || pitch <= 0) {
        return 1;
    }
    
    // Recreate texture if dimensions changed
    if (width != currentWidth || height != currentHeight) {
        if (!createTexture(width, height)) {
            NSLog(@"Metal: Failed to resize texture to %dx%d", width, height);
            return 2;
        }
    }
    
    // Update the texture with new data
    if (frameTexture) {
        MTLRegion region = MTLRegionMake2D(0, 0, width, height);
        
        if (frameTexture.storageMode == MTLStorageModePrivate) {
            // For discrete GPUs, use a staging buffer and blit encoder
            id<MTLBuffer> stagingBuffer = [device newBufferWithBytes:frameData
                                                             length:height * pitch
                                                            options:MTLResourceStorageModeShared];
            
            id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
            id<MTLBlitCommandEncoder> blitEncoder = [commandBuffer blitCommandEncoder];
            
            [blitEncoder copyFromBuffer:stagingBuffer
                          sourceOffset:0
                     sourceBytesPerRow:pitch
                   sourceBytesPerImage:height * pitch
                            sourceSize:MTLSizeMake(width, height, 1)
                             toTexture:frameTexture
                      destinationSlice:0
                      destinationLevel:0
                     destinationOrigin:MTLOriginMake(0, 0, 0)];
            
            [blitEncoder endEncoding];
            [commandBuffer commit];
            [commandBuffer waitUntilCompleted];
        } else {
            // For unified memory, we can update directly
            [frameTexture replaceRegion:region
                          mipmapLevel:0
                            withBytes:frameData
                          bytesPerRow:pitch];
        }
        
        // Force a redraw if view delegate exists
        if (metalView && !metalView.paused) {
            [metalView setNeedsDisplay:YES];
        }
        
        return 0;
    }
    
    return 3;
}

// Draw current frame - can be called manually to force redraw
void MetalRenderer_Draw(void) {
    if (metalView && !metalView.paused) {
        [metalView draw];
    }
}

// Resize the renderer
int MetalRenderer_Resize(int width, int height) {
    if (width <= 0 || height <= 0) {
        NSLog(@"Invalid resize dimensions: %dx%d", width, height);
        return 1;
    }
    
    // Create new texture with updated dimensions
    if (!createTexture(width, height)) {
        NSLog(@"Failed to resize texture to %dx%d", width, height);
        return 1;
    }
    
    // Update uniform buffer with new dimensions
    if (uniformBuffer) {
        ShaderUniforms* uniforms = (ShaderUniforms*)[uniformBuffer contents];
        uniforms->textureWidth = (float)width;
        uniforms->textureHeight = (float)height;
    }
    
    NSLog(@"Metal renderer resized to %dx%d", width, height);
    return 0;
}

// Set VSync mode
void MetalRenderer_SetVSync(int enabled) {
    useVSync = enabled != 0;
    
    // Update Metal layer if available
    if ([metalView.layer isKindOfClass:[CAMetalLayer class]]) {
        CAMetalLayer* layer = (CAMetalLayer*)metalView.layer;
        layer.displaySyncEnabled = useVSync;
    }
}

// Set texture filtering mode
void MetalRenderer_SetFilter(int useBilinear) {
    useBilinearFilter = useBilinear != 0;
    
    // Recreate sampler state with new filter setting
    createSamplerState();
}

// Set scanline effect
void MetalRenderer_SetScanlines(int enabled, float intensity) {
    useScanlines = enabled != 0;
    scanlineIntensity = intensity;
    
    // Update uniform buffer
    if (uniformBuffer) {
        ShaderUniforms* uniforms = (ShaderUniforms*)[uniformBuffer contents];
        uniforms->scanlineIntensity = useScanlines ? scanlineIntensity : 0.0f;
    }
}

// Set CRT effect
void MetalRenderer_SetCRTEffect(int enabled, float curvature) {
    useCRTEffect = enabled != 0;
    crtCurvature = curvature;
    
    // Update uniform buffer
    if (uniformBuffer) {
        ShaderUniforms* uniforms = (ShaderUniforms*)[uniformBuffer contents];
        uniforms->crtCurvature = useCRTEffect ? crtCurvature : 0.0f;
    }
}

// Get renderer info
const char* MetalRenderer_GetInfo(void) {
    static char info[256];
    if (device) {
        snprintf(info, sizeof(info), "Metal: %s, %dx%d, %s", 
                 device.name.UTF8String,
                 currentWidth, currentHeight, 
                 [device hasUnifiedMemory] ? "Unified Memory" : "Discrete GPU");
    } else {
        snprintf(info, sizeof(info), "Metal: Not initialized");
    }
    return info;
}

// Get current width
int MetalRenderer_GetWidth(void) {
    return currentWidth;
}

// Get current height
int MetalRenderer_GetHeight(void) {
    return currentHeight;
}

// Toggle fullscreen mode
void MetalRenderer_ToggleFullscreen(void) {
    // In a real implementation, this would toggle fullscreen mode
    // For now, just log
    NSLog(@"Metal: Toggle fullscreen not implemented");
}

// Render to a command buffer
void MetalRenderer_Render(void* commandBuffer) {
    // In a real implementation, this would render to the provided command buffer
    // For now, just log
    NSLog(@"Metal: Direct render not implemented");
}

// Draw next frame
void MetalRenderer_DrawNextFrame(void) {
    // In a real implementation, this would update and draw the next frame
    // For now, just call Draw
    MetalRenderer_Draw();
}

// Set scale
void MetalRenderer_SetScale(float scale) {
    // In a real implementation, this would set the rendering scale
    // For now, just log
    NSLog(@"Metal: Set scale %f not implemented", scale);
}

// Set core rendering
void MetalRenderer_SetUseCoreRendering(bool useCoreRendering) {
    // In a real implementation, this would enable/disable core rendering
    // For now, just log
    NSLog(@"Metal: Set core rendering %d not implemented", useCoreRendering);
}

// Get core rendering state
bool MetalRenderer_GetUseCoreRendering(void) {
    // In a real implementation, this would return the core rendering state
    // For now, return true
    return true;
}

// Set continuous rendering
void MetalRenderer_SetContinuousRendering(bool continuous) {
    // In a real implementation, this would enable/disable continuous rendering
    // For now, just update the view's paused state
    if (metalView) {
        metalView.paused = !continuous;
    }
}

// Set scaling mode
int MetalRenderer_SetScalingMode(int mode) {
    // In a real implementation, this would set the scaling mode
    // For now, just log
    NSLog(@"Metal: Set scaling mode %d not implemented", mode);
    return 0;
}

// Set aspect ratio
int MetalRenderer_SetAspectRatio(int mode) {
    // In a real implementation, this would set the aspect ratio mode
    // For now, just log
    NSLog(@"Metal: Set aspect ratio %d not implemented", mode);
    return 0;
}

// Take screenshot
void MetalRenderer_TakeScreenshot(const char* filename) {
    // In a real implementation, this would save a screenshot
    // For now, just log
    NSLog(@"Metal: Screenshot not implemented");
}

// C wrapper functions for Metal texture updates
int Metal_UpdateTexture(void* data, int width, int height, int pitch) {
    return MetalRenderer_UpdateFrame(data, width, height, pitch);
} 