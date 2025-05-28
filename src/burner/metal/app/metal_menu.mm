#import "metal_menu.h"
#import "metal_app.h"

// Private interface for internal properties
@interface FBNeoMenuManager () {
    FBNeoMetalDelegate *_appDelegate;
}
@end

@implementation FBNeoMenuManager

- (instancetype)initWithAppDelegate:(FBNeoMetalDelegate *)delegate {
    self = [super init];
    if (self) {
        _appDelegate = delegate;
        
        // Initialize settings with defaults
        FBNeoSettings settings = {
            // Video defaults
            .fullscreen = NO,
            .scalingMode = 0,     // Normal scaling
            .aspectRatio = 0,     // Original aspect ratio
            .scanlines = NO,
            .smoothing = NO,
            .frameSkip = 0,       // No frame skipping
            .vsync = YES,
            .showFPS = NO,
            
            // Audio defaults
            .audioEnabled = YES,
            .volume = 80,         // 80% volume
            .sampleRate = 44100,  // CD quality
            
            // Input defaults
            .autoFire = NO,
            .autoFireRate = 5,    // 5 frames between autofire
            
            // Debug defaults
            .hitboxViewer = NO,
            .frameCounter = NO,
            .inputDisplay = NO,
            
            // AI defaults
            .aiEnabled = NO,
            .aiControlledPlayer = 0,  // None
            .aiDifficulty = 5,        // Medium difficulty
            .aiTrainingMode = NO,
            .aiDebugOverlay = NO,
            
            // Display mode default
            .displayMode = 0      // Normal display
        };
        
        _settings = settings;
        
        // Initialize recent ROMs list
        _recentRoms = [NSMutableArray array];
        
        // Load settings and recent ROMs list
        [self loadSettings];
        [self loadRecentRomsList];
    }
    return self;
}

- (void)createMainMenu {
    // Create the main menu if it doesn't exist
    NSMenu *mainMenu = [NSApp mainMenu];
    if (!mainMenu) {
        mainMenu = [[NSMenu alloc] init];
        [NSApp setMainMenu:mainMenu];
    }
    
    // Create application menu
    NSMenuItem *appMenuItem = [[NSMenuItem alloc] init];
    NSMenu *appMenu = [[NSMenu alloc] init];
    [appMenu addItemWithTitle:@"About FBNeo Metal" 
                       action:@selector(orderFrontStandardAboutPanel:) 
                keyEquivalent:@""];
    [appMenu addItem:[NSMenuItem separatorItem]];
    [appMenu addItemWithTitle:@"Quit" 
                       action:@selector(terminate:) 
                keyEquivalent:@"q"];
    [appMenuItem setSubmenu:appMenu];
    
    // Create game menu
    self.gameMenuItem = [[NSMenuItem alloc] init];
    self.gameMenu = [[NSMenu alloc] initWithTitle:@"Game"];
    
    [self.gameMenu addItemWithTitle:@"Load ROM..." 
                             action:@selector(loadRom:) 
                      keyEquivalent:@"o"];
    
    self.recentRomsMenuItem = [[NSMenuItem alloc] initWithTitle:@"Recent ROMs" 
                                                         action:nil 
                                                  keyEquivalent:@""];
    self.recentRomsMenu = [[NSMenu alloc] init];
    [self.recentRomsMenuItem setSubmenu:self.recentRomsMenu];
    [self.gameMenu addItem:self.recentRomsMenuItem];
    
    // Update recent ROMs menu
    [self updateRecentRomsMenu];
    
    // Finish setting up main menu
    [self.gameMenuItem setSubmenu:self.gameMenu];
    
    // Add to main menu
    [mainMenu addItem:appMenuItem];
    [mainMenu addItem:self.gameMenuItem];
    
    NSLog(@"Created main menu");
}

