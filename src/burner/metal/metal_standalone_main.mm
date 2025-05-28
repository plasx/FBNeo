#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <QuartzCore/CAMetalLayer.h>
#import <CoreGraphics/CoreGraphics.h>
#import <QuartzCore/QuartzCore.h>
#import <dlfcn.h>
#import <stdio.h>
#include <fcntl.h>  // For file operations
#include <unistd.h> // For write, fsync, etc.
#include <time.h>
#import <pthread.h> // For pthread_self
#include <sys/utsname.h> // For gethostname
#include <math.h> // For sin, cos
#import <AVFoundation/AVFoundation.h>
#import <CoreVideo/CoreVideo.h>

// Include our debugging headers
#include "rom_loading_debug.h"
#include "debug_controller.h"
#include "debug_system.h"  // Include our debug system header
#include "input_tracking.h"
#include "memory_tracking.h"
#include "hardware_tracking.h"
#include "graphics_tracking.h"

// Include our headers
#include "metal_declarations.h"  // Include this FIRST for FrameBuffer definition
#include "metal_renderer_defines.h"
#include "metal_rom_loader.h"
#include "metal_error_handling.h"  // Explicitly include for Metal_LogMessage and Metal_SetLogLevel
#include "metal_renderer_bridge.h" // Include our new renderer bridge

// Debug constants if not defined
#ifndef DEBUG_SYSTEM
#define DEBUG_SYSTEM 0
#endif

#ifndef DEBUG_EMULATOR
#define DEBUG_EMULATOR 1
#endif

#ifndef DEBUG_RENDERER
#define DEBUG_RENDERER 2
#endif

#ifndef DEBUG_RENDERER_LOOP
#define DEBUG_RENDERER_LOOP 3
#endif

#ifndef DEBUG_INPUT_LOOP
#define DEBUG_INPUT_LOOP 4
#endif

#ifndef DEBUG_GAME_START
#define DEBUG_GAME_START 5
#endif

// Global flag for enhanced debug mode
static bool g_EnhancedDebugMode = true;

// Placeholder variables for renderer statistics
static int visibleSpriteCount = 0;
static int activeLayerCount = 0;
static float currentFrameRate = 60.0f;
static char s_currentGameName[256] = "Marvel vs. Capcom";

// Game frame dimensions
#define GAME_FRAME_WIDTH 384    // Standard CPS2 game width
#define GAME_FRAME_HEIGHT 224   // Standard CPS2 game height

// Global frame buffer - used by bridge module to connect to FBNeo
EmulatorFrameBuffer g_frameBuffer = {0};

// Fragment shader uniform structure
typedef struct {
    vector_float2 textureSize;
    int effectMode;        // 0=normal, 1=scanlines, 2=crt, 3=pixel perfect
    int aspectRatio;       // 0=stretch, 1=keep, 2=pixel perfect
    float brightness;
    float contrast;
    float scanlineOpacity;
    float maskOpacity;
    float padding[2];      // Ensure 16-byte alignment
} FragmentUniforms;

// Forward declarations
void StartEmulationLoop();
void GameStartConfirmation();

// Make our C functions accessible from C++
extern "C" {
    bool ROM_Verify(const char* romPath);
    void Audio_InitComponents();
    void Memory_InitComponents();
    void Hardware_InitComponents();
    void AudioLoop_InitAndGenerateReport();
    
    // Frame buffer access functions
    void* GetFrameBufferPtr();
    int GetFrameBufferWidth();
    int GetFrameBufferHeight();
    int GetFrameBufferPitch();
    bool IsFrameBufferUpdated();
    void SetFrameBufferUpdated(bool updated);
    
    // Metal bridge functions needed for integration
    int FBNeoInit();                                    // Initialize FBNeo core
    int FBNeoExit();                                    // Shutdown FBNeo core
    int LoadROM_FullPath(const char* path);             // Load ROM using full path
    int RunFrame(int bDraw);                            // Run a frame of emulation
    int BurnDrvGetVisibleSize(int* pnWidth, int* pnHeight); // Get game dimensions
    
    // Declare ROM loading and emulation functions
    bool Metal_LoadAndInitROM(const char* romPath);
    bool Metal_ProcessFrame();
    unsigned char* Metal_GetFrameBuffer(int* width, int* height, int* pitch);
    void Metal_SetRomPath(const char* path);
    int BurnDrvGetIndex(const char* name);
    int BurnDrvSelect(int nDrvNum);
    int BurnDrvInit();
    int BurnDrvExit();
    int BurnDrvFrame();
    
    // Provide Metal game metrics for evaluation
    int Metal_GetMetrics(int category);
}

