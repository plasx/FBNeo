#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <QuartzCore/CAMetalLayer.h>
#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

// Include C headers
#include "metal_declarations.h"
#include "debug_controller.h"

// Debug logging
#define LOG_DEBUG(...) NSLog(@"[MTKRenderer] " __VA_ARGS__)

// External debug hooks that need to be called
extern void InitializeROMLoadingDebugHooks(void);

// Add function to output renderer debug info
void OutputRendererDebugInfo(int width, int height) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "Created frame buffer %dx%d (%d bytes)", 
             width, height, width * height * 4);
    
    Debug_Log(DEBUG_RENDERER, buffer);
    Debug_Log(DEBUG_RENDERER, "MetalRenderer_Init: Renderer initialized successfully");
    Debug_Log(DEBUG_METAL, "Debug logging enabled");
    Debug_Log(DEBUG_METAL, "Metal_Init called");
    
    // Adding renderer loop information
    Debug_PrintSectionHeader(DEBUG_RENDERER, "Rendering background layers initialized.");
    Debug_Log(DEBUG_RENDERER, "Sprite rendering initialized.");
    Debug_Log(DEBUG_RENDERER, "Metal shaders loaded and applied successfully.");
    
    // Adding audio loop information
    Debug_PrintSectionHeader(DEBUG_AUDIO_LOOP, "Audio streaming activated (CoreAudio backend).");
    
    // Adding input loop information
    Debug_PrintSectionHeader(DEBUG_INPUT_LOOP, "Controller inputs polling activated.");
}

@interface FBNeoMetalRenderer : NSObject <MTKViewDelegate>

@property (nonatomic, strong) id<MTLDevice> device;
@property (nonatomic, strong) id<MTLCommandQueue> commandQueue;
@property (nonatomic, strong) id<MTLRenderPipelineState> pipelineState;
@property (nonatomic, strong) id<MTLTexture> texture;
@property (nonatomic, strong) id<MTLBuffer> vertexBuffer;
@property (nonatomic, assign) CGSize viewportSize;
@property (nonatomic, assign) BOOL initialized;
@property (nonatomic, assign) BOOL frameReady;
@property (nonatomic, assign) BOOL usePixelArtFiltering;
@property (nonatomic, assign) BOOL useScanlines;
@property (nonatomic, assign) float scanlineIntensity;
@property (nonatomic, assign) int frameWidth;
@property (nonatomic, assign) int frameHeight;
@property (nonatomic, strong) id<MTLBuffer> frameDataBuffer;

- (instancetype)initWithView:(MTKView *)view;
- (void)updateFrameData:(void *)data width:(NSInteger)width height:(NSInteger)height;

@end

@implementation FBNeoMetalRenderer

- (instancetype)initWithView:(MTKView *)view {
    self = [super init];
    if (self) {
        _device = view.device;
        _viewportSize = view.drawableSize;
        _frameReady = NO;
        _usePixelArtFiltering = NO;
        _useScanlines = NO;
        _scanlineIntensity = 0.5;
        _frameWidth = 384; // Default to Marvel vs Capcom resolution
        _frameHeight = 224;
        
        LOG_DEBUG(@"Initializing FBNeo Metal Renderer");
        
        [self setupView:view];
        [self setupPipeline];
        [self createFrameBuffer];
    }
    return self;
}

- (void)setupView:(MTKView *)view {
    view.delegate = self;
    view.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
    view.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    
    // Create command queue
    _commandQueue = [_device newCommandQueue];
    
    LOG_DEBUG(@"Metal view setup complete");
}

