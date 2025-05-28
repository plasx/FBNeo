#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <Cocoa/Cocoa.h>

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "metal_declarations.h"
#include "metal_renderer_defines.h"

// FBNeo basic types
typedef unsigned char UINT8;
typedef unsigned int UINT32;
typedef int INT32;
typedef short INT16;

// Global variables
UINT8* g_pFrameBuffer = NULL;
int g_nFrameWidth = 384;
int g_nFrameHeight = 224;
int g_nBPP = 32;
bool g_bMetalInitialized = false;
char g_ROMPath[1024] = "/Users/plasx/dev/ROMs";

// Metal rendering objects
static id<MTLDevice> g_device = nil;
static id<MTLTexture> g_texture = nil;
static MTKView* g_metalView = nil;

// FBNeo variables needed by the Metal port
UINT8* pBurnDraw_Metal = NULL;
INT32 nBurnPitch_Metal = 0;
INT32 nBurnBpp_Metal = 0;

// Metal texture information
static MTLPixelFormat g_textureFormat = MTLPixelFormatBGRA8Unorm;
static int g_textureWidth = 0;
static int g_textureHeight = 0;
static int g_textureBufferSize = 0;
static unsigned char* g_textureBuffer = NULL;

// Metal view and device
static MTKView* g_mtkView = nil;

// Forward declarations for FBNeo core functions
extern "C" {
    // Core FBNeo functions
    INT32 BurnDrvGetIndexByName(const char* szName);
    INT32 BurnDrvInit_Metal(INT32 nDrvNum);
    INT32 BurnDrvExit_Metal();
    INT32 BurnLibInit_Metal();
    INT32 BurnLibExit_Metal();
    INT32 Metal_RunFrame(bool bDraw);
    
    // Helper functions we implement
    void Metal_SetFrameBuffer(UINT8* buffer, int width, int height, int bpp);
    void Metal_UpdateDisplay();
    int Metal_Init();
    int Metal_Exit();
    int Metal_LoadROM(const char* romPath);
    
    // Input/AI related
    int Metal_InitAI();
    int Metal_ShutdownAI();
    bool Metal_IsAIModuleLoaded();
    bool Metal_IsAIActive();
    void MetalInput_Init();
    void MetalInput_Exit();
    void MetalInput_Make(bool pause);
}

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
    
    // Create a default texture
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
@property (nonatomic, strong) NSTimer *gameTimer;
@property (nonatomic, assign) BOOL gameLoaded;
@property (nonatomic, strong) NSString *gameName;
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
    
    self.window.title = @"FBNeo Metal";
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
    self.statusLabel.stringValue = @"FBNeo Metal - No ROM loaded";
    self.statusLabel.bezeled = NO;
    self.statusLabel.drawsBackground = NO;
    self.statusLabel.editable = NO;
    self.statusLabel.selectable = NO;
    self.statusLabel.textColor = [NSColor whiteColor];
    
    [self.metalView addSubview:self.statusLabel];
    
    // Initialize FBNeo core
    Metal_Init();
    BurnLibInit_Metal();
    MetalInput_Init();
    Metal_InitAI();
    
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
    
    // Process command line arguments if any
    NSArray *args = [[NSProcessInfo processInfo] arguments];
    if (args.count > 1) {
        // Last argument is the ROM path
        NSString *romPath = [args lastObject];
        if ([[NSFileManager defaultManager] fileExistsAtPath:romPath]) {
            [self loadROM:romPath];
        }
    }
    
    // Start a timer to update the display
    self.gameTimer = [NSTimer scheduledTimerWithTimeInterval:1.0/60.0
                                                     target:self
                                                   selector:@selector(gameLoop)
                                                   userInfo:nil
                                                    repeats:YES];
}