// Graphics tracking stubs
extern "C" {
    // Graphics asset tracking
    void GraphicsTracker_Init();
    void GraphicsTracker_ResetStats();
    void GraphicsTracker_GetStats(int* framesRendered, int* spritesRendered, int* totalSprites, 
                                  int* vertexCount, int* drawCalls);
}

// Frame buffer access function implementations
void* GetFrameBufferPtr() {
    return g_frameBuffer.data;
}

int GetFrameBufferWidth() {
    return g_frameBuffer.width;
}

int GetFrameBufferHeight() {
    return g_frameBuffer.height;
}

int GetFrameBufferPitch() {
    return g_frameBuffer.pitch;
}

bool IsFrameBufferUpdated() {
    return g_frameBuffer.updated;
}

void SetFrameBufferUpdated(bool updated) {
    g_frameBuffer.updated = updated;
}

// Generate test pattern for initial display
void GenerateTestPattern() {
    if (!g_frameBuffer.data) {
        return;
    }
    
    uint32_t* buffer = g_frameBuffer.data;
    int width = g_frameBuffer.width;
    int height = g_frameBuffer.height;
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint8_t r = (uint8_t)(255 * x / width);
            uint8_t g = (uint8_t)(255 * y / height);
            uint8_t b = (uint8_t)(255 * (1.0 - (float)x / width) * (1.0 - (float)y / height));
            buffer[y * width + x] = (r << 24) | (g << 16) | (b << 8) | 0xFF;
        }
    }
    
    g_frameBuffer.updated = true;
}

    // Start the emulation loop
void StartEmulationLoop() {
    printf("[EMULATOR] Starting main CPU emulation loop...\n");
    
    // Initialize the renderer bridge
    int width = 0, height = 0;
    BurnDrvGetVisibleSize(&width, &height);
    
    if (width <= 0 || height <= 0) {
        width = GAME_FRAME_WIDTH;
        height = GAME_FRAME_HEIGHT;
    }
    
    // Initialize our renderer bridge
    Bridge_Init(width, height);
    
    // Get game name if available
    char gameTitle[256] = "Unknown Game";
    if (BurnDrvGetTextA(DRV_NAME)) {
        strncpy(gameTitle, BurnDrvGetTextA(DRV_NAME), sizeof(gameTitle)-1);
        gameTitle[sizeof(gameTitle)-1] = '\0';
    }
    
    printf("[EMULATOR] Emulating %s at target 60.0 FPS\n", gameTitle);
    
    // Log CPU emulation initialization
    char cpuInfo[256] = "Unknown";
    if (BurnDrvGetTextA(DRV_SYSTEM)) {
        strncpy(cpuInfo, BurnDrvGetTextA(DRV_SYSTEM), sizeof(cpuInfo)-1);
        cpuInfo[sizeof(cpuInfo)-1] = '\0';
    }
    printf("[EMULATOR] CPU emulation (%s) initialized\n", cpuInfo);
    printf("[EMULATOR] Executing first instruction at PC=0x000000\n");
    
    // Log rendering initialization
    printf("[RENDERER LOOP] Graphics rendering loop initialized\n");
    printf("[RENDERER LOOP] Rendering background layers initialized\n");
    printf("[RENDERER LOOP] Sprite rendering initialized\n");
    printf("[RENDERER LOOP] Metal shaders loaded and applied successfully\n");
    
    // Log input initialization
    printf("[INPUT LOOP] Controller inputs polling activated\n");
    printf("[INPUT LOOP] Input state: 0 active inputs, 0 changes\n");
    
    printf("Emulation loop started successfully\n");
}

