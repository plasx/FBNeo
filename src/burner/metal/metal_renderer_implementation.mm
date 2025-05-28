#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <Cocoa/Cocoa.h>
#import <CoreVideo/CoreVideo.h>
#import <simd/simd.h>
#include "metal_renderer.h"
#include "metal_renderer_c.h"

// Metal renderer implementation
@interface MetalRenderer : NSObject <MTKViewDelegate>

// Metal objects
@property (nonatomic, strong) id<MTLDevice> device;
@property (nonatomic, strong) id<MTLCommandQueue> commandQueue;
@property (nonatomic, strong) id<MTLLibrary> defaultLibrary;
@property (nonatomic, strong) id<MTLRenderPipelineState> pipelineState;
@property (nonatomic, strong) MTKView *view;

// Texture for the frame
@property (nonatomic, strong) id<MTLTexture> frameTexture;
@property (nonatomic, assign) NSUInteger textureWidth;
@property (nonatomic, assign) NSUInteger textureHeight;

// Settings
@property (nonatomic, assign) BOOL useFiltering;
@property (nonatomic, assign) BOOL useVSync;
@property (nonatomic, assign) BOOL useCRT;
@property (nonatomic, assign) BOOL useScanlines;
@property (nonatomic, assign) float scanlineIntensity;
@property (nonatomic, assign) BOOL needsRedraw;

// Shared instance
+ (instancetype)sharedRenderer;

// Initialization
- (instancetype)initWithView:(NSView *)view;

// Rendering
- (void)updateTextureWithBuffer:(uint8_t *)buffer width:(int)width height:(int)height pitch:(int)pitch;
- (void)renderFrame;

// Settings
- (void)setVSync:(BOOL)enabled;
- (void)setScanlines:(BOOL)enabled intensity:(float)intensity;
- (void)setCRTEffect:(BOOL)enabled;

@end

// Singleton instance
static MetalRenderer* _sharedRenderer = nil;

@implementation MetalRenderer

// Get the shared renderer instance
+ (instancetype)sharedRenderer {
    return _sharedRenderer;
}

// Initialize with a view
- (instancetype)initWithView:(NSView *)view {
    self = [super init];
    if (self) {
        // Create a Metal device
        _device = MTLCreateSystemDefaultDevice();
        if (!_device) {
            NSLog(@"Metal is not supported on this device");
            return nil;
        }
        
        // Create a Metal view
        _view = [[MTKView alloc] initWithFrame:view.bounds device:_device];
        _view.delegate = self;
        _view.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
        _view.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
        [view addSubview:_view];
        
        // Create a command queue
        _commandQueue = [_device newCommandQueue];
        
        // Load the shader library
        [self loadShaderLibrary];
        
        // Create the render pipeline
        [self createRenderPipeline];
        
        // Default settings
        _useFiltering = YES;
        _useVSync = YES;
        _useCRT = NO;
        _useScanlines = NO;
        _scanlineIntensity = 0.3f;
        
        // Store the shared instance
        _sharedRenderer = self;
    }
    return self;
}

// Load the shader library
- (void)loadShaderLibrary {
    NSError* error = nil;
    _defaultLibrary = [_device newDefaultLibrary];
    if (!_defaultLibrary) {
        NSLog(@"Failed to load default library: %@", error);
    }
}

// Create the render pipeline
- (void)createRenderPipeline {
    // Vertex function
    id<MTLFunction> vertexFunction = [_defaultLibrary newFunctionWithName:@"vertexShader"];
    
    // Fragment function - choose based on settings
    NSString *fragmentName = @"fragmentShader";
    if (_useCRT) {
        fragmentName = @"fragmentShaderCRT";
    } else if (_useScanlines) {
        fragmentName = @"fragmentShaderScanlines";
    }
    
    id<MTLFunction> fragmentFunction = [_defaultLibrary newFunctionWithName:fragmentName];
    
    // Create pipeline descriptor
    MTLRenderPipelineDescriptor *pipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDescriptor.vertexFunction = vertexFunction;
    pipelineDescriptor.fragmentFunction = fragmentFunction;
    pipelineDescriptor.colorAttachments[0].pixelFormat = _view.colorPixelFormat;
    
    // Create the pipeline state
    NSError *error = nil;
    _pipelineState = [_device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
    if (!_pipelineState) {
        NSLog(@"Failed to create pipeline state: %@", error);
    }
}

// Update texture with frame data
- (void)updateTextureWithBuffer:(uint8_t *)buffer width:(int)width height:(int)height pitch:(int)pitch {
    if (!buffer || width <= 0 || height <= 0) return;
    
    // Create or resize texture if needed
    if (!_frameTexture || _textureWidth != width || _textureHeight != height) {
        MTLTextureDescriptor *textureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                                                              width:width
                                                                                             height:height
                                                                                          mipmapped:NO];
        textureDesc.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
        
        _frameTexture = [_device newTextureWithDescriptor:textureDesc];
        _textureWidth = width;
        _textureHeight = height;
    }
    
    // Update texture contents
    MTLRegion region = MTLRegionMake2D(0, 0, width, height);
    [_frameTexture replaceRegion:region mipmapLevel:0 withBytes:buffer bytesPerRow:pitch];
    
    _needsRedraw = YES;
}