- (void)loadROM:(NSString *)romPath {
    NSLog(@"Loading ROM: %@", romPath);
    
    // Call the Metal_LoadROM function
    int result = Metal_LoadROM([romPath UTF8String]);
    
    if (result == 0) {
        self.gameLoaded = YES;
        self.gameName = [romPath lastPathComponent];
        self.window.title = [NSString stringWithFormat:@"FBNeo Metal - %@", self.gameName];
        self.statusLabel.stringValue = [NSString stringWithFormat:@"FBNeo Metal - Game: %@", self.gameName];
        NSLog(@"ROM loaded successfully: %@", self.gameName);
    } else {
        NSLog(@"Failed to load ROM: %@, result: %d", romPath, result);
        self.statusLabel.stringValue = [NSString stringWithFormat:@"FBNeo Metal - Failed to load ROM: %@", [romPath lastPathComponent]];
        
        NSAlert *alert = [[NSAlert alloc] init];
        alert.messageText = @"ROM Load Failed";
        alert.informativeText = [NSString stringWithFormat:@"Failed to load ROM: %@", [romPath lastPathComponent]];
        [alert runModal];
    }
}

- (void)gameLoop {
    // Process input
    MetalInput_Make(false);
    
    // If a game is loaded, run a frame
    if (self.gameLoaded) {
        // Run a frame of the emulation with the full FBNeo core
        Metal_RunFrame(true);
        
        // Update the display
        Metal_UpdateDisplay();
    } else {
        // If no game loaded, just update the status label with current time
        NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
        [formatter setDateFormat:@"HH:mm:ss"];
        NSString *timeString = [formatter stringFromDate:[NSDate date]];
        
        if (self.gameName) {
            self.statusLabel.stringValue = [NSString stringWithFormat:@"FBNeo Metal - Game: %@ - %@", 
                                       self.gameName, timeString];
        } else {
            self.statusLabel.stringValue = [NSString stringWithFormat:@"FBNeo Metal - No ROM loaded - %@", 
                                       timeString];
        }
    }
    
    // Make the view redraw
    [self.metalView setNeedsDisplay:YES];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

- (void)applicationWillTerminate:(NSNotification *)notification {
    NSLog(@"Application terminating...");
    [self.gameTimer invalidate];
    
    // Clean up emulator
    if (self.gameLoaded) {
        BurnDrvExit_Metal();
    }
    
    // Shutdown core components
    MetalInput_Exit();
    Metal_ShutdownAI();
    BurnLibExit_Metal();
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

// Callback function from Metal implementation to update the frame in Metal
void MetalRenderer_UpdateFrame(const void* data, int width, int height) {
    static int frameCount = 0;
    bool logFrame = (++frameCount % 60) == 0;
    
    if (!g_bMetalInitialized) {
        if (logFrame) printf("MetalRenderer_UpdateFrame: Metal not initialized\n");
        return;
    }
    
    if (!g_texture) {
        if (logFrame) printf("MetalRenderer_UpdateFrame: No texture available\n");
        return;
    }
    
    if (!data) {
        if (logFrame) printf("MetalRenderer_UpdateFrame: Null data provided\n");
        return;
    }
    
    if (width <= 0 || height <= 0) {
        if (logFrame) printf("MetalRenderer_UpdateFrame: Invalid dimensions: %dx%d\n", width, height);
        return;
    }
    
    // Check if texture dimensions match
    if (g_texture.width != width || g_texture.height != height) {
        printf("MetalRenderer_UpdateFrame: Texture dimensions (%dx%d) don't match frame (%dx%d) - recreating\n", 
               (int)g_texture.width, (int)g_texture.height, width, height);
        
        // Recreate texture with correct dimensions
        MTLTextureDescriptor *textureDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                                                     width:width
                                                                                                    height:height
                                                                                                 mipmapped:NO];
        
        // Set up texture for optimal performance
        textureDescriptor.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
        if ([g_device hasUnifiedMemory]) {
            textureDescriptor.storageMode = MTLStorageModeShared;
        } else {
            textureDescriptor.storageMode = MTLStorageModePrivate;
        }
        
        // Create new texture
        id<MTLTexture> newTexture = [g_device newTextureWithDescriptor:textureDescriptor];
        if (!newTexture) {
            printf("Failed to create new texture with dimensions %dx%d\n", width, height);
            return;
        }
        
        // Replace old texture with new one
        g_texture = newTexture;
        printf("Successfully created new texture: %dx%d, format: %lu\n", 
               (int)g_texture.width, (int)g_texture.height, (unsigned long)g_texture.pixelFormat);
    }
    
    // Enhanced logging
    if (logFrame) {
        printf("=== TEXTURE UPDATE DETAILS (Frame %d) ===\n", frameCount);
        printf("Texture: %p, Dimensions: %dx%d, Format: %lu\n", 
               g_texture, (int)g_texture.width, (int)g_texture.height, 
               (unsigned long)g_texture.pixelFormat);
        
        // Check data content - first few bytes
        const uint8_t* byteData = (const uint8_t*)data;
        uint32_t checksum = 0;
        int nonZeroBytes = 0;
        int sampleSize = width * 4 * 4; // First 4 rows
        
        for (int i = 0; i < sampleSize && i < width * height * 4; i++) {
            checksum += byteData[i];
            if (byteData[i] != 0) nonZeroBytes++;
        }
        
        float nonZeroPercent = (sampleSize > 0) ? ((float)nonZeroBytes / sampleSize) * 100.0f : 0;
        
        printf("Data checksum (first %d bytes): 0x%08X, Non-zero bytes: %d (%.1f%%)\n", 
               sampleSize, checksum, nonZeroBytes, nonZeroPercent);
        
        // Sample a few pixels (RGBA format)
        if (nonZeroBytes > 0) {
            printf("Sample pixels (RGBA format):\n");
            for (int i = 0; i < 4 && i < width; i++) {
                printf("  Pixel %d: [%02X,%02X,%02X,%02X]\n", i, 
                       byteData[i*4], byteData[i*4+1], byteData[i*4+2], byteData[i*4+3]);
            }
        }
        
        printf("=========================================\n");
    }
    
    // Update the texture with the provided data
    MTLRegion region = MTLRegionMake2D(0, 0, width, height);
    
    // Use proper storage mode for update
    if (g_texture.storageMode == MTLStorageModePrivate) {
        // For discrete GPUs, we need to use a buffer and blit
        id<MTLBuffer> stagingBuffer = [g_device newBufferWithBytes:data
                                                           length:width * height * 4
                                                          options:MTLResourceStorageModeShared];
        
        id<MTLCommandBuffer> commandBuffer = [g_commandQueue commandBuffer];
        id<MTLBlitCommandEncoder> blitEncoder = [commandBuffer blitCommandEncoder];
        
        [blitEncoder copyFromBuffer:stagingBuffer
                       sourceOffset:0
                  sourceBytesPerRow:width * 4
                sourceBytesPerImage:width * height * 4
                         sourceSize:MTLSizeMake(width, height, 1)
                          toTexture:g_texture
                   destinationSlice:0
                   destinationLevel:0
                  destinationOrigin:MTLOriginMake(0, 0, 0)];
        
        [blitEncoder endEncoding];
        [commandBuffer commit];
        
        // Synchronize for testing/debugging - don't do this in production
        if (logFrame) printf("Waiting for GPU command buffer to complete...\n");
        [commandBuffer waitUntilCompleted];
        if (logFrame) printf("GPU command buffer completed\n");
    } else {
        // For unified memory (Apple Silicon), we can update directly
        [g_texture replaceRegion:region
                     mipmapLevel:0
                       withBytes:data
                     bytesPerRow:width * 4];
        
        if (logFrame) printf("Updated texture directly via unified memory\n");
    }
    
    // Store the current width and height
    g_nFrameWidth = width;
    g_nFrameHeight = height;
    
    // Make sure the view gets redrawn - use main thread for UI updates
    if (g_metalView) {
        if (logFrame) printf("Requesting redraw of Metal view\n");
        dispatch_async(dispatch_get_main_queue(), ^{
            [g_metalView setNeedsDisplay:YES];
        });
    } else if (logFrame) {
        printf("WARNING: No Metal view available for redraw\n");
    }
}

// Load a ROM
int Metal_LoadROM(const char* romPath) {
    if (!romPath) {
        NSLog(@"ROM path is NULL");
        return 1;
    }
    
    NSLog(@"Metal_LoadROM: Forwarding to enhanced ROM loader: %s", romPath);
    
    // Forward to the enhanced ROM loader implementation
    extern int Metal_LoadROM_Enhanced(const char* romPath);
    return Metal_LoadROM_Enhanced(romPath);
}

// Enhanced ROM loader implementation
int Metal_LoadROM_Enhanced(const char* romPath) {
    // This just forwards to the C++ implementation in metal_rom_loader.cpp
    if (!romPath) {
        NSLog(@"ROM path is NULL");
        return 1;
    }
    
    // Include our custom ROM loader header
    #include "metal_rom_loader.h"
    
    // Forward to the C++ implementation
    return Metal_LoadROM(romPath);
}

} // extern "C"

