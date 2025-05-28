#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <Cocoa/Cocoa.h>
#include "burnint.h"  // For INT32 type definition
// Remove direct import of metal_renderer.mm to avoid duplicates
// #import "metal_renderer.mm"

// Forward declaration instead of importing
@class MetalViewDelegate;

@interface MetalView : MTKView
@property (nonatomic, strong) id<MTLRenderPipelineState> pipelineState;
@property (nonatomic, strong) id<MTLCommandQueue> commandQueue;
@property (nonatomic, strong) id<MTLTexture> texture;
@property (nonatomic, strong) id<MTLBuffer> vertexBuffer;
@property (nonatomic, strong) id<MTLBuffer> indexBuffer;
@property (nonatomic, strong) id<MTLDevice> renderer;
@end

@implementation MetalView

- (instancetype)initWithFrame:(NSRect)frameRect device:(id<MTLDevice>)device {
    NSLog(@"MetalView initWithFrame called: %@, device: %@", NSStringFromRect(frameRect), device);
    self = [super initWithFrame:frameRect device:device];
    if (self) {
        [self setupMetal];
    }
    return self;
}

- (void)setupMetal {
    NSLog(@"Setting up Metal renderer...");
    
    // Check if device is valid
    if (!self.device) {
        NSLog(@"ERROR: Metal device is nil!");
        return;
    }
    
    // Create a command queue
    self.commandQueue = [self.device newCommandQueue];
    if (!self.commandQueue) {
        NSLog(@"ERROR: Failed to create command queue!");
        return;
    }
    
    // Enable continuous drawing
    self.enableSetNeedsDisplay = NO;
    self.paused = NO;
    self.clearColor = MTLClearColorMake(0.2, 0.0, 0.3, 1.0); // Purple background for visibility
    
    // Create a simple texture with a color pattern
    MTLTextureDescriptor *textureDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                                                 width:256
                                                                                                height:256
                                                                                             mipmapped:NO];
    self.texture = [self.device newTextureWithDescriptor:textureDescriptor];
    
    // Fill the texture with a gradient pattern
    uint32_t *textureData = (uint32_t*)malloc(256 * 256 * 4);
    for (int y = 0; y < 256; y++) {
        for (int x = 0; x < 256; x++) {
            uint8_t r = (uint8_t)(x);
            uint8_t g = (uint8_t)(y);
            uint8_t b = (uint8_t)(x ^ y);
            uint8_t a = 255;
            textureData[y * 256 + x] = (a << 24) | (b << 16) | (g << 8) | r;
        }
    }
    
    // Upload texture data
    MTLRegion region = MTLRegionMake2D(0, 0, 256, 256);
    [self.texture replaceRegion:region mipmapLevel:0 withBytes:textureData bytesPerRow:256 * 4];
    free(textureData);
    
    // Create vertex buffer (quad)
    float quadVertices[] = {
        -1.0, -1.0, 0.0, 1.0,  0.0, 1.0, // bottom left
         1.0, -1.0, 0.0, 1.0,  1.0, 1.0, // bottom right
         1.0,  1.0, 0.0, 1.0,  1.0, 0.0, // top right
        -1.0,  1.0, 0.0, 1.0,  0.0, 0.0  // top left
    };
    self.vertexBuffer = [self.device newBufferWithBytes:quadVertices
                                                 length:sizeof(quadVertices)
                                                options:MTLResourceStorageModeShared];
    
    // Create index buffer
    uint16_t quadIndices[] = {
        0, 1, 2,
        2, 3, 0
    };
    self.indexBuffer = [self.device newBufferWithBytes:quadIndices
                                                length:sizeof(quadIndices)
                                               options:MTLResourceStorageModeShared];
    
    // Create a simple shader
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
                             "vertex VertexOut vertexShader(uint vertexID [[vertex_id]],\n"
                             "                              const device VertexIn* vertices [[buffer(0)]]) {\n"
                             "    VertexOut out;\n"
                             "    out.position = vertices[vertexID].position;\n"
                             "    out.texCoord = vertices[vertexID].texCoord;\n"
                             "    return out;\n"
                             "}\n"
                             "\n"
                             "fragment float4 fragmentShader(VertexOut in [[stage_in]],\n"
                             "                               texture2d<float> colorTexture [[texture(0)]]) {\n"
                             "    constexpr sampler textureSampler (mag_filter::linear, min_filter::linear);\n"
                             "    return colorTexture.sample(textureSampler, in.texCoord);\n"
                             "}\n";
    
    // Create shader library and render pipeline
    NSError *error = nil;
    id<MTLLibrary> library = [self.device newLibraryWithSource:shaderSource options:nil error:&error];
    if (!library) {
        NSLog(@"Failed to create shader library: %@", error);
        return;
    }
    
    id<MTLFunction> vertexFunction = [library newFunctionWithName:@"vertexShader"];
    id<MTLFunction> fragmentFunction = [library newFunctionWithName:@"fragmentShader"];
    
    if (!vertexFunction || !fragmentFunction) {
        NSLog(@"Failed to get shader functions!");
        return;
    }
    
    // Set up pipeline descriptor
    MTLRenderPipelineDescriptor *pipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDescriptor.vertexFunction = vertexFunction;
    pipelineDescriptor.fragmentFunction = fragmentFunction;
    pipelineDescriptor.colorAttachments[0].pixelFormat = self.colorPixelFormat;
    
    // Create vertex descriptor
    MTLVertexDescriptor *vertexDescriptor = [MTLVertexDescriptor vertexDescriptor];
    vertexDescriptor.attributes[0].format = MTLVertexFormatFloat4;
    vertexDescriptor.attributes[0].offset = 0;
    vertexDescriptor.attributes[0].bufferIndex = 0;
    vertexDescriptor.attributes[1].format = MTLVertexFormatFloat2;
    vertexDescriptor.attributes[1].offset = 16;
    vertexDescriptor.attributes[1].bufferIndex = 0;
    vertexDescriptor.layouts[0].stride = 24;
    pipelineDescriptor.vertexDescriptor = vertexDescriptor;
    
    // Create pipeline state
    self.pipelineState = [self.device newRenderPipelineStateWithDescriptor:pipelineDescriptor
                                                                     error:&error];
    if (!self.pipelineState) {
        NSLog(@"Failed to create pipeline state: %@", error);
        return;
    }
    
    NSLog(@"Metal setup completed successfully");
}

