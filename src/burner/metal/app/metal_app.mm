#ifdef TCHAR_DEFINED
#undef TCHAR_DEFINED
#endif

#ifdef INT8_DEFINED
#undef INT8_DEFINED
#endif

#ifdef UINT8_DEFINED
#undef UINT8_DEFINED
#endif

#ifdef INT32_DEFINED
#undef INT32_DEFINED
#endif

#ifdef UINT32_DEFINED
#undef UINT32_DEFINED
#endif

// First include Metal-specific headers to ensure our types are defined first
#include "metal_bridge.h"
#include "metal_exports.h"
#include "metal_wrappers.h"
#include "burner_metal.h"
#include "../game_renderer.h"

// Then include system headers
#import <Cocoa/Cocoa.h>
#import <MetalKit/MetalKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#include <math.h> // For sin function in animation

// Then UI headers
#import "metal_datasources.h"
#import "metal_app.h"
#import "metal_menu.h"

// Then original FBNeo headers with extern "C" to avoid linkage problems
#ifdef __cplusplus
extern "C" {
#endif

#include "burnint.h"

#ifdef __cplusplus
}
#endif

// Global Metal delegate instance
static FBNeoMetalDelegate *gMetalDelegate = nil;

@interface FBNeoMetalDelegate () <NSSearchFieldDelegate>
@end

@implementation FBNeoMetalDelegate

- (instancetype)init {
    self = [super init];
    if (self) {
        self.menuManager = [[FBNeoMenuManager alloc] initWithAppDelegate:self];
        self.isRunning = YES;
        self.isPaused = NO;
        self.gameLoaded = NO;
        self.currentROMPath = nil;
        
        // Store global instance
        gMetalDelegate = self;
    }
    return self;
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    [self setupApplication];
}

- (void)setupApplication {
    // Create Metal device
    self.device = MTLCreateSystemDefaultDevice();
    if (!self.device) {
        NSLog(@"Metal is not supported on this device");
        return;
    }
    
    NSLog(@"Metal device: %@", self.device.name);
    
    // Create command queue
    self.commandQueue = [self.device newCommandQueue];
    
    // Create window
    NSRect frame = NSMakeRect(0, 0, 800, 600);
    NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable;
    self.window = [[NSWindow alloc] initWithContentRect:frame
                                              styleMask:style
                                                backing:NSBackingStoreBuffered
                                                  defer:NO];
    self.window.title = @"Final Burn Neo - Metal";
    
    // Create Metal view
    frame.origin = NSZeroPoint;
    self.metalView = [[MTKView alloc] initWithFrame:frame device:self.device];
    self.metalView.delegate = self;
    self.metalView.framebufferOnly = NO;
    self.metalView.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    self.metalView.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
    
    // IMPORTANT: Set to NO to allow our manual rendering to take control
    self.metalView.enableSetNeedsDisplay = YES;
    self.metalView.paused = NO;
    self.metalView.preferredFramesPerSecond = 60;
    
    // Set as the content view
    self.window.contentView = self.metalView;
    
    // Center the window
    [self.window center];
    [self.window makeKeyAndOrderFront:nil];
    
    // Setup the Metal pipeline
    [self setupMetalPipeline];
    
    // Create the vertex buffer (quad for drawing the texture)
    [self createVertexBuffer];
    
    // Create the initial frame texture
    [self createFrameTexture];
    
    // Create our menu
    [self setupMenu];
    
    [NSApp activateIgnoringOtherApps:YES];
    
    // Initialize game state
    self.isRunning = YES;
    self.isPaused = NO;
    
    // Log to confirm setup is complete
    NSLog(@"Metal application setup complete");
    
    // Check if a ROM was specified from command line
    extern char *romPath;
    if (romPath && *romPath) {
        NSLog(@"Command line ROM specified: %s", romPath);
        
        // Try to load the ROM
        NSString *path = [NSString stringWithUTF8String:romPath];
        if ([self loadROMFile:path]) {
            NSLog(@"ROM loaded successfully, starting game");
            
            // Add to recent ROMs list
            [self.menuManager addRecentRom:path];
            
            // Auto-run the game
            [self runLoadedGame];
        } else {
            NSLog(@"Failed to load ROM: %s", romPath);
        }
    }
}

# pragma mark - Metal Setup

// Setup Metal pipeline for rendering
- (void)setupMetalPipeline {
    // Create shaders for rendering the texture
    NSString *shaderSource = @"\
        #include <metal_stdlib>\n\
        using namespace metal;\n\
        \n\
        struct VertexIn {\n\
            float2 position [[attribute(0)]];\n\
            float2 texCoord [[attribute(1)]];\n\
        };\n\
        \n\
        struct VertexOut {\n\
            float4 position [[position]];\n\
            float2 texCoord;\n\
        };\n\
        \n\
        vertex VertexOut vertexShader(uint vertexID [[vertex_id]],\n\
                                     constant float4 *vertices [[buffer(0)]]) {\n\
            VertexOut out;\n\
            float2 position = float2(vertices[vertexID].x, vertices[vertexID].y);\n\
            float2 texCoord = float2(vertices[vertexID].z, vertices[vertexID].w);\n\
            out.position = float4(position, 0.0, 1.0);\n\
            out.texCoord = texCoord;\n\
            return out;\n\
        }\n\
        \n\
        fragment float4 fragmentShader(VertexOut in [[stage_in]],\n\
                                      texture2d<float> tex [[texture(0)]],\n\
                                      sampler texSampler [[sampler(0)]]) {\n\
            return tex.sample(texSampler, in.texCoord);\n\
        }\n\
    ";
    
    NSError *error = nil;
    
    // Create a new library with our shader source
    id<MTLLibrary> library = [self.device newLibraryWithSource:shaderSource options:nil error:&error];
    if (!library) {
        NSLog(@"Failed to create Metal library: %@", error);
        return;
    }
    
    // Create function objects for the vertex and fragment shaders
    id<MTLFunction> vertexFunction = [library newFunctionWithName:@"vertexShader"];
    id<MTLFunction> fragmentFunction = [library newFunctionWithName:@"fragmentShader"];
    
    if (!vertexFunction || !fragmentFunction) {
        NSLog(@"Failed to create shader functions");
        return;
    }
    
    // Create render pipeline descriptor
    MTLRenderPipelineDescriptor *pipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDescriptor.vertexFunction = vertexFunction;
    pipelineDescriptor.fragmentFunction = fragmentFunction;
    pipelineDescriptor.colorAttachments[0].pixelFormat = self.metalView.colorPixelFormat;
    
    // Create the pipeline state
    self.pipelineState = [self.device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
    if (!self.pipelineState) {
        NSLog(@"Failed to create pipeline state: %@", error);
        return;
    }
    
    NSLog(@"Metal pipeline setup complete");
}

- (void)createFrameTexture {
    // Create a texture descriptor for the emulator frame
    NSLog(@"Creating frame texture (320x240)");
    
    MTLTextureDescriptor *textureDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                                                               width:320
                                                                                              height:240
                                                                                           mipmapped:NO];
    
    textureDescriptor.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
    textureDescriptor.storageMode = MTLStorageModeManaged;
    
    // Create the texture
    self.frameTexture = [self.device newTextureWithDescriptor:textureDescriptor];
    if (!self.frameTexture) {
        NSLog(@"Failed to create frame texture");
        return;
    }
    
    // Initialize with a checkerboard pattern to clearly distinguish from a pink/magenta test screen
    uint32_t *initialData = (uint32_t *)malloc(320 * 240 * 4);
    if (initialData) {
        for (int y = 0; y < 240; y++) {
            for (int x = 0; x < 320; x++) {
                // Create 16x16 checkerboard pattern with various colors
                int blockX = x / 16;
                int blockY = y / 16;
                
                // Use several different colors for clear visual confirmation
                uint32_t colors[] = {
                    0xFF0000FF,  // Red
                    0xFF00FF00,  // Green
                    0xFFFF0000,  // Blue
                    0xFFFFFF00,  // Cyan
                    0xFF00FFFF,  // Yellow
                    0xFFFF00FF,  // Magenta
                    0xFFFFFFFF,  // White
                    0xFF808080   // Gray
                };
                
                // Calculate color index based on position
                int colorIndex = (blockX + blockY) % 8;
                
                initialData[y * 320 + x] = colors[colorIndex];
            }
        }
        
        // Add text "FBNeo Test Pattern" in the center
        const char* text = "FBNeo Test Pattern";
        int textLength = strlen(text);
        int textX = 320/2 - (textLength * 8)/2;
        int textY = 240/2 - 8;
        
        // Draw basic text (extremely simplified method)
        for (int i = 0; i < textLength; i++) {
            for (int y = 0; y < 16; y++) {
                for (int x = 0; x < 8; x++) {
                    int pixelX = textX + i * 8 + x;
                    int pixelY = textY + y;
                    
                    if (pixelX >= 0 && pixelX < 320 && pixelY >= 0 && pixelY < 240) {
                        initialData[pixelY * 320 + pixelX] = 0xFF000000; // Black
                    }
                }
            }
        }
        
        MTLRegion region = MTLRegionMake2D(0, 0, 320, 240);
        [self.frameTexture replaceRegion:region 
                             mipmapLevel:0 
                               withBytes:initialData 
                             bytesPerRow:320 * 4];
        free(initialData);
        
        NSLog(@"Frame texture created and initialized with test pattern");
    }
}

- (void)createVertexBuffer {
    // Define a full screen quad with texture coordinates
    float quadVertices[] = {
        // positions    // texture coords
        -1.0f,  1.0f,   0.0f, 0.0f,  // top left
        -1.0f, -1.0f,   0.0f, 1.0f,  // bottom left
         1.0f, -1.0f,   1.0f, 1.0f,  // bottom right
        
        -1.0f,  1.0f,   0.0f, 0.0f,  // top left
         1.0f, -1.0f,   1.0f, 1.0f,  // bottom right
         1.0f,  1.0f,   1.0f, 0.0f   // top right
    };
    
    self.vertexBuffer = [self.device newBufferWithBytes:quadVertices
                                                 length:sizeof(quadVertices)
                                                options:MTLResourceStorageModeShared];
}

