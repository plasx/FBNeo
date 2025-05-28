// Define TCHAR first, before any other includes
#ifndef TCHAR_DEFINED
#define TCHAR_DEFINED
typedef char TCHAR;
#endif

#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#include "metal_compat_layer.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include "metal_savestate_stubs.h"
#include "metal_debug_overlay_stubs.h"
#include "metal_input_stubs.h"
#include "metal_input_defs.h"
#include "metal_audio_stubs.h"
#include "metal_renderer.h" // Include the dedicated Metal renderer header

// External function declarations to fix linker errors
#ifdef __cplusplus
extern "C" {
#endif
    // ROM validation functions
    INT32 Metal_ValidateROMFile(const char* romPath);
    INT32 FindCps2Rom(const char* romPath);
    
    // BurnLib functions
    INT32 BurnLibInit_Metal();
    INT32 BurnLibExit_Metal();

    // CPS2 system functions
    void Metal_SetCurrentROMPath(const char* romPath);
    INT32 Metal_CPS2_Init();
    INT32 Metal_CPS2_LoadGame(int gameIndex);
    INT32 Metal_RunFrame(INT32 bDraw);
#ifdef __cplusplus
}
#endif

// Import the external Metal renderer functions
extern int MetalRenderer_Init(void* viewPtr, int width, int height);
extern void MetalRenderer_Exit();
extern bool MetalRenderer_Resize(int width, int height);
extern void MetalRenderer_SetPaused(bool paused);
extern void MetalRenderer_ForceRedraw();
extern bool MetalRenderer_UpdateFrameTexture(void* frameData, int width, int height, int pitch);

// Global variables for our simple app
static bool g_bVerboseLogging = false;
static bool g_bShowDisplay = true;  // Changed to true by default to show Metal window
static bool g_bAudioEnabled = true;
static bool g_bInputEnabled = true;
static bool g_bDumpFrames = false;
static bool g_bTestSaveState = false;
static int g_nRunForFrames = 300;  // Default: run for 5 seconds (300 frames)
static char g_dumpDirectory[512] = "./frame_dumps";
static int g_nFrameCount = 0;
static bool g_bEmulationRunning = false;
static bool g_bQuitRequested = false;
static int g_nEmulationMode = EMULATION_MODE_MINIMAL_MACRO; // Updated to use new macro name
static int g_nGameIndex = 0; // Default game index
static char g_szRomPath[512] = {0}; // ROM path

// Metal display objects
static NSWindow *g_window = nil;
static MTKView *g_metalView = nil;

// Timer-based emulation for Metal display
static NSTimer *g_emulationTimer = nil;

// Custom MTKView subclass to handle keyboard input
@interface FBNeoMetalView : NSView
@property (nonatomic, assign) BOOL commandKeyDown;
@end

@implementation FBNeoMetalView

- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) {
        // Ensure the view is properly configured for rendering
        self.wantsLayer = YES;
        self.layerContentsRedrawPolicy = NSViewLayerContentsRedrawDuringViewResize;
        self.commandKeyDown = NO;
    }
    return self;
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)keyDown:(NSEvent *)event {
    // Check if this is a command key combination
    if (self.commandKeyDown && !event.isARepeat) {
        int keyCode = (int)[event keyCode];
        
        // Handle Command+S (Save State)
        if (keyCode == kVK_ANSI_S) {
            extern INT32 Metal_QuickSave();
            Metal_QuickSave();
            return;
        }
        
        // Handle Command+L (Load State)
        if (keyCode == kVK_ANSI_L) {
            extern INT32 Metal_QuickLoad();
            Metal_QuickLoad();
            return;
        }
        
        // Handle Command+F (Toggle Fullscreen)
        if (keyCode == kVK_ANSI_F) {
            [self.window toggleFullScreen:nil];
            return;
        }
        
        // Handle Command+1 (Switch to Minimal Emulation Mode)
        if (keyCode == kVK_ANSI_1) {
            if (g_nEmulationMode != EMULATION_MODE_MINIMAL_MACRO) {
                g_nEmulationMode = EMULATION_MODE_MINIMAL_MACRO;
                MetalRenderer_SetEmulationMode(EMULATION_MODE_MINIMAL_MACRO);
                NSLog(@"Switched to Minimal Emulation Mode");
            }
            return;
        }
        
        // Handle Command+2 (Switch to CPS2 Emulation Mode)
        if (keyCode == kVK_ANSI_2) {
            if (g_nEmulationMode != EMULATION_MODE_CPS2_MACRO) {
                g_nEmulationMode = EMULATION_MODE_CPS2_MACRO;
                MetalRenderer_SetEmulationMode(EMULATION_MODE_CPS2_MACRO);
                NSLog(@"Switched to CPS2 Emulation Mode");
            }
            return;
        }
    }
    
    // Handle regular key press
    unsigned short keyCode = [event keyCode];
    Metal_HandleKeyDown((int)keyCode);
}