// Render a frame
- (void)renderFrame {
    if (!_needsRedraw || !_frameTexture) return;
    
    id<CAMetalDrawable> drawable = _view.currentDrawable;
    if (!drawable) return;
    
    MTLRenderPassDescriptor *renderPassDescriptor = _view.currentRenderPassDescriptor;
    if (!renderPassDescriptor) return;
    
    id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
    id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
    
    [renderEncoder setRenderPipelineState:_pipelineState];
    [renderEncoder setFragmentTexture:_frameTexture atIndex:0];
    
    // Draw quad
    [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
    [renderEncoder endEncoding];
    
    [commandBuffer presentDrawable:drawable];
    [commandBuffer commit];
    
    _needsRedraw = NO;
}

// MTKViewDelegate methods
- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
    // Handle view size changes if needed
}

- (void)drawInMTKView:(MTKView *)view {
    [self renderFrame];
}

// Settings methods
- (void)setVSync:(BOOL)enabled {
    _useVSync = enabled;
    _view.enableSetNeedsDisplay = enabled;
}

- (void)setScanlines:(BOOL)enabled intensity:(float)intensity {
    _useScanlines = enabled;
    _scanlineIntensity = intensity;
    [self createRenderPipeline];
}

- (void)setCRTEffect:(BOOL)enabled {
    _useCRT = enabled;
    [self createRenderPipeline];
}

@end

// C interface implementation
bool Metal_InitRenderer(id<MTLDevice> device) {
    @autoreleasepool {
        if (!device) {
            NSLog(@"Error: No Metal device provided");
            return false;
        }
        
        // Create a new renderer instance
        MetalRenderer *renderer = [[MetalRenderer alloc] initWithDevice:device];
        if (!renderer) {
            NSLog(@"Error: Failed to create Metal renderer");
            return false;
        }
        
        // Set as shared instance
        _sharedRenderer = renderer;
        NSLog(@"Metal renderer initialized successfully");
        
        return true;
    }
}

void Metal_ShutdownRenderer() {
    // No action needed for shutdown
}

void* Metal_GetFrameBuffer() {
    // We don't expose the frame buffer directly
    return NULL;
}

int Metal_RenderFrame(void* frameData, int width, int height) {
    if (!frameData || width <= 0 || height <= 0) {
        NSLog(@"Metal_RenderFrame: Invalid parameters");
        return 0;
    }
    
    @autoreleasepool {
        MetalRenderer *renderer = [MetalRenderer sharedRenderer];
        if (!renderer) {
            NSLog(@"Metal_RenderFrame: No renderer available");
            return 0;
        }
        
        // Calculate pitch (bytes per row)
        int pitch = width * 4; // RGBA format (32-bit color)
        
        // Update texture with frame data
        [renderer updateTextureWithBuffer:(uint8_t*)frameData 
                                   width:width 
                                  height:height 
                                   pitch:pitch];
        return 1;
    }
}

void Metal_UpdateTexture(void* data, int width, int height) {
    Metal_RenderFrame(data, width, height);
}

void Metal_SetRenderState(int state, int value) {
    @autoreleasepool {
        MetalRenderer *renderer = [MetalRenderer sharedRenderer];
        if (!renderer) return;
        
        switch (state) {
            case 0: // VSYNC
                [renderer setVSync:value != 0];
                break;
                
            case 2: // CRT effect
                [renderer setCRTEffect:value != 0];
                break;
                
            case 3: // Scanlines
                [renderer setScanlines:value != 0 intensity:0.3f];
                break;
        }
    }
}

const char* Metal_GetRendererInfo() {
    return "Metal Renderer";
}

int Metal_RunFrame(int bDraw) {
    // If drawing is requested, mark for redraw
    if (bDraw) {
        @autoreleasepool {
            MetalRenderer *renderer = [MetalRenderer sharedRenderer];
            if (renderer) {
                renderer.needsRedraw = YES;
            }
        }
    }
    return 0;
}

int Metal_InitRendererWithView(NSView *view) {
    @autoreleasepool {
        // Create a Metal device
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        if (!device) {
            NSLog(@"Metal is not supported on this device");
            return 0;
        }
        
        // Create renderer instance
        MetalRenderer *renderer = [[MetalRenderer alloc] initWithView:view];
        if (!renderer) {
            NSLog(@"Failed to initialize Metal renderer");
            return 0;
        }
        
        return 1;
    }
} 