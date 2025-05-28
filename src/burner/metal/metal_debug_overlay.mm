#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#include "metal_compat_layer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

// External declarations
extern "C" {
    extern INT32 g_nFrameCount;
    extern void Metal_PrintCPS2InputState();
    extern void Metal_GetROMValidationStats(int* totalROMs, int* validatedROMs, const char** currentPath);
    extern float Metal_GetAudioLatency();
    extern bool Metal_IsAudioInitialized();
    extern int Metal_GetActiveInputs();
    
    // CPS2 input variables
    extern UINT8 CpsReset;
    extern UINT8 CpsInp000[8];
    extern UINT8 CpsInp001[8];
    extern UINT8 CpsInp011[8];
    extern UINT8 CpsInp020[8];
}

// Forward declarations
extern "C" void* Metal_GetRawFrameBuffer();

// Debug overlay state
static NSWindow* g_parentWindow = nil;
static NSTextField* g_debugTextField = nil;
static bool g_debugOverlayVisible = false;
static bool g_debugOverlayInitialized = false;
static bool g_quitRequested = false;

// Application control functions
extern "C" void Metal_RequestQuit() {
    printf("[Metal_RequestQuit] Quit requested\n");
    g_quitRequested = true;
    
    // Post a notification to the main thread to quit
    dispatch_async(dispatch_get_main_queue(), ^{
        [NSApp terminate:nil];
    });
}

extern "C" bool Metal_IsQuitRequested() {
    return g_quitRequested;
}

// FPS calculation
static int g_frameCount = 0;
static double g_lastFPSUpdate = 0.0;
static double g_currentFPS = 0.0;
static int g_framesSinceLastUpdate = 0;

// Performance tracking
static double g_frameTimeAccumulator = 0.0;
static double g_minFrameTime = 999.0;
static double g_maxFrameTime = 0.0;
static int g_frameTimeCount = 0;

// Debug overlay
typedef int INT32;

// Get current time in seconds
static double getCurrentTime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

// Debug overlay state
static NSWindow* g_debugWindow = nil;
static NSTextView* g_debugTextView = nil;
static bool g_debugOverlayEnabled = false;
static NSTimer* g_updateTimer = nil;
static uint64_t g_lastFrameTime = 0;
static float g_avgFrameTime = 0.0f;
static int g_frameTimeHistoryIndex = 0;
static float g_frameTimeHistory[60] = {0};

// Initialize debug overlay
INT32 Metal_InitDebugOverlay(NSWindow* parentWindow) {
    printf("[Metal_InitDebugOverlay] Initializing debug overlay\n");
    
    if (g_debugWindow) {
        printf("[Metal_InitDebugOverlay] Debug overlay already initialized\n");
        return 0;
    }
    
    // Create debug window
    NSRect frame = NSMakeRect(100, 100, 400, 600);
    g_debugWindow = [[NSWindow alloc] initWithContentRect:frame
                                               styleMask:NSWindowStyleMaskTitled |
                                                        NSWindowStyleMaskClosable |
                                                        NSWindowStyleMaskResizable
                                                 backing:NSBackingStoreBuffered
                                                   defer:NO];
    
    [g_debugWindow setTitle:@"FBNeo CPS2 Debug Info"];
    [g_debugWindow setLevel:NSFloatingWindowLevel];
    
    // Create text view for debug output
    NSScrollView* scrollView = [[NSScrollView alloc] initWithFrame:[[g_debugWindow contentView] bounds]];
    scrollView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    scrollView.hasVerticalScroller = YES;
    
    g_debugTextView = [[NSTextView alloc] initWithFrame:scrollView.bounds];
    g_debugTextView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    g_debugTextView.editable = NO;
    g_debugTextView.richText = YES;
    g_debugTextView.font = [NSFont fontWithName:@"Menlo" size:11.0];
    g_debugTextView.backgroundColor = [NSColor blackColor];
    g_debugTextView.textColor = [NSColor greenColor];
    
    scrollView.documentView = g_debugTextView;
    [g_debugWindow.contentView addSubview:scrollView];
    
    // Start update timer
    g_updateTimer = [NSTimer scheduledTimerWithTimeInterval:1.0/10.0  // 10Hz update
                                                     target:[NSBlockOperation blockOperationWithBlock:^{
                                                         Metal_UpdateDebugOverlay(0);
                                                     }]
                                                   selector:@selector(main)
                                                   userInfo:nil
                                                    repeats:YES];
    
    printf("[Metal_InitDebugOverlay] Debug overlay initialized\n");
    return 0;
}