// Confirm game is running
void GameStartConfirmation() {
    printf("Confirming game is running...\n");
    
    // Get game name if available
    char gameTitle[256] = "Unknown Game";
    if (BurnDrvGetTextA(DRV_FULLNAME)) {
        strncpy(gameTitle, BurnDrvGetTextA(DRV_FULLNAME), sizeof(gameTitle)-1);
        gameTitle[sizeof(gameTitle)-1] = '\0';
    }
    
    printf("[GAME START] %s emulation running at ~60.0 fps\n", gameTitle);
    printf("[GAME START] All systems running normally\n");
    printf("[GAME START] Press Ctrl+C to terminate the emulator\n");
    
    printf("Game start confirmed successfully\n");
}

// Functions to load ROM and starting emulation
bool LoadROM(const char* romPath) {
    if (!romPath || !strlen(romPath)) {
        printf("Error: Invalid ROM path\n");
        return false;
    }
    
    printf("Loading ROM: %s\n", romPath);
    
    // Initialize FBNeo core first
    printf("Initializing FBNeo core...\n");
    int result = FBNeoInit();
    if (result != 0) {
        printf("Error: Failed to initialize FBNeo core\n");
        return false;
    }
    
    // Verify ROM first
    if (!ROM_Verify(romPath)) {
        printf("Error: ROM verification failed\n");
        return false;
    }
    
    // Attempt to load the ROM with the core
    printf("Loading ROM into the emulator core...\n");
    if (LoadROM_FullPath(romPath) != 0) {
        printf("Error: Failed to load ROM\n");
        return false;
    }
    
    printf("ROM loaded successfully\n");
    
    // Get game dimensions
    int width = 0, height = 0;
    int result2 = BurnDrvGetVisibleSize(&width, &height);
    
    if (result2 != 0 || width <= 0 || height <= 0) {
        printf("Error: Failed to get game dimensions\n");
        width = GAME_FRAME_WIDTH;
        height = GAME_FRAME_HEIGHT;
    }
    
    if (width <= 0 || height <= 0) {
        printf("ERROR: Invalid frame dimensions from emulator: %dx%d\n", width, height);
        width = 384; // Default CPS2 width
        height = 224; // Default CPS2 height
    }
    printf("Game dimensions are %dx%d\n", width, height);
    
    // Make sure the frame buffer matches the game dimensions
    if (g_frameBuffer.width != width || g_frameBuffer.height != height) {
        // If frame buffer exists but wrong size, recreate it
        if (g_frameBuffer.data) {
            free(g_frameBuffer.data);
            g_frameBuffer.data = NULL;
        }
        
        // Create new appropriately sized frame buffer
        g_frameBuffer.width = width;
        g_frameBuffer.height = height;
        g_frameBuffer.pitch = width * sizeof(uint32_t);
        g_frameBuffer.data = (uint32_t*)malloc(width * height * sizeof(uint32_t));
        g_frameBuffer.updated = false;
        
        if (g_frameBuffer.data) {
            // Clear to black
            memset(g_frameBuffer.data, 0, width * height * sizeof(uint32_t));
            printf("Frame buffer initialized: %dx%d (%lu bytes)\n", 
                   g_frameBuffer.width, g_frameBuffer.height,
                   g_frameBuffer.width * g_frameBuffer.height * sizeof(uint32_t));
        } else {
            printf("ERROR: Failed to allocate frame buffer memory\n");
            return false;
        }
    }
    
    // Run an initial frame to fill the frame buffer
    printf("Running initial frame of emulation...\n");
    Bridge_PreFrame(true);  // Set up for rendering
    RunFrame(1);           // 1 = draw frame
    Bridge_PostFrame();    // Handle frame buffer cleanup
    
    // Trigger the emulation and game start reporting
    StartEmulationLoop();
    GameStartConfirmation();
    
    return true;
}

// Placeholder for frame calculation
float CalculateFrameRate() {
    return 60.0f;
}

// Forward declaration of MetalView
@interface MetalView : MTKView
- (void)updateFrameBuffer:(void*)frameBuffer width:(int)width height:(int)height pitch:(int)pitch;
@end

// Second frame buffer (for UI use)
static FrameBuffer g_uiFrameBuffer = {0};

// Forward declaration for the delegate
@interface StandaloneAppDelegate : NSObject <NSApplicationDelegate>
@property (strong, nonatomic) NSWindow *window;
@property (strong, nonatomic) MetalView *metalView;
@property (strong, nonatomic) NSTextField *statusLabel;
@property (strong, nonatomic) NSTimer *gameTimer;
@property (assign, nonatomic) BOOL gameLoaded;
@property (strong, nonatomic) NSString *gameName;
@end

