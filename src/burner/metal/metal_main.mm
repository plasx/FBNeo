#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <Cocoa/Cocoa.h>
#include "metal_declarations.h"

// Basic implementation of standalone Metal based emulator window
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

extern "C" {
    // External C functions for ROM loading
    bool Metal_LoadAndInitROM(const char* romPath);
    bool Metal_ProcessFrame();
    unsigned char* Metal_GetFrameBuffer(int* width, int* height, int* pitch);
}

// MetalView implementation
@interface MetalView : MTKView
- (void)updateFrameBuffer:(void*)buffer width:(int)width height:(int)height pitch:(int)pitch;
@end

@implementation MetalView {
    id<MTLTexture> _texture;
    id<MTLRenderPipelineState> _pipelineState;
    id<MTLCommandQueue> _commandQueue;
    BOOL _textureInitialized;
}

- (instancetype)initWithFrame:(CGRect)frame device:(id<MTLDevice>)device {
    self = [super initWithFrame:frame device:device];
    if (self) {
        // Set up the Metal view
        self.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
        self.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
        
        // Create a command queue
        _commandQueue = [device newCommandQueue];
        
        // Load the default library
        id<MTLLibrary> defaultLibrary = [device newDefaultLibrary];
        if (!defaultLibrary) {
            NSLog(@"Failed to load default Metal library");
            return nil;
        }
        
        // Create a render pipeline
        MTLRenderPipelineDescriptor *pipelineStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
        pipelineStateDescriptor.label = @"Render Pipeline";
        pipelineStateDescriptor.colorAttachments[0].pixelFormat = self.colorPixelFormat;
        
        // Load the vertex function from the library
        id<MTLFunction> vertexFunction = [defaultLibrary newFunctionWithName:@"default_vertexShader"];
        pipelineStateDescriptor.vertexFunction = vertexFunction;
        
        // Load the fragment function from the library
        id<MTLFunction> fragmentFunction = [defaultLibrary newFunctionWithName:@"default_fragmentShader"];
        pipelineStateDescriptor.fragmentFunction = fragmentFunction;
        
        // Create the render pipeline state
        NSError *error = nil;
        _pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
        if (!_pipelineState) {
            NSLog(@"Failed to create pipeline state: %@", error);
            return nil;
        }
        
        _textureInitialized = NO;
    }
    return self;
}

- (void)updateFrameBuffer:(void*)buffer width:(int)width height:(int)height pitch:(int)pitch {
    if (!buffer || width <= 0 || height <= 0) {
        return;
    }
    
    // Check if the texture needs to be created or recreated
    if (!_textureInitialized || !_texture || _texture.width != width || _texture.height != height) {
        MTLTextureDescriptor *textureDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                                                                width:width
                                                                                               height:height
                                                                                            mipmapped:NO];
        textureDescriptor.usage = MTLTextureUsageShaderRead;
        
        _texture = [self.device newTextureWithDescriptor:textureDescriptor];
        if (!_texture) {
            NSLog(@"Failed to create texture");
            return;
        }
        
        _textureInitialized = YES;
    }
    
    // Update the texture with the frame buffer data
    MTLRegion region = MTLRegionMake2D(0, 0, width, height);
    [_texture replaceRegion:region mipmapLevel:0 withBytes:buffer bytesPerRow:pitch];
}

- (void)drawRect:(NSRect)dirtyRect {
    // Create a command buffer
    id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
    
    // If we have a valid texture and pipeline state
    if (_texture && _pipelineState) {
        // Get a render pass descriptor
        MTLRenderPassDescriptor *renderPassDescriptor = self.currentRenderPassDescriptor;
        if (renderPassDescriptor) {
            // Create a render command encoder
            id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
            
            // Set the render pipeline state
            [renderEncoder setRenderPipelineState:_pipelineState];
            
            // Set the texture
            [renderEncoder setFragmentTexture:_texture atIndex:0];
            
            // Draw a quad (two triangles)
            [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
            
            // End encoding
            [renderEncoder endEncoding];
            
            // Present the drawable
            [commandBuffer presentDrawable:self.currentDrawable];
        }
    }
    
    // Commit the command buffer
    [commandBuffer commit];
}

@end

// MainApp delegate
@interface MainAppDelegate : NSObject <NSApplicationDelegate>
@property (strong) NSWindow *window;
@property (strong) MetalView *metalView;
@property (strong) NSTimer *gameTimer;
@property (strong) NSString *romPath;
@property (assign) BOOL gameLoaded;
@end

@implementation MainAppDelegate

- (id)initWithROMPath:(NSString *)romPath {
    self = [super init];
    if (self) {
        _romPath = romPath;
        _gameLoaded = NO;
    }
    return self;
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    // Create a window
    NSRect frame = NSMakeRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    _window = [[NSWindow alloc] initWithContentRect:frame
                                          styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                                            backing:NSBackingStoreBuffered
                                              defer:NO];
    [_window setTitle:@"FBNeo Metal"];
    [_window center];
    
    // Create a Metal device
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device) {
        NSLog(@"Metal is not supported on this device");
        [NSApp terminate:nil];
        return;
    }
    
    // Create a Metal view
    _metalView = [[MetalView alloc] initWithFrame:_window.contentView.bounds device:device];
    _metalView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    [_window.contentView addSubview:_metalView];
    
    // Show the window
    [_window makeKeyAndOrderFront:nil];
    
    // Load ROM if specified
    if (_romPath) {
        NSLog(@"Loading ROM: %@", _romPath);
        if (Metal_LoadAndInitROM([_romPath UTF8String])) {
            _gameLoaded = YES;
            
            // Start the game loop
            _gameTimer = [NSTimer scheduledTimerWithTimeInterval:1.0/60.0
                                                         target:self
                                                       selector:@selector(gameLoop)
                                                       userInfo:nil
                                                        repeats:YES];
        } else {
            NSLog(@"Failed to load ROM: %@", _romPath);
        }
    }
}

- (void)gameLoop {
    if (_gameLoaded) {
        // Run a frame of emulation
        Metal_ProcessFrame();
        
        // Get frame buffer data
        int width, height, pitch;
        unsigned char* frameBuffer = Metal_GetFrameBuffer(&width, &height, &pitch);
        
        // Update the Metal view with the frame buffer
        if (frameBuffer && width > 0 && height > 0) {
            [_metalView updateFrameBuffer:frameBuffer width:width height:height pitch:pitch];
        }
    }
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

@end

// Main entry point
int main(int argc, const char* argv[]) {
    @autoreleasepool {
        NSLog(@"FBNeo Metal Starting...");
        
        // Create the application
        NSApplication *app = [NSApplication sharedApplication];
        
        // Check for ROM path argument
        NSString *romPath = nil;
        if (argc > 1) {
            romPath = [NSString stringWithUTF8String:argv[1]];
        }
        
        // Create the application delegate
        MainAppDelegate *delegate = [[MainAppDelegate alloc] initWithROMPath:romPath];
        [app setDelegate:delegate];
        
        // Run the application
        [app run];
    }
    return 0;
}