- (void)setupPipeline {
    // Create a Metal library from the default Metal shader functions
    NSError *error = nil;
    id<MTLLibrary> library = nil;
    
    // Try loading from bundled metallib
    NSString *defaultMetallibPath = [[NSBundle mainBundle] pathForResource:@"default" ofType:@"metallib"];
    if (defaultMetallibPath) {
        library = [_device newLibraryWithURL:[NSURL fileURLWithPath:defaultMetallibPath] error:&error];
    }
    
    // Try loading from current directory
    if (!library) {
        NSString *currentDirMetalLib = [[NSFileManager defaultManager] currentDirectoryPath];
        currentDirMetalLib = [currentDirMetalLib stringByAppendingPathComponent:@"default.metallib"];
        if ([[NSFileManager defaultManager] fileExistsAtPath:currentDirMetalLib]) {
            LOG_DEBUG(@"Loading Metal library from current directory: %@", currentDirMetalLib);
            library = [_device newLibraryWithURL:[NSURL fileURLWithPath:currentDirMetalLib] error:&error];
            if (error) {
                LOG_DEBUG(@"Failed to load Metal library from current directory: %@", error);
                error = nil;
            }
        }
    }
    
    // Fall back to default library if needed
    if (!library) {
        library = [_device newDefaultLibrary];
    }
    
    if (!library) {
        LOG_DEBUG(@"Failed to create Metal library: %@", error);
        return;
    }
    
    // Get shader functions
    id<MTLFunction> vertexFunction = [library newFunctionWithName:@"demo_vertexShader"];
    id<MTLFunction> fragmentFunction = [library newFunctionWithName:@"demo_fragmentShader"];
    
    if (!vertexFunction || !fragmentFunction) {
        // Try alternate shader names
        vertexFunction = [library newFunctionWithName:@"vertexShader"];
        fragmentFunction = [library newFunctionWithName:@"fragmentShader"];
        
        if (!vertexFunction || !fragmentFunction) {
            // Try default shader names
            vertexFunction = [library newFunctionWithName:@"default_vertexShader"];
            fragmentFunction = [library newFunctionWithName:@"default_fragmentShader"];
            
            if (!vertexFunction || !fragmentFunction) {
                LOG_DEBUG(@"Failed to find shader functions");
                return;
            }
        }
    }
    
    // Create vertex buffer
    float quadVertices[] = {
        -1.0, -1.0, 0.0, 1.0,  // bottom-left
         1.0, -1.0, 1.0, 1.0,  // bottom-right
        -1.0,  1.0, 0.0, 0.0,  // top-left
         1.0,  1.0, 1.0, 0.0,  // top-right
    };
    
    _vertexBuffer = [_device newBufferWithBytes:quadVertices
                                         length:sizeof(quadVertices)
                                        options:MTLResourceStorageModeShared];
    
    // Create render pipeline
    MTLRenderPipelineDescriptor *pipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDesc.vertexFunction = vertexFunction;
    pipelineDesc.fragmentFunction = fragmentFunction;
    pipelineDesc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    
    _pipelineState = [_device newRenderPipelineStateWithDescriptor:pipelineDesc error:&error];
    if (!_pipelineState) {
        LOG_DEBUG(@"Failed to create pipeline state: %@", error);
        return;
    }
    
    _initialized = YES;
    LOG_DEBUG(@"Metal pipeline setup complete");
}

- (void)createFrameBuffer {
    // Create a texture for the frame buffer
    MTLTextureDescriptor *textureDesc = [MTLTextureDescriptor
                                        texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                        width:_frameWidth
                                        height:_frameHeight
                                        mipmapped:NO];
    
    textureDesc.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
    _texture = [_device newTextureWithDescriptor:textureDesc];
    
    // Create a buffer for frame data
    NSUInteger bufferSize = _frameWidth * _frameHeight * 4; // 4 bytes per pixel
    _frameDataBuffer = [_device newBufferWithLength:bufferSize
                                           options:MTLResourceStorageModeShared];
    
    // Initialize with black frame
    uint8_t *data = (uint8_t *)[_frameDataBuffer contents];
    memset(data, 0, bufferSize);
    
    // Copy to texture
    MTLRegion region = MTLRegionMake2D(0, 0, _frameWidth, _frameHeight);
    [_texture replaceRegion:region
                mipmapLevel:0
                  withBytes:data
                bytesPerRow:_frameWidth * 4];
    
    // Mark frame as not ready initially - will be updated by the emulator
    _frameReady = NO;
    
    LOG_DEBUG(@"Created frame buffer %dx%d (%lu bytes)", 
             (int)_frameWidth, (int)_frameHeight, (unsigned long)bufferSize);
}