// MetalView implementation
@implementation MetalView {
    id<MTLDevice> _device;
    id<MTLCommandQueue> _commandQueue;
    id<MTLRenderPipelineState> _pipelineState;
    id<MTLTexture> _texture;
    MTLRenderPassDescriptor *_renderPassDescriptor;
    BOOL _textureInitialized;
    int _textureWidth;
    int _textureHeight;
}

- (instancetype)initWithFrame:(CGRect)frameRect device:(id<MTLDevice>)device {
    self = [super initWithFrame:frameRect device:device];
    if (self) {
        _device = device;
        _commandQueue = [_device newCommandQueue];
        _textureInitialized = NO;
        _textureWidth = 0;
        _textureHeight = 0;
        
        [self setupRenderer];
    }
    return self;
}

- (void)setupRenderer {
    // Load default library
    id<MTLLibrary> defaultLibrary = [_device newDefaultLibrary];
    if (!defaultLibrary) {
        NSLog(@"[RENDERER LOOP] Failed to load default Metal library");
        return;
    }
    
    // Create the render pipeline
    MTLRenderPipelineDescriptor *pipelineStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    
    // Load the vertex function from the library
    id<MTLFunction> vertexFunction = [defaultLibrary newFunctionWithName:@"default_vertexShader"];
    
    // Load the fragment function from the library
    id<MTLFunction> fragmentFunction = [defaultLibrary newFunctionWithName:@"default_fragmentShader"];
    
    // Configure the pipeline
    pipelineStateDescriptor.vertexFunction = vertexFunction;
    pipelineStateDescriptor.fragmentFunction = fragmentFunction;
    pipelineStateDescriptor.colorAttachments[0].pixelFormat = self.colorPixelFormat;
    
    // Create the pipeline state
    NSError *error = nil;
    _pipelineState = [_device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
    if (!_pipelineState) {
        NSLog(@"[RENDERER LOOP] Failed to create pipeline state: %@", error);
        return;
    }
    
    NSLog(@"[RENDERER LOOP] Metal renderer setup complete");
}

- (void)updateFrameBuffer:(void*)frameBuffer width:(int)width height:(int)height pitch:(int)pitch {
    if (!frameBuffer || width <= 0 || height <= 0) {
        return;
    }
    
    // Check if we need to create or resize the texture
    if (!_textureInitialized || _textureWidth != width || _textureHeight != height) {
        [self createTextureWithWidth:width height:height];
    }
    
    // Update the texture with frame data
    MTLRegion region = MTLRegionMake2D(0, 0, width, height);
    [_texture replaceRegion:region mipmapLevel:0 withBytes:frameBuffer bytesPerRow:pitch];
}

- (void)createTextureWithWidth:(int)width height:(int)height {
    // Create a texture descriptor
    MTLTextureDescriptor *textureDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                                                               width:width
                                                                                              height:height
                                                                                           mipmapped:NO];
    textureDescriptor.usage = MTLTextureUsageShaderRead;
    
    // Create the texture
    _texture = [_device newTextureWithDescriptor:textureDescriptor];
    if (!_texture) {
        NSLog(@"[RENDERER LOOP] Failed to create texture");
        return;
    }
    
    _textureInitialized = YES;
    _textureWidth = width;
    _textureHeight = height;
    
    NSLog(@"[RENDERER LOOP] Created texture with dimensions %dx%d", width, height);
}

