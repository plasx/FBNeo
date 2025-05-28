#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#include <stdio.h>
#include <stdlib.h>
#include "metal_debug_overlay_stubs.h"

// Basic type definitions
typedef int INT32;
typedef unsigned int UINT32;

// Forward declarations with proper C linkage
extern "C" INT32 Metal_InitDebugOverlay(NSWindow* parentWindow);
extern "C" INT32 Metal_ExitDebugOverlay();
extern "C" void Metal_UpdateDebugOverlay(int frameCount);
extern "C" void Debug_Log(int category, const char* message);
extern "C" void Debug_PrintSectionHeader(int category, const char* header);

// Forward declarations
extern "C" {
    // State management functions
    int Metal_GetCurrentSaveSlot();
    const char* Metal_GetSaveStateStatus();
}

// Global state
static NSWindow* g_debugWindow = nil;
static NSTextField* g_debugTextField = nil;
static NSTextField* g_stateInfoField = nil;
static bool g_debugOverlayVisible = false;
static bool g_debugOverlayEnabled = false;
static NSWindow* g_parentWindow = nil;

// For save state status message
static NSTextField* g_statusMessageField = nil;
static NSTimer* g_statusTimer = nil;
static int g_statusTimeout = 0;

// Initialize debug overlay
INT32 Metal_InitDebugOverlay(NSWindow* parentWindow) {
    printf("[Metal_InitDebugOverlay] Initializing debug overlay\n");
    g_parentWindow = parentWindow;
    
    // Create the debug window if it doesn't exist
    if (!g_debugWindow) {
        NSRect frame = NSMakeRect(20, 20, 300, 160);
        NSWindowStyleMask style = NSWindowStyleMaskTitled | 
                                 NSWindowStyleMaskClosable | 
                                 NSWindowStyleMaskUtilityWindow;
        
        g_debugWindow = [[NSWindow alloc] initWithContentRect:frame
                                                    styleMask:style
                                                      backing:NSBackingStoreBuffered
                                                        defer:NO];
        [g_debugWindow setTitle:@"FBNeo Debug"];
        [g_debugWindow setReleasedWhenClosed:NO];
        
        // Create a container view
        NSView* contentView = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 300, 160)];
        
        // Create debug text field
        g_debugTextField = [[NSTextField alloc] initWithFrame:NSMakeRect(10, 50, 280, 100)];
        [g_debugTextField setEditable:NO];
        [g_debugTextField setBordered:NO];
        [g_debugTextField setDrawsBackground:NO];
        [g_debugTextField setFont:[NSFont fontWithName:@"Menlo" size:11.0]];
        [g_debugTextField setStringValue:@"FBNeo Debug Overlay\nFrame: 0"];
        [contentView addSubview:g_debugTextField];
        
        // Create save state info field
        g_stateInfoField = [[NSTextField alloc] initWithFrame:NSMakeRect(10, 10, 280, 30)];
        [g_stateInfoField setEditable:NO];
        [g_stateInfoField setBordered:NO];
        [g_stateInfoField setDrawsBackground:NO];
        [g_stateInfoField setFont:[NSFont fontWithName:@"Menlo" size:10.0]];
        [g_stateInfoField setStringValue:@"Save State: Slot 1 (No saves)"];
        [contentView addSubview:g_stateInfoField];
        
        [g_debugWindow setContentView:contentView];
        
        // Position the window relative to the parent
        if (parentWindow) {
            NSRect parentFrame = [parentWindow frame];
            NSRect debugFrame = [g_debugWindow frame];
            
            [g_debugWindow setFrameOrigin:NSMakePoint(
                parentFrame.origin.x + parentFrame.size.width + 10,
                parentFrame.origin.y + parentFrame.size.height - debugFrame.size.height
            )];
        }
    }
    
    g_debugOverlayEnabled = false;
    return 0;
}

// Exit debug overlay
INT32 Metal_ExitDebugOverlay() {
    printf("[Metal_ExitDebugOverlay] Exiting debug overlay\n");
    
    if (g_debugWindow) {
        [g_debugWindow close];
        g_debugWindow = nil;
        g_debugTextField = nil;
        g_stateInfoField = nil;
    }
    
    g_debugOverlayVisible = false;
    g_debugOverlayEnabled = false;
    g_parentWindow = nil;
    
    return 0;
}

// Toggle debug overlay visibility
void Metal_ToggleDebugOverlay() {
    g_debugOverlayEnabled = !g_debugOverlayEnabled;
    g_debugOverlayVisible = g_debugOverlayEnabled;
    
    if (g_debugOverlayVisible) {
        [g_debugWindow makeKeyAndOrderFront:nil];
        printf("[Metal_ToggleDebugOverlay] Debug overlay shown\n");
    } else {
        [g_debugWindow orderOut:nil];
        printf("[Metal_ToggleDebugOverlay] Debug overlay hidden\n");
    }
}