// Exit debug overlay
INT32 Metal_ExitDebugOverlay() {
    printf("[Metal_ExitDebugOverlay] Shutting down debug overlay\n");
    
    if (g_updateTimer) {
        [g_updateTimer invalidate];
        g_updateTimer = nil;
    }
    
    if (g_debugWindow) {
        [g_debugWindow close];
        g_debugWindow = nil;
    }
    
    g_debugTextView = nil;
    g_debugOverlayEnabled = false;
    
    return 0;
}

// Toggle debug overlay visibility
void Metal_ToggleDebugOverlay() {
    if (!g_debugWindow) {
        return;
    }
    
    g_debugOverlayEnabled = !g_debugOverlayEnabled;
    
    if (g_debugOverlayEnabled) {
        [g_debugWindow orderFront:nil];
    } else {
        [g_debugWindow orderOut:nil];
    }
    
    printf("[Metal_ToggleDebugOverlay] Debug overlay %s\n", 
           g_debugOverlayEnabled ? "enabled" : "disabled");
}

// Update debug overlay content
void Metal_UpdateDebugOverlay(int frameCount) {
    if (!g_debugTextView || !g_debugOverlayEnabled) {
        return;
    }
    
    // Calculate frame timing
    uint64_t currentTime = mach_absolute_time();
    if (g_lastFrameTime > 0) {
        mach_timebase_info_data_t timebase;
        mach_timebase_info(&timebase);
        
        uint64_t elapsed = currentTime - g_lastFrameTime;
        float frameTime = (float)(elapsed * timebase.numer / timebase.denom) / 1000000.0f; // Convert to ms
        
        // Update frame time history
        g_frameTimeHistory[g_frameTimeHistoryIndex] = frameTime;
        g_frameTimeHistoryIndex = (g_frameTimeHistoryIndex + 1) % 60;
        
        // Calculate average frame time
        float sum = 0.0f;
        for (int i = 0; i < 60; i++) {
            sum += g_frameTimeHistory[i];
        }
        g_avgFrameTime = sum / 60.0f;
    }
    g_lastFrameTime = currentTime;
    
    // Build debug text
    NSMutableString* debugText = [NSMutableString string];
    
    // Header
    [debugText appendString:@"╔══════════════════════════════════════╗\n"];
    [debugText appendString:@"║     FBNeo CPS2 Debug Information     ║\n"];
    [debugText appendString:@"╚══════════════════════════════════════╝\n\n"];
    
    // Frame timing
    [debugText appendString:@"═══ Frame Timing ═══\n"];
    [debugText appendFormat:@"Frame Count: %d\n", frameCount];
    [debugText appendFormat:@"Avg Frame Time: %.2f ms (%.1f FPS)\n", 
              g_avgFrameTime, g_avgFrameTime > 0 ? 1000.0f / g_avgFrameTime : 0.0f];
    [debugText appendFormat:@"Target: 16.67 ms (60 FPS)\n"];
    [debugText appendFormat:@"Performance: %s\n\n", 
              g_avgFrameTime <= 16.67f ? "✅ Good" : "⚠️ Slow"];
    
    // ROM status
    [debugText appendString:@"═══ ROM Status ═══\n"];
    int totalROMs = 0, validatedROMs = 0;
    const char* romPath = NULL;
    Metal_GetROMValidationStats(&totalROMs, &validatedROMs, &romPath);
    [debugText appendFormat:@"ROM Path: %s\n", romPath ? romPath : "None"];
    [debugText appendFormat:@"ROMs Loaded: %d/%d\n\n", validatedROMs, totalROMs];
    
    // Audio status
    [debugText appendString:@"═══ Audio Status ═══\n"];
    [debugText appendFormat:@"Audio System: %s\n", 
              Metal_IsAudioInitialized() ? "✅ Initialized" : "❌ Not Initialized"];
    [debugText appendFormat:@"Latency: %.1f ms\n\n", Metal_GetAudioLatency()];
    
    // Input status
    [debugText appendString:@"═══ Input Status ═══\n"];
    [debugText appendFormat:@"Active Inputs: %d\n", Metal_GetActiveInputs()];
    
    // Player 1 status
    [debugText appendString:@"\nPlayer 1:\n"];
    [debugText appendFormat:@"  D-Pad: %s%s%s%s\n",
              CpsInp001[3] ? "↑" : " ",
              CpsInp001[2] ? "↓" : " ",
              CpsInp001[1] ? "←" : " ",
              CpsInp001[0] ? "→" : " "];
    [debugText appendFormat:@"  Buttons: %s %s %s %s %s %s\n",
              CpsInp001[4] ? "LP" : "  ",
              CpsInp001[5] ? "MP" : "  ",
              CpsInp001[6] ? "HP" : "  ",
              CpsInp011[0] ? "LK" : "  ",
              CpsInp011[1] ? "MK" : "  ",
              CpsInp011[2] ? "HK" : "  "];
    [debugText appendFormat:@"  System: %s %s\n",
              CpsInp020[0] ? "START" : "     ",
              CpsInp020[4] ? "COIN" : "    "];
    
    // Player 2 status
    [debugText appendString:@"\nPlayer 2:\n"];
    [debugText appendFormat:@"  D-Pad: %s%s%s%s\n",
              CpsInp000[3] ? "↑" : " ",
              CpsInp000[2] ? "↓" : " ",
              CpsInp000[1] ? "←" : " ",
              CpsInp000[0] ? "→" : " "];
    [debugText appendFormat:@"  Buttons: %s %s %s %s %s %s\n",
              CpsInp000[4] ? "LP" : "  ",
              CpsInp000[5] ? "MP" : "  ",
              CpsInp000[6] ? "HP" : "  ",
              CpsInp011[4] ? "LK" : "  ",
              CpsInp011[5] ? "MK" : "  ",
              CpsInp011[6] ? "HK" : "  "];
    [debugText appendFormat:@"  System: %s %s\n",
              CpsInp020[1] ? "START" : "     ",
              CpsInp020[5] ? "COIN" : "    "];
    
    // System status
    [debugText appendString:@"\nSystem:\n"];
    [debugText appendFormat:@"  Reset: %s\n\n", CpsReset ? "YES" : "NO"];
    
    // Memory usage
    [debugText appendString:@"═══ Memory Usage ═══\n"];
    struct task_basic_info info;
    mach_msg_type_number_t size = TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info, &size) == KERN_SUCCESS) {
        [debugText appendFormat:@"Resident: %.1f MB\n", info.resident_size / 1024.0 / 1024.0];
        [debugText appendFormat:@"Virtual: %.1f MB\n", info.virtual_size / 1024.0 / 1024.0];
    }
    
    // Update text view
    dispatch_async(dispatch_get_main_queue(), ^{
        g_debugTextView.string = debugText;
    });
}

// Show debug overlay
void Metal_ShowDebugOverlay() {
    if (g_debugWindow && !g_debugOverlayEnabled) {
        Metal_ToggleDebugOverlay();
    }
}

// Hide debug overlay
void Metal_HideDebugOverlay() {
    if (g_debugWindow && g_debugOverlayEnabled) {
        Metal_ToggleDebugOverlay();
    }
}

// Check if debug overlay is visible
bool Metal_IsDebugOverlayVisible() {
    return g_debugOverlayEnabled;
}

// Debug log functions
void Debug_Log(int category, const char* message) {
    printf("[Debug] %s\n", message);
}

void Debug_PrintSectionHeader(int category, const char* header) {
    printf("[Debug] === %s ===\n", header);
}

// Debug categories
#define DEBUG_RENDERER 1
#define DEBUG_METAL 2
#define DEBUG_AUDIO_LOOP 3
#define DEBUG_INPUT_LOOP 4 