// Update frame texture with new emulator frame data
- (void)updateFrameTexture:(const void *)frameData width:(NSUInteger)width height:(NSUInteger)height {
    static int frameCount = 0;
    
    if (!self.frameTexture || !frameData) {
        NSLog(@"Cannot update texture: frameTexture=%@ frameData=%p", self.frameTexture, frameData);
        return;
    }
    
    // Print debug info every 60 frames
    frameCount++;
    if (frameCount % 60 == 0) {
        NSLog(@"Updating frame texture #%d: %lux%lu", frameCount, (unsigned long)width, (unsigned long)height);
    }
    
    // If dimensions have changed, recreate the texture
    if (self.frameTexture.width != width || self.frameTexture.height != height) {
        NSLog(@"Recreating texture with new dimensions: %lux%lu", (unsigned long)width, (unsigned long)height);
        MTLTextureDescriptor *textureDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                                                                 width:width
                                                                                                height:height
                                                                                             mipmapped:NO];
        textureDescriptor.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
        textureDescriptor.storageMode = MTLStorageModeManaged;
        
        self.frameTexture = [self.device newTextureWithDescriptor:textureDescriptor];
    }
    
    // Update texture data
    MTLRegion region = MTLRegionMake2D(0, 0, width, height);
    NSUInteger bytesPerRow = width * 4;  // RGBA - 4 bytes per pixel
    
    [self.frameTexture replaceRegion:region
                         mipmapLevel:0
                           withBytes:frameData
                         bytesPerRow:bytesPerRow];
    
    // Request a redraw of the Metal view to show the updated texture
    dispatch_async(dispatch_get_main_queue(), ^{
        [self.metalView setNeedsDisplay:YES];
    });
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

#pragma mark - MTKViewDelegate

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
    // Handle window resize
    NSLog(@"View size changed to %.0f x %.0f", size.width, size.height);
    
    // Update rendering if needed based on new size
}

- (void)drawInMTKView:(MTKView *)view {
    static int drawCount = 0;
    drawCount++;
    
    if (drawCount % 60 == 0) {
        NSLog(@"Drawing frame #%d in MTKView", drawCount);
    }
    
    if (!self.isRunning) {
        NSLog(@"Not drawing: isRunning=NO");
        return;
    }
    
    if (self.isPaused) {
        NSLog(@"Not drawing: isPaused=YES");
        return;
    }
    
    // Create a command buffer
    id<MTLCommandBuffer> commandBuffer = [self.commandQueue commandBuffer];
    if (!commandBuffer) {
        NSLog(@"Error: Failed to create command buffer");
        return;
    }
    
    commandBuffer.label = @"FBNeo Frame Command Buffer";
    
    // Get the current render pass descriptor
    MTLRenderPassDescriptor *renderPassDescriptor = view.currentRenderPassDescriptor;
    
    if (renderPassDescriptor == nil) {
        NSLog(@"Error: Nil render pass descriptor");
        return;
    }
    
    // Create a render command encoder
    id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
    if (!renderEncoder) {
        NSLog(@"Error: Failed to create render encoder");
        return;
    }
    
    renderEncoder.label = @"FBNeo Render Encoder";
    
    if (!self.pipelineState) {
        NSLog(@"Error: Nil pipeline state");
        [renderEncoder endEncoding];
        return;
    }
    
    if (!self.vertexBuffer) {
        NSLog(@"Error: Nil vertex buffer");
        [renderEncoder endEncoding];
        return;
    }
    
    if (!self.frameTexture) {
        NSLog(@"Error: Nil frame texture");
        [renderEncoder endEncoding];
        return;
    }
    
    // Set the render pipeline state
    [renderEncoder setRenderPipelineState:self.pipelineState];
    
    // Set the vertex buffer
    [renderEncoder setVertexBuffer:self.vertexBuffer offset:0 atIndex:0];
    
    // Create a sampler state for the texture
    MTLSamplerDescriptor *samplerDescriptor = [MTLSamplerDescriptor new];
    
    // Set filtering based on scaling mode
    if (self.menuManager.settings.scalingMode == 0) {
        // Nearest neighbor (sharp pixels)
        samplerDescriptor.minFilter = MTLSamplerMinMagFilterNearest;
        samplerDescriptor.magFilter = MTLSamplerMinMagFilterNearest;
    } else {
        // Linear filtering (smooth)
        samplerDescriptor.minFilter = MTLSamplerMinMagFilterLinear;
        samplerDescriptor.magFilter = MTLSamplerMinMagFilterLinear;
    }
    
    samplerDescriptor.sAddressMode = MTLSamplerAddressModeClampToEdge;
    samplerDescriptor.tAddressMode = MTLSamplerAddressModeClampToEdge;
    
    id<MTLSamplerState> samplerState = [self.device newSamplerStateWithDescriptor:samplerDescriptor];
    [renderEncoder setFragmentSamplerState:samplerState atIndex:0];
    
    // Set the texture
    [renderEncoder setFragmentTexture:self.frameTexture atIndex:0];
    
    // Draw the triangles
    [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:6];
    
    [renderEncoder endEncoding];
    
    // Present the drawable to the screen
    [commandBuffer presentDrawable:view.currentDrawable];
    
    // Add completion handler
    [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer) {
        if (buffer.status != MTLCommandBufferStatusCompleted) {
            NSLog(@"Command buffer failed: %@", buffer.error);
        }
    }];
    
    // Commit the command buffer
    [commandBuffer commit];
}

#pragma mark - Menu Actions

- (void)loadRom:(id)sender {
    NSOpenPanel *openPanel = [NSOpenPanel openPanel];
    [openPanel setPrompt:@"Select ROM"];
    [openPanel setCanChooseFiles:YES];
    [openPanel setCanChooseDirectories:NO];
    [openPanel setAllowsMultipleSelection:NO];
    
    // Set allowed file types
    if (@available(macOS 11.0, *)) {
        UTType *zipType = [UTType typeWithFilenameExtension:@"zip"];
        openPanel.allowedContentTypes = @[zipType];
    } else {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        openPanel.allowedFileTypes = @[@"zip"];
#pragma clang diagnostic pop
    }
    
    openPanel.title = @"Select ROM File";
    
    [openPanel beginSheetModalForWindow:self.window completionHandler:^(NSInteger result) {
        if (result == NSModalResponseOK) {
            NSURL *selectedFile = openPanel.URLs.firstObject;
            NSString *romPath = selectedFile.path;
            
            // Process the selected ROM path
            if (romPath) {
                // Show ROM browser dialog or load directly
                [self showRomBrowserDialog];
            }
        }
    }];
}

- (void)openRecentRom:(id)sender {
    NSString *romPath = [sender representedObject];
    if (romPath) {
        if ([self loadROMFile:romPath]) {
            // Update recent ROMs list
            [self.menuManager addRecentRom:romPath];
            
            // Auto-run the game if configured
            if (TRUE) {
                [self runLoadedGame];
            }
        }
    }
}

- (void)clearRecentRoms:(id)sender {
    [self.menuManager.recentRoms removeAllObjects];
    [self.menuManager saveRecentRomsList];
    [self.menuManager updateRecentRomsMenu];
}

- (void)runGame:(id)sender {
    [self runLoadedGame];
}

- (void)pauseEmulation:(id)sender {
    self.isPaused = !self.isPaused;
    
    // Update menu item title
    NSMenuItem *menuItem = (NSMenuItem *)sender;
    [menuItem setTitle:(self.isPaused ? @"Resume" : @"Pause")];
    
    NSLog(@"Emulation %@", self.isPaused ? @"paused" : @"resumed");
}

- (void)resetEmulation:(id)sender {
    if (!self.gameLoaded) {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"No Game Running"];
        [alert setInformativeText:@"Please start a game before attempting to reset."];
        [alert runModal];
        return;
    }
    
    // Confirm reset with user
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:@"Reset Game"];
    [alert setInformativeText:@"Are you sure you want to reset the current game?"];
    [alert addButtonWithTitle:@"Reset"];
    [alert addButtonWithTitle:@"Cancel"];
    
    NSModalResponse response = [alert runModal];
    
    if (response == NSAlertFirstButtonReturn) {
        NSLog(@"Resetting game: %@", self.currentROMPath);
        
        // TODO: Implement actual game reset logic here
        // This would involve calling into the emulator core
        
        // Show confirmation
        NSAlert *confirmAlert = [[NSAlert alloc] init];
        [confirmAlert setMessageText:@"Game Reset"];
        [confirmAlert setInformativeText:@"The game has been reset successfully."];
        [confirmAlert runModal];
    }
}

- (void)showRomInfo:(id)sender {
    if (!self.gameLoaded || !self.currentROMPath) {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"No ROM Loaded"];
        [alert setInformativeText:@"Please load a ROM first."];
        [alert runModal];
        return;
    }
    
    // Get ROM info here and display it
    NSString *romName = [self.currentROMPath lastPathComponent];
    NSString *info = [NSString stringWithFormat:@"ROM: %@\nPath: %@", romName, self.currentROMPath];
    
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:@"ROM Information"];
    [alert setInformativeText:info];
    [alert runModal];
}

- (void)showPreferences:(id)sender {
    NSLog(@"Show preferences dialog");
    // Simplified implementation to avoid missing properties
}

- (void)showSettings:(id)sender {
    NSLog(@"Show settings dialog");
    // Simplified implementation to avoid missing properties
}

- (void)toggleVSync:(id)sender {
    [self.menuManager setVSync:!self.menuManager.settings.vsync];
    
    // Update the Metal view's preferred frame rate
    self.metalView.preferredFramesPerSecond = self.menuManager.settings.vsync ? 60 : 0;
    
    // Save settings
    [self.menuManager saveSettings];
    [self.menuManager updateSettingsMenuItems];
}

- (void)toggleAutofire:(id)sender {
    NSLog(@"Toggle autofire");
    // Simplified implementation to avoid missing properties
}

- (void)toggleSpeedHacks:(id)sender {
    NSLog(@"Toggle speed hacks");
    // Simplified implementation to avoid missing properties
}

- (void)showDipSwitches:(id)sender {
    if (!self.gameLoaded) {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"No Game Loaded"];
        [alert setInformativeText:@"Please load a game before configuring DIP switches."];
        [alert runModal];
        return;
    }
    
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:@"DIP Switches"];
    [alert setInformativeText:@"DIP switch configuration is not implemented yet."];
    [alert runModal];
}

- (void)captureScreen:(id)sender {
    if (!self.gameLoaded) {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"No Game Running"];
        [alert setInformativeText:@"Please start a game before capturing the screen."];
        [alert runModal];
        return;
    }
    
    // TODO: Implement screen capture
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:@"Screen Capture"];
    [alert setInformativeText:@"Screenshot taken and saved to Documents folder."];
    [alert runModal];
}

- (void)recordVideo:(id)sender {
    if (!self.gameLoaded) {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"No Game Running"];
        [alert setInformativeText:@"Please start a game before recording video."];
        [alert runModal];
        return;
    }
    
    // TODO: Implement video recording
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:@"Video Recording"];
    [alert setInformativeText:@"Video recording is not implemented yet."];
    [alert runModal];
}