- (void)drawRect:(NSRect)dirtyRect {
    static int frameCount = 0;
    frameCount++;
    
    if (frameCount % 60 == 0) {
        NSLog(@"MetalView drawing frame %d", frameCount);
    }
    
    id<MTLCommandBuffer> commandBuffer = [self.commandQueue commandBuffer];
    
    MTLRenderPassDescriptor *renderPassDescriptor = self.currentRenderPassDescriptor;
    if (renderPassDescriptor != nil) {
        id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        
        [renderEncoder setRenderPipelineState:self.pipelineState];
        [renderEncoder setVertexBuffer:self.vertexBuffer offset:0 atIndex:0];
        [renderEncoder setFragmentTexture:self.texture atIndex:0];
        [renderEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                  indexCount:6
                                   indexType:MTLIndexTypeUInt16
                                 indexBuffer:self.indexBuffer
                           indexBufferOffset:0];
        
        [renderEncoder endEncoding];
        
        [commandBuffer presentDrawable:self.currentDrawable];
    } else {
        if (frameCount % 60 == 0) {
            NSLog(@"Warning: renderPassDescriptor is nil on frame %d", frameCount);
        }
    }
    
    [commandBuffer commit];
}

- (void)updateFrameBuffer:(void*)buffer width:(int)width height:(int)height pitch:(int)pitch {
    if (!buffer || width <= 0 || height <= 0 || pitch <= 0) {
        return;
    }
    
    // Update the frame buffer with new data
    MTLRegion region = MTLRegionMake2D(0, 0, width, height);
    [self.texture replaceRegion:region
                    mipmapLevel:0
                      withBytes:buffer
                    bytesPerRow:pitch];
    
    // Force a redraw
    [self setNeedsDisplay:YES];
}