// Main entry point
int main(int argc, const char* argv[]) {
    @autoreleasepool {
        NSLog(@"Starting FBNeo Metal");
        
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

// Configure frame buffer
void Metal_SetFrameBufferSize(int width, int height) {
    // Validate dimensions
    if (width <= 0 || height <= 0) {
        printf("Invalid dimensions: %d x %d\n", width, height);
        return;
    }
    
    // Calculate buffer size (RGBA, 4 bytes per pixel)
    int bufferSize = width * height * 4;
    
    // Reallocate buffer if needed
    if (g_textureWidth != width || g_textureHeight != height || !g_textureBuffer) {
        if (g_textureBuffer) {
            free(g_textureBuffer);
        }
        
        g_textureBuffer = (unsigned char*)malloc(bufferSize);
        if (!g_textureBuffer) {
            printf("Failed to allocate texture buffer\n");
            return;
        }
        
        // Clear the buffer to black
        memset(g_textureBuffer, 0, bufferSize);
        
        g_textureBufferSize = bufferSize;
        g_textureWidth = width;
        g_textureHeight = height;
        
        printf("Metal texture buffer resized to %d x %d (%d bytes)\n", width, height, bufferSize);
    }
    
    // Set frame buffer values for FBNeo core
    pBurnDraw_Metal = g_textureBuffer;
    nBurnPitch_Metal = width * 4;
    nBurnBpp_Metal = 4;
}

// Update Metal texture with current frame data
int MetalRenderer_UpdateTexture(void* data, int width, int height, int pitch) {
    if (!data || width <= 0 || height <= 0 || pitch <= 0) {
        printf("MetalRenderer_UpdateTexture: Invalid parameters\n");
        return METAL_ERROR_TEXTURE_CREATE;
    }
    
    // If dimensions don't match current texture, resize
    if (width != g_textureWidth || height != g_textureHeight) {
        Metal_SetFrameBufferSize(width, height);
    }
    
    // Verify we have a buffer
    if (!g_textureBuffer) {
        printf("MetalRenderer_UpdateTexture: No texture buffer\n");
        return METAL_ERROR_TEXTURE_CREATE;
    }
    
    // Copy frame data to texture buffer
    unsigned char* src = (unsigned char*)data;
    unsigned char* dst = g_textureBuffer;
    
    // If pitch matches width*4, copy the entire frame at once
    if (pitch == width * 4) {
        memcpy(dst, src, width * height * 4);
    } else {
        // Otherwise, copy line by line
        for (int y = 0; y < height; y++) {
            memcpy(dst + y * width * 4, src + y * pitch, width * 4);
        }
    }
    
    // Detailed logging of the first few pixels for debugging
    printf("MetalRenderer_UpdateTexture: Updated %dx%d texture (pitch: %d)\n", width, height, pitch);
    printf("First 4 pixels (BGRA): ");
    for (int i = 0; i < 4; i++) {
        int offset = i * 4;
        printf("[%02X,%02X,%02X,%02X] ", 
               dst[offset], dst[offset+1], dst[offset+2], dst[offset+3]);
    }
    printf("\n");
    
    // Calculate the non-zero bytes in the buffer
    int nonZeroBytes = 0;
    for (int i = 0; i < width * height * 4; i++) {
        if (dst[i] != 0) {
            nonZeroBytes++;
        }
    }
    printf("Non-zero bytes in texture: %d (%.2f%%)\n", 
           nonZeroBytes, (float)nonZeroBytes / (width * height * 4) * 100.0f);
    
    return METAL_ERROR_NONE;
}

// Update the current frame (called by the game engine)
int MetalRenderer_UpdateFrame() {
    if (!g_textureBuffer || g_textureWidth <= 0 || g_textureHeight <= 0) {
        return METAL_ERROR_TEXTURE_CREATE;
    }
    
    // Get the current frame dimensions
    int width = Metal_GetFrameWidth();
    int height = Metal_GetFrameHeight();
    
    printf("MetalRenderer_UpdateFrame: Frame dimensions %dx%d\n", width, height);
    
    // Validate the frame buffer
    if (!pBurnDraw_Metal) {
        printf("MetalRenderer_UpdateFrame: No frame buffer\n");
        return METAL_ERROR_TEXTURE_CREATE;
    }
    
    // Compute checksum of the first few scan lines to check for content
    unsigned int checksum = 0;
    for (int i = 0; i < 100 && i < g_textureBufferSize; i++) {
        checksum = (checksum * 31) + pBurnDraw_Metal[i];
    }
    printf("MetalRenderer_UpdateFrame: Frame buffer checksum: 0x%08X\n", checksum);
    
    // Pass the frame data to the Metal renderer
    int result = MetalRenderer_UpdateTexture(pBurnDraw_Metal, width, height, nBurnPitch_Metal);
    
    return result;
}

// Initialize the Metal renderer
int MetalRenderer_Init(void* view) {
    // Store the Metal view
    g_mtkView = (__bridge MTKView*)view;
    if (!g_mtkView) {
        printf("MetalRenderer_Init: Invalid view\n");
        return METAL_ERROR_NO_VIEW;
    }
    
    // Get the Metal device
    g_device = g_mtkView.device;
    if (!g_device) {
        printf("MetalRenderer_Init: No Metal device\n");
        return METAL_ERROR_NO_DEVICE;
    }
    
    // Configure the view
    g_mtkView.colorPixelFormat = g_textureFormat;
    g_mtkView.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
    
    printf("MetalRenderer_Init: Initialized with device: %s\n", [g_device.name UTF8String]);
    
    // Allocate initial frame buffer
    Metal_SetFrameBufferSize(320, 240);
    
    return METAL_ERROR_NONE;
}

// Shutdown the Metal renderer
void MetalRenderer_Shutdown() {
    printf("MetalRenderer_Shutdown: Shutting down\n");
    
    // Free texture buffer
    if (g_textureBuffer) {
        free(g_textureBuffer);
        g_textureBuffer = NULL;
    }
    
    // Release Metal objects
    g_device = nil;
    g_mtkView = nil;
}

// Set Metal renderer option
void MetalRenderer_SetOption(int option, int value) {
    printf("MetalRenderer_SetOption: Setting option %d to %d\n", option, value);
    
    // In a real implementation, this would configure the renderer options
}

// Draw the current frame
int MetalRenderer_DrawFrame(int draw) {
    if (!g_mtkView) {
        return METAL_ERROR_NO_VIEW;
    }
    
    if (draw) {
        // Trigger a redraw in the Metal view
        [g_mtkView setNeedsDisplay];
    }
    
    return METAL_ERROR_NONE;
}

// Render the current frame
int MetalRenderer_RenderFrame(void* frameData, int width, int height, int pitch) {
    if (!frameData || width <= 0 || height <= 0) {
        return METAL_ERROR_TEXTURE_CREATE;
    }
    
    // Update the texture with the frame data
    int result = MetalRenderer_UpdateTexture(frameData, width, height, pitch);
    if (result != METAL_ERROR_NONE) {
        return result;
    }
    
    // Draw the frame
    return MetalRenderer_DrawFrame(1);
}

// Run a frame of the game
int Metal_RunFrame(int bDraw) {
    // In a real implementation, this would run a frame of the game
    // and update the display if bDraw is true
    
    if (bDraw) {
        // Validate our frame buffer
        if (!pBurnDraw_Metal) {
            printf("Metal_RunFrame: No frame buffer\n");
            return 1;
        }
        
        // Calculate a simple checksum of the frame buffer to detect if it has content
        unsigned int checksum = 0;
        size_t bufferSize = g_textureWidth * g_textureHeight * 4;
        size_t checkSize = bufferSize > 1024 ? 1024 : bufferSize;
        
        for (size_t i = 0; i < checkSize; i++) {
            checksum = (checksum * 31) + pBurnDraw_Metal[i];
        }
        
        printf("Metal_RunFrame: Frame buffer checksum: 0x%08X\n", checksum);
        
        // Count non-zero bytes in the buffer
        int nonZeroBytes = 0;
        for (size_t i = 0; i < checkSize; i++) {
            if (pBurnDraw_Metal[i] != 0) {
                nonZeroBytes++;
            }
        }
        
        float nonZeroPercent = (float)nonZeroBytes / checkSize * 100.0f;
        printf("Metal_RunFrame: Non-zero bytes in first %zu bytes: %d (%.2f%%)\n", 
               checkSize, nonZeroBytes, nonZeroPercent);
        
        // Update the Metal texture
        MetalRenderer_UpdateFrame();
        
        // Draw the frame
        MetalRenderer_DrawFrame(1);
    }
    
    return 0;
}

// Update texture with frame data
int Metal_UpdateTexture(void* data, int width, int height, int pitch) {
    return MetalRenderer_UpdateTexture(data, width, height, pitch);
}

// Initialize Metal
int Metal_Initialize() {
    printf("Metal_Initialize: Initializing Metal\n");
    return 0;
}

// Shutdown Metal
void Metal_Shutdown() {
    printf("Metal_Shutdown: Shutting down Metal\n");
    MetalRenderer_Shutdown();
}

// Generate a test pattern for debugging
int Metal_GenerateTestPattern(int patternType) {
    if (!g_textureBuffer || g_textureWidth <= 0 || g_textureHeight <= 0) {
        printf("Metal_GenerateTestPattern: No texture buffer\n");
        return 1;
    }
    
    printf("Metal_GenerateTestPattern: Generating pattern type %d (%dx%d)\n", 
           patternType, g_textureWidth, g_textureHeight);
    
    // Color defined as [B, G, R, A] for BGRA format
    unsigned char colors[8][4] = {
        {0, 0, 0, 255},       // Black
        {0, 0, 255, 255},     // Red
        {0, 255, 0, 255},     // Green
        {0, 255, 255, 255},   // Yellow
        {255, 0, 0, 255},     // Blue
        {255, 0, 255, 255},   // Magenta
        {255, 255, 0, 255},   // Cyan
        {255, 255, 255, 255}  // White
    };
    
    // Generate different test patterns based on type
    switch (patternType) {
        case 0: // Color bars
            {
                int barWidth = g_textureWidth / 8;
                for (int y = 0; y < g_textureHeight; y++) {
                    for (int x = 0; x < g_textureWidth; x++) {
                        int barIndex = x / barWidth;
                        if (barIndex >= 8) barIndex = 7;
                        
                        int offset = (y * g_textureWidth + x) * 4;
                        g_textureBuffer[offset] = colors[barIndex][0];     // B
                        g_textureBuffer[offset+1] = colors[barIndex][1];   // G
                        g_textureBuffer[offset+2] = colors[barIndex][2];   // R
                        g_textureBuffer[offset+3] = colors[barIndex][3];   // A
                    }
                }
            }
            break;
            
        case 1: // Gradient
            {
                for (int y = 0; y < g_textureHeight; y++) {
                    for (int x = 0; x < g_textureWidth; x++) {
                        int r = (x * 255) / g_textureWidth;
                        int g = (y * 255) / g_textureHeight;
                        int b = 255 - ((x + y) * 255) / (g_textureWidth + g_textureHeight);
                        
                        int offset = (y * g_textureWidth + x) * 4;
                        g_textureBuffer[offset] = b;     // B
                        g_textureBuffer[offset+1] = g;   // G
                        g_textureBuffer[offset+2] = r;   // R
                        g_textureBuffer[offset+3] = 255; // A
                    }
                }
            }
            break;
            
        case 2: // Checkerboard
            {
                int squareSize = 32;
                for (int y = 0; y < g_textureHeight; y++) {
                    for (int x = 0; x < g_textureWidth; x++) {
                        int isWhite = ((x / squareSize) + (y / squareSize)) % 2;
                        
                        int offset = (y * g_textureWidth + x) * 4;
                        g_textureBuffer[offset] = isWhite ? 255 : 0;     // B
                        g_textureBuffer[offset+1] = isWhite ? 255 : 0;   // G
                        g_textureBuffer[offset+2] = isWhite ? 255 : 0;   // R
                        g_textureBuffer[offset+3] = 255;                 // A
                    }
                }
            }
            break;
            
        case 3: // Border
            {
                int borderWidth = 16;
                
                // Fill with black
                memset(g_textureBuffer, 0, g_textureWidth * g_textureHeight * 4);
                
                // Draw red border
                for (int y = 0; y < g_textureHeight; y++) {
                    for (int x = 0; x < g_textureWidth; x++) {
                        if (x < borderWidth || x >= g_textureWidth - borderWidth || 
                            y < borderWidth || y >= g_textureHeight - borderWidth) {
                            
                            int offset = (y * g_textureWidth + x) * 4;
                            g_textureBuffer[offset] = 0;     // B
                            g_textureBuffer[offset+1] = 0;   // G
                            g_textureBuffer[offset+2] = 255; // R (red)
                            g_textureBuffer[offset+3] = 255; // A
                        }
                    }
                }
            }
            break;
            
        default: // RGB pattern
            {
                for (int y = 0; y < g_textureHeight; y++) {
                    for (int x = 0; x < g_textureWidth; x++) {
                        int section = (3 * x) / g_textureWidth;
                        
                        int offset = (y * g_textureWidth + x) * 4;
                        g_textureBuffer[offset] = (section == 0) ? 255 : 0;     // B
                        g_textureBuffer[offset+1] = (section == 1) ? 255 : 0;   // G
                        g_textureBuffer[offset+2] = (section == 2) ? 255 : 0;   // R
                        g_textureBuffer[offset+3] = 255;                        // A
                    }
                }
            }
            break;
    }
    
    // Update the texture
    if (g_mtkView) {
        [g_mtkView setNeedsDisplay];
    }
    
    return 0;
}

// Verify that the frame pipeline is working
int Metal_VerifyFramePipeline(int width, int height) {
    printf("Metal_VerifyFramePipeline: Checking pipeline for %dx%d frames\n", width, height);
    
    // Resize the frame buffer if needed
    if (g_textureWidth != width || g_textureHeight != height) {
        Metal_SetFrameBufferSize(width, height);
    }
    
    // Generate a test pattern
    Metal_GenerateTestPattern(0); // Color bars
    
    // Render the frame
    int result = MetalRenderer_DrawFrame(1);
    
    return result;
} 