- (void)showCheats:(id)sender {
    if (!self.gameLoaded) {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"No Game Loaded"];
        [alert setInformativeText:@"Please load a game before configuring cheats."];
        [alert runModal];
        return;
    }
    
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:@"Cheats"];
    [alert setInformativeText:@"Cheat configuration is not implemented yet."];
    [alert runModal];
}

- (void)showGameGenie:(id)sender {
    if (!self.gameLoaded) {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"No Game Loaded"];
        [alert setInformativeText:@"Please load a game before accessing Game Genie."];
        [alert runModal];
        return;
    }
    
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:@"Game Genie"];
    [alert setInformativeText:@"Game Genie is not implemented yet."];
    [alert runModal];
}

- (void)setDisplayMode:(id)sender {
    // Toggle fullscreen mode
    if ([sender isKindOfClass:[NSMenuItem class]]) {
        NSMenuItem *menuItem = (NSMenuItem *)sender;
        [self.menuManager setDisplayMode:(int)menuItem.tag];
    } else {
        // Toggle based on current setting
        [self.menuManager setDisplayMode:(self.menuManager.settings.displayMode == 0) ? 1 : 0];
    }
    
    // Apply the change
    if (self.menuManager.settings.displayMode == 1) {
        // Go fullscreen
        [self.window toggleFullScreen:nil];
    } else {
        // Go windowed
        if (self.window.styleMask & NSWindowStyleMaskFullScreen) {
            [self.window toggleFullScreen:nil];
        }
    }
    
    // Save settings
    [self.menuManager saveSettings];
    [self.menuManager updateSettingsMenuItems];
}

- (void)setScalingMode:(id)sender {
    if ([sender isKindOfClass:[NSMenuItem class]]) {
        NSMenuItem *menuItem = (NSMenuItem *)sender;
        [self.menuManager setScalingMode:(int)menuItem.tag];
        
        // Update the pipeline for the new scaling mode
        [self setupMetalPipeline];
        
        // Save settings
        [self.menuManager saveSettings];
        [self.menuManager updateSettingsMenuItems];
    }
}

- (void)setAspectRatio:(id)sender {
    if ([sender isKindOfClass:[NSMenuItem class]]) {
        NSMenuItem *menuItem = (NSMenuItem *)sender;
        [self.menuManager setAspectRatio:(int)menuItem.tag];
        
        // Update vertex buffer to adjust aspect ratio
        [self createVertexBuffer];
        
        // Save settings
        [self.menuManager saveSettings];
        [self.menuManager updateSettingsMenuItems];
    }
}

- (void)toggleFullScreen:(id)sender {
    [self.window toggleFullScreen:nil];
}

- (void)showScanlineSettings:(id)sender {
    // Create a simple dialog for scanline intensity
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:@"Scanline Intensity"];
    [alert setInformativeText:@"Set scanline intensity (0-100%):"];
    
    // Add a text field for the intensity value
    NSTextField *textField = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 50, 24)];
    [textField setStringValue:[NSString stringWithFormat:@"%d", self.menuManager.settings.scanlineIntensity]];
    [alert setAccessoryView:textField];
    
    // Add buttons
    [alert addButtonWithTitle:@"OK"];
    [alert addButtonWithTitle:@"Cancel"];
    
    if ([alert runModal] == NSAlertFirstButtonReturn) {
        // User clicked OK
        int intensity = [textField.stringValue intValue];
        
        // Clamp to valid range
        intensity = MAX(0, MIN(100, intensity));
        
        // Update setting
        [self.menuManager setScanlineIntensity:intensity];
        
        // Save settings
        [self.menuManager saveSettings];
    }
}

- (void)setFrameSkip:(id)sender {
    if ([sender isKindOfClass:[NSMenuItem class]]) {
        NSMenuItem *menuItem = (NSMenuItem *)sender;
        [self.menuManager setFrameSkip:(int)menuItem.tag];
        
        // Apply frame skip setting
        // TODO: Implement frame skip
        
        // Save settings
        [self.menuManager saveSettings];
        [self.menuManager updateSettingsMenuItems];
    }
}

- (void)showHelp:(id)sender {
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:@"FBNeo Help"];
    [alert setInformativeText:@"Help information is not available yet."];
    [alert runModal];
}

- (void)showCompatibility:(id)sender {
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:@"ROM Compatibility"];
    [alert setInformativeText:@"ROM compatibility information is not available yet."];
    [alert runModal];
}

- (void)showMemoryViewer:(id)sender {
    if (!self.gameLoaded) {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"No Game Running"];
        [alert setInformativeText:@"Please start a game before using the memory viewer."];
        [alert runModal];
        return;
    }
    
    // Create a memory viewer window
    NSWindow *memoryWindow = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 600, 500)
                                                         styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                                                           backing:NSBackingStoreBuffered
                                                             defer:NO];
    [memoryWindow setTitle:@"FBNeo Memory Viewer"];
    
    // Create a split view for the memory regions and hex view
    NSSplitView *splitView = [[NSSplitView alloc] initWithFrame:NSMakeRect(0, 40, 600, 420)];
    [splitView setVertical:YES];
    
    // Create the memory regions list
    NSScrollView *regionsScrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 0, 150, 420)];
    NSTableView *regionsTableView = [[NSTableView alloc] initWithFrame:NSMakeRect(0, 0, 150, 420)];
    NSTableColumn *regionColumn = [[NSTableColumn alloc] initWithIdentifier:@"region"];
    [regionColumn setWidth:140];
    [regionColumn setTitle:@"Memory Regions"];
    [regionsTableView addTableColumn:regionColumn];
    [regionsScrollView setDocumentView:regionsTableView];
    [regionsScrollView setBorderType:NSBezelBorder];
    [regionsScrollView setHasVerticalScroller:YES];
    [regionsScrollView setHasHorizontalScroller:NO];
    
    // Create the memory hex view
    NSScrollView *hexScrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 0, 440, 420)];
    NSTextView *hexTextView = [[NSTextView alloc] initWithFrame:NSMakeRect(0, 0, 440, 420)];
    [hexTextView setFont:[NSFont fontWithName:@"Menlo" size:12.0]];
    [hexTextView setString:@"00000000: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................\n00000010: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................\n00000020: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................\n00000030: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................\n00000040: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................\n00000050: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................\n00000060: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................\n00000070: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................\n"];
    [hexTextView setEditable:NO];
    [hexScrollView setDocumentView:hexTextView];
    [hexScrollView setBorderType:NSBezelBorder];
    [hexScrollView setHasVerticalScroller:YES];
    [hexScrollView setHasHorizontalScroller:YES];
    
    // Add the views to the split view
    [splitView addSubview:regionsScrollView];
    [splitView addSubview:hexScrollView];
    [splitView setPosition:150 ofDividerAtIndex:0];
    
    // Create controls for address navigation
    NSTextField *addressLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(10, 10, 70, 20)];
    [addressLabel setStringValue:@"Address:"];
    [addressLabel setBezeled:NO];
    [addressLabel setDrawsBackground:NO];
    [addressLabel setEditable:NO];
    [addressLabel setSelectable:NO];
    
    NSTextField *addressField = [[NSTextField alloc] initWithFrame:NSMakeRect(80, 10, 80, 20)];
    [addressField setPlaceholderString:@"00000000"];
    
    NSButton *goButton = [[NSButton alloc] initWithFrame:NSMakeRect(170, 10, 50, 20)];
    [goButton setTitle:@"Go"];
    [goButton setBezelStyle:NSBezelStyleRounded];
    
    NSButton *refreshButton = [[NSButton alloc] initWithFrame:NSMakeRect(230, 10, 70, 20)];
    [refreshButton setTitle:@"Refresh"];
    [refreshButton setBezelStyle:NSBezelStyleRounded];
    
    // Add a data source for the memory regions
    MemoryViewerDataSource *dataSource = [[MemoryViewerDataSource alloc] init];
    [regionsTableView setDataSource:dataSource];
    [regionsTableView setDelegate:dataSource];
    
    // Add everything to the window content view
    [[memoryWindow contentView] addSubview:splitView];
    [[memoryWindow contentView] addSubview:addressLabel];
    [[memoryWindow contentView] addSubview:addressField];
    [[memoryWindow contentView] addSubview:goButton];
    [[memoryWindow contentView] addSubview:refreshButton];
    
    // Center and show the window
    [memoryWindow center];
    [memoryWindow makeKeyAndOrderFront:nil];
}