- (void)updateRecentRomsMenu {
    // Clear existing items
    [self.recentRomsMenu removeAllItems];
    
    // Add recent ROMs
    if ([self.recentRoms count] > 0) {
        for (NSString *romPath in self.recentRoms) {
            NSString *displayName = [romPath lastPathComponent];
            NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:displayName
                                                          action:@selector(openRecentRom:)
                                                   keyEquivalent:@""];
            [item setRepresentedObject:romPath];
            [self.recentRomsMenu addItem:item];
        }
        
        [self.recentRomsMenu addItem:[NSMenuItem separatorItem]];
        [self.recentRomsMenu addItemWithTitle:@"Clear Recent ROMs"
                                       action:@selector(clearRecentRoms:)
                                keyEquivalent:@""];
    } else {
        NSMenuItem *noRecentItem = [[NSMenuItem alloc] initWithTitle:@"No Recent ROMs"
                                                              action:nil
                                                       keyEquivalent:@""];
        [noRecentItem setEnabled:NO];
        [self.recentRomsMenu addItem:noRecentItem];
    }
}

- (void)addRecentRom:(NSString *)romPath {
    // Remove the path if it already exists in the list
    [self.recentRoms removeObject:romPath];
    
    // Add to the beginning of the list
    [self.recentRoms insertObject:romPath atIndex:0];
    
    // Limit to 10 recent ROMs
    while ([self.recentRoms count] > 10) {
        [self.recentRoms removeLastObject];
    }
    
    // Update the menu
    [self updateRecentRomsMenu];
    
    // Save the list
    [self saveRecentRomsList];
}

- (void)saveRecentRomsList {
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    [defaults setObject:self.recentRoms forKey:@"FBNeoRecentROMs"];
    [defaults synchronize];
}

- (void)loadRecentRomsList {
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    NSArray *savedList = [defaults arrayForKey:@"FBNeoRecentROMs"];
    
    [self.recentRoms removeAllObjects];
    
    if (savedList) {
        [self.recentRoms addObjectsFromArray:savedList];
    }
}

- (void)saveSettings {
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    
    // Video settings
    [defaults setBool:self.settings.fullscreen forKey:@"FBNeoFullscreen"];
    [defaults setInteger:self.settings.scalingMode forKey:@"FBNeoScalingMode"];
    [defaults setInteger:self.settings.aspectRatio forKey:@"FBNeoAspectRatio"];
    [defaults setBool:self.settings.scanlines forKey:@"FBNeoScanlines"];
    [defaults setBool:self.settings.smoothing forKey:@"FBNeoSmoothing"];
    [defaults setInteger:self.settings.frameSkip forKey:@"FBNeoFrameSkip"];
    [defaults setBool:self.settings.vsync forKey:@"FBNeoVSync"];
    [defaults setBool:self.settings.showFPS forKey:@"FBNeoShowFPS"];
    
    // Audio settings
    [defaults setBool:self.settings.audioEnabled forKey:@"FBNeoAudioEnabled"];
    [defaults setInteger:self.settings.volume forKey:@"FBNeoVolume"];
    [defaults setInteger:self.settings.sampleRate forKey:@"FBNeoSampleRate"];
    
    // Input settings
    [defaults setBool:self.settings.autoFire forKey:@"FBNeoAutoFire"];
    [defaults setInteger:self.settings.autoFireRate forKey:@"FBNeoAutoFireRate"];
    
    // Debug settings
    [defaults setBool:self.settings.hitboxViewer forKey:@"FBNeoHitboxViewer"];
    [defaults setBool:self.settings.frameCounter forKey:@"FBNeoFrameCounter"];
    [defaults setBool:self.settings.inputDisplay forKey:@"FBNeoInputDisplay"];
    
    // AI settings
    [defaults setBool:self.settings.aiEnabled forKey:@"FBNeoAIEnabled"];
    [defaults setInteger:self.settings.aiControlledPlayer forKey:@"FBNeoAIControlledPlayer"];
    [defaults setInteger:self.settings.aiDifficulty forKey:@"FBNeoAIDifficulty"];
    [defaults setBool:self.settings.aiTrainingMode forKey:@"FBNeoAITrainingMode"];
    [defaults setBool:self.settings.aiDebugOverlay forKey:@"FBNeoAIDebugOverlay"];
    
    // Display mode
    [defaults setInteger:self.settings.displayMode forKey:@"FBNeoDisplayMode"];
    
    [defaults synchronize];
}