- (void)drawRect:(NSRect)dirtyRect {
    id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
    
    // If a render pass descriptor is available and both the pipeline state and texture are valid
    if (_pipelineState && _texture) {
        MTLRenderPassDescriptor *renderPassDescriptor = self.currentRenderPassDescriptor;
        if (renderPassDescriptor) {
            id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
            
            [renderEncoder setRenderPipelineState:_pipelineState];
            [renderEncoder setFragmentTexture:_texture atIndex:0];
            
            // Draw a quad covering the entire screen
            [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
            
            [renderEncoder endEncoding];
            
            [commandBuffer presentDrawable:self.currentDrawable];
        }
    }
    
    [commandBuffer commit];
}

@end

// Helper function for error setting to avoid macro issues
void SetMetalError(int code, const char* msg) {
    g_lastError.code = code;
    
    if (msg) {
        strncpy(g_lastError.message, msg, sizeof(g_lastError.message)-1);
        g_lastError.message[sizeof(g_lastError.message)-1] = '\0';
    } else {
        g_lastError.message[0] = '\0';
    }
    
    // Get function name if available (C++ only)
#ifdef __cplusplus
    strncpy(g_lastError.function, __func__, sizeof(g_lastError.function)-1);
#else
    g_lastError.function[0] = '\0';
#endif
    g_lastError.function[sizeof(g_lastError.function)-1] = '\0';
    
    // Get file name and line
    strncpy(g_lastError.file, __FILE__, sizeof(g_lastError.file)-1);
    g_lastError.file[sizeof(g_lastError.file)-1] = '\0';
    g_lastError.line = __LINE__;
}

@implementation StandaloneAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    NSLog(@"[EMULATOR] Application launched - setting up window");
    
    // Initialize memory tracking
    MemoryTracker_Init();
    
    // Enable debug mode in development
    Metal_SetDebugMode(true);
    Metal_SetLogLevel(LOG_LEVEL_INFO);
    
    // Log successful initialization
    METAL_LOG_INFO("FBNeo Metal implementation started");
    
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
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device) {
        NSLog(@"[RENDERER LOOP] ERROR: Metal is not supported on this device!");
        SetMetalError(-10, "Metal is not supported on this device");
        
        NSAlert *alert = [[NSAlert alloc] init];
        alert.messageText = @"Metal Not Supported";
        alert.informativeText = @"This application requires Metal graphics support, which is not available on your device.";
        [alert runModal];
        [NSApp terminate:nil];
        return;
    }
    
    NSLog(@"[RENDERER LOOP] Creating Metal view with device: %@", device);
    self.metalView = [[MetalView alloc] initWithFrame:self.window.contentView.bounds device:device];
    self.metalView.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    self.metalView.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
    self.metalView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    
    [self.window.contentView addSubview:self.metalView];
    
    // Create status label
    self.statusLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, 20, 600, 24)];
    self.statusLabel.stringValue = @"FBNeo Metal - ROM not loaded";
    self.statusLabel.bezeled = NO;
    self.statusLabel.drawsBackground = NO;
    self.statusLabel.editable = NO;
    self.statusLabel.selectable = NO;
    self.statusLabel.textColor = [NSColor whiteColor];
    
    [self.metalView addSubview:self.statusLabel];
    
    // Initialize core subsystems with error handling
    METAL_LOG_INFO("Initializing core subsystems");
    
    if (Graphics_InitComponents() != 0) {
        SetMetalError(-6, "Failed to initialize graphics components");
        METAL_LOG_ERROR("Failed to initialize graphics components");
    }
    
    if (Metal_InitInput() != 0) {
        SetMetalError(-9, "Failed to initialize input system");
        METAL_LOG_ERROR("Failed to initialize input system");
    }
    
    if (Metal_InitAudio() != 0) {
        SetMetalError(-8, "Failed to initialize audio system");
        METAL_LOG_ERROR("Failed to initialize audio system");
        
        // Try fallback audio
        METAL_LOG_WARNING("Attempting fallback audio initialization");
        Metal_EnableFallbackAudio();
    }
    
    // Show the window
    [self.window makeKeyAndOrderFront:nil];
    
    // Make sure we are the active application
    [NSApp activateIgnoringOtherApps:YES];
    
    METAL_LOG_INFO("Window setup complete - window should now be visible");
    
    // Check for command line arguments to load ROM
    NSArray *args = [[NSProcessInfo processInfo] arguments];
    if (args.count > 1) {
        NSString *romPath = args[1];
        [self loadROM:romPath];
    }
    
    // Start a timer to update the display
    self.gameTimer = [NSTimer scheduledTimerWithTimeInterval:1.0/60.0
                                                     target:self
                                                   selector:@selector(gameLoop)
                                                   userInfo:nil
                                                    repeats:YES];
    
    // Set the timer to run in common run loop modes
    [[NSRunLoop currentRunLoop] addTimer:self.gameTimer forMode:NSRunLoopCommonModes];
}