- (void)showDisassembly:(id)sender {
    if (!self.gameLoaded) {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"No Game Running"];
        [alert setInformativeText:@"Please start a game before using the disassembler."];
        [alert runModal];
        return;
    }
    
    // Create a disassembly window
    NSWindow *disasmWindow = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 600, 500)
                                                        styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                                                          backing:NSBackingStoreBuffered
                                                            defer:NO];
    [disasmWindow setTitle:@"FBNeo Disassembler"];
    
    // Create a split view
    NSSplitView *splitView = [[NSSplitView alloc] initWithFrame:NSMakeRect(0, 40, 600, 420)];
    [splitView setVertical:YES];
    
    // Create the code regions list on the left
    NSScrollView *regionsScrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 0, 150, 420)];
    NSTableView *regionsTableView = [[NSTableView alloc] initWithFrame:NSMakeRect(0, 0, 150, 420)];
    NSTableColumn *regionColumn = [[NSTableColumn alloc] initWithIdentifier:@"region"];
    [regionColumn setWidth:140];
    [regionColumn setTitle:@"Code Regions"];
    [regionsTableView addTableColumn:regionColumn];
    [regionsScrollView setDocumentView:regionsTableView];
    [regionsScrollView setBorderType:NSBezelBorder];
    [regionsScrollView setHasVerticalScroller:YES];
    [regionsScrollView setHasHorizontalScroller:NO];
    
    // Create the disassembly view on the right
    NSScrollView *disasmScrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 0, 440, 420)];
    NSTableView *disasmTableView = [[NSTableView alloc] initWithFrame:NSMakeRect(0, 0, 440, 420)];
    
    NSTableColumn *addressColumn = [[NSTableColumn alloc] initWithIdentifier:@"address"];
    [addressColumn setWidth:80];
    [addressColumn setTitle:@"Address"];
    [disasmTableView addTableColumn:addressColumn];
    
    NSTableColumn *bytesColumn = [[NSTableColumn alloc] initWithIdentifier:@"bytes"];
    [bytesColumn setWidth:120];
    [bytesColumn setTitle:@"Bytes"];
    [disasmTableView addTableColumn:bytesColumn];
    
    NSTableColumn *instructionColumn = [[NSTableColumn alloc] initWithIdentifier:@"instruction"];
    [instructionColumn setWidth:220];
    [instructionColumn setTitle:@"Instruction"];
    [disasmTableView addTableColumn:instructionColumn];
    
    [disasmScrollView setDocumentView:disasmTableView];
    [disasmScrollView setBorderType:NSBezelBorder];
    [disasmScrollView setHasVerticalScroller:YES];
    [disasmScrollView setHasHorizontalScroller:YES];
    
    // Add the views to the split view
    [splitView addSubview:regionsScrollView];
    [splitView addSubview:disasmScrollView];
    [splitView setPosition:150 ofDividerAtIndex:0];
    
    // Create controls for address navigation
    NSTextField *addressLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(10, 10, 70, 20)];
    [addressLabel setStringValue:@"Address:"];
    [addressLabel setBezeled:NO];
    [addressLabel setDrawsBackground:NO];
    [addressLabel setEditable:NO];
    [addressLabel setSelectable:NO];
    
    NSTextField *addressField = [[NSTextField alloc] initWithFrame:NSMakeRect(80, 10, 80, 20)];
    [addressField setPlaceholderString:@"00000000"];
    
    NSButton *goButton = [[NSButton alloc] initWithFrame:NSMakeRect(170, 10, 50, 20)];
    [goButton setTitle:@"Go"];
    [goButton setBezelStyle:NSBezelStyleRounded];
    
    NSButton *refreshButton = [[NSButton alloc] initWithFrame:NSMakeRect(230, 10, 70, 20)];
    [refreshButton setTitle:@"Refresh"];
    [refreshButton setBezelStyle:NSBezelStyleRounded];
    
    NSButton *stepButton = [[NSButton alloc] initWithFrame:NSMakeRect(310, 10, 70, 20)];
    [stepButton setTitle:@"Step"];
    [stepButton setBezelStyle:NSBezelStyleRounded];
    
    NSButton *runButton = [[NSButton alloc] initWithFrame:NSMakeRect(390, 10, 70, 20)];
    [runButton setTitle:@"Run"];
    [runButton setBezelStyle:NSBezelStyleRounded];
    
    NSButton *breakButton = [[NSButton alloc] initWithFrame:NSMakeRect(470, 10, 70, 20)];
    [breakButton setTitle:@"Break"];
    [breakButton setBezelStyle:NSBezelStyleRounded];
    
    // Create data sources
    DisassemblerDataSource *disasmDataSource = [[DisassemblerDataSource alloc] init];
    [regionsTableView setDataSource:disasmDataSource];
    [regionsTableView setDelegate:disasmDataSource];
    [disasmTableView setDataSource:disasmDataSource];
    
    // Add everything to the window content view
    [[disasmWindow contentView] addSubview:splitView];
    [[disasmWindow contentView] addSubview:addressLabel];
    [[disasmWindow contentView] addSubview:addressField];
    [[disasmWindow contentView] addSubview:goButton];
    [[disasmWindow contentView] addSubview:refreshButton];
    [[disasmWindow contentView] addSubview:stepButton];
    [[disasmWindow contentView] addSubview:runButton];
    [[disasmWindow contentView] addSubview:breakButton];
    
    // Center and show the window
    [disasmWindow center];
    [disasmWindow makeKeyAndOrderFront:nil];
}

#pragma mark - ROM Loading and Game Management

- (BOOL)loadROMFile:(NSString *)path {
    NSLog(@"Loading ROM: %@", path);
    
    // Store the ROM path
    self.currentROMPath = path;
    
    // Call into C++ code to load the ROM
    if (Metal_LoadROM([path UTF8String]) != 0) {
        NSLog(@"Failed to load ROM");
        self.currentROMPath = nil;
        self.gameLoaded = NO;
        return NO;
    }
    
    // Update game state
    self.gameLoaded = YES;
    self.isPaused = NO;
    
    // Update window title with game name
    NSString *gameName = [NSString stringWithUTF8String:BurnDrvGetTextA_Metal(DRV_FULLNAME)];
    [self.window setTitle:[NSString stringWithFormat:@"FBNeo - %@", gameName]];
    
    // TODO: Update menu state
    // [self.menuManager updateMenuState:YES];
    
    return YES;
}

- (void)runLoadedGame {
    if (!self.gameLoaded) {
        NSLog(@"No game loaded to run");
        return;
    }
    
    NSLog(@"Running game: %@", self.currentROMPath);
    self.isPaused = NO;
    
    // Call into C++ code to run the game
    Metal_RunGame();
}

- (void)resetLoadedGame {
    if (!self.gameLoaded) {
        NSLog(@"No game loaded to reset");
        return;
    }
    
    NSLog(@"Resetting game: %@", self.currentROMPath);
    
    // Call into C++ code to reset the game
    if (Metal_ResetGame() != 0) {
        NSLog(@"Failed to reset game");
        return;
    }
    
    // Show confirmation
    NSAlert *confirmAlert = [[NSAlert alloc] init];
    [confirmAlert setMessageText:@"Game Reset"];
    [confirmAlert setInformativeText:@"The game has been reset successfully."];
    [confirmAlert runModal];
}

#pragma mark - ROM Browser Methods

- (void)showRomBrowserDialog {
    // Create a window for the ROM browser with modern styling
    NSRect frame = NSMakeRect(0, 0, 900, 600);
    NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;
    NSWindow *browserWindow = [[NSWindow alloc] initWithContentRect:frame 
                                                        styleMask:style 
                                                          backing:NSBackingStoreBuffered 
                                                            defer:NO];
    [browserWindow setTitle:@"FBNeo ROM Browser"];
    [browserWindow setTitlebarAppearsTransparent:YES];
    
    // Use vibrant background for modern macOS look
    NSVisualEffectView *backgroundView = [[NSVisualEffectView alloc] initWithFrame:[[browserWindow contentView] bounds]];
    [backgroundView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    [backgroundView setBlendingMode:NSVisualEffectBlendingModeBehindWindow];
    [backgroundView setState:NSVisualEffectStateActive];
    [backgroundView setMaterial:NSVisualEffectMaterialSidebar];
    [[browserWindow contentView] addSubview:backgroundView];
    
    // Create a split view
    NSSplitView *splitView = [[NSSplitView alloc] initWithFrame:NSMakeRect(0, 60, 900, 540)];
    [splitView setVertical:YES];
    [splitView setDividerStyle:NSSplitViewDividerStyleThin];
    
    // Left panel - Hardware filters with sidebar styling
    NSScrollView *filtersScrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 0, 220, 540)];
    NSOutlineView *filtersOutlineView = [[NSOutlineView alloc] initWithFrame:NSMakeRect(0, 0, 220, 540)];
    NSTableColumn *filterColumn = [[NSTableColumn alloc] initWithIdentifier:@"filterColumn"];
    [filterColumn setTitle:@"Hardware"];
    [filterColumn setWidth:200];
    [filtersOutlineView addTableColumn:filterColumn];
    [filtersOutlineView setOutlineTableColumn:filterColumn];
    [filtersOutlineView setHeaderView:nil]; // Hide header
    
    // Use the modern style API if available
    if (@available(macOS 11.0, *)) {
        [filtersOutlineView setStyle:NSTableViewStyleSourceList]; // Modern style for macOS 11+
    } else {
        // Fall back to deprecated API for older macOS versions
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        [filtersOutlineView setSelectionHighlightStyle:NSTableViewSelectionHighlightStyleSourceList];
#pragma clang diagnostic pop
    }
    
    [filtersOutlineView setBackgroundColor:[NSColor clearColor]];
    [filtersScrollView setDocumentView:filtersOutlineView];
    [filtersScrollView setBorderType:NSNoBorder];
    [filtersScrollView setHasVerticalScroller:YES];
    [filtersScrollView setHasHorizontalScroller:NO];
    [filtersScrollView setAutohidesScrollers:YES];
    
    // Right panel - Game list with modern styling
    NSScrollView *gamesScrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 0, 670, 540)];
    NSTableView *gamesTableView = [[NSTableView alloc] initWithFrame:NSMakeRect(0, 0, 670, 540)];
    [gamesTableView setGridStyleMask:NSTableViewSolidHorizontalGridLineMask];
    [gamesTableView setGridColor:[NSColor separatorColor]];
    [gamesTableView setUsesAlternatingRowBackgroundColors:YES];
    [gamesTableView setSelectionHighlightStyle:NSTableViewSelectionHighlightStyleRegular];
    [gamesTableView setAllowsMultipleSelection:NO];
    [gamesTableView setAllowsEmptySelection:NO];
    [gamesTableView setRowHeight:24.0]; // More modern row height
    [gamesTableView setIntercellSpacing:NSMakeSize(8.0, 3.0)]; // Better spacing between cells
    
    // Setup table columns with modern styling
    NSTableColumn *nameColumn = [[NSTableColumn alloc] initWithIdentifier:@"nameColumn"];
    [nameColumn setTitle:@"Game Name"];
    [nameColumn setWidth:350];
    [gamesTableView addTableColumn:nameColumn];
    
    NSTableColumn *yearColumn = [[NSTableColumn alloc] initWithIdentifier:@"yearColumn"];
    [yearColumn setTitle:@"Year"];
    [yearColumn setWidth:80];
    [gamesTableView addTableColumn:yearColumn];
    
    NSTableColumn *manufacturerColumn = [[NSTableColumn alloc] initWithIdentifier:@"manufacturerColumn"];
    [manufacturerColumn setTitle:@"Manufacturer"];
    [manufacturerColumn setWidth:220];
    [gamesTableView addTableColumn:manufacturerColumn];
    
    [gamesScrollView setDocumentView:gamesTableView];
    [gamesScrollView setBorderType:NSNoBorder];
    [gamesScrollView setHasVerticalScroller:YES];
    [gamesScrollView setHasHorizontalScroller:YES];
    [gamesScrollView setAutohidesScrollers:YES];
    
    // Add views to split view
    [splitView addSubview:filtersScrollView];
    [splitView addSubview:gamesScrollView];
    
    // Add search field for modern UX
    NSSearchField *searchField = [[NSSearchField alloc] initWithFrame:NSMakeRect(10, 20, 250, 25)];
    [searchField setPlaceholderString:@"Search Games..."];
    [searchField setDelegate:self]; // Set the delegate
    [searchField setAction:@selector(searchFieldChanged:)];
    [searchField setTarget:self];
    
    // Store dataSource in a tag for access in search handler
    [searchField setTag:1001]; // Tag for search field
    
    // Create buttons with modern styling
    NSButton *playButton = [[NSButton alloc] initWithFrame:NSMakeRect(780, 15, 100, 32)];
    [playButton setTitle:@"Play"];
    [playButton setBezelStyle:NSBezelStyleRounded];
    [playButton setButtonType:NSButtonTypeMomentaryPushIn];
    [playButton setAction:@selector(playSelectedRom:)];
    [playButton setTarget:self];
    
    // Apply modern styling to the button
    [playButton setWantsLayer:YES];
    [[playButton layer] setCornerRadius:4.0];
    [playButton setFont:[NSFont systemFontOfSize:13.0 weight:NSFontWeightMedium]];
    
    NSButton *closeButton = [[NSButton alloc] initWithFrame:NSMakeRect(670, 15, 100, 32)];
    [closeButton setTitle:@"Close"];
    [closeButton setBezelStyle:NSBezelStyleRounded];
    [closeButton setButtonType:NSButtonTypeMomentaryPushIn];
    [closeButton setAction:@selector(closeRomBrowser:)];
    [closeButton setTarget:self];
    
    // Create data sources
    RomBrowserDataSource *dataSource = [[RomBrowserDataSource alloc] init];
    [filtersOutlineView setDataSource:dataSource];
    [filtersOutlineView setDelegate:dataSource];
    
    // Custom implementation for the games table view
    [gamesTableView setDataSource:(id<NSTableViewDataSource>)dataSource];
    [gamesTableView setDelegate:(id<NSTableViewDelegate>)dataSource];
    
    // Add everything to the background view
    [backgroundView addSubview:splitView];
    [backgroundView addSubview:searchField];
    [backgroundView addSubview:playButton];
    [backgroundView addSubview:closeButton];
    
    // Enable double-click to play
    [gamesTableView setDoubleAction:@selector(playSelectedRom:)];
    [gamesTableView setTarget:self];
    
    // Expand all categories by default
    for (int i = 0; i < [filtersOutlineView numberOfRows]; i++) {
        [filtersOutlineView expandItem:[filtersOutlineView itemAtRow:i]];
    }
    
    // Center and show the window using animation
    [browserWindow center];
    [browserWindow makeKeyAndOrderFront:nil];
}