- (void)loadSettings {
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    
    // Only update settings if there are saved values
    if ([defaults objectForKey:@"FBNeoScalingMode"]) {
        // Copy the current values to ensure we're not missing any
        FBNeoSettings settings = self.settings;
        
        // Video settings
        if ([defaults objectForKey:@"FBNeoFullscreen"]) {
            settings.fullscreen = [defaults boolForKey:@"FBNeoFullscreen"];
        }
        if ([defaults objectForKey:@"FBNeoScalingMode"]) {
            settings.scalingMode = [defaults integerForKey:@"FBNeoScalingMode"];
        }
        if ([defaults objectForKey:@"FBNeoAspectRatio"]) {
            settings.aspectRatio = [defaults integerForKey:@"FBNeoAspectRatio"];
        }
        if ([defaults objectForKey:@"FBNeoScanlines"]) {
            settings.scanlines = [defaults boolForKey:@"FBNeoScanlines"];
        }
        if ([defaults objectForKey:@"FBNeoSmoothing"]) {
            settings.smoothing = [defaults boolForKey:@"FBNeoSmoothing"];
        }
        if ([defaults objectForKey:@"FBNeoFrameSkip"]) {
            settings.frameSkip = [defaults integerForKey:@"FBNeoFrameSkip"];
        }
        if ([defaults objectForKey:@"FBNeoVSync"]) {
            settings.vsync = [defaults boolForKey:@"FBNeoVSync"];
        }
        if ([defaults objectForKey:@"FBNeoShowFPS"]) {
            settings.showFPS = [defaults boolForKey:@"FBNeoShowFPS"];
        }
        
        // Audio settings
        if ([defaults objectForKey:@"FBNeoAudioEnabled"]) {
            settings.audioEnabled = [defaults boolForKey:@"FBNeoAudioEnabled"];
        }
        if ([defaults objectForKey:@"FBNeoVolume"]) {
            settings.volume = [defaults integerForKey:@"FBNeoVolume"];
        }
        if ([defaults objectForKey:@"FBNeoSampleRate"]) {
            settings.sampleRate = [defaults integerForKey:@"FBNeoSampleRate"];
        }
        
        // Input settings
        if ([defaults objectForKey:@"FBNeoAutoFire"]) {
            settings.autoFire = [defaults boolForKey:@"FBNeoAutoFire"];
        }
        if ([defaults objectForKey:@"FBNeoAutoFireRate"]) {
            settings.autoFireRate = [defaults integerForKey:@"FBNeoAutoFireRate"];
        }
        
        // Debug settings
        if ([defaults objectForKey:@"FBNeoHitboxViewer"]) {
            settings.hitboxViewer = [defaults boolForKey:@"FBNeoHitboxViewer"];
        }
        if ([defaults objectForKey:@"FBNeoFrameCounter"]) {
            settings.frameCounter = [defaults boolForKey:@"FBNeoFrameCounter"];
        }
        if ([defaults objectForKey:@"FBNeoInputDisplay"]) {
            settings.inputDisplay = [defaults boolForKey:@"FBNeoInputDisplay"];
        }
        
        // AI settings
        if ([defaults objectForKey:@"FBNeoAIEnabled"]) {
            settings.aiEnabled = [defaults boolForKey:@"FBNeoAIEnabled"];
        }
        if ([defaults objectForKey:@"FBNeoAIControlledPlayer"]) {
            settings.aiControlledPlayer = [defaults integerForKey:@"FBNeoAIControlledPlayer"];
        }
        if ([defaults objectForKey:@"FBNeoAIDifficulty"]) {
            settings.aiDifficulty = [defaults integerForKey:@"FBNeoAIDifficulty"];
        }
        if ([defaults objectForKey:@"FBNeoAITrainingMode"]) {
            settings.aiTrainingMode = [defaults boolForKey:@"FBNeoAITrainingMode"];
        }
        if ([defaults objectForKey:@"FBNeoAIDebugOverlay"]) {
            settings.aiDebugOverlay = [defaults boolForKey:@"FBNeoAIDebugOverlay"];
        }
        
        // Display mode
        if ([defaults objectForKey:@"FBNeoDisplayMode"]) {
            settings.displayMode = [defaults integerForKey:@"FBNeoDisplayMode"];
        }
        
        // Assign the updated settings
        self.settings = settings;
    }
}

