#pragma once

#import <Cocoa/Cocoa.h>

// Forward declarations
@class FBNeoMetalDelegate;

// Import the settings struct and FBNeoMenuManager interface from metal_app.h
#import "metal_app.h"

// We already have FBNeoMenuManager defined in metal_app.h, 
// so here we just add method declarations that should be implemented

// Methods to be added to FBNeoMenuManager
@interface FBNeoMenuManager (MenuExtensions)

// Settings management
- (void)setScalingMode:(int)mode;
- (void)setAspectRatio:(int)ratio;
- (void)setScanlines:(BOOL)enabled;
- (void)setSmoothing:(BOOL)enabled;
- (void)setFullscreen:(BOOL)enabled;
- (void)setFrameSkip:(int)frames;
- (void)setVsync:(BOOL)enabled;
- (void)setAudioEnabled:(BOOL)enabled;
- (void)setVolume:(int)volume;
- (void)setSampleRate:(int)rate;
- (void)setHitboxViewer:(BOOL)enabled;
- (void)setFrameCounter:(BOOL)enabled;
- (void)setInputDisplay:(BOOL)enabled;
- (void)setAIEnabled:(BOOL)enabled;
- (void)setAIControlledPlayer:(int)player;
- (void)setAIDifficulty:(int)difficulty;
- (void)setAITrainingMode:(BOOL)enabled;
- (void)setAIDebugOverlay:(BOOL)enabled;
- (void)setDisplayMode:(int)mode;

// Update menu items to reflect current settings
- (void)updateSettingsMenuItems;

@end 