- (void)closeRomBrowser:(NSButton *)sender {
    NSWindow *window = [sender window];
    [window close];
}

- (void)playSelectedRom:(NSButton *)sender {
    // Get the window - might be called from button or double-click
    NSWindow *window = nil;
    if ([sender isKindOfClass:[NSButton class]]) {
        window = [sender window];
    } else {
        window = [NSApp keyWindow];
    }
    
    if (!window) {
        NSLog(@"Cannot find ROM browser window");
        return;
    }
    
    // Find the game table view
    NSTableView *gamesTableView = nil;
    for (NSView *view in [[window contentView] subviews]) {
        if ([view isKindOfClass:[NSVisualEffectView class]]) {
            NSVisualEffectView *backgroundView = (NSVisualEffectView *)view;
            for (NSView *subview in backgroundView.subviews) {
                if ([subview isKindOfClass:[NSSplitView class]]) {
                    NSSplitView *splitView = (NSSplitView *)subview;
                    if (splitView.subviews.count >= 2) {
                        NSScrollView *scrollView = splitView.subviews[1];
                        if ([scrollView.documentView isKindOfClass:[NSTableView class]]) {
                            gamesTableView = (NSTableView *)scrollView.documentView;
                            break;
                        }
                    }
                }
            }
        }
    }
    
    if (!gamesTableView) {
        NSLog(@"Cannot find games table view");
        return;
    }
    
    NSInteger selectedRow = [gamesTableView selectedRow];
    if (selectedRow < 0) {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"No Game Selected"];
        [alert setInformativeText:@"Please select a game from the list."];
        [alert beginSheetModalForWindow:window completionHandler:nil];
        return;
    }
    
    RomBrowserDataSource *dataSource = (RomBrowserDataSource *)gamesTableView.dataSource;
    NSString *selectedGame = [dataSource gameNameAtIndex:selectedRow];
    
    if (selectedGame) {
        NSLog(@"Selected game: %@", selectedGame);
        
        // Get the game index from the data source
        NSDictionary *gameData = dataSource.filteredRoms[selectedRow];
        NSNumber *index = gameData[@"index"];
        NSString *shortName = gameData[@"shortName"];
        
        // Create a progress indicator
        NSProgressIndicator *progressIndicator = [[NSProgressIndicator alloc] initWithFrame:NSMakeRect(300, 300, 200, 20)];
        [progressIndicator setStyle:NSProgressIndicatorStyleBar];
        [progressIndicator setIndeterminate:YES];
        [progressIndicator startAnimation:nil];
        [[[window contentView] subviews][0] addSubview:progressIndicator];
        
        // Show loading message
        NSTextField *loadingText = [[NSTextField alloc] initWithFrame:NSMakeRect(300, 325, 200, 20)];
        [loadingText setStringValue:@"Loading game..."];
        [loadingText setBezeled:NO];
        [loadingText setDrawsBackground:NO];
        [loadingText setEditable:NO];
        [loadingText setSelectable:NO];
        [loadingText setAlignment:NSTextAlignmentCenter];
        [[[window contentView] subviews][0] addSubview:loadingText];
        
        // Debug information
        NSLog(@"Selected game: %@", selectedGame);
        NSLog(@"Game data: %@", gameData);
        
        // Close the window
        [window close];
        
        // Search for ROM in potential locations
        NSArray *potentialRomPaths = [self findRomPathsForGame:shortName];
        NSString *romPath = nil;
        
        // Check each path until we find a valid ROM
        for (NSString *path in potentialRomPaths) {
            if ([[NSFileManager defaultManager] fileExistsAtPath:path]) {
                romPath = path;
                NSLog(@"Found ROM at: %@", romPath);
                break;
            }
        }
        
        if (!romPath) {
            // No ROM found, show error
            NSAlert *alert = [[NSAlert alloc] init];
            [alert setMessageText:@"ROM Not Found"];
            [alert setInformativeText:[NSString stringWithFormat:@"Could not find ROM for %@. Please make sure it exists in your ROM paths.", shortName]];
            [alert runModal];
            return;
        }
        
        // Load the ROM
        if (Metal_LoadROM([romPath UTF8String]) == 0) {
            // Game loaded successfully
            self.gameLoaded = YES;
            self.currentROMPath = romPath;
            
            // Update window title with game name
            [self.window setTitle:[NSString stringWithFormat:@"FBNeo - %@", selectedGame]];
            
            // Auto-run the game if enabled
            if (TRUE) {
                [self runLoadedGame];
            }
        } else {
            NSAlert *alert = [[NSAlert alloc] init];
            [alert setMessageText:@"Error Loading Game"];
            [alert setInformativeText:[NSString stringWithFormat:@"Failed to load %@", romPath]];
            [alert runModal];
        }
    }
}

// Helper method to find potential ROM paths for a game
- (NSArray *)findRomPathsForGame:(NSString *)shortName {
    NSMutableArray *paths = [NSMutableArray array];
    NSString *zipExtension = @".zip";
    
    // Check standard ROM paths
    for (int i = 0; i < DIRS_MAX && szAppRomPaths[i][0]; i++) {
        NSString *basePath = [NSString stringWithUTF8String:szAppRomPaths[i]];
        if (basePath.length > 0) {
            // Look for [path]/[shortname].zip
            NSString *fullPath = [basePath stringByAppendingPathComponent:[shortName stringByAppendingString:zipExtension]];
            [paths addObject:fullPath];
        }
    }
    
    // Also check some common locations
    NSArray *commonLocations = @[
        [NSString stringWithFormat:@"%@/ROMs", NSHomeDirectory()],
        [NSString stringWithFormat:@"%@/Downloads", NSHomeDirectory()],
        [NSString stringWithFormat:@"%@/Documents/ROMs", NSHomeDirectory()],
        [NSString stringWithFormat:@"%@/roms", [[NSBundle mainBundle] resourcePath]],
        @"/Users/Shared/ROMs",
        @"roms"  // Relative to current directory
    ];
    
    for (NSString *location in commonLocations) {
        NSString *fullPath = [location stringByAppendingPathComponent:[shortName stringByAppendingString:zipExtension]];
        [paths addObject:fullPath];
    }
    
    return paths;
}

- (void)loadSelectedGame:(NSString *)gameName {
    NSLog(@"Loading game: %@", gameName);
    
    // Get ROM path from home directory
    NSString *romPath = [NSString stringWithFormat:@"%s/%@.zip", 
                         szAppRomPaths[0] ? szAppRomPaths[0] : "roms", 
                         [gameName stringByReplacingOccurrencesOfString:@" " withString:@""]];
    
    // Load the ROM
    if ([self loadROMFile:romPath]) {
        // Add to recent ROMs
        [self.menuManager addRecentRom:romPath];
        
        // Auto-run if configured
        if (TRUE) {
            [self runLoadedGame];
        }
    } else {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"Error Loading Game"];
        [alert setInformativeText:[NSString stringWithFormat:@"Failed to load %@", gameName]];
        [alert runModal];
    }
}

#pragma mark - Search Handling

- (void)searchFieldChanged:(NSSearchField *)searchField {
    // Find the data source
    NSWindow *window = [searchField window];
    if (!window) return;
    
    // Find splitview in the visual effect view
    NSVisualEffectView *backgroundView = nil;
    for (NSView *view in [[window contentView] subviews]) {
        if ([view isKindOfClass:[NSVisualEffectView class]]) {
            backgroundView = (NSVisualEffectView *)view;
            break;
        }
    }
    if (!backgroundView) return;
    
    // Find split view
    NSSplitView *splitView = nil;
    for (NSView *view in [backgroundView subviews]) {
        if ([view isKindOfClass:[NSSplitView class]]) {
            splitView = (NSSplitView *)view;
            break;
        }
    }
    if (!splitView) return;
    
    // Find table view
    NSTableView *gamesTableView = nil;
    if (splitView.subviews.count >= 2) {
        NSScrollView *scrollView = splitView.subviews[1];
        if ([scrollView.documentView isKindOfClass:[NSTableView class]]) {
            gamesTableView = (NSTableView *)scrollView.documentView;
        }
    }
    if (!gamesTableView) return;
    
    // Get data source
    RomBrowserDataSource *dataSource = (RomBrowserDataSource *)gamesTableView.dataSource;
    if (!dataSource) return;
    
    // Filter games
    NSString *searchText = [searchField stringValue];
    [dataSource filterGamesBySearchText:searchText];
    
    // Reload the table view
    [gamesTableView reloadData];
    
    // Also update the outline view
    NSOutlineView *outlineView = nil;
    if (splitView.subviews.count >= 1) {
        NSScrollView *scrollView = splitView.subviews[0];
        if ([scrollView.documentView isKindOfClass:[NSOutlineView class]]) {
            outlineView = (NSOutlineView *)scrollView.documentView;
            [outlineView reloadData];
            
            // Expand all categories after search
            for (int i = 0; i < [outlineView numberOfRows]; i++) {
                [outlineView expandItem:[outlineView itemAtRow:i]];
            }
        }
    }
}