- (void)updateSettingsMenuItems {
    // This is a stub implementation - in a real implementation, we would update all menu items
    // to reflect the current settings values
    NSLog(@"Updating settings menu items with current settings");
    
    // Since we can't directly access the parent class's implementation, we need our own implementation
    NSMenu *mainMenu = [NSApp mainMenu];
    if (!mainMenu) return;
    
    // Log current settings for debugging
    NSLog(@"Current settings:");
    NSLog(@"Display mode: %d", self.settings.displayMode);
    NSLog(@"Scaling mode: %d", self.settings.scalingMode);
    NSLog(@"Aspect ratio: %d", self.settings.aspectRatio);
    NSLog(@"Scanlines: %@", self.settings.scanlines ? @"ON" : @"OFF");
    NSLog(@"Smoothing: %@", self.settings.smoothing ? @"ON" : @"OFF");
    NSLog(@"Fullscreen: %@", self.settings.fullscreen ? @"ON" : @"OFF");
    NSLog(@"Audio enabled: %@", self.settings.audioEnabled ? @"ON" : @"OFF");
    NSLog(@"Volume: %d", self.settings.volume);
    
    // In a real implementation, we would iterate through menu items and update their states
    // to reflect the current settings
}

// Basic setting accessors
- (void)setDisplayMode:(int)mode {
    FBNeoSettings tempSettings = self.settings;
    tempSettings.displayMode = mode;
    self.settings = tempSettings;
    [self updateSettingsMenuItems];
    
    // TODO: Implement display mode setting
    NSLog(@"Display mode set to: %d", mode);
}

- (void)setVSync:(BOOL)enabled {
    FBNeoSettings tempSettings = self.settings;
    tempSettings.vsync = enabled;
    self.settings = tempSettings;
    [self updateSettingsMenuItems];
}

- (void)setScanlineIntensity:(int)intensity {
    // For backward compatibility, we'll now just turn scanlines on/off
    // based on whether intensity is > 0
    FBNeoSettings tempSettings = self.settings;
    tempSettings.scanlines = (intensity > 0);
    self.settings = tempSettings;
    [self updateSettingsMenuItems];
}

- (void)setControllerType:(int)type {
    // For backward compatibility, just log this call
    NSLog(@"Controller type would be set to %d (compatibility mode)", type);
    [self updateSettingsMenuItems];
}

- (void)setFullscreen:(BOOL)enabled {
    FBNeoSettings tempSettings = self.settings;
    tempSettings.fullscreen = enabled;
    self.settings = tempSettings;
    [self updateSettingsMenuItems];
    
    // Call the fullscreen toggle function
    extern int VidMetalSetFullscreen(bool bFullscreen);
    VidMetalSetFullscreen(enabled);
    
    NSLog(@"Fullscreen setting changed to: %@", enabled ? @"YES" : @"NO");
}

- (void)setFrameSkip:(int)frames {
    FBNeoSettings tempSettings = self.settings;
    tempSettings.frameSkip = frames;
    self.settings = tempSettings;
    [self updateSettingsMenuItems];
    
    // TODO: Implement frame skip
    NSLog(@"Frame skip set to: %d", frames);
}

- (void)setVsync:(BOOL)enabled {
    FBNeoSettings tempSettings = self.settings;
    tempSettings.vsync = enabled;
    self.settings = tempSettings;
    [self updateSettingsMenuItems];
    
    // TODO: Implement vsync toggle
    NSLog(@"VSync setting changed to: %@", enabled ? @"YES" : @"NO");
}

