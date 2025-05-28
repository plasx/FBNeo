#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <QuartzCore/CAMetalLayer.h>
#import "metal_interface.h"

// Metal renderer implementation for FBNeo
// This file implements the Metal rendering interface for the FBNeo Metal backend

// Max frames in flight (for triple buffering)
#define MAX_FRAMES_IN_FLIGHT 3

// Uniform buffer structure (matches shader definition)
typedef struct {
    simd_float4x4 projectionMatrix;
    simd_float4 tint;
    float time;
    float scanlineIntensity;
    float crtCurvature;
    float sharpenAmount;
} Uniforms;

// Vertex structure
typedef struct {
    simd_float2 position;
    simd_float2 texCoord;
} Vertex;

// Private Metal state
static id<MTLDevice> device = nil;
static id<MTLCommandQueue> commandQueue = nil;
static id<MTLLibrary> defaultLibrary = nil;
static id<MTLRenderPipelineState> pipelineState = nil;
static id<MTLTexture> gameTexture = nil;
static id<MTLTexture> aiTexture = nil;
static id<MTLSamplerState> samplerState = nil;
static id<MTLBuffer> vertexBuffer = nil;
static id<MTLBuffer> indexBuffer = nil;
static id<MTLBuffer> uniformBuffer = nil;

static CAMetalLayer* metalLayer = nil;
static NSUInteger currentFrameIndex = 0;
static dispatch_semaphore_t frameBoundarySemaphore;

static MTLRenderPassDescriptor* renderPassDescriptor = nil;
static id<MTLDepthStencilState> depthStencilState = nil;

static MetalRendererConfig config = {0};
static MetalShaderType currentShaderType = ShaderTypeBasic;
static NSTimeInterval startTime = 0;

static Vertex quadVertices[4] = {
    // Position              // TexCoord
    { {-1.0, -1.0},          {0.0, 1.0} },
    { {-1.0,  1.0},          {0.0, 0.0} },
    { { 1.0, -1.0},          {1.0, 1.0} },
    { { 1.0,  1.0},          {1.0, 0.0} }
};

static uint16_t indices[6] = {
    0, 1, 2,
    2, 1, 3
};

// Forward declarations
static bool setupPipeline();
static bool setupBuffers();
static bool createTexture(unsigned char* buffer, int width, int height, int pitch, id<MTLTexture>* texture);
static id<MTLRenderPipelineState> createPipelineState(MetalShaderType shaderType);

// Initialize Metal renderer
bool Metal_InitRenderer(void* viewPtr, int width, int height) {
    NSView* view = (__bridge NSView*)viewPtr;
    
    // Set up default configuration
    config.width = width;
    config.height = height;
    config.scanlineIntensity = 0.5f;
    config.crtCurvature = 0.1f;
    config.sharpenAmount = 0.2f;
    config.useVSync = true;
    config.useTripleBuffering = true;
    config.gameAspectRatio = 4.0f / 3.0f;
    config.preserveAspectRatio = true;
    config.useIntegerScaling = false;
    config.renderMode = RenderModeStandard;
    
    // Create Metal device
    device = MTLCreateSystemDefaultDevice();
    if (!device) {
        NSLog(@"Metal is not supported on this device");
        return false;
    }
    
    // Create command queue
    commandQueue = [device newCommandQueue];
    if (!commandQueue) {
        NSLog(@"Failed to create command queue");
        return false;
    }
    
    // Create Metal layer for view
    metalLayer = [CAMetalLayer layer];
    metalLayer.device = device;
    metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    metalLayer.framebufferOnly = YES;
    metalLayer.frame = view.bounds;
    
    if (config.useVSync) {
        metalLayer.displaySyncEnabled = YES;
    } else {
        metalLayer.displaySyncEnabled = NO;
    }
    
    [view setWantsLayer:YES];
    view.layer = metalLayer;
    
    // Create semaphore for triple buffering
    frameBoundarySemaphore = dispatch_semaphore_create(MAX_FRAMES_IN_FLIGHT);
    
    // Load default library with our shaders
    NSString* path = [[NSBundle mainBundle] pathForResource:@"Shaders" ofType:@"metallib"];
    if (path) {
        NSError* error = nil;
        defaultLibrary = [device newLibraryWithFile:path error:&error];
        if (!defaultLibrary) {
            NSLog(@"Failed to load Metal shader library: %@", error);
            return false;
        }
    } else {
        // Fallback to default library
        defaultLibrary = [device newDefaultLibrary];
        if (!defaultLibrary) {
            NSLog(@"Failed to load default Metal library");
            return false;
        }
    }
    
    // Create pipeline and buffers
    if (!setupPipeline() || !setupBuffers()) {
        return false;
    }
    
    // Set up render pass descriptor
    renderPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
    renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
    renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
    renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
    
    // Create depth stencil state
    MTLDepthStencilDescriptor* depthStencilDesc = [[MTLDepthStencilDescriptor alloc] init];
    depthStencilDesc.depthCompareFunction = MTLCompareFunctionAlways;
    depthStencilDesc.depthWriteEnabled = NO;
    depthStencilState = [device newDepthStencilStateWithDescriptor:depthStencilDesc];
    
    // Record start time for animated effects
    startTime = CACurrentMediaTime();
    
    return true;
}

