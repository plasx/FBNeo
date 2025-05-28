#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#include <stdio.h>

// Struct for frame buffer
typedef struct {
    uint32_t* data;
    int width;
    int height;
    int pitch;
    bool updated;
} FrameBuffer;

// C interface functions declared as extern "C"
extern "C" {
    // Frame buffer access
    extern FrameBuffer g_frameBuffer;
    
    // Game status functions
    extern const char* Metal_GetGameTitle(void);
    extern bool Metal_IsGameRunning(void);
    extern int Metal_GenerateTestPattern(int patternType);
    
    // Error handling
    extern void Metal_SetError(int code, const char* message);
    
    // ROM loading
    extern bool Metal_LoadAndInitROM(const char* path);
    extern void Metal_UnloadROM(void);
}

@interface MetalRenderer : NSObject<MTKViewDelegate>
@property (nonatomic, strong) id<MTLDevice> device;
@property (nonatomic, strong) id<MTLCommandQueue> commandQueue;
@property (nonatomic, strong) id<MTLTexture> texture;
@property (nonatomic, strong) id<MTLRenderPipelineState> pipelineState;
@property (nonatomic, strong) MTLRenderPassDescriptor *renderPassDescriptor;
@end

@implementation MetalRenderer

- (instancetype)initWithView:(MTKView *)view {
    self = [super init];
    if (self) {
        _device = view.device = MTLCreateSystemDefaultDevice();
        if (!_device) {
            Metal_SetError(-1, "Failed to create Metal device");
            return nil;
        }
        
        _commandQueue = [_device newCommandQueue];
        if (!_commandQueue) {
            Metal_SetError(-1, "Failed to create command queue");
            return nil;
        }
        
        // Initialize the frame buffer
        InitFrameBuffer(384, 224);
        
        // Generate a test pattern
        Metal_GenerateTestPattern(1);
        
        // Create texture descriptor
        MTLTextureDescriptor *textureDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                                                     width:g_frameBuffer.width
                                                                                                    height:g_frameBuffer.height
                                                                                                 mipmapped:NO];
        textureDescriptor.usage = MTLTextureUsageShaderRead;
        _texture = [_device newTextureWithDescriptor:textureDescriptor];
        
        // Create render pipeline
        id<MTLLibrary> defaultLibrary = [_device newDefaultLibrary];
        id<MTLFunction> vertexFunction = [defaultLibrary newFunctionWithName:@"vertexShader"];
        id<MTLFunction> fragmentFunction = [defaultLibrary newFunctionWithName:@"fragmentShader"];
        
        MTLRenderPipelineDescriptor *pipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
        pipelineDescriptor.vertexFunction = vertexFunction;
        pipelineDescriptor.fragmentFunction = fragmentFunction;
        pipelineDescriptor.colorAttachments[0].pixelFormat = view.colorPixelFormat;
        
        NSError *error = nil;
        _pipelineState = [_device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
        if (!_pipelineState) {
            NSLog(@"Failed to create pipeline state: %@", error);
            Metal_SetError(-1, "Failed to create pipeline state");
            return nil;
        }
        
        // Set view properties
        view.delegate = self;
        view.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
    }
    return self;
}

- (void)updateTexture {
    if (g_frameBuffer.data && g_frameBuffer.updated) {
        [_texture replaceRegion:MTLRegionMake2D(0, 0, g_frameBuffer.width, g_frameBuffer.height)
                    mipmapLevel:0
                      withBytes:g_frameBuffer.data
                    bytesPerRow:g_frameBuffer.pitch];
        g_frameBuffer.updated = false;
    }
}

- (void)drawInMTKView:(MTKView *)view {
    [self updateTexture];
    
    id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
    MTLRenderPassDescriptor *renderPassDescriptor = view.currentRenderPassDescriptor;
    
    if (renderPassDescriptor) {
        id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        
        [renderEncoder setRenderPipelineState:_pipelineState];
        // Add drawing code here when we have proper rendering
        
        [renderEncoder endEncoding];
        [commandBuffer presentDrawable:view.currentDrawable];
    }
    
    [commandBuffer commit];
}

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
    // Handle resize if needed
}

@end

// Main application class
@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (nonatomic, strong) NSWindow *window;
@property (nonatomic, strong) MTKView *metalView;
@property (nonatomic, strong) MetalRenderer *renderer;
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    NSRect frame = NSMakeRect(0, 0, 800, 600);
    _window = [[NSWindow alloc] initWithContentRect:frame
                                          styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                                            backing:NSBackingStoreBuffered
                                              defer:NO];
    [_window setTitle:@"FBNeo Metal"];
    
    _metalView = [[MTKView alloc] initWithFrame:frame];
    _metalView.device = MTLCreateSystemDefaultDevice();
    _metalView.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    _metalView.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
    
    _renderer = [[MetalRenderer alloc] initWithView:_metalView];
    
    _window.contentView = _metalView;
    [_window makeKeyAndOrderFront:nil];
    [_window center];
    
    // Process command line arguments
    NSArray *args = [[NSProcessInfo processInfo] arguments];
    if (args.count > 1) {
        NSString *romPath = args[1];
        Metal_LoadAndInitROM([romPath UTF8String]);
        Metal_SetGameTitle([[NSString stringWithFormat:@"FBNeo - %@", [romPath lastPathComponent]] UTF8String]);
        [_window setTitle:[NSString stringWithUTF8String:Metal_GetGameTitle()]];
    }
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

- (void)applicationWillTerminate:(NSNotification *)notification {
    Metal_UnloadROM();
}

@end

// Main function
int main(int argc, const char * argv[]) {
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        AppDelegate *appDelegate = [[AppDelegate alloc] init];
        [app setDelegate:appDelegate];
        [app run];
    }
    return 0;
}