- (void)keyUp:(NSEvent *)event {
    unsigned short keyCode = [event keyCode];
    Metal_HandleKeyUp((int)keyCode);
}

- (void)flagsChanged:(NSEvent *)event {
    NSEventModifierFlags flags = [event modifierFlags];
    
    // Track Command key state for Command+key combinations
    BOOL wasCommandKeyDown = self.commandKeyDown;
    self.commandKeyDown = (flags & NSEventModifierFlagCommand) != 0;
    
    // If Command key state changed, update the handler
    if (wasCommandKeyDown != self.commandKeyDown) {
        Metal_HandleKeyDown(kVK_Command);
    } else {
        Metal_HandleKeyUp(kVK_Command);
    }
}

- (void)viewDidMoveToWindow {
    [super viewDidMoveToWindow];
    if (self.window) {
        // Make this view the first responder to receive keyboard events
        [self.window makeFirstResponder:self];
    }
}

- (BOOL)performKeyEquivalent:(NSEvent *)event {
    // Handle key equivalents that would normally be intercepted by the menu system
    if (event.modifierFlags & NSEventModifierFlagCommand) {
        // Let our keyDown method handle Command key combinations
        [self keyDown:event];
        return YES;
    }
    
    return [super performKeyEquivalent:event];
}

@end

// Forward declarations for internal functions
void log_message(const char* format, ...);
void verbose_log(const char* format, ...);
void cleanup_emulation();
bool initialize_emulation(int emulationMode, const char* romPath);
void checkForBlankScreen();
void emulation_timer_callback(NSTimer *timer);

// Forward declarations for new systems
extern INT32 Metal_InitSaveState();
extern INT32 Metal_ExitSaveState();
extern INT32 Metal_InitDebugOverlay(NSWindow* parentWindow);
extern INT32 Metal_ExitDebugOverlay();
extern void Metal_UpdateDebugOverlay(int frameCount);

// Forward declaration for debug function
extern "C" void Metal_PrintFrameBufferInfo();

// Define variables to fix linker errors
int g_nFrameWidth = 384;  // Default CPS2 width
int g_nFrameHeight = 224; // Default CPS2 height

// Request quit function
void Metal_RequestQuit() {
    printf("[Metal_RequestQuit] Quit requested by user\n");
    g_bQuitRequested = true;
    g_bEmulationRunning = false;
}