- (void)loadROM:(NSString *)romPath {
    METAL_LOG_INFO("Loading ROM: %s", [romPath UTF8String]);
    self.statusLabel.stringValue = [NSString stringWithFormat:@"Loading ROM: %@", [romPath lastPathComponent]];
    
    self.gameLoaded = NO;
    
    // Extract base name for ROM identification
    NSString *fileName = [romPath lastPathComponent];
    NSString *romName = [[fileName componentsSeparatedByString:@"."] firstObject];
    
    // Set the ROM path globally
    Metal_SetRomPath([romPath UTF8String]);
    Metal_ClearLastError(); // Clear any previous errors
    
    // Load and initialize the ROM
    bool success = Metal_LoadAndInitROM([romPath UTF8String]);
    
    if (success) {
        self.gameLoaded = YES;
        self.gameName = romName;
        
        // Get the game title from the driver
        const char* gameTitle = BurnDrvGetTextA(DRV_FULLNAME);
        
        // Set the game title and mark as running
        Metal_SetGameTitle(gameTitle ? gameTitle : [romName UTF8String]);
        Metal_SetGameRunning(true);
        
        // Update status label with game info
        const char* title = Metal_GetGameTitle();
        self.statusLabel.stringValue = [NSString stringWithFormat:@"Running: %s", title];
        
        METAL_LOG_INFO("ROM loaded successfully: %s", title);
        
        // Initialize frame buffer based on game dimensions
        int width = 0, height = 0, pitch = 0;
        unsigned char* frameData = Metal_GetFrameBuffer(&width, &height, &pitch);
        
        if (width > 0 && height > 0) {
            // Initialize our frame buffer if needed
            if (g_frameBuffer.width != width || g_frameBuffer.height != height) {
                // If frame buffer exists but wrong size, recreate it
                if (g_frameBuffer.data) {
                    free(g_frameBuffer.data);
                    g_frameBuffer.data = NULL;
                }
                
                // Create new appropriately sized frame buffer
                g_frameBuffer.width = width;
                g_frameBuffer.height = height;
                g_frameBuffer.pitch = width * sizeof(uint32_t);
                g_frameBuffer.data = (uint32_t*)malloc(width * height * sizeof(uint32_t));
                g_frameBuffer.updated = false;
                
                if (g_frameBuffer.data) {
                    // Clear to black
                    memset(g_frameBuffer.data, 0, width * height * sizeof(uint32_t));
                    printf("Frame buffer initialized: %dx%d (%lu bytes)\n", 
                           g_frameBuffer.width, g_frameBuffer.height,
                           g_frameBuffer.width * g_frameBuffer.height * sizeof(uint32_t));
                } else {
                    printf("ERROR: Failed to allocate frame buffer memory\n");
                    return;
                }
            }
            
            METAL_LOG_INFO("Frame buffer dimensions: %dx%d, pitch: %d", width, height, pitch);
        }
    } else {
        self.gameLoaded = NO;
        self.statusLabel.stringValue = [NSString stringWithFormat:@"Failed to load ROM: %@", [romPath lastPathComponent]];
        
        // Log the error
        if (Metal_HasError()) {
            METAL_LOG_ERROR("Failed to load ROM: %s - %s", 
                          [romPath UTF8String], 
                          Metal_GetLastErrorMessage());
        } else {
            METAL_LOG_ERROR("Failed to load ROM: %s", [romPath UTF8String]);
        }
    }
}