@end

@interface MetalAppDelegate : NSObject <NSApplicationDelegate>
@property (nonatomic, strong) NSWindow *window;
@property (nonatomic, strong) MetalView *metalView;
@property (nonatomic, strong) NSTextField *statusLabel;
@end

@implementation MetalAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    NSLog(@"Application launched - setting up window...");
    
    // Create the application menu
    NSMenu *menubar = [NSMenu new];
    NSMenuItem *appMenuItem = [NSMenuItem new];
    [menubar addItem:appMenuItem];
    [NSApp setMainMenu:menubar];
    
    NSMenu *appMenu = [NSMenu new];
    NSMenuItem *quitMenuItem = [[NSMenuItem alloc] initWithTitle:@"Quit" 
                                                         action:@selector(terminate:) 
                                                  keyEquivalent:@"q"];
    [appMenu addItem:quitMenuItem];
    [appMenuItem setSubmenu:appMenu];
    
    // Create window
    NSRect frame = NSMakeRect(100, 100, 640, 480);
    NSUInteger styleMask = NSWindowStyleMaskTitled | 
                           NSWindowStyleMaskClosable | 
                           NSWindowStyleMaskMiniaturizable | 
                           NSWindowStyleMaskResizable;
    
    self.window = [[NSWindow alloc] initWithContentRect:frame
                                             styleMask:styleMask
                                               backing:NSBackingStoreBuffered
                                                 defer:NO];
    
    self.window.title = @"Metal Standalone Window";
    [self.window center];
    [self.window setReleasedWhenClosed:NO]; // Prevent premature release
    
    // Check if Metal is supported
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device) {
        NSLog(@"ERROR: Metal is not supported on this device!");
        NSAlert *alert = [[NSAlert alloc] init];
        alert.messageText = @"Metal Not Supported";
        alert.informativeText = @"This application requires Metal graphics support, which is not available on your device.";
        [alert runModal];
        [NSApp terminate:nil];
        return;
    }
    
    NSLog(@"Creating Metal view with device: %@", device);
    self.metalView = [[MetalView alloc] initWithFrame:self.window.contentView.bounds device:device];
    self.metalView.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    self.metalView.clearColor = MTLClearColorMake(0.2, 0.0, 0.3, 1.0);
    self.metalView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    
    [self.window.contentView addSubview:self.metalView];
    
    // Create status label with current time
    NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
    [formatter setDateFormat:@"HH:mm:ss"];
    NSString *timeString = [formatter stringFromDate:[NSDate date]];
    
    self.statusLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, 20, 600, 24)];
    self.statusLabel.stringValue = [NSString stringWithFormat:@"Metal Window - %@", timeString];
    self.statusLabel.bezeled = NO;
    self.statusLabel.drawsBackground = NO;
    self.statusLabel.editable = NO;
    self.statusLabel.selectable = NO;
    self.statusLabel.textColor = [NSColor whiteColor];
    
    [self.metalView addSubview:self.statusLabel];
    
    // Show the window
    [self.window makeKeyAndOrderFront:nil];
    
    // Make sure we are the active application
    [NSApp activateIgnoringOtherApps:YES];
    
    NSLog(@"Window setup complete - window should now be visible");
    
    // Start a timer to update the display
    [NSTimer scheduledTimerWithTimeInterval:1.0/60.0
                                     target:self
                                   selector:@selector(gameLoop)
                                   userInfo:nil
                                    repeats:YES];
}