// Control text delegate methods to handle live search
- (void)controlTextDidChange:(NSNotification *)notification {
    NSTextField *textField = [notification object];
    if ([textField isKindOfClass:[NSSearchField class]]) {
        [self searchFieldChanged:(NSSearchField *)textField];
    }
}

#pragma mark - Game Timer

// This is called by the game timer to update the game state
- (void)gameFrameUpdate:(NSTimer *)timer {
    if (!self.isRunning || self.isPaused) {
        return;
    }
    
    // Call into the emulator to run one frame
    if (self.gameLoaded) {
        // Update game logic here
        
        // Render the frame
        GameRenderer_RenderFrame();
    }
}

#pragma mark - Game Control Methods

- (void)pauseGame:(id)sender {
    // Toggle pause state
    self.isPaused = !self.isPaused;
    
    // Update menu item title
    NSMenuItem *menuItem = (NSMenuItem *)sender;
    [menuItem setTitle:(self.isPaused ? @"Resume" : @"Pause")];
    
    NSLog(@"Game %@", self.isPaused ? @"paused" : @"resumed");
    
    // Update emulator state
    Metal_PauseGame(self.isPaused ? 1 : 0);
}

- (void)resetGame:(id)sender {
    NSLog(@"Resetting game");
    
    // Ask for confirmation
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:@"Reset Game"];
    [alert setInformativeText:@"Are you sure you want to reset the game?"];
    [alert addButtonWithTitle:@"Reset"];
    [alert addButtonWithTitle:@"Cancel"];
    
    [alert beginSheetModalForWindow:self.window completionHandler:^(NSModalResponse returnCode) {
        if (returnCode == NSAlertFirstButtonReturn) {
            // User confirmed reset
            Metal_ResetGame();
            NSLog(@"Game reset");
        }
    }];
}

// Setup application menu
- (void)setupMenu {
    // Create the main menu
    NSMenu *mainMenu = [[NSMenu alloc] init];
    
    // Application menu
    NSMenuItem *appMenuItem = [[NSMenuItem alloc] init];
    NSMenu *appMenu = [[NSMenu alloc] init];
    
    [appMenu addItemWithTitle:@"About FBNeo" action:@selector(orderFrontStandardAboutPanel:) keyEquivalent:@""];
    [appMenu addItem:[NSMenuItem separatorItem]];
    [appMenu addItemWithTitle:@"Preferences..." action:@selector(showPreferences:) keyEquivalent:@","];
    [appMenu addItem:[NSMenuItem separatorItem]];
    [appMenu addItemWithTitle:@"Quit FBNeo" action:@selector(terminate:) keyEquivalent:@"q"];
    
    [appMenuItem setSubmenu:appMenu];
    [mainMenu addItem:appMenuItem];
    
    // File menu
    NSMenuItem *fileMenuItem = [[NSMenuItem alloc] init];
    NSMenu *fileMenu = [[NSMenu alloc] initWithTitle:@"File"];
    
    [fileMenu addItemWithTitle:@"Open ROM..." action:@selector(loadRom:) keyEquivalent:@"o"];
    
    // Recent ROMs submenu
    NSMenuItem *recentMenuItem = [[NSMenuItem alloc] initWithTitle:@"Open Recent" action:nil keyEquivalent:@""];
    NSMenu *recentMenu = [[NSMenu alloc] init];
    [recentMenuItem setSubmenu:recentMenu];
    [fileMenu addItem:recentMenuItem];
    
    [fileMenu addItem:[NSMenuItem separatorItem]];
    [fileMenu addItemWithTitle:@"Close" action:@selector(performClose:) keyEquivalent:@"w"];
    
    [fileMenuItem setSubmenu:fileMenu];
    [mainMenu addItem:fileMenuItem];
    
    // Game menu
    NSMenuItem *gameMenuItem = [[NSMenuItem alloc] init];
    NSMenu *gameMenu = [[NSMenu alloc] initWithTitle:@"Game"];
    
    [gameMenu addItemWithTitle:@"Run" action:@selector(runGame:) keyEquivalent:@"r"];
    [gameMenu addItemWithTitle:@"Pause" action:@selector(pauseEmulation:) keyEquivalent:@"p"];
    [gameMenu addItemWithTitle:@"Reset" action:@selector(resetEmulation:) keyEquivalent:@"t"];
    [gameMenu addItem:[NSMenuItem separatorItem]];
    [gameMenu addItemWithTitle:@"ROM Information" action:@selector(showRomInfo:) keyEquivalent:@"i"];
    
    [gameMenuItem setSubmenu:gameMenu];
    [mainMenu addItem:gameMenuItem];
    
    // View menu
    NSMenuItem *viewMenuItem = [[NSMenuItem alloc] init];
    NSMenu *viewMenu = [[NSMenu alloc] initWithTitle:@"View"];
    
    NSMenuItem *fullscreenItem = [[NSMenuItem alloc] initWithTitle:@"Fullscreen" action:@selector(toggleFullScreen:) keyEquivalent:@"f"];
    [viewMenu addItem:fullscreenItem];
    
    [viewMenu addItem:[NSMenuItem separatorItem]];
    
    NSMenuItem *smoothingItem = [[NSMenuItem alloc] initWithTitle:@"Bilinear Filtering" action:@selector(toggleSmoothing:) keyEquivalent:@"b"];
    [smoothingItem setState:NSControlStateValueOff];
    [viewMenu addItem:smoothingItem];
    
    NSMenuItem *scanlinesItem = [[NSMenuItem alloc] initWithTitle:@"Scanlines" action:@selector(toggleScanlines:) keyEquivalent:@"l"];
    [scanlinesItem setState:NSControlStateValueOff];
    [viewMenu addItem:scanlinesItem];
    
    [viewMenu addItem:[NSMenuItem separatorItem]];
    
    NSMenuItem *hitboxesItem = [[NSMenuItem alloc] initWithTitle:@"Show Hitboxes" action:@selector(toggleHitboxes:) keyEquivalent:@"h"];
    [hitboxesItem setState:NSControlStateValueOff];
    [viewMenu addItem:hitboxesItem];
    
    [viewMenuItem setSubmenu:viewMenu];
    [mainMenu addItem:viewMenuItem];
    
    // AI menu
    NSMenuItem *aiMenuItem = [[NSMenuItem alloc] init];
    NSMenu *aiMenu = [[NSMenu alloc] initWithTitle:@"AI"];
    [aiMenuItem setTitle:@"AI"];
    
    // Enable AI toggle
    NSMenuItem *enableAIItem = [[NSMenuItem alloc] initWithTitle:@"Enable AI" action:@selector(toggleAI:) keyEquivalent:@"a"];
    [enableAIItem setState:(self.menuManager.settings.aiEnabled ? NSControlStateValueOn : NSControlStateValueOff)];
    [aiMenu addItem:enableAIItem];
    
    // Player control submenu
    NSMenuItem *aiPlayerMenuItem = [[NSMenuItem alloc] initWithTitle:@"AI Controls" action:nil keyEquivalent:@""];
    NSMenu *aiPlayerMenu = [[NSMenu alloc] init];
    
    NSMenuItem *aiControlP1Item = [[NSMenuItem alloc] initWithTitle:@"Control Player 1" action:@selector(toggleAIControlP1:) keyEquivalent:@"1"];
    [aiControlP1Item setState:(self.menuManager.settings.aiControlledPlayer == 1 || self.menuManager.settings.aiControlledPlayer == 3 ? NSControlStateValueOn : NSControlStateValueOff)];
    [aiPlayerMenu addItem:aiControlP1Item];
    
    NSMenuItem *aiControlP2Item = [[NSMenuItem alloc] initWithTitle:@"Control Player 2" action:@selector(toggleAIControlP2:) keyEquivalent:@"2"];
    [aiControlP2Item setState:(self.menuManager.settings.aiControlledPlayer == 2 || self.menuManager.settings.aiControlledPlayer == 3 ? NSControlStateValueOn : NSControlStateValueOff)];
    [aiPlayerMenu addItem:aiControlP2Item];
    
    [aiPlayerMenuItem setSubmenu:aiPlayerMenu];
    [aiMenu addItem:aiPlayerMenuItem];
    
    // Difficulty submenu
    NSMenuItem *aiDifficultyMenuItem = [[NSMenuItem alloc] initWithTitle:@"Difficulty" action:nil keyEquivalent:@""];
    NSMenu *aiDifficultyMenu = [[NSMenu alloc] init];
    
    NSMenuItem *aiBeginnerItem = [[NSMenuItem alloc] initWithTitle:@"Beginner" action:@selector(setAIDifficulty:) keyEquivalent:@""];
    [aiBeginnerItem setTag:1];
    [aiBeginnerItem setState:(self.menuManager.settings.aiDifficulty == 1 ? NSControlStateValueOn : NSControlStateValueOff)];
    [aiDifficultyMenu addItem:aiBeginnerItem];
    
    NSMenuItem *aiEasyItem = [[NSMenuItem alloc] initWithTitle:@"Easy" action:@selector(setAIDifficulty:) keyEquivalent:@""];
    [aiEasyItem setTag:2];
    [aiEasyItem setState:(self.menuManager.settings.aiDifficulty == 2 ? NSControlStateValueOn : NSControlStateValueOff)];
    [aiDifficultyMenu addItem:aiEasyItem];
    
    NSMenuItem *aiMediumItem = [[NSMenuItem alloc] initWithTitle:@"Medium" action:@selector(setAIDifficulty:) keyEquivalent:@""];
    [aiMediumItem setTag:5];
    [aiMediumItem setState:(self.menuManager.settings.aiDifficulty == 5 ? NSControlStateValueOn : NSControlStateValueOff)];
    [aiDifficultyMenu addItem:aiMediumItem];
    
    NSMenuItem *aiHardItem = [[NSMenuItem alloc] initWithTitle:@"Hard" action:@selector(setAIDifficulty:) keyEquivalent:@""];
    [aiHardItem setTag:7];
    [aiHardItem setState:(self.menuManager.settings.aiDifficulty == 7 ? NSControlStateValueOn : NSControlStateValueOff)];
    [aiDifficultyMenu addItem:aiHardItem];
    
    NSMenuItem *aiExpertItem = [[NSMenuItem alloc] initWithTitle:@"Expert" action:@selector(setAIDifficulty:) keyEquivalent:@""];
    [aiExpertItem setTag:10];
    [aiExpertItem setState:(self.menuManager.settings.aiDifficulty == 10 ? NSControlStateValueOn : NSControlStateValueOff)];
    [aiDifficultyMenu addItem:aiExpertItem];
    
    [aiDifficultyMenuItem setSubmenu:aiDifficultyMenu];
    [aiMenu addItem:aiDifficultyMenuItem];
    
    // AI Models submenu
    NSMenuItem *aiModelsMenuItem = [[NSMenuItem alloc] initWithTitle:@"AI Models" action:nil keyEquivalent:@""];
    NSMenu *aiModelsMenu = [[NSMenu alloc] init];
    
    [aiModelsMenu addItemWithTitle:@"Load Model..." action:@selector(loadAIModel:) keyEquivalent:@""];
    [aiModelsMenu addItemWithTitle:@"Refresh Models" action:@selector(refreshAIModels:) keyEquivalent:@""];
    
    [aiModelsMenuItem setSubmenu:aiModelsMenu];
    [aiMenu addItem:aiModelsMenuItem];
    
    // Training Mode toggle
    NSMenuItem *aiTrainingItem = [[NSMenuItem alloc] initWithTitle:@"Training Mode" action:@selector(toggleAITraining:) keyEquivalent:@""];
    [aiTrainingItem setState:(self.menuManager.settings.aiTrainingMode ? NSControlStateValueOn : NSControlStateValueOff)];
    [aiMenu addItem:aiTrainingItem];
    
    // Debug submenu
    NSMenuItem *aiDebugMenuItem = [[NSMenuItem alloc] initWithTitle:@"Debug Tools" action:nil keyEquivalent:@""];
    NSMenu *aiDebugMenu = [[NSMenu alloc] init];
    
    // Debug Overlay toggle
    NSMenuItem *aiDebugOverlayItem = [[NSMenuItem alloc] initWithTitle:@"Show Debug Overlay" action:@selector(toggleAIDebugOverlay:) keyEquivalent:@""];
    [aiDebugOverlayItem setState:(self.menuManager.settings.aiDebugOverlay ? NSControlStateValueOn : NSControlStateValueOff)];
    [aiDebugMenu addItem:aiDebugOverlayItem];
    
    // Memory Viewer
    [aiDebugMenu addItemWithTitle:@"Memory Viewer" action:@selector(showAIMemoryViewer:) keyEquivalent:@""];
    
    // Register Viewer
    [aiDebugMenu addItemWithTitle:@"Register Viewer" action:@selector(showRegisterViewer:) keyEquivalent:@""];
    
    // Disassembly Viewer
    [aiDebugMenu addItemWithTitle:@"Disassembly Viewer" action:@selector(showDisassemblyViewer:) keyEquivalent:@""];
    
    // Hitbox Viewer
    [aiDebugMenu addItemWithTitle:@"Hitbox Viewer" action:@selector(toggleHitboxViewer:) keyEquivalent:@""];
    
    // Frame Data
    [aiDebugMenu addItemWithTitle:@"Frame Data" action:@selector(toggleFrameData:) keyEquivalent:@""];
    
    // Input Display
    [aiDebugMenu addItemWithTitle:@"Input Display" action:@selector(toggleInputDisplay:) keyEquivalent:@""];
    
    // Game State
    [aiDebugMenu addItemWithTitle:@"Game State" action:@selector(toggleGameState:) keyEquivalent:@""];
    
    [aiDebugMenuItem setSubmenu:aiDebugMenu];
    [aiMenu addItem:aiDebugMenuItem];
    
    [aiMenuItem setSubmenu:aiMenu];
    [mainMenu addItem:aiMenuItem];
    
    // Set the menu
    [NSApp setMainMenu:mainMenu];
    
    // Create menu manager for storing menu state and settings
    self.menuManager = [[FBNeoMenuManager alloc] init];
    
    NSLog(@"Menu setup complete");
}