// Set up Metal pipeline
static bool setupPipeline() {
    // Create initial pipeline state
    pipelineState = createPipelineState(currentShaderType);
    if (!pipelineState) {
        return false;
    }
    
    // Create sampler state
    MTLSamplerDescriptor* samplerDesc = [[MTLSamplerDescriptor alloc] init];
    samplerDesc.minFilter = MTLSamplerMinMagFilterLinear;
    samplerDesc.magFilter = MTLSamplerMinMagFilterLinear;
    samplerDesc.sAddressMode = MTLSamplerAddressModeClampToEdge;
    samplerDesc.tAddressMode = MTLSamplerAddressModeClampToEdge;
    samplerDesc.rAddressMode = MTLSamplerAddressModeClampToEdge;
    samplerDesc.normalizedCoordinates = YES;
    samplerState = [device newSamplerStateWithDescriptor:samplerDesc];
    
    return true;
}

// Create pipeline state for shader type
static id<MTLRenderPipelineState> createPipelineState(MetalShaderType shaderType) {
    // Set up render pipeline
    MTLRenderPipelineDescriptor* pipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
    
    // Get vertex shader function
    id<MTLFunction> vertexFunction = [defaultLibrary newFunctionWithName:@"vertexShader"];
    if (!vertexFunction) {
        NSLog(@"Failed to load vertex function");
        return nil;
    }
    
    // Get fragment shader function based on shader type
    NSString* fragmentFunctionName = nil;
    switch (shaderType) {
        case ShaderTypeBasic:
            fragmentFunctionName = @"fragmentShader";
            break;
        case ShaderTypeCRT:
            fragmentFunctionName = @"crtFragmentShader";
            break;
        case ShaderTypePixelPerfect:
            fragmentFunctionName = @"pixelPerfectShader";
            break;
        case ShaderTypeAIEnhanced:
            fragmentFunctionName = @"aiEnhancedShader";
            break;
        default:
            fragmentFunctionName = @"fragmentShader";
    }
    
    id<MTLFunction> fragmentFunction = [defaultLibrary newFunctionWithName:fragmentFunctionName];
    if (!fragmentFunction) {
        NSLog(@"Failed to load fragment function: %@", fragmentFunctionName);
        return nil;
    }
    
    // Set up vertex descriptor
    MTLVertexDescriptor* vertexDesc = [MTLVertexDescriptor vertexDescriptor];
    
    // Position attribute
    vertexDesc.attributes[0].format = MTLVertexFormatFloat2;
    vertexDesc.attributes[0].offset = 0;
    vertexDesc.attributes[0].bufferIndex = 0;
    
    // Texture coordinate attribute
    vertexDesc.attributes[1].format = MTLVertexFormatFloat2;
    vertexDesc.attributes[1].offset = sizeof(simd_float2);
    vertexDesc.attributes[1].bufferIndex = 0;
    
    // Buffer layout
    vertexDesc.layouts[0].stride = sizeof(Vertex);
    vertexDesc.layouts[0].stepRate = 1;
    vertexDesc.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
    
    // Configure pipeline
    pipelineDesc.vertexFunction = vertexFunction;
    pipelineDesc.fragmentFunction = fragmentFunction;
    pipelineDesc.vertexDescriptor = vertexDesc;
    pipelineDesc.colorAttachments[0].pixelFormat = metalLayer.pixelFormat;
    pipelineDesc.colorAttachments[0].blendingEnabled = YES;
    pipelineDesc.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
    pipelineDesc.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
    pipelineDesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
    pipelineDesc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
    pipelineDesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    pipelineDesc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    
    // Create pipeline state
    NSError* error = nil;
    id<MTLRenderPipelineState> newPipelineState = [device newRenderPipelineStateWithDescriptor:pipelineDesc error:&error];
    if (!newPipelineState) {
        NSLog(@"Failed to create pipeline state: %@", error);
        return nil;
    }
    
    return newPipelineState;
}

// Set up buffers
static bool setupBuffers() {
    // Create vertex buffer
    vertexBuffer = [device newBufferWithBytes:quadVertices
                                      length:sizeof(quadVertices)
                                     options:MTLResourceStorageModeShared];
    
    // Create index buffer
    indexBuffer = [device newBufferWithBytes:indices
                                     length:sizeof(indices)
                                    options:MTLResourceStorageModeShared];
    
    // Create uniform buffer (triple buffered)
    uniformBuffer = [device newBufferWithLength:sizeof(Uniforms) * MAX_FRAMES_IN_FLIGHT
                                       options:MTLResourceStorageModeShared];
    
    return (vertexBuffer != nil && indexBuffer != nil && uniformBuffer != nil);
}