// Function to dump frame buffer for debugging
void dump_frame_buffer_to_file(const char* filename, void* buffer, int width, int height) {
    if (!buffer) {
        printf("[dump_frame_buffer_to_file] ERROR: Buffer is NULL\n");
        return;
    }
    
    FILE* f = fopen(filename, "wb");
    if (!f) {
        printf("[dump_frame_buffer_to_file] ERROR: Could not open file %s for writing\n", filename);
        return;
    }
    
    // Write PPM format (simple uncompressed image)
    fprintf(f, "P6\n%d %d\n255\n", width, height);
    
    // Convert BGRA to RGB for PPM
    unsigned char* bgra = (unsigned char*)buffer;
    for (int i = 0; i < width * height; i++) {
        fputc(bgra[i*4+2], f); // R
        fputc(bgra[i*4+1], f); // G
        fputc(bgra[i*4+0], f); // B
    }
    
    fclose(f);
    printf("[dump_frame_buffer_to_file] Dumped frame to %s\n", filename);
}

// Emulation timer callback - performs one frame of emulation
void emulation_timer_callback(NSTimer *timer) {
    if (!g_bEmulationRunning || g_bQuitRequested) {
        return;
    }
    
    g_nFrameCount++;
    
    // Log frame once per second
    if (g_nFrameCount % 60 == 0) {
        printf("Frame: %d (%.1f seconds)\n", g_nFrameCount, (float)g_nFrameCount / 60.0f);
    }
    
    // Frame limit check
    if (g_nRunForFrames > 0 && g_nFrameCount >= g_nRunForFrames) {
        printf("=== EMULATION TIMER COMPLETED ===\n");
        printf("Total frames executed: %d\n", g_nFrameCount);
        printf("Total time: %.2f seconds\n", (float)g_nFrameCount / 60.0f);
        
        // Stop emulation and clean up
        g_bEmulationRunning = false;
        g_bQuitRequested = true;
        
        // Check that NSApplication exists
        if (NSApp) {
            [NSApp terminate:nil];
        }
        
        return;
    }
    
    // Run one frame of emulation based on the selected mode
    bool frameResult = false;
    switch (g_nEmulationMode) {
        case EMULATION_MODE_CPS2_MACRO:
            // Use Metal_RunFrame for all emulation modes
            frameResult = (Metal_RunFrame(g_bShowDisplay ? 1 : 0) == 0);
            break;
            
        case EMULATION_MODE_MINIMAL_MACRO:
        default:
            frameResult = (Metal_RunFrame(g_bShowDisplay ? 1 : 0) == 0);
            break;
    }
    
    // Check frame execution result
    if (!frameResult) {
        printf("Frame execution failed!\n");
        // Don't quit yet, just report the error
    }
    
    // Dump frame if requested or on specific frames
    if (g_bDumpFrames || g_nFrameCount % 60 == 0) {
        // Get the frame buffer from our bridge
        extern void* Metal_GetFrameBuffer();
        extern void* Metal_GetRawFrameBuffer();
        
        void* frameBuffer = Metal_GetFrameBuffer();
        void* rawBuffer = Metal_GetRawFrameBuffer();
        
        // Get the dimensions
        extern int g_nFrameWidth;
        extern int g_nFrameHeight;
        
        if (frameBuffer) {
            char filename[256];
            snprintf(filename, sizeof(filename), "frame_dump_%03d.ppm", g_nFrameCount);
            dump_frame_buffer_to_file(filename, frameBuffer, g_nFrameWidth, g_nFrameHeight);
        }
        
        if (rawBuffer && rawBuffer != frameBuffer) {
            char filename[256];
            snprintf(filename, sizeof(filename), "raw_frame_%03d.ppm", g_nFrameCount);
            dump_frame_buffer_to_file(filename, rawBuffer, g_nFrameWidth, g_nFrameHeight);
        }
    }
    
    // Update debug overlay if needed
    if (g_nFrameCount % 5 == 0) {
        Metal_UpdateDebugOverlay(g_nFrameCount);
    }
}