- (void)setAudioEnabled:(BOOL)enabled {
    FBNeoSettings tempSettings = self.settings;
    tempSettings.audioEnabled = enabled;
    self.settings = tempSettings;
    [self updateSettingsMenuItems];
    
    // TODO: Implement audio toggle
    NSLog(@"Audio setting changed to: %@", enabled ? @"YES" : @"NO");
}

- (void)setVolume:(int)volume {
    FBNeoSettings tempSettings = self.settings;
    tempSettings.volume = volume;
    self.settings = tempSettings;
    [self updateSettingsMenuItems];
    
    // TODO: Implement volume control
    NSLog(@"Volume set to: %d", volume);
}

- (void)setSampleRate:(int)rate {
    FBNeoSettings tempSettings = self.settings;
    tempSettings.sampleRate = rate;
    self.settings = tempSettings;
    [self updateSettingsMenuItems];
    
    // TODO: Implement sample rate setting
    NSLog(@"Sample rate set to: %d", rate);
}

- (void)setHitboxViewer:(BOOL)enabled {
    FBNeoSettings tempSettings = self.settings;
    tempSettings.hitboxViewer = enabled;
    self.settings = tempSettings;
    [self updateSettingsMenuItems];
    
    // TODO: Implement hitbox viewer toggle
    NSLog(@"Hitbox viewer setting changed to: %@", enabled ? @"YES" : @"NO");
}

- (void)setFrameCounter:(BOOL)enabled {
    FBNeoSettings tempSettings = self.settings;
    tempSettings.frameCounter = enabled;
    self.settings = tempSettings;
    [self updateSettingsMenuItems];
    
    // TODO: Implement frame counter toggle
    NSLog(@"Frame counter setting changed to: %@", enabled ? @"YES" : @"NO");
}

- (void)setInputDisplay:(BOOL)enabled {
    FBNeoSettings tempSettings = self.settings;
    tempSettings.inputDisplay = enabled;
    self.settings = tempSettings;
    [self updateSettingsMenuItems];
    
    // TODO: Implement input display toggle
    NSLog(@"Input display setting changed to: %@", enabled ? @"YES" : @"NO");
}

- (void)setAIEnabled:(BOOL)enabled {
    FBNeoSettings tempSettings = self.settings;
    tempSettings.aiEnabled = enabled;
    self.settings = tempSettings;
    [self updateSettingsMenuItems];
    
    // TODO: Implement AI toggle
    NSLog(@"AI setting changed to: %@", enabled ? @"YES" : @"NO");
}

- (void)setAIControlledPlayer:(int)player {
    FBNeoSettings tempSettings = self.settings;
    tempSettings.aiControlledPlayer = player;
    self.settings = tempSettings;
    [self updateSettingsMenuItems];
    
    // TODO: Implement AI player control setting
    NSLog(@"AI controlled player set to: %d", player);
}

- (void)setAIDifficulty:(int)difficulty {
    FBNeoSettings tempSettings = self.settings;
    tempSettings.aiDifficulty = difficulty;
    self.settings = tempSettings;
    [self updateSettingsMenuItems];
    
    // TODO: Implement AI difficulty setting
    NSLog(@"AI difficulty set to: %d", difficulty);
}

- (void)setAITrainingMode:(BOOL)enabled {
    FBNeoSettings tempSettings = self.settings;
    tempSettings.aiTrainingMode = enabled;
    self.settings = tempSettings;
    [self updateSettingsMenuItems];
    
    // TODO: Implement AI training mode toggle
    NSLog(@"AI training mode setting changed to: %@", enabled ? @"YES" : @"NO");
}

- (void)setAIDebugOverlay:(BOOL)enabled {
    FBNeoSettings tempSettings = self.settings;
    tempSettings.aiDebugOverlay = enabled;
    self.settings = tempSettings;
    [self updateSettingsMenuItems];
    
    // TODO: Implement AI debug overlay toggle
    NSLog(@"AI debug overlay setting changed to: %@", enabled ? @"YES" : @"NO");
}

@end 