// Shut down Metal renderer
void Metal_ShutdownRenderer() {
    device = nil;
    commandQueue = nil;
    defaultLibrary = nil;
    pipelineState = nil;
    gameTexture = nil;
    aiTexture = nil;
    samplerState = nil;
    vertexBuffer = nil;
    indexBuffer = nil;
    uniformBuffer = nil;
    metalLayer = nil;
    renderPassDescriptor = nil;
    depthStencilState = nil;
}

// Reset the renderer
void Metal_ResetRenderer() {
    Metal_ShutdownRenderer();
    // Would re-create everything here for a real implementation
}

// Set renderer configuration
void Metal_SetRendererConfig(MetalRendererConfig* newConfig) {
    if (newConfig) {
        config = *newConfig;
        
        // Update metal layer vsync setting
        if (metalLayer) {
            metalLayer.displaySyncEnabled = config.useVSync;
        }
        
        // Update shader type based on render mode
        switch (config.renderMode) {
            case RenderModeStandard:
                Metal_SetShaderType(ShaderTypeBasic);
                break;
            case RenderModeScanlines:
            case RenderModeCRT:
                Metal_SetShaderType(ShaderTypeCRT);
                break;
            case RenderModePixelPerfect:
                Metal_SetShaderType(ShaderTypePixelPerfect);
                break;
            case RenderModeAIEnhanced:
                Metal_SetShaderType(ShaderTypeAIEnhanced);
                break;
        }
    }
}

// Get renderer configuration
void Metal_GetRendererConfig(MetalRendererConfig* outConfig) {
    if (outConfig) {
        *outConfig = config;
    }
}

// Update a game frame
bool Metal_UpdateFrame(unsigned char* buffer, int width, int height, int pitch) {
    if (!buffer || width <= 0 || height <= 0) {
        return false;
    }
    
    // Create or update the game texture
    return createTexture(buffer, width, height, pitch, &gameTexture);
}

// Load AI-enhanced texture
bool Metal_LoadAIEnhancedTexture(unsigned char* buffer, int width, int height, int pitch) {
    if (!buffer || width <= 0 || height <= 0) {
        return false;
    }
    
    // Create or update the AI texture
    return createTexture(buffer, width, height, pitch, &aiTexture);
}

// Create or update a texture
static bool createTexture(unsigned char* buffer, int width, int height, int pitch, id<MTLTexture>* texture) {
    if (!buffer || !texture) {
        return false;
    }
    
    // Create texture descriptor
    MTLTextureDescriptor *textureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                                                          width:width
                                                                                         height:height
                                                                                      mipmapped:NO];
    textureDesc.usage = MTLTextureUsageShaderRead;
    
    // Create texture if it doesn't exist or if dimensions have changed
    if (*texture == nil || 
        (*texture).width != width || 
        (*texture).height != height) {
        *texture = [device newTextureWithDescriptor:textureDesc];
        if (*texture == nil) {
            NSLog(@"Failed to create texture");
            return false;
        }
    }
    
    // Region for the entire texture
    MTLRegion region = MTLRegionMake2D(0, 0, width, height);
    
    // Copy data into texture
    [*texture replaceRegion:region 
               mipmapLevel:0 
                 withBytes:buffer 
               bytesPerRow:pitch];
    
    return true;
}

// Set current shader type
void Metal_SetShaderType(MetalShaderType shaderType) {
    if (shaderType == currentShaderType) {
        return;
    }
    
    currentShaderType = shaderType;
    
    // Create new pipeline state for shader type
    id<MTLRenderPipelineState> newPipelineState = createPipelineState(shaderType);
    if (newPipelineState) {
        pipelineState = newPipelineState;
    }
}

// Get current shader type
MetalShaderType Metal_GetShaderType() {
    return currentShaderType;
}

// Set render mode
void Metal_SetRenderMode(MetalRenderMode renderMode) {
    config.renderMode = renderMode;
    
    // Update shader type based on render mode
    switch (renderMode) {
        case RenderModeStandard:
            Metal_SetShaderType(ShaderTypeBasic);
            break;
        case RenderModeScanlines:
        case RenderModeCRT:
            Metal_SetShaderType(ShaderTypeCRT);
            break;
        case RenderModePixelPerfect:
            Metal_SetShaderType(ShaderTypePixelPerfect);
            break;
        case RenderModeAIEnhanced:
            Metal_SetShaderType(ShaderTypeAIEnhanced);
            break;
    }
}