// Initialize the Metal window
bool initialize_metal_window() {
    // Create the main application
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    
    // Create the menu bar
    NSMenu *mainMenu = [[NSMenu alloc] init];
    [NSApp setMainMenu:mainMenu];
    
    // Create the application menu
    NSMenuItem *appMenuItem = [[NSMenuItem alloc] init];
    [mainMenu addItem:appMenuItem];
    
    NSMenu *appMenu = [[NSMenu alloc] init];
    [appMenuItem setSubmenu:appMenu];
    
    NSMenuItem *quitMenuItem = [[NSMenuItem alloc] initWithTitle:@"Quit"
                                                         action:@selector(terminate:)
                                                  keyEquivalent:@"q"];
    [appMenu addItem:quitMenuItem];
    
    // Create the window
    NSRect windowRect = NSMakeRect(100, 100, 384*2, 224*2); // Standard CPS2 resolution (scaled up)
    NSWindowStyleMask styleMask = NSWindowStyleMaskTitled |
                                  NSWindowStyleMaskClosable |
                                  NSWindowStyleMaskMiniaturizable |
                                  NSWindowStyleMaskResizable;
    
    g_window = [[NSWindow alloc] initWithContentRect:windowRect
                                           styleMask:styleMask
                                             backing:NSBackingStoreBuffered
                                               defer:NO];
    
    [g_window setTitle:@"FBNeo Metal CPS2 Emulator"];
    [g_window center];
    
    // Create a custom view for keyboard input handling
    FBNeoMetalView *containerView = [[FBNeoMetalView alloc] initWithFrame:g_window.contentView.bounds];
    containerView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    [g_window setContentView:containerView];
    
    // Create a Metal device and MTKView manually
    id<MTLDevice> metalDevice = MTLCreateSystemDefaultDevice();
    if (!metalDevice) {
        NSLog(@"Error: Could not create Metal device");
        return false;
    }
    
    // Create Metal view with device
    MTKView *mtkView = [[MTKView alloc] initWithFrame:containerView.bounds device:metalDevice];
    mtkView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    mtkView.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    mtkView.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
    mtkView.framebufferOnly = YES;
    mtkView.paused = NO;
    mtkView.enableSetNeedsDisplay = YES;
    
    // Add Metal view to container view
    [containerView addSubview:mtkView];
    
    // Initialize the Metal renderer with our Metal view
    if (MetalRenderer_Init((__bridge void*)mtkView, 384, 224) != 0) {
        NSLog(@"Failed to initialize Metal renderer");
        return false;
    }
    
    // Store MTK view for future reference
    g_metalView = mtkView;
    
    // Show the window
    [g_window makeKeyAndOrderFront:nil];
    
    // Set up emulation timer (60 FPS)
    g_emulationTimer = [NSTimer scheduledTimerWithTimeInterval:1.0/60.0
                                                       target:[NSBlockOperation blockOperationWithBlock:^{
                                                           emulation_timer_callback(nil);
                                                       }]
                                                     selector:@selector(main)
                                                     userInfo:nil
                                                      repeats:YES];
    
    [[NSRunLoop currentRunLoop] addTimer:g_emulationTimer forMode:NSRunLoopCommonModes];
    
    // Initialize input system
    if (g_bInputEnabled) {
        Metal_InitInput();
    }
    
    // Initialize audio system
    if (g_bAudioEnabled) {
        Metal_InitAudio();
    }
    
    // Initialize save state system
    Metal_InitSaveState();
    
    // Initialize debug overlay
    Metal_InitDebugOverlay(g_window);
    
    return true;
}