#pragma mark - C Bridge Functions

extern "C" {
// C function to get the Metal delegate
void* GetMetalDelegate() {
    return (__bridge_retained void*)gMetalDelegate;
}

// C function to update the frame texture
void UpdateMetalFrameTexture(const void *frameData, unsigned int width, unsigned int height) {
    if (gMetalDelegate) {
        [gMetalDelegate updateFrameTexture:frameData width:width height:height];
    }
}

// C function to toggle pause state
void ToggleMetalPause(int isPaused) {
    if (gMetalDelegate) {
        gMetalDelegate.isPaused = (BOOL)isPaused;
    }
}

// C function to load a ROM
int LoadROM(const char *romPath) {
    if (gMetalDelegate && romPath) {
        NSString *path = [NSString stringWithUTF8String:romPath];
        return [gMetalDelegate loadROMFile:path] ? 1 : 0;
    }
    return 0;
}

// C function to run the currently loaded ROM
int RunGame() {
    if (gMetalDelegate) {
        [gMetalDelegate runLoadedGame];
        return 1;
    }
    return 0;
}

// C function to reset the currently loaded game
void ResetGame() {
    // Get the Metal delegate instance
    if (gMetalDelegate) {
        // Call BurnDrvReset from internal core
        if (BurnDrvReset_Metal() == 0) {
            // Log the reset operation
            NSLog(@"Reset game: %s", BurnDrvGetTextA_Metal(DRV_FULLNAME));
        } else {
            NSLog(@"Error: Failed to reset game");
        }
    } else {
        NSLog(@"Error: Metal delegate not available for game reset");
    }
}

// C function to set fullscreen mode
void SetFullscreen(int fullscreen) {
    if (gMetalDelegate) {
        [gMetalDelegate.menuManager setDisplayMode:fullscreen ? 1 : 0];
        [gMetalDelegate setDisplayMode:nil];
        
        // Call our new Metal fullscreen toggle function
        extern int VidMetalSetFullscreen(bool bFullscreen);
        VidMetalSetFullscreen((bool)fullscreen);
    }
}

// C function to set scaling mode
void SetScalingMode(int mode) {
    if (gMetalDelegate) {
        NSMenuItem *item = [[NSMenuItem alloc] init];
        [item setTag:mode];
        [gMetalDelegate setScalingMode:item];
    }
}

// C function to set aspect ratio
void SetAspectRatio(int ratio) {
    if (gMetalDelegate) {
        NSMenuItem *item = [[NSMenuItem alloc] init];
        [item setTag:ratio];
        [gMetalDelegate setAspectRatio:item];
    }
}

// C function to enable/disable audio
void SetAudioEnabled(int enabled) {
    if (gMetalDelegate) {
        [gMetalDelegate.menuManager setAudioEnabled:(BOOL)enabled];
        [gMetalDelegate toggleAudio:nil];
    }
}

// C function to set volume
void SetVolume(int volume) {
    if (gMetalDelegate) {
        NSMenuItem *item = [[NSMenuItem alloc] init];
        [item setTag:volume];
        [gMetalDelegate setVolume:item];
    }
}

// C function to check if a game is running
int IsGameRunning() {
    return (gMetalDelegate && gMetalDelegate.gameLoaded && !gMetalDelegate.isPaused) ? 1 : 0;
}

// C function to check if a game is paused
int IsGamePaused() {
    return (gMetalDelegate && gMetalDelegate.isPaused) ? 1 : 0;
}

// Application entry point for Metal renderer
int RunMetalApplication() {
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];
        
        gMetalDelegate = [[FBNeoMetalDelegate alloc] init];
        [app setDelegate:gMetalDelegate];
        
        [app finishLaunching];
        printf("Metal renderer initialized successfully\n");
        [app run];
        
        return 0;
    }
}

// C function to create a game timer
int CreateGameTimer() {
    // Create a repeating timer to update the game state
    NSTimer *gameTimer = [NSTimer scheduledTimerWithTimeInterval:1.0/60.0 // 60 fps
                                                        target:[NSApp delegate]
                                                        selector:@selector(gameFrameUpdate:)
                                                        userInfo:nil
                                                        repeats:YES];
    
    // Add the timer to the run loop
    [[NSRunLoop currentRunLoop] addTimer:gameTimer forMode:NSRunLoopCommonModes];
    
    return 0;
}
} // extern "C"

// Add implementations for missing methods
- (void)showAbout:(id)sender {
    NSLog(@"Show About dialog");
    [NSApp orderFrontStandardAboutPanel:sender];
}

- (void)toggleAudio:(id)sender {
    BOOL newState = !self.menuManager.settings.audioEnabled;
    [self.menuManager setAudioEnabled:newState];
    NSLog(@"Audio %@", newState ? @"enabled" : @"disabled");
}

- (void)setSampleRate:(id)sender {
    NSMenuItem *menuItem = (NSMenuItem *)sender;
    int sampleRate = [menuItem.title intValue];
    [self.menuManager setSampleRate:sampleRate];
    NSLog(@"Sample rate set to %d", sampleRate);
}

- (void)setVolume:(id)sender {
    NSSlider *slider = (NSSlider *)sender;
    int volume = (int)slider.intValue;
    [self.menuManager setVolume:volume];
    NSLog(@"Volume set to %d", volume);
}

- (void)configureControls:(id)sender {
    NSLog(@"Configure controls dialog");
    // Would show controls configuration dialog here
}

- (void)setControllerType:(id)sender {
    NSMenuItem *menuItem = (NSMenuItem *)sender;
    int type = (int)menuItem.tag;
    [self.menuManager setControllerType:type];
    NSLog(@"Controller type set to %d", type);
}

- (void)setupRenderPipeline {
    NSLog(@"Setting up Metal render pipeline");
    
    // Create a library from the default Metal shaders
    NSError *libraryError = nil;
    id<MTLLibrary> defaultLibrary = [self.device newDefaultLibrary];
    if (!defaultLibrary) {
        NSLog(@"Failed to create default Metal library: %@", libraryError);
        return;
    }
    
    // Load the vertex and fragment shader functions
    id<MTLFunction> vertexFunction = [defaultLibrary newFunctionWithName:@"vertexShader"];
    id<MTLFunction> fragmentFunction = [defaultLibrary newFunctionWithName:@"fragmentShader"];
    
    // Create a render pipeline descriptor
    MTLRenderPipelineDescriptor *pipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDescriptor.label = @"FBNeo Basic Pipeline";
    pipelineDescriptor.vertexFunction = vertexFunction;
    pipelineDescriptor.fragmentFunction = fragmentFunction;
    pipelineDescriptor.colorAttachments[0].pixelFormat = self.metalView.colorPixelFormat;
    
    // Create the pipeline state
    NSError *pipelineError = nil;
    self.pipelineState = [self.device newRenderPipelineStateWithDescriptor:pipelineDescriptor 
                                                                      error:&pipelineError];
    if (!self.pipelineState) {
        NSLog(@"Failed to create pipeline state: %@", pipelineError);
    }
}

- (void)toggleFPS:(id)sender {
    BOOL newState = !self.menuManager.settings.showFPS;
    // We can't directly assign to the settings struct field, so use a temporary
    FBNeoSettings tempSettings = self.menuManager.settings;
    tempSettings.showFPS = newState;
    self.menuManager.settings = tempSettings;
    
    NSLog(@"FPS display %@", newState ? @"enabled" : @"disabled");
    // Additional implementation would update the FPS display in the renderer
}