// Get render mode
MetalRenderMode Metal_GetRenderMode() {
    return config.renderMode;
}

// Render the current frame
void Metal_RenderFrame() {
    // Check if layer and texture are ready
    if (metalLayer == nil || gameTexture == nil || pipelineState == nil) {
        return;
    }
    
    // Wait for a drawable
    id<CAMetalDrawable> drawable = [metalLayer nextDrawable];
    if (drawable == nil) {
        return;
    }
    
    // Wait for the next command buffer to be available
    dispatch_semaphore_wait(frameBoundarySemaphore, DISPATCH_TIME_FOREVER);
    
    // Update uniform buffer
    Uniforms* uniforms = (Uniforms*)((uint8_t*)[uniformBuffer contents] + sizeof(Uniforms) * currentFrameIndex);
    
    // Set up projection matrix (identity for full-screen quad)
    uniforms->projectionMatrix = matrix_identity_float4x4;
    
    // Set tint color (white = no tint)
    uniforms->tint = (simd_float4){1.0f, 1.0f, 1.0f, 1.0f};
    
    // Set time for animated effects
    uniforms->time = CACurrentMediaTime() - startTime;
    
    // Set effect parameters based on render mode
    switch (config.renderMode) {
        case RenderModeStandard:
            uniforms->scanlineIntensity = 0.0f;
            uniforms->crtCurvature = 0.0f;
            uniforms->sharpenAmount = 0.0f;
            break;
        case RenderModeScanlines:
            uniforms->scanlineIntensity = config.scanlineIntensity;
            uniforms->crtCurvature = 0.0f;
            uniforms->sharpenAmount = 0.2f;
            break;
        case RenderModeCRT:
            uniforms->scanlineIntensity = config.scanlineIntensity;
            uniforms->crtCurvature = config.crtCurvature;
            uniforms->sharpenAmount = 0.1f;
            break;
        case RenderModePixelPerfect:
            uniforms->scanlineIntensity = 0.0f;
            uniforms->crtCurvature = 0.0f;
            uniforms->sharpenAmount = 0.0f;
            break;
        case RenderModeAIEnhanced:
            uniforms->scanlineIntensity = 0.0f;
            uniforms->crtCurvature = 0.0f;
            uniforms->sharpenAmount = 0.0f;
            break;
    }
    
    // Set up render pass descriptor
    renderPassDescriptor.colorAttachments[0].texture = drawable.texture;
    
    // Create command buffer
    id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
    
    // Set completion handler
    __block dispatch_semaphore_t blockSemaphore = frameBoundarySemaphore;
    [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> _Nonnull buffer) {
        dispatch_semaphore_signal(blockSemaphore);
    }];
    
    // Create render command encoder
    id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
    [renderEncoder setLabel:@"Game Render Encoder"];
    [renderEncoder setRenderPipelineState:pipelineState];
    [renderEncoder setDepthStencilState:depthStencilState];
    
    // Set vertex and index buffers
    [renderEncoder setVertexBuffer:vertexBuffer offset:0 atIndex:0];
    [renderEncoder setVertexBuffer:uniformBuffer offset:sizeof(Uniforms) * currentFrameIndex atIndex:1];
    
    // Set fragment shader resources
    [renderEncoder setFragmentBuffer:uniformBuffer offset:sizeof(Uniforms) * currentFrameIndex atIndex:1];
    [renderEncoder setFragmentTexture:gameTexture atIndex:0];
    [renderEncoder setFragmentSamplerState:samplerState atIndex:0];
    
    // If using AI-enhanced mode and we have an AI texture, set it as the second texture
    if (config.renderMode == RenderModeAIEnhanced && aiTexture != nil) {
        [renderEncoder setFragmentTexture:aiTexture atIndex:1];
    }
    
    // Draw indexed primitives
    [renderEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                              indexCount:6
                               indexType:MTLIndexTypeUInt16
                             indexBuffer:indexBuffer
                       indexBufferOffset:0];
    
    // End encoding
    [renderEncoder endEncoding];
    
    // Present drawable
    [commandBuffer presentDrawable:drawable];
    
    // Commit command buffer
    [commandBuffer commit];
    
    // Update current frame index
    currentFrameIndex = (currentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

// Create a texture from buffer
id<MTLTexture> Metal_CreateTextureFromBuffer(unsigned char* buffer, int width, int height, int pitch) {
    id<MTLTexture> texture = nil;
    createTexture(buffer, width, height, pitch, &texture);
    return texture;
}

// Take screenshot
bool Metal_TakeScreenshot(const char* filename) {
    // This would need a more complete implementation to capture the current frame
    // For now, it's just a placeholder
    return false;
}

// Get current FPS
float Metal_GetFPS() {
    // This would need frame timing implementation
    // For now, it's just a placeholder
    return 60.0f;
} 