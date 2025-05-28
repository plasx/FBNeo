#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>
#include "metal_compat_layer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

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

// Initialize debug overlay
extern "C" INT32 Metal_InitDebugOverlay(NSWindow* parentWindow) {
    printf("[Metal_InitDebugOverlay] Initializing debug overlay\n");
    
    if (g_debugOverlayInitialized) {
        printf("[Metal_InitDebugOverlay] Debug overlay already initialized\n");
        return 0;
    }
    
    g_parentWindow = parentWindow;
    
    if (!g_parentWindow) {
        printf("[Metal_InitDebugOverlay] ERROR: No parent window provided\n");
        return 1;
    }
    
    // Create debug text field
    NSRect overlayFrame = NSMakeRect(10, 10, 400, 200);
    g_debugTextField = [[NSTextField alloc] initWithFrame:overlayFrame];
    
    // Configure text field appearance
    [g_debugTextField setBezeled:YES];
    [g_debugTextField setDrawsBackground:YES];
    [g_debugTextField setBackgroundColor:[NSColor colorWithRed:0.0 green:0.0 blue:0.0 alpha:0.8]];
    [g_debugTextField setTextColor:[NSColor greenColor]];
    [g_debugTextField setFont:[NSFont fontWithName:@"Monaco" size:11]];
    [g_debugTextField setEditable:NO];
    [g_debugTextField setSelectable:NO];
    [g_debugTextField setBordered:YES];
    [g_debugTextField setAlignment:NSTextAlignmentLeft];
    
    // Set initial text
    [g_debugTextField setStringValue:@"FBNeo Metal Debug Overlay\nInitializing..."];
    
    // Initially hidden
    [g_debugTextField setHidden:YES];
    
    // Add to parent window
    [[g_parentWindow contentView] addSubview:g_debugTextField];
    
    // Initialize FPS tracking
    g_lastFPSUpdate = getCurrentTime();
    g_frameCount = 0;
    g_framesSinceLastUpdate = 0;
    g_currentFPS = 0.0;
    
    g_debugOverlayInitialized = true;
    printf("[Metal_InitDebugOverlay] Debug overlay initialized\n");
    
    return 0;
}

// Exit debug overlay
extern "C" INT32 Metal_ExitDebugOverlay() {
    printf("[Metal_ExitDebugOverlay] Shutting down debug overlay\n");
    
    if (g_debugTextField) {
        [g_debugTextField removeFromSuperview];
        g_debugTextField = nil;
    }
    
    g_parentWindow = nil;
    g_debugOverlayVisible = false;
    g_debugOverlayInitialized = false;
    
    return 0;
}

// Toggle debug overlay visibility
void Metal_ToggleDebugOverlay() {
    if (!g_debugOverlayInitialized || !g_debugTextField) {
        printf("[Metal_ToggleDebugOverlay] Debug overlay not initialized\n");
        return;
    }
    
    g_debugOverlayVisible = !g_debugOverlayVisible;
    [g_debugTextField setHidden:!g_debugOverlayVisible];
    
    printf("[Metal_ToggleDebugOverlay] Debug overlay %s\n", 
           g_debugOverlayVisible ? "shown" : "hidden");
}

// Update debug overlay with current information
extern "C" void Metal_UpdateDebugOverlay(int frameCount) {
    if (!g_debugOverlayInitialized || !g_debugTextField || !g_debugOverlayVisible) {
        return;
    }
    
    double currentTime = getCurrentTime();
    g_framesSinceLastUpdate++;
    
    // Update FPS every second
    if (currentTime - g_lastFPSUpdate >= 1.0) {
        g_currentFPS = g_framesSinceLastUpdate / (currentTime - g_lastFPSUpdate);
        g_lastFPSUpdate = currentTime;
        g_framesSinceLastUpdate = 0;
    }
    
    // Get frame buffer info
    extern void* Metal_GetFrameBuffer();
    void* frameBuffer = Metal_GetFrameBuffer();
    
    // Calculate frame checksum
    UINT32 frameChecksum = 0;
    if (frameBuffer) {
        UINT32* pixels = (UINT32*)frameBuffer;
        for (int i = 0; i < 384 * 224; i++) {
            frameChecksum ^= pixels[i];
            frameChecksum = (frameChecksum << 1) | (frameChecksum >> 31);
        }
    }
    
    // Get audio info
    extern bool Metal_IsAudioInitialized();
    extern float Metal_GetAudioVolume();
    extern float Metal_GetAudioLatency();
    bool audioInit = Metal_IsAudioInitialized();
    float audioVolume = Metal_GetAudioVolume();
    float audioLatency = Metal_GetAudioLatency();
    
    // Get save state info
    extern int Metal_GetCurrentSaveSlot();
    extern const char* Metal_GetSaveStateStatus();
    int saveSlot = Metal_GetCurrentSaveSlot();
    const char* saveStatus = Metal_GetSaveStateStatus();
    
    // Get input state info
    extern int Metal_GetActiveInputs();
    int activeInputs = Metal_GetActiveInputs();
    
    // Build debug string
    NSMutableString* debugText = [NSMutableString string];
    [debugText appendString:@"=== FBNeo Metal Debug Info ===\n"];
    [debugText appendFormat:@"Frame: %d\n", frameCount];
    [debugText appendFormat:@"FPS: %.1f\n", g_currentFPS];
    [debugText appendFormat:@"Frame Checksum: 0x%08X\n", frameChecksum];
    [debugText appendString:@"\n"];
    
    // ROM info
    [debugText appendString:@"ROM: Marvel vs. Capcom\n"];
    [debugText appendString:@"Resolution: 384x224\n"];
    [debugText appendFormat:@"Frame Buffer: %p\n", frameBuffer];
    [debugText appendString:@"\n"];
    
    // Audio info
    [debugText appendFormat:@"Audio: %s\n", audioInit ? "ON" : "OFF"];
    if (audioInit) {
        [debugText appendFormat:@"Volume: %.0f%%\n", audioVolume * 100.0f];
        [debugText appendFormat:@"Latency: %.1fms\n", audioLatency];
    }
    [debugText appendString:@"\n"];
    
    // Save state info
    [debugText appendFormat:@"Save Slot: %d\n", saveSlot];
    [debugText appendFormat:@"Save Status: %s\n", saveStatus ? saveStatus : "None"];
    [debugText appendString:@"\n"];
    
    // Input info
    [debugText appendFormat:@"Active Inputs: 0x%04X\n", activeInputs];
    [debugText appendString:@"\n"];
    
    // Controls
    [debugText appendString:@"Controls:\n"];
    [debugText appendString:@"F1: Toggle this overlay\n"];
    [debugText appendString:@"F5: Quick save\n"];
    [debugText appendString:@"F8: Quick load\n"];
    [debugText appendString:@"F11: Fullscreen\n"];
    [debugText appendString:@"ESC: Quit\n"];
    [debugText appendString:@"⌘S: Save state\n"];
    [debugText appendString:@"⌘L: Load state\n"];
    
    // Update the text field
    [g_debugTextField setStringValue:debugText];
}

// Show debug overlay
void Metal_ShowDebugOverlay() {
    if (g_debugOverlayInitialized && g_debugTextField) {
        g_debugOverlayVisible = true;
        [g_debugTextField setHidden:NO];
    }
}

// Hide debug overlay
void Metal_HideDebugOverlay() {
    if (g_debugOverlayInitialized && g_debugTextField) {
        g_debugOverlayVisible = false;
        [g_debugTextField setHidden:YES];
    }
}

// Check if debug overlay is visible
bool Metal_IsDebugOverlayVisible() {
    return g_debugOverlayVisible;
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