#pragma mark - AI Methods

- (void)toggleAI:(id)sender {
    BOOL newState = !self.menuManager.settings.aiEnabled;
    [self.menuManager setAIEnabled:newState];
    
    // Update menu state
    if ([sender isKindOfClass:[NSMenuItem class]]) {
        [(NSMenuItem *)sender setState:(newState ? NSControlStateValueOn : NSControlStateValueOff)];
    }
    
    NSLog(@"AI %@", newState ? @"enabled" : @"disabled");
    
    // Call C API function
    // Call AI_Initialize() first if needed
    extern void AI_Initialize();
    extern void AI_Shutdown();
    
    if (newState) {
        AI_Initialize();
    } else {
        AI_Shutdown();
    }
}

- (void)toggleAIControlP1:(id)sender {
    // The aiControlledPlayer value can be:
    // 0: No AI control
    // 1: AI controls player 1
    // 2: AI controls player 2
    // 3: AI controls both players
    
    int currentSetting = self.menuManager.settings.aiControlledPlayer;
    int newSetting;
    
    // Toggle P1 control
    if (currentSetting == 0) {
        newSetting = 1; // P1 only
    } else if (currentSetting == 1) {
        newSetting = 0; // None
    } else if (currentSetting == 2) {
        newSetting = 3; // Both
    } else { // currentSetting == 3
        newSetting = 2; // P2 only
    }
    
    [self.menuManager setAIControlledPlayer:newSetting];
    
    // Update menu state
    if ([sender isKindOfClass:[NSMenuItem class]]) {
        BOOL isP1Controlled = (newSetting == 1 || newSetting == 3);
        [(NSMenuItem *)sender setState:(isP1Controlled ? NSControlStateValueOn : NSControlStateValueOff)];
    }
    
    NSLog(@"AI control of Player 1 %@", (newSetting == 1 || newSetting == 3) ? @"enabled" : @"disabled");
    
    // Call C API function
    extern void AI_SetControlledPlayer(int playerIndex);
    AI_SetControlledPlayer(newSetting);
}

- (void)toggleAIControlP2:(id)sender {
    // Toggle P2 control using the same logic as P1
    int currentSetting = self.menuManager.settings.aiControlledPlayer;
    int newSetting;
    
    if (currentSetting == 0) {
        newSetting = 2; // P2 only
    } else if (currentSetting == 1) {
        newSetting = 3; // Both
    } else if (currentSetting == 2) {
        newSetting = 0; // None
    } else { // currentSetting == 3
        newSetting = 1; // P1 only
    }
    
    [self.menuManager setAIControlledPlayer:newSetting];
    
    // Update menu state
    if ([sender isKindOfClass:[NSMenuItem class]]) {
        BOOL isP2Controlled = (newSetting == 2 || newSetting == 3);
        [(NSMenuItem *)sender setState:(isP2Controlled ? NSControlStateValueOn : NSControlStateValueOff)];
    }
    
    NSLog(@"AI control of Player 2 %@", (newSetting == 2 || newSetting == 3) ? @"enabled" : @"disabled");
    
    // Call C API function
    extern void AI_SetControlledPlayer(int playerIndex);
    AI_SetControlledPlayer(newSetting);
}

- (void)setAIDifficulty:(id)sender {
    if (![sender isKindOfClass:[NSMenuItem class]]) {
        return;
    }
    
    NSMenuItem *menuItem = (NSMenuItem *)sender;
    int difficultyLevel = (int)menuItem.tag;
    
    // Update the menu states - uncheck all items first
    NSMenu *parentMenu = menuItem.menu;
    for (NSMenuItem *item in parentMenu.itemArray) {
        [item setState:NSControlStateValueOff];
    }
    
    // Check the selected item
    [menuItem setState:NSControlStateValueOn];
    
    // Set the difficulty
    [self.menuManager setAIDifficulty:difficultyLevel];
    
    NSLog(@"AI difficulty set to %d", difficultyLevel);
    
    // Call C API function
    extern void AI_SetDifficulty(int level);
    AI_SetDifficulty(difficultyLevel);
}

- (void)toggleAITraining:(id)sender {
    BOOL newState = !self.menuManager.settings.aiTrainingMode;
    [self.menuManager setAITrainingMode:newState];
    
    // Update menu state
    if ([sender isKindOfClass:[NSMenuItem class]]) {
        [(NSMenuItem *)sender setState:(newState ? NSControlStateValueOn : NSControlStateValueOff)];
    }
    
    NSLog(@"AI training mode %@", newState ? @"enabled" : @"disabled");
    
    // Call C API function
    extern void AI_EnableTrainingMode(int enable);
    AI_EnableTrainingMode(newState ? 1 : 0);
}

- (void)toggleAIDebugOverlay:(id)sender {
    BOOL newState = !self.menuManager.settings.aiDebugOverlay;
    [self.menuManager setAIDebugOverlay:newState];
    
    // Update menu state
    if ([sender isKindOfClass:[NSMenuItem class]]) {
        [(NSMenuItem *)sender setState:(newState ? NSControlStateValueOn : NSControlStateValueOff)];
    }
    
    NSLog(@"AI debug overlay %@", newState ? @"enabled" : @"disabled");
    
    // Call C API function
    extern void AI_EnableDebugOverlay(int enable);
    AI_EnableDebugOverlay(newState ? 1 : 0);
}

- (void)showAIMemoryViewer:(id)sender {
    // This would show the memory viewer UI
    NSLog(@"Show AI memory viewer");
    
    // Show a simple alert for now
    NSAlert *alert = [[NSAlert alloc] init];
    alert.messageText = @"Memory Viewer";
    alert.informativeText = @"The memory viewer would be shown if AI was fully implemented";
    [alert runModal];
}

- (void)showRegisterViewer:(id)sender {
    // This would show the register viewer UI
    NSLog(@"Show register viewer");
    
    // Show a simple alert for now
    NSAlert *alert = [[NSAlert alloc] init];
    alert.messageText = @"Register Viewer";
    alert.informativeText = @"The register viewer would be shown if AI was fully implemented";
    [alert runModal];
}

- (void)showDisassemblyViewer:(id)sender {
    // This would show the disassembly viewer UI
    NSLog(@"Show disassembly viewer");
    
    // Show a simple alert for now
    NSAlert *alert = [[NSAlert alloc] init];
    alert.messageText = @"Disassembly Viewer";
    alert.informativeText = @"The disassembly viewer would be shown if AI was fully implemented";
    [alert runModal];
}

- (void)toggleHitboxViewer:(id)sender {
    // This would be implemented with real hitbox visualization
    NSLog(@"Toggle hitbox viewer");
    
    // Update menu state
    if ([sender isKindOfClass:[NSMenuItem class]]) {
        BOOL currentState = ([(NSMenuItem *)sender state] == NSControlStateValueOn);
        [(NSMenuItem *)sender setState:(currentState ? NSControlStateValueOff : NSControlStateValueOn)];
    }
}

- (void)toggleFrameData:(id)sender {
    // This would be implemented with real frame data visualization
    NSLog(@"Toggle frame data display");
    
    // Update menu state
    if ([sender isKindOfClass:[NSMenuItem class]]) {
        BOOL currentState = ([(NSMenuItem *)sender state] == NSControlStateValueOn);
        [(NSMenuItem *)sender setState:(currentState ? NSControlStateValueOff : NSControlStateValueOn)];
    }
}

- (void)toggleInputDisplay:(id)sender {
    // This would be implemented with real input display visualization
    NSLog(@"Toggle input display");
    
    // Update menu state
    if ([sender isKindOfClass:[NSMenuItem class]]) {
        BOOL currentState = ([(NSMenuItem *)sender state] == NSControlStateValueOn);
        [(NSMenuItem *)sender setState:(currentState ? NSControlStateValueOff : NSControlStateValueOn)];
    }
}

- (void)toggleGameState:(id)sender {
    // This would be implemented with real game state visualization
    NSLog(@"Toggle game state display");
    
    // Update menu state
    if ([sender isKindOfClass:[NSMenuItem class]]) {
        BOOL currentState = ([(NSMenuItem *)sender state] == NSControlStateValueOn);
        [(NSMenuItem *)sender setState:(currentState ? NSControlStateValueOff : NSControlStateValueOn)];
    }
}

- (void)scanlineIntensityChanged:(id)sender {
    NSSlider *slider = (NSSlider *)sender;
    int intensity = (int)slider.intValue;
    [self.menuManager setScanlineIntensity:intensity];
    NSLog(@"Scanline intensity set to %d", intensity);
}

- (void)loadAIModel:(id)sender {
    NSOpenPanel *openPanel = [NSOpenPanel openPanel];
    openPanel.title = @"Select AI Model";
    openPanel.allowsMultipleSelection = NO;
    openPanel.canChooseDirectories = NO;
    openPanel.canChooseFiles = YES;
    
    // Set allowed file types using modern API
    if (@available(macOS 11.0, *)) {
        NSArray<UTType *> *fileTypes = @[
            [UTType typeWithFilenameExtension:@"pt"],
            [UTType typeWithFilenameExtension:@"onnx"],
            [UTType typeWithFilenameExtension:@"mlmodel"]
        ];
        openPanel.allowedContentTypes = fileTypes;
    } else {
        // Legacy approach with compiler warning suppressed
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        openPanel.allowedFileTypes = @[@"pt", @"onnx", @"mlmodel"];
#pragma clang diagnostic pop
    }
    
    [openPanel beginWithCompletionHandler:^(NSModalResponse result) {
        if (result == NSModalResponseOK) {
            NSURL *modelURL = openPanel.URLs.firstObject;
            NSLog(@"Selected AI model: %@", modelURL.path);
            
            // Call C API function
            extern void AI_LoadModel(const char* modelPath);
            AI_LoadModel([modelURL.path UTF8String]);
            
            // Show a confirmation dialog
            NSAlert *alert = [[NSAlert alloc] init];
            alert.messageText = @"AI Model Loaded";
            alert.informativeText = [NSString stringWithFormat:@"Model %@ has been loaded", modelURL.lastPathComponent];
            [alert runModal];
        }
    }];
}

- (void)refreshAIModels:(id)sender {
    // This would refresh the list of available models
    NSLog(@"Refreshing AI models");
    
    // Show a simple alert for now
    NSAlert *alert = [[NSAlert alloc] init];
    alert.messageText = @"AI Models Refreshed";
    alert.informativeText = @"Available AI models have been refreshed";
    [alert runModal];
}
@end // End of FBNeoMetalDelegate implementation 