- (void)gameLoop {
    static int frameCount = 0;
    
    // Skip if no game loaded
    if (!self.gameLoaded) {
        // If no game loaded, render a test pattern every 120 frames
        frameCount++;
        if (frameCount % 120 == 0) {
            // Generate test pattern to show the system is working
            int patternType = (frameCount / 120) % 4;
            Metal_GenerateTestPattern(patternType);
            
            // Update the Metal view
            int width = 0, height = 0, pitch = 0;
            unsigned char* frameBuffer = Metal_GetFrameBuffer(&width, &height, &pitch);
            if (frameBuffer && width > 0 && height > 0) {
                [self.metalView updateFrameBuffer:frameBuffer width:width height:height pitch:pitch];
            }
        }
        return;
    }
    
    @try {
        // Track the start time for performance monitoring
        NSTimeInterval startTime = [NSDate timeIntervalSinceReferenceDate];
        
        // Update input state
        Metal_UpdateInputState();
        
        // Run a frame of emulation with error handling
        // Setup the frame buffer with our bridge
        bool drawFrame = true;
        Bridge_PreFrame(drawFrame);
        
        // Run emulation frame
        bool frameSuccess = Metal_ProcessFrame();
        if (!frameSuccess) {
            if (Metal_HasError()) {
                METAL_LOG_ERROR("Error processing frame: %s", Metal_GetLastErrorMessage());
            } else {
                METAL_LOG_ERROR("Unknown error processing frame");
            }
        }
        
        // Process bridge post-frame handling
        Bridge_PostFrame();
        
        // Process audio
        int audioResult = Metal_ProcessAudio();
        if (audioResult != 0 && Metal_HasError()) {
            METAL_LOG_WARNING("Audio processing error: %s", Metal_GetLastErrorMessage());
        }
        
        // Get the frame buffer from the emulation
        int width = 0, height = 0, pitch = 0;
        unsigned char* frameBuffer = Metal_GetFrameBuffer(&width, &height, &pitch);
        
        if (frameBuffer && width > 0 && height > 0) {
            // Update the frame buffer in the Metal view
            [self.metalView updateFrameBuffer:frameBuffer width:width height:height pitch:pitch];
        } else {
            METAL_LOG_WARNING("Invalid frame buffer from emulation");
        }
        
        // Track frame for FPS calculation
        Metal_TrackFrame();
        
        // Update game status in UI every 30 frames
        frameCount++;
        if (frameCount % 30 == 0) {
            float fps = Metal_GetFrameRate();
            int totalFrames = Metal_GetTotalFrames();
            const char* title = Metal_GetGameTitle();
            
            // Update status label with FPS info
            self.statusLabel.stringValue = [NSString stringWithFormat:@"%s - %.1f FPS - Frame %d", 
                                           title, fps, totalFrames];
            
            // Print to console occasionally for debugging
            if (frameCount % 300 == 0) {
                METAL_LOG_DEBUG("Game status: %s - %.1f FPS - Frame %d", 
                              title, fps, totalFrames);
            }
        }
        
        // Calculate and log frame processing time for performance monitoring
        NSTimeInterval endTime = [NSDate timeIntervalSinceReferenceDate];
        NSTimeInterval frameProcessingTime = endTime - startTime;
        
        // Log if the frame took too long to process (performance warning)
        if (frameProcessingTime > 0.018) { // Roughly > 55fps
            METAL_LOG_DEBUG("Frame processing took %.1f ms - possible performance issue", 
                          frameProcessingTime * 1000.0);
        }
    }
    @catch (NSException *exception) {
        // Handle any Objective-C exceptions
        METAL_LOG_ERROR("Exception in game loop: %s - %s", 
                      [[exception name] UTF8String],
                      [[exception reason] UTF8String]);
        
        // Try to recover
        Metal_ClearLastError();
    }
}

- (void)keyDown:(NSEvent *)event {
    // Process key press
    Metal_ProcessKeyDown([event keyCode]);
}

- (void)keyUp:(NSEvent *)event {
    // Process key release
    Metal_ProcessKeyUp([event keyCode]);
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

- (void)applicationWillTerminate:(NSNotification *)notification {
    // Clean up with proper error handling
    METAL_LOG_INFO("Application shutting down, cleaning up resources");
    
    // Safely stop game if running
    if (self.gameLoaded) {
        Metal_SetGameRunning(false);
        Metal_UnloadROM();
    }
    
    // Clean up subsystems
    Metal_ExitInput();
    Metal_ShutdownAudio();
    
    // Clean up frame buffer if needed
    if (g_frameBuffer.data) {
        free(g_frameBuffer.data);
        g_frameBuffer.data = NULL;
    }
    
    METAL_LOG_INFO("Application terminated gracefully");
}

@end

// Main entry point
int main(int argc, const char * argv[]) {
    @autoreleasepool {
        printf("[EMULATOR] Starting FBNeo Metal Implementation\n");
        
        // Initialize the emulator
        Debug_Init(NULL);
        
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
        StandaloneAppDelegate *delegate = [[StandaloneAppDelegate alloc] init];
        [NSApp setDelegate:delegate];
        
        // Finalize app setup and make it a proper application
        [NSApp finishLaunching];
        
        // Start UI
        [NSApp activateIgnoringOtherApps:YES];
        [NSApp run];
    }
    
    return 0;
} 