// Update debug overlay
void Metal_UpdateDebugOverlay(int frameCount) {
    if (!g_debugOverlayEnabled || !g_debugOverlayVisible || !g_debugTextField) {
        return;
    }
    
    static int lastUpdated = 0;
    
    // Only update text every 30 frames to reduce overhead
    if (frameCount - lastUpdated < 30) {
        return;
    }
    
    lastUpdated = frameCount;
    
    // Update debug text
    NSString* debugText = [NSString stringWithFormat:@"FBNeo Debug Overlay\nFrame: %d\nFPS: %.1f", 
                          frameCount, 60.0]; // Assuming 60fps
    
    // Update save state info - with safety checks
    int currentSlot = 1;
    const char* stateStatus = "Unknown";
    
    @try {
        currentSlot = Metal_GetCurrentSaveSlot();
        stateStatus = Metal_GetSaveStateStatus();
    } @catch (NSException *exception) {
        NSLog(@"Error getting save state info: %@", exception);
        stateStatus = "Error";
    }
    
    NSString* stateText = [NSString stringWithFormat:@"Save State: Slot %d (%s)", 
                          currentSlot, stateStatus];
    
    // Update UI on main thread
    dispatch_async(dispatch_get_main_queue(), ^{
        if (g_debugTextField) {  // Double-check the field still exists
            [g_debugTextField setStringValue:debugText];
        }
        if (g_stateInfoField) {  // Double-check the field still exists
            [g_stateInfoField setStringValue:stateText];
        }
    });
}

// Debug log functions
void Debug_Log(int category, const char* message) {
    printf("[Debug] %s\n", message);
}

// Debug sections and categories (to keep compatibility with the original API)
void Debug_SetCategory(int category) {
    // Stub implementation
}

void Debug_PrintSectionHeader(int category, const char* header) {
    // Stub implementation
}

// Debug categories
#define DEBUG_RENDERER 1
#define DEBUG_METAL 2
#define DEBUG_AUDIO_LOOP 3
#define DEBUG_INPUT_LOOP 4

// Show a temporary status message
void Metal_ShowStatusMessage(const char* message, int seconds) {
    if (!g_debugOverlayEnabled || !g_debugWindow) {
        return;
    }
    
    // Create the status field if needed
    if (!g_statusMessageField) {
        dispatch_async(dispatch_get_main_queue(), ^{
            // Create a floating status message field
            NSRect contentRect = [[g_debugWindow contentView] bounds];
            NSRect statusFrame = NSMakeRect(10, contentRect.size.height - 40, contentRect.size.width - 20, 30);
            
            g_statusMessageField = [[NSTextField alloc] initWithFrame:statusFrame];
            [g_statusMessageField setEditable:NO];
            [g_statusMessageField setBordered:YES];
            [g_statusMessageField setDrawsBackground:YES];
            [g_statusMessageField setBackgroundColor:[NSColor colorWithCalibratedRed:0.0 green:0.0 blue:0.0 alpha:0.7]];
            [g_statusMessageField setTextColor:[NSColor greenColor]];
            [g_statusMessageField setFont:[NSFont boldSystemFontOfSize:14.0]];
            [g_statusMessageField setStringValue:@""];
            [g_statusMessageField setAlignment:NSTextAlignmentCenter];
            [g_statusMessageField setHidden:YES];
            
            [[g_debugWindow contentView] addSubview:g_statusMessageField];
        });
    }
    
    // Cancel existing timer
    if (g_statusTimer) {
        [g_statusTimer invalidate];
        g_statusTimer = nil;
    }
    
    dispatch_async(dispatch_get_main_queue(), ^{
        if (g_statusMessageField) {
            [g_statusMessageField setStringValue:[NSString stringWithUTF8String:message]];
            [g_statusMessageField setHidden:NO];
            
            // Set timer to hide the message
            g_statusTimer = [NSTimer scheduledTimerWithTimeInterval:seconds
                                                            target:[NSBlockOperation blockOperationWithBlock:^{
                [g_statusMessageField setHidden:YES];
                g_statusTimer = nil;
            }]
                                                          selector:@selector(main)
                                                          userInfo:nil
                                                           repeats:NO];
        }
    });
}

// Show save state status
void Metal_ShowSaveStateStatus(bool isSave) {
    // Get current slot and status
    int slot = Metal_GetCurrentSaveSlot();
    const char* status = Metal_GetSaveStateStatus();
    
    char message[256];
    if (isSave) {
        snprintf(message, sizeof(message), "State saved to slot %d", slot);
    } else {
        snprintf(message, sizeof(message), "State loaded from slot %d", slot);
    }
    
    Metal_ShowStatusMessage(message, 3); // Show for 3 seconds
} 