// Initialize emulation with specified mode
bool initialize_emulation(int emulationMode, const char* romPath) {
    log_message("Initializing emulation in mode %d with ROM: %s\n", emulationMode, romPath ? romPath : "None");
    
    g_nEmulationMode = emulationMode;
    
    // First initialize the core - always do this regardless of mode
    if (BurnLibInit_Metal() != 0) {
        log_message("Failed to initialize FBNeo core\n");
        return false;
    }
    
    // Store the ROM path for CPS2 system
    if (romPath && romPath[0] != '\0') {
        Metal_SetCurrentROMPath(romPath);
    }
    
    // Set the emulation mode in the renderer
    if (!MetalRenderer_SetEmulationMode(emulationMode)) {
        log_message("Failed to set emulation mode %d\n", emulationMode);
        return false;
    }
    
    // If ROM path is provided, try to load it
    if (romPath && romPath[0] != '\0') {
        // Validate the ROM file first
        if (Metal_ValidateROMFile(romPath) != 0) {
            log_message("Failed to validate ROM file: %s\n", romPath);
            return false;
        }
        
        log_message("ROM file validation successful\n");
        
        // Find the game index
        int gameIndex = FindCps2Rom(romPath);
        if (gameIndex >= 0) {
            g_nGameIndex = gameIndex;
            strncpy(g_szRomPath, romPath, sizeof(g_szRomPath) - 1);
            
            log_message("Found game index: %d\n", gameIndex);
            
            // Now initialize the CPS2 system
            if (Metal_CPS2_Init() != 0) {
                log_message("Failed to initialize CPS2 system\n");
                return false;
            }
            
            log_message("CPS2 system initialized successfully\n");
            
            // Force CPS2 emulation mode
            g_nEmulationMode = EMULATION_MODE_CPS2_MACRO;
            
            // Load the CPS2 game
            if (Metal_CPS2_LoadGame(gameIndex) != 0) {
                log_message("Failed to load CPS2 game %d\n", gameIndex);
                return false;
            }
            
            log_message("Successfully loaded CPS2 game %d\n", gameIndex);
        } else {
            log_message("ROM found but no matching game index found\n");
            return false;
        }
    } else {
        log_message("No ROM specified, running in demo mode\n");
    }
    
    g_bEmulationRunning = true;
    return true;
}

// Clean up emulation resources
void cleanup_emulation() {
    log_message("Cleaning up emulation resources\n");
    
    // Stop emulation timer
    if (g_emulationTimer) {
        [g_emulationTimer invalidate];
        g_emulationTimer = nil;
    }
    
    // Shut down all subsystems
    Metal_ExitDebugOverlay();
    Metal_ExitSaveState();
    
    if (g_bAudioEnabled) {
        Metal_ExitAudio();
    }
    
    if (g_bInputEnabled) {
        Metal_ExitInput();
    }
    
    // Clean up Metal renderer
    MetalRenderer_Cleanup();
    
    log_message("Emulation cleanup complete\n");
}

// Simple logging functions
void log_message(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void verbose_log(const char* format, ...) {
    if (g_bVerboseLogging) {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }
}

// Helper function to check if graphics are working
void checkForBlankScreen() {
    // Get a frame buffer to check
    extern void* Metal_GetFrameBuffer();
    void* frameBuffer = Metal_GetFrameBuffer();
    
    if (!frameBuffer) {
        NSLog(@"❌ CRITICAL ERROR: No frame buffer available to check!");
        return;
    }
    
    // Check if the buffer has content
    uint32_t* pixels = (uint32_t*)frameBuffer;
    int width = 384;
    int height = 224;
    int nonZeroPixels = 0;
    int sampleSize = 1000;
    
    // Count non-zero pixels in a sample area
    for (int i = 0; i < sampleSize && i < (width * height); i++) {
        if (pixels[i] != 0) nonZeroPixels++;
    }
    
    float nonZeroPercent = ((float)nonZeroPixels / sampleSize) * 100.0f;
    
    // Report status
    if (nonZeroPercent < 1.0f) {
        NSLog(@"❌❌❌ BLANK SCREEN DETECTED! Only %.1f%% non-zero pixels", nonZeroPercent);
        NSLog(@"❌❌❌ This means graphics aren't showing up properly!");
        
        // Add debug pattern
        NSLog(@"⚠️ Adding diagnostic pattern to screen");
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                // Checkerboard pattern
                if ((x / 16 + y / 16) % 2 == 0) {
                    pixels[y * width + x] = 0xFF0000FF; // Red
                } else {
                    pixels[y * width + x] = 0xFF00FF00; // Green
                }
                
                // Add border
                if (x < 10 || x >= width - 10 || y < 10 || y >= height - 10) {
                    pixels[y * width + x] = 0xFFFFFF00; // Yellow border
                }
            }
        }
        
        // Add "BLANK SCREEN" text
        int textX = 120;
        int textY = 100;
        for (int y = 0; y < 40; y++) {
            for (int x = 0; x < 200; x++) {
                if (y < 5 || y >= 35 || x < 5 || x >= 195) {
                    pixels[(textY + y) * width + (textX + x)] = 0xFF000000; // Black border
                } else {
                    pixels[(textY + y) * width + (textX + x)] = 0xFFFFFFFF; // White background
                }
            }
        }
    } else {
        NSLog(@"✅✅✅ SCREEN CONTENT DETECTED! %.1f%% non-zero pixels", nonZeroPercent);
    }
}