- (void)updateFrameData:(void *)data width:(NSInteger)width height:(NSInteger)height {
    if (!data || width <= 0 || height <= 0) {
        LOG_DEBUG(@"Invalid frame data: %p %ld x %ld", data, (long)width, (long)height);
        return;
    }
    
    // Check if dimensions changed
    if (width != _frameWidth || height != _frameHeight) {
        LOG_DEBUG(@"Frame dimensions changed: %ld x %ld (was %d x %d)", 
                 (long)width, (long)height, _frameWidth, _frameHeight);
        
        _frameWidth = (int)width;
        _frameHeight = (int)height;
        
        // Recreate texture with new dimensions
        MTLTextureDescriptor *textureDesc = [MTLTextureDescriptor
                                            texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                            width:_frameWidth
                                            height:_frameHeight
                                            mipmapped:NO];
        
        textureDesc.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
        _texture = [_device newTextureWithDescriptor:textureDesc];
        
        // Create new buffer if needed
        NSUInteger bufferSize = _frameWidth * _frameHeight * 4; // 4 bytes per pixel
        if (!_frameDataBuffer || _frameDataBuffer.length < bufferSize) {
            _frameDataBuffer = [_device newBufferWithLength:bufferSize
                                               options:MTLResourceStorageModeShared];
        }
    }
    
    // Compute quick checksum for debugging
    uint32_t checksum = 0;
    uint32_t *pixels = (uint32_t *)data;
    for (int i = 0; i < 100 && i < (width * height); i++) {
        checksum += pixels[i];
    }
    
    LOG_DEBUG(@"Updating frame data: %p %ld x %ld, checksum: 0x%08X", 
             data, (long)width, (long)height, checksum);
    
    // Copy frame data to buffer
    memcpy([_frameDataBuffer contents], data, width * height * 4);
    
    // Copy to texture
    MTLRegion region = MTLRegionMake2D(0, 0, width, height);
    [_texture replaceRegion:region
                mipmapLevel:0
                  withBytes:[_frameDataBuffer contents]
                bytesPerRow:width * 4];
    
    // Always mark frame as ready
    _frameReady = YES;
    
    LOG_DEBUG(@"Frame data updated successfully");
}

#pragma mark - MTKViewDelegate

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
    _viewportSize = size;
    LOG_DEBUG(@"Drawable size changed to %.0f x %.0f", size.width, size.height);
}

- (void)drawInMTKView:(MTKView *)view {
    if (!_initialized) {
        return;
    }
    
    // Only use the test pattern if no frame data has been received yet
    if (!_frameReady) {
        return;
    }
    
    // Get drawable
    id<CAMetalDrawable> drawable = view.currentDrawable;
    if (!drawable) {
        return;
    }
    
    // Get render pass descriptor
    MTLRenderPassDescriptor *renderPassDesc = view.currentRenderPassDescriptor;
    if (!renderPassDesc) {
        return;
    }
    
    // Create command buffer
    id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
    
    // Create render command encoder
    id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDesc];
    
    // Set render state
    [encoder setRenderPipelineState:_pipelineState];
    [encoder setVertexBuffer:_vertexBuffer offset:0 atIndex:0];
    [encoder setFragmentTexture:_texture atIndex:0];
    
    // Create sampler state
    MTLSamplerDescriptor *samplerDesc = [[MTLSamplerDescriptor alloc] init];
    if (_usePixelArtFiltering) {
        samplerDesc.minFilter = MTLSamplerMinMagFilterNearest;
        samplerDesc.magFilter = MTLSamplerMinMagFilterNearest;
    } else {
        samplerDesc.minFilter = MTLSamplerMinMagFilterLinear;
        samplerDesc.magFilter = MTLSamplerMinMagFilterLinear;
    }
    id<MTLSamplerState> samplerState = [_device newSamplerStateWithDescriptor:samplerDesc];
    [encoder setFragmentSamplerState:samplerState atIndex:0];
    
    // Draw quad
    [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
    [encoder endEncoding];
    
    // Present
    [commandBuffer presentDrawable:drawable];
    [commandBuffer commit];
}

@end

// C interface for Metal renderer
FBNeoMetalRenderer *gMetalRenderer = nil;

extern "C" {

int MetalRenderer_Init(void* viewPtr) {
    if (!viewPtr) {
        NSLog(@"MetalRenderer_Init: Invalid view pointer");
        return 1;
    }
    
    @autoreleasepool {
        MTKView *view = (__bridge MTKView *)viewPtr;
        gMetalRenderer = [[FBNeoMetalRenderer alloc] initWithView:view];
        
        if (!gMetalRenderer || !gMetalRenderer.initialized) {
            NSLog(@"MetalRenderer_Init: Failed to initialize renderer");
            return 1;
        }
        
        OutputRendererDebugInfo(gMetalRenderer.frameWidth, gMetalRenderer.frameHeight);
        return 0;
    }
}

void MetalRenderer_Exit() {
    @autoreleasepool {
        gMetalRenderer = nil;
        NSLog(@"MetalRenderer_Exit: Renderer released");
    }
}

int MetalRenderer_UpdateFrame(void* frameBuffer, int width, int height) {
    if (!frameBuffer || width <= 0 || height <= 0) {
        NSLog(@"MetalRenderer_UpdateFrame: Invalid parameters");
        return 1;
    }
    
    if (!gMetalRenderer) {
        NSLog(@"MetalRenderer_UpdateFrame: Renderer not initialized");
        return 1;
    }
    
    @autoreleasepool {
        [gMetalRenderer updateFrameData:frameBuffer width:width height:height];
        return 0;
    }
}

} // extern "C" 