- (void)gameLoop {
    // Update status label with current time
    NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
    [formatter setDateFormat:@"HH:mm:ss"];
    NSString *timeString = [formatter stringFromDate:[NSDate date]];
    
    self.statusLabel.stringValue = [NSString stringWithFormat:@"Metal Window - %@", timeString];
    
    // Make the view redraw
    [self.metalView setNeedsDisplay:YES];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

- (void)applicationWillTerminate:(NSNotification *)notification {
    NSLog(@"Application terminating...");
}

@end

// Forward declarations
@class MetalWindow;
@class MetalView;

// Window delegate
@interface MetalWindowDelegate : NSObject <NSWindowDelegate>
{
    MetalWindow* _window;
}

- (instancetype)initWithWindow:(MetalWindow*)window;

@end

// Metal window class
@interface MetalWindow : NSWindow
{
    MetalView* _metalView;
    MetalWindowDelegate* _delegate;
}

- (instancetype)initWithFrame:(NSRect)frame title:(NSString*)title;
- (void)updateFrameBuffer:(uint32_t*)buffer width:(int)width height:(int)height pitch:(int)pitch;

@end

// Window delegate implementation
@implementation MetalWindowDelegate

- (instancetype)initWithWindow:(MetalWindow*)window {
    self = [super init];
    if (self) {
        _window = window;
    }
    return self;
}

- (void)windowWillClose:(NSNotification*)notification {
    [NSApp terminate:nil];
}

@end

// Metal window implementation
@implementation MetalWindow

- (instancetype)initWithFrame:(NSRect)frame title:(NSString*)title {
    self = [super initWithContentRect:frame
                           styleMask:NSWindowStyleMaskTitled |
                                   NSWindowStyleMaskClosable |
                                   NSWindowStyleMaskMiniaturizable |
                                   NSWindowStyleMaskResizable
                             backing:NSBackingStoreBuffered
                               defer:NO];
    if (self) {
        self.title = title;
        self.delegate = _delegate = [[MetalWindowDelegate alloc] initWithWindow:self];
        
        // Create Metal view
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        if (!device) {
            NSLog(@"Metal is not supported on this device");
            return nil;
        }
        
        _metalView = [[MetalView alloc] initWithFrame:frame device:device];
        self.contentView = _metalView;
        
        // Center window on screen
        [self center];
    }
    return self;
}

- (void)updateFrameBuffer:(uint32_t*)buffer width:(int)width height:(int)height pitch:(int)pitch {
    [_metalView updateFrameBuffer:buffer width:width height:height pitch:pitch];
}

@end

// C interface for Metal window
extern "C" {
    void* Metal_CreateWindow(int width, int height, const char* title) {
        NSRect frame = NSMakeRect(0, 0, width, height);
        NSString* nsTitle = [NSString stringWithUTF8String:title];
        MetalWindow* window = [[MetalWindow alloc] initWithFrame:frame title:nsTitle];
        [window makeKeyAndOrderFront:nil];
        return (__bridge_retained void*)window;
    }
    
    void Metal_DestroyWindow(void* window) {
        if (window) {
            MetalWindow* w = (__bridge_transfer MetalWindow*)window;
            w = nil;
        }
    }
    
    void Metal_UpdateWindowFrameBuffer(void* window, uint32_t* buffer, int width, int height, int pitch) {
        if (window && buffer) {
            MetalWindow* w = (__bridge MetalWindow*)window;
            [w updateFrameBuffer:buffer width:width height:height pitch:pitch];
        }
    }
    
    // Input handling
    void Metal_ProcessKeyEvent(int keyCode, bool isKeyDown) {
        // Convert key code to FBNeo key code
        int fbneoKey = -1;
        
        // Map common keys
        switch (keyCode) {
            case 0x00: fbneoKey = 0x01; break; // A
            case 0x01: fbneoKey = 0x02; break; // S
            case 0x02: fbneoKey = 0x04; break; // D
            case 0x0D: fbneoKey = 0x08; break; // W
            case 0x31: fbneoKey = 0x10; break; // Space
            case 0x35: fbneoKey = 0x20; break; // Escape
            case 0x7B: fbneoKey = 0x40; break; // Left
            case 0x7C: fbneoKey = 0x80; break; // Right
            case 0x7D: fbneoKey = 0x100; break; // Down
            case 0x7E: fbneoKey = 0x200; break; // Up
        }
        
        if (fbneoKey != -1) {
            // Update input state
            extern INT32 BurnInputSetKey(INT32 key, INT32 pressed);
            BurnInputSetKey(fbneoKey, isKeyDown);
        }
    }
}