// Main entry point
int main(int argc, char* argv[]) {
    @autoreleasepool {
        // Parse command-line arguments
        char romPath[512] = {0};
        
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-v") == 0) {
                g_bVerboseLogging = true;
            } else if (strcmp(argv[i], "-no-display") == 0) {
                g_bShowDisplay = false;
            } else if (strcmp(argv[i], "-no-audio") == 0) {
                g_bAudioEnabled = false;
            } else if (strcmp(argv[i], "-no-input") == 0) {
                g_bInputEnabled = false;
            } else if (strcmp(argv[i], "-dump-frames") == 0) {
                g_bDumpFrames = true;
            } else if (strcmp(argv[i], "-test-savestate") == 0) {
                g_bTestSaveState = true;
            } else if (strcmp(argv[i], "-frames") == 0 && i + 1 < argc) {
                g_nRunForFrames = atoi(argv[++i]);
            } else if (strcmp(argv[i], "-dump-dir") == 0 && i + 1 < argc) {
                strncpy(g_dumpDirectory, argv[++i], sizeof(g_dumpDirectory) - 1);
            } else if (strcmp(argv[i], "-mode") == 0 && i + 1 < argc) {
                g_nEmulationMode = atoi(argv[++i]);
            } else if (argv[i][0] != '-') {
                // Assume this is the ROM path
                strncpy(romPath, argv[i], sizeof(romPath) - 1);
            }
        }
        
        log_message("FBNeo Metal CPS2 Emulator\n");
        log_message("========================\n");
        log_message("Verbose logging: %s\n", g_bVerboseLogging ? "Yes" : "No");
        log_message("Display: %s\n", g_bShowDisplay ? "Yes" : "No");
        log_message("Audio: %s\n", g_bAudioEnabled ? "Yes" : "No");
        log_message("Input: %s\n", g_bInputEnabled ? "Yes" : "No");
        log_message("Dump frames: %s\n", g_bDumpFrames ? "Yes" : "No");
        log_message("Test save state: %s\n", g_bTestSaveState ? "Yes" : "No");
        log_message("Frame limit: %d\n", g_nRunForFrames);
        log_message("Dump directory: %s\n", g_dumpDirectory);
        log_message("Emulation mode: %d\n", g_nEmulationMode);
        log_message("ROM path: %s\n", romPath[0] ? romPath : "None");
        log_message("========================\n");
        
        // Initialize the Metal window
        if (g_bShowDisplay) {
            if (!initialize_metal_window()) {
                log_message("Failed to initialize Metal window\n");
                return 1;
            }
        }
        
        // Initialize emulation
        if (!initialize_emulation(g_nEmulationMode, romPath)) {
            log_message("Failed to initialize emulation\n");
            return 1;
        }
        
        // Run the application main loop
        [NSApp activateIgnoringOtherApps:YES];
        [NSApp run];
    }
    
    return 0;
} 