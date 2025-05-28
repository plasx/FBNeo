#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <Cocoa/Cocoa.h>
#include "metal_minimal.h"

// Global variables
UINT8* g_pFrameBuffer = NULL;
int g_nFrameWidth = 384;
int g_nFrameHeight = 224;
int g_nBPP = 32;
bool g_bMetalInitialized = false;

// Metal rendering objects
static id<MTLDevice> g_device = nil;
static id<MTLTexture> g_texture = nil;
static MTKView* g_metalView = nil;

@interface MetalView : MTKView
@property (nonatomic, strong) id<MTLRenderPipelineState> pipelineState;
@property (nonatomic, strong) id<MTLCommandQueue> commandQueue;
@property (nonatomic, strong) id<MTLBuffer> vertexBuffer;
@property (nonatomic, strong) id<MTLBuffer> indexBuffer;
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
    
    // Create a default texture (will be replaced with actual game texture)
    MTLTextureDescriptor *textureDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                                                 width:g_nFrameWidth
                                                                                                height:g_nFrameHeight
                                                                                             mipmapped:NO];
    g_texture = [self.device newTextureWithDescriptor:textureDescriptor];
    
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
    
    // Set global metal view for C++ access
    g_metalView = self;
    
    NSLog(@"Metal setup completed successfully");
    g_bMetalInitialized = true;
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
        [renderEncoder setFragmentTexture:g_texture atIndex:0];
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

@end

@interface FBNeoMetalDelegate : NSObject <NSApplicationDelegate>
@property (nonatomic, strong) NSWindow *window;
@property (nonatomic, strong) MetalView *metalView;
@property (nonatomic, strong) NSTextField *statusLabel;
@end

@implementation FBNeoMetalDelegate

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
    
    self.window.title = @"FBNeo Metal Simplified";
    [self.window center];
    [self.window setReleasedWhenClosed:NO]; // Prevent premature release
    
    // Check if Metal is supported
    g_device = MTLCreateSystemDefaultDevice();
    if (!g_device) {
        NSLog(@"ERROR: Metal is not supported on this device!");
        NSAlert *alert = [[NSAlert alloc] init];
        alert.messageText = @"Metal Not Supported";
        alert.informativeText = @"This application requires Metal graphics support, which is not available on your device.";
        [alert runModal];
        [NSApp terminate:nil];
        return;
    }
    
    NSLog(@"Creating Metal view with device: %@", g_device);
    self.metalView = [[MetalView alloc] initWithFrame:self.window.contentView.bounds device:g_device];
    self.metalView.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    self.metalView.clearColor = MTLClearColorMake(0.2, 0.0, 0.3, 1.0);
    self.metalView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    
    [self.window.contentView addSubview:self.metalView];
    
    // Create status label with current time
    NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
    [formatter setDateFormat:@"HH:mm:ss"];
    NSString *timeString = [formatter stringFromDate:[NSDate date]];
    
    self.statusLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, 20, 600, 24)];
    self.statusLabel.stringValue = [NSString stringWithFormat:@"FBNeo Metal - %@", timeString];
    self.statusLabel.bezeled = NO;
    self.statusLabel.drawsBackground = NO;
    self.statusLabel.editable = NO;
    self.statusLabel.selectable = NO;
    self.statusLabel.textColor = [NSColor whiteColor];
    
    [self.metalView addSubview:self.statusLabel];
    
    // Create a test pattern texture
    if (g_bMetalInitialized) {
        uint32_t *textureData = (uint32_t*)malloc(g_nFrameWidth * g_nFrameHeight * 4);
        for (int y = 0; y < g_nFrameHeight; y++) {
            for (int x = 0; x < g_nFrameWidth; x++) {
                uint8_t r = (uint8_t)(x % 256);
                uint8_t g = (uint8_t)(y % 256);
                uint8_t b = (uint8_t)((x ^ y) % 256);
                uint8_t a = 255;
                textureData[y * g_nFrameWidth + x] = (a << 24) | (b << 16) | (g << 8) | r;
            }
        }
        
        // Update texture data
        MTLRegion region = MTLRegionMake2D(0, 0, g_nFrameWidth, g_nFrameHeight);
        [g_texture replaceRegion:region mipmapLevel:0 withBytes:textureData bytesPerRow:g_nFrameWidth * 4];
        free(textureData);
    }
    
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
    
    self.statusLabel.stringValue = [NSString stringWithFormat:@"FBNeo Metal - %@", timeString];
    
    // Make the view redraw
    [self.metalView setNeedsDisplay:YES];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

- (void)applicationWillTerminate:(NSNotification *)notification {
    NSLog(@"Application terminating...");
    Metal_Exit();
}

@end

// C/C++ interface functions for FBNeo integration
extern "C" {

// Set the frame buffer for rendering
void Metal_SetFrameBuffer(UINT8* buffer, int width, int height, int bpp) {
    // Save the frame buffer information
    g_pFrameBuffer = buffer;
    g_nFrameWidth = width;
    g_nFrameHeight = height;
    g_nBPP = bpp;
    
    NSLog(@"Metal_SetFrameBuffer: %p, %dx%d, %d BPP", buffer, width, height, bpp);
    
    // Recreate the texture if dimensions have changed
    if (g_texture != nil && g_device != nil) {
        MTLTextureDescriptor *textureDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                                                     width:width
                                                                                                    height:height
                                                                                                 mipmapped:NO];
        g_texture = [g_device newTextureWithDescriptor:textureDescriptor];
    }
}

// Update the display from the frame buffer
void Metal_UpdateDisplay() {
    if (!g_bMetalInitialized || !g_pFrameBuffer || !g_texture) {
        return;
    }
    
    // Update texture with frame buffer data
    MTLRegion region = MTLRegionMake2D(0, 0, g_nFrameWidth, g_nFrameHeight);
    [g_texture replaceRegion:region mipmapLevel:0 withBytes:g_pFrameBuffer bytesPerRow:g_nFrameWidth * (g_nBPP / 8)];
    
    // Redraw view with updated texture
    if (g_metalView != nil) {
        [g_metalView setNeedsDisplay:YES];
    }
}

// Initialize Metal
int Metal_Init() {
    // This is called from Objective-C during setup
    NSLog(@"Metal_Init called - waiting for Objective-C setup...");
    
    // Allocate a default frame buffer
    if (g_pFrameBuffer == NULL) {
        g_pFrameBuffer = (UINT8*)malloc(g_nFrameWidth * g_nFrameHeight * 4);
        if (g_pFrameBuffer) {
            // Fill with a pattern
            uint32_t* pixel = (uint32_t*)g_pFrameBuffer;
            for (int y = 0; y < g_nFrameHeight; y++) {
                for (int x = 0; x < g_nFrameWidth; x++) {
                    *(pixel++) = 0xFF000000 | (y << 16) | (x << 8) | ((x^y) & 0xFF);
                }
            }
        }
    }
    
    return 0;
}

// Clean up Metal resources
int Metal_Exit() {
    NSLog(@"Metal_Exit called");
    
    // Free frame buffer
    if (g_pFrameBuffer) {
        free(g_pFrameBuffer);
        g_pFrameBuffer = NULL;
    }
    
    // Metal objects will be released by ARC
    g_device = nil;
    g_texture = nil;
    g_metalView = nil;
    
    g_bMetalInitialized = false;
    return 0;
}

} // extern "C"

// Main entry point
int main(int argc, const char* argv[]) {
    @autoreleasepool {
        NSLog(@"Starting FBNeo Metal Simplified");
        
        // Initialize Metal from C interface
        Metal_Init();
        
        // Create application
        [NSApplication sharedApplication];
        
        // Set up basic app menu (needed for proper macOS integration)
        NSMenu *menubar = [[NSMenu alloc] init];
        NSMenuItem *appMenuItem = [[NSMenuItem alloc] init];
        [menubar addItem:appMenuItem];
        NSMenu *appMenu = [[NSMenu alloc] init];
        NSMenuItem *quitMenuItem = [[NSMenuItem alloc] initWithTitle:@"Quit" 
                                                             action:@selector(terminate:) 
                                                      keyEquivalent:@"q"];
        [appMenu addItem:quitMenuItem];
        [appMenuItem setSubmenu:appMenu];
        [NSApp setMainMenu:menubar];
        
        // Create and set delegate
        FBNeoMetalDelegate *delegate = [[FBNeoMetalDelegate alloc] init];
        [NSApp setDelegate:delegate];
        
        // Finalize app setup and make it a proper application
        [NSApp finishLaunching];
        
        // Start UI
        [NSApp activateIgnoringOtherApps:YES];
        [NSApp run];
    }
    
    return 0;
}
