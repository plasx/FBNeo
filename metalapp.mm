#import <Cocoa/Cocoa.h>
#import <MetalKit/MetalKit.h>
#import <objc/runtime.h>

// Simple settings structure
typedef struct {
    BOOL fullscreen;
    int scaling;  // 0 = nearest, 1 = linear, 2 = CRT
    float scanlines;
    BOOL showFPS;
    
    // AI settings
    BOOL aiEnabled;
    int aiDifficulty;
    int aiControlledPlayer;
    BOOL aiTrainingMode;
    BOOL aiDebugOverlay;
} FBNeoSettings;

// FBNeo Metal application delegate
@interface FBNeoMetalApp : NSObject <NSApplicationDelegate, MTKViewDelegate, NSWindowDelegate>
@property (strong) NSWindow *window;
@property (strong) MTKView *metalView;
@property (strong) id<MTLDevice> device;
@property (strong) id<MTLCommandQueue> commandQueue;
@property (strong) id<MTLRenderPipelineState> pipelineState;
@property (strong) id<MTLTexture> frameTexture;
@property (strong) id<MTLBuffer> vertexBuffer;
@property FBNeoSettings settings;
@property (assign) BOOL isPaused;
@property (assign) BOOL isRunning;
@property (assign) NSTimeInterval lastFrameTime;
@property (assign) float frameRate;
@property (assign) int frameCount;
@property (strong) NSString *currentModelPath;
@property (assign) BOOL gameLoaded;
@property (strong) NSString *currentROMPath;

// Settings accessors
- (void)setFullscreen:(BOOL)fullscreen;
- (void)setScaling:(int)mode;
- (void)setShowFPS:(BOOL)show;

// AI settings accessors
- (void)setAIEnabled:(BOOL)enabled;
- (void)setAIDifficulty:(int)level;
- (void)setAIControlledPlayer:(int)player;
- (void)setAITrainingMode:(BOOL)enabled;
- (void)setAIDebugOverlay:(BOOL)enabled;
- (void)loadAIModel:(NSString *)path;

- (void)showRomBrowserDialog;

// ROM loading and game management
- (BOOL)loadROMFile:(NSString *)path;
- (void)runLoadedGame;
- (void)resetLoadedGame;
@end

@implementation FBNeoMetalApp

- (instancetype)init {
    self = [super init];
    if (self) {
        // Default settings
        _settings.fullscreen = NO;
        _settings.scaling = 0;
        _settings.scanlines = 0.0f;
        _settings.showFPS = YES;
        _isPaused = NO;
        _isRunning = YES;
        _frameCount = 0;
        _frameRate = 0;
        _lastFrameTime = 0;
        
        // AI defaults
        _settings.aiEnabled = NO;
        _settings.aiDifficulty = 5;
        _settings.aiControlledPlayer = 0;
        _settings.aiTrainingMode = NO;
        _settings.aiDebugOverlay = NO;
        _currentModelPath = nil;
        _gameLoaded = NO;
        _currentROMPath = nil;
    }
    return self;
}

- (void)setFullscreen:(BOOL)fullscreen {
    _settings.fullscreen = fullscreen;
}

- (void)setScaling:(int)mode {
    _settings.scaling = mode;
}

- (void)setShowFPS:(BOOL)show {
    _settings.showFPS = show;
}

- (void)setAIEnabled:(BOOL)enabled {
    _settings.aiEnabled = enabled;
    NSLog(@"AI %@", enabled ? @"enabled" : @"disabled");
}

- (void)setAIDifficulty:(int)level {
    _settings.aiDifficulty = level;
    NSLog(@"AI difficulty set to %d", level);
}

- (void)setAIControlledPlayer:(int)player {
    _settings.aiControlledPlayer = player;
    NSLog(@"AI controlling %@", player == 0 ? @"none" : (player == 1 ? @"Player 1" : (player == 2 ? @"Player 2" : @"Both Players")));
}

- (void)setAITrainingMode:(BOOL)enabled {
    _settings.aiTrainingMode = enabled;
    NSLog(@"Training mode %@", enabled ? @"enabled" : @"disabled");
}

- (void)setAIDebugOverlay:(BOOL)enabled {
    _settings.aiDebugOverlay = enabled;
    NSLog(@"Debug overlay %@", enabled ? @"enabled" : @"disabled");
}

- (void)loadAIModel:(NSString *)path {
    self.currentModelPath = path;
    NSLog(@"Loading AI model: %@", path);
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    // Create Metal device
    self.device = MTLCreateSystemDefaultDevice();
    if (!self.device) {
        NSLog(@"Metal is not supported on this device");
        [NSApp terminate:nil];
        return;
    }
    
    // Create command queue
    self.commandQueue = [self.device newCommandQueue];
    
    // Create window
    NSRect frame = NSMakeRect(0, 0, 800, 600);
    NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable;
    self.window = [[NSWindow alloc] initWithContentRect:frame styleMask:style backing:NSBackingStoreBuffered defer:NO];
    [self.window setTitle:@"FBNeo - Metal Implementation"];
    [self.window center];
    [self.window setDelegate:self];
    
    // Create Metal view
    self.metalView = [[MTKView alloc] initWithFrame:frame device:self.device];
    self.metalView.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
    self.metalView.delegate = self;
    self.metalView.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    self.metalView.depthStencilPixelFormat = MTLPixelFormatInvalid;
    self.metalView.sampleCount = 1;
    self.metalView.preferredFramesPerSecond = 60;
    
    // Set as content view
    self.window.contentView = self.metalView;
    
    // Create render pipeline
    [self setupRenderPipeline];
    
    // Create frame texture
    [self createFrameTexture];
    
    // Create vertex buffer
    [self createVertexBuffer];
    
    // Create menus
    [self setupMenus];
    
    // Show window
    [self.window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
}

- (void)setupRenderPipeline {
    // Create simple embedded shader code
    NSString *shaderSource = @"#include <metal_stdlib>\n\
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
        out.position = float4(vertices[vertexID].xy, 0.0, 1.0);\n\
        out.texCoord = vertices[vertexID].zw;\n\
        return out;\n\
    }\n\
    \n\
    fragment float4 fragmentShader(VertexOut in [[stage_in]],\n\
                                  texture2d<float> tex [[texture(0)]],\n\
                                  sampler samplr [[sampler(0)]]) {\n\
        return tex.sample(samplr, in.texCoord);\n\
    }\n";
    
    // Compile shader
    NSError *error = nil;
    id<MTLLibrary> library = [self.device newLibraryWithSource:shaderSource options:nil error:&error];
    if (!library) {
        NSLog(@"Failed to compile library: %@", error);
        return;
    }
    
    // Get functions
    id<MTLFunction> vertexFunc = [library newFunctionWithName:@"vertexShader"];
    id<MTLFunction> fragmentFunc = [library newFunctionWithName:@"fragmentShader"];
    
    // Create pipeline descriptor
    MTLRenderPipelineDescriptor *pipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDesc.label = @"FBNeo Pipeline";
    pipelineDesc.vertexFunction = vertexFunc;
    pipelineDesc.fragmentFunction = fragmentFunc;
    pipelineDesc.colorAttachments[0].pixelFormat = self.metalView.colorPixelFormat;
    
    // Create pipeline state
    self.pipelineState = [self.device newRenderPipelineStateWithDescriptor:pipelineDesc error:&error];
    if (!self.pipelineState) {
        NSLog(@"Failed to create pipeline state: %@", error);
    }
}

- (void)createFrameTexture {
    // Create a test texture (just a colorful pattern)
    const int width = 320;
    const int height = 240;
    
    // Create descriptor
    MTLTextureDescriptor *desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                                                    width:width
                                                                                   height:height
                                                                                mipmapped:NO];
    desc.usage = MTLTextureUsageShaderRead;
    
    // Create texture
    self.frameTexture = [self.device newTextureWithDescriptor:desc];
    
    // Create a test pattern
    uint32_t *pixels = (uint32_t *)malloc(width * height * sizeof(uint32_t));
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint8_t r = (x * 256) / width;
            uint8_t g = (y * 256) / height;
            uint8_t b = ((x+y) * 128) / (width+height);
            pixels[y * width + x] = (255 << 24) | (r << 16) | (g << 8) | b; // RGBA
        }
    }
    
    // Upload to texture
    [self.frameTexture replaceRegion:MTLRegionMake2D(0, 0, width, height) 
                         mipmapLevel:0 
                           withBytes:pixels 
                         bytesPerRow:width * sizeof(uint32_t)];
    
    free(pixels);
}

- (void)createVertexBuffer {
    // Create a fullscreen quad
    // Positions (xy) and texture coordinates (zw)
    float vertices[] = {
        -1.0f,  1.0f, 0.0f, 0.0f, // Top left
        -1.0f, -1.0f, 0.0f, 1.0f, // Bottom left
         1.0f, -1.0f, 1.0f, 1.0f, // Bottom right
         
        -1.0f,  1.0f, 0.0f, 0.0f, // Top left
         1.0f, -1.0f, 1.0f, 1.0f, // Bottom right
         1.0f,  1.0f, 1.0f, 0.0f  // Top right
    };
    
    // Create buffer
    self.vertexBuffer = [self.device newBufferWithBytes:vertices 
                                                 length:sizeof(vertices) 
                                                options:MTLResourceStorageModeShared];
}

- (void)setupMenus {
    // Create main menu
    NSMenu *mainMenu = [[NSMenu alloc] init];
    
    // Application menu
    NSMenuItem *appMenuItem = [[NSMenuItem alloc] init];
    NSMenu *appMenu = [[NSMenu alloc] init];
    
    [appMenu addItemWithTitle:@"About FBNeo" action:@selector(showAbout:) keyEquivalent:@""];
    [appMenu addItem:[NSMenuItem separatorItem]];
    [appMenu addItemWithTitle:@"Preferences" action:@selector(showPreferences:) keyEquivalent:@","];
    [appMenu addItem:[NSMenuItem separatorItem]];
    [appMenu addItemWithTitle:@"Quit" action:@selector(terminate:) keyEquivalent:@"q"];
    
    [appMenuItem setSubmenu:appMenu];
    [mainMenu addItem:appMenuItem];
    
    // Game menu
    NSMenuItem *gameMenuItem = [[NSMenuItem alloc] init];
    [gameMenuItem setTitle:@"Game"];
    NSMenu *gameMenu = [[NSMenu alloc] init];
    [gameMenu setTitle:@"Game"];
    
    [gameMenu addItemWithTitle:@"Load ROM..." action:@selector(loadRom:) keyEquivalent:@"o"];
    [gameMenu addItem:[NSMenuItem separatorItem]];
    [gameMenu addItemWithTitle:@"Pause" action:@selector(togglePause:) keyEquivalent:@"p"];
    [gameMenu addItemWithTitle:@"Reset" action:@selector(resetEmulation:) keyEquivalent:@"r"];
    
    [gameMenuItem setSubmenu:gameMenu];
    [mainMenu addItem:gameMenuItem];
    
    // Settings menu
    NSMenuItem *settingsMenuItem = [[NSMenuItem alloc] init];
    [settingsMenuItem setTitle:@"Settings"];
    NSMenu *settingsMenu = [[NSMenu alloc] init];
    [settingsMenu setTitle:@"Settings"];
    
    // Video settings submenu
    NSMenuItem *videoMenuItem = [[NSMenuItem alloc] init];
    [videoMenuItem setTitle:@"Video"];
    NSMenu *videoMenu = [[NSMenu alloc] init];
    
    NSMenuItem *fullscreenItem = [[NSMenuItem alloc] initWithTitle:@"Fullscreen" 
                                                           action:@selector(toggleFullscreen:) 
                                                    keyEquivalent:@"f"];
    [fullscreenItem setState:self.settings.fullscreen ? NSControlStateValueOn : NSControlStateValueOff];
    [videoMenu addItem:fullscreenItem];
    
    // Scaling options
    NSMenuItem *scalingMenuItem = [[NSMenuItem alloc] init];
    [scalingMenuItem setTitle:@"Scaling"];
    NSMenu *scalingMenu = [[NSMenu alloc] init];
    
    NSArray *scalingOptions = @[@"Nearest Neighbor", @"Bilinear", @"CRT Effect"];
    for (int i = 0; i < scalingOptions.count; i++) {
        NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:scalingOptions[i] 
                                                     action:@selector(setScalingOption:) 
                                              keyEquivalent:@""];
        [item setTag:i];
        [item setState:(self.settings.scaling == i) ? NSControlStateValueOn : NSControlStateValueOff];
        [scalingMenu addItem:item];
    }
    
    [scalingMenuItem setSubmenu:scalingMenu];
    [videoMenu addItem:scalingMenuItem];
    
    // Show FPS option
    NSMenuItem *fpsItem = [[NSMenuItem alloc] initWithTitle:@"Show FPS" 
                                                    action:@selector(toggleFPS:) 
                                             keyEquivalent:@""];
    [fpsItem setState:self.settings.showFPS ? NSControlStateValueOn : NSControlStateValueOff];
    [videoMenu addItem:fpsItem];
    
    [videoMenuItem setSubmenu:videoMenu];
    [settingsMenu addItem:videoMenuItem];
    
    // Audio settings submenu
    NSMenuItem *audioMenuItem = [[NSMenuItem alloc] init];
    [audioMenuItem setTitle:@"Audio"];
    NSMenu *audioMenu = [[NSMenu alloc] init];
    
    [audioMenu addItemWithTitle:@"Enable Audio" action:@selector(toggleAudio:) keyEquivalent:@""];
    [audioMenu addItem:[NSMenuItem separatorItem]];
    [audioMenu addItemWithTitle:@"Volume" action:nil keyEquivalent:@""];
    
    [audioMenuItem setSubmenu:audioMenu];
    [settingsMenu addItem:audioMenuItem];
    
    // Input settings submenu
    NSMenuItem *inputMenuItem = [[NSMenuItem alloc] init];
    [inputMenuItem setTitle:@"Input"];
    NSMenu *inputMenu = [[NSMenu alloc] init];
    
    [inputMenu addItemWithTitle:@"Configure Controls..." action:@selector(configureControls:) keyEquivalent:@""];
    
    [inputMenuItem setSubmenu:inputMenu];
    [settingsMenu addItem:inputMenuItem];
    
    // Add settings menu
    [settingsMenuItem setSubmenu:settingsMenu];
    [mainMenu addItem:settingsMenuItem];
    
    // AI menu
    NSMenuItem *aiMenuItem = [[NSMenuItem alloc] init];
    [aiMenuItem setTitle:@"AI"];
    NSMenu *aiMenu = [[NSMenu alloc] init];
    [aiMenu setTitle:@"AI"];
    
    // Enable/disable AI
    NSMenuItem *enableAIItem = [[NSMenuItem alloc] initWithTitle:@"Enable AI" 
                                                         action:@selector(toggleAI:) 
                                                  keyEquivalent:@""];
    [enableAIItem setState:self.settings.aiEnabled ? NSControlStateValueOn : NSControlStateValueOff];
    [aiMenu addItem:enableAIItem];
    
    [aiMenu addItem:[NSMenuItem separatorItem]];
    
    // AI-controlled player submenu
    NSMenuItem *controlledPlayerItem = [[NSMenuItem alloc] initWithTitle:@"AI Controls" 
                                                                  action:nil 
                                                           keyEquivalent:@""];
    NSMenu *controlledPlayerMenu = [[NSMenu alloc] init];
    
    NSArray *playerOptions = @[@"None", @"Player 1", @"Player 2", @"Both Players"];
    for (int i = 0; i < playerOptions.count; i++) {
        NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:playerOptions[i] 
                                                     action:@selector(setAIPlayerControl:) 
                                              keyEquivalent:@""];
        [item setTag:i];
        [item setState:self.settings.aiControlledPlayer == i ? NSControlStateValueOn : NSControlStateValueOff];
        [controlledPlayerMenu addItem:item];
    }
    
    [controlledPlayerItem setSubmenu:controlledPlayerMenu];
    [aiMenu addItem:controlledPlayerItem];
    
    // AI Difficulty submenu
    NSMenuItem *difficultyItem = [[NSMenuItem alloc] initWithTitle:@"Difficulty" 
                                                           action:nil 
                                                    keyEquivalent:@""];
    NSMenu *difficultyMenu = [[NSMenu alloc] init];
    
    for (int i = 0; i <= 10; i++) {
        NSString *label;
        if (i == 0) label = @"Beginner";
        else if (i == 5) label = @"Medium";
        else if (i == 10) label = @"Expert";
        else label = [NSString stringWithFormat:@"Level %d", i];
        
        NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:label 
                                                     action:@selector(setAIDifficultyLevel:) 
                                              keyEquivalent:@""];
        [item setTag:i];
        [item setState:self.settings.aiDifficulty == i ? NSControlStateValueOn : NSControlStateValueOff];
        [difficultyMenu addItem:item];
    }
    
    [difficultyItem setSubmenu:difficultyMenu];
    [aiMenu addItem:difficultyItem];
    
    // AI Models submenu
    NSMenuItem *modelsItem = [[NSMenuItem alloc] initWithTitle:@"AI Models" 
                                                       action:nil 
                                                keyEquivalent:@""];
    NSMenu *modelsMenu = [[NSMenu alloc] init];
    
    [modelsMenu addItemWithTitle:@"Load Model..." action:@selector(loadAIModelFile:) keyEquivalent:@""];
    [modelsMenu addItem:[NSMenuItem separatorItem]];
    [modelsMenu addItemWithTitle:@"Default Model" action:@selector(selectAIModel:) keyEquivalent:@""];
    
    [modelsItem setSubmenu:modelsMenu];
    [aiMenu addItem:modelsItem];
    
    [aiMenu addItem:[NSMenuItem separatorItem]];
    
    // Training mode
    NSMenuItem *trainingModeItem = [[NSMenuItem alloc] initWithTitle:@"Training Mode" 
                                                            action:@selector(toggleTrainingMode:) 
                                                     keyEquivalent:@""];
    [trainingModeItem setState:self.settings.aiTrainingMode ? NSControlStateValueOn : NSControlStateValueOff];
    [aiMenu addItem:trainingModeItem];
    
    // Debug Overlay
    NSMenuItem *debugOverlayItem = [[NSMenuItem alloc] initWithTitle:@"Show Debug Overlay" 
                                                             action:@selector(toggleAIDebugOverlay:) 
                                                      keyEquivalent:@""];
    [debugOverlayItem setState:self.settings.aiDebugOverlay ? NSControlStateValueOn : NSControlStateValueOff];
    [aiMenu addItem:debugOverlayItem];
    
    // Debug tools submenu
    NSMenuItem *debugToolsItem = [[NSMenuItem alloc] initWithTitle:@"Debug Tools" 
                                                          action:nil 
                                                   keyEquivalent:@""];
    NSMenu *debugToolsMenu = [[NSMenu alloc] init];
    
    [debugToolsMenu addItemWithTitle:@"Memory Viewer" action:@selector(showMemoryViewer:) keyEquivalent:@""];
    [debugToolsMenu addItemWithTitle:@"Register Viewer" action:@selector(showRegisterViewer:) keyEquivalent:@""];
    [debugToolsMenu addItemWithTitle:@"Disassembly" action:@selector(showDisassembly:) keyEquivalent:@""];
    [debugToolsMenu addItemWithTitle:@"Breakpoints" action:@selector(showBreakpoints:) keyEquivalent:@""];
    [debugToolsMenu addItemWithTitle:@"Watchpoints" action:@selector(showWatchpoints:) keyEquivalent:@""];
    [debugToolsMenu addItemWithTitle:@"Call Stack" action:@selector(showCallStack:) keyEquivalent:@""];
    
    [debugToolsMenu addItem:[NSMenuItem separatorItem]];
    
    [debugToolsMenu addItemWithTitle:@"Hitbox Viewer" action:@selector(toggleHitboxViewer:) keyEquivalent:@""];
    [debugToolsMenu addItemWithTitle:@"Frame Data" action:@selector(toggleFrameData:) keyEquivalent:@""];
    [debugToolsMenu addItemWithTitle:@"Input Display" action:@selector(toggleInputDisplay:) keyEquivalent:@""];
    [debugToolsMenu addItemWithTitle:@"Game State" action:@selector(toggleGameStateDisplay:) keyEquivalent:@""];
    
    [debugToolsItem setSubmenu:debugToolsMenu];
    [aiMenu addItem:debugToolsItem];
    
    [aiMenuItem setSubmenu:aiMenu];
    [mainMenu addItem:aiMenuItem];
    
    // Tools menu
    NSMenuItem *toolsMenuItem = [[NSMenuItem alloc] init];
    [toolsMenuItem setTitle:@"Tools"];
    NSMenu *toolsMenu = [[NSMenu alloc] init];
    [toolsMenu setTitle:@"Tools"];
    
    [toolsMenu addItemWithTitle:@"Screen Capture" action:@selector(captureScreen:) keyEquivalent:@"s"];
    [toolsMenu addItemWithTitle:@"Record Video..." action:@selector(recordVideo:) keyEquivalent:@""];
    [toolsMenu addItem:[NSMenuItem separatorItem]];
    [toolsMenu addItemWithTitle:@"Cheats..." action:@selector(showCheats:) keyEquivalent:@"c"];
    
    [toolsMenuItem setSubmenu:toolsMenu];
    [mainMenu addItem:toolsMenuItem];
    
    // Help menu
    NSMenuItem *helpMenuItem = [[NSMenuItem alloc] init];
    [helpMenuItem setTitle:@"Help"];
    NSMenu *helpMenu = [[NSMenu alloc] init];
    [helpMenu setTitle:@"Help"];
    
    [helpMenu addItemWithTitle:@"FBNeo Help" action:@selector(showHelp:) keyEquivalent:@"?"];
    [helpMenu addItemWithTitle:@"ROM Compatibility" action:@selector(showCompatibility:) keyEquivalent:@""];
    [helpMenu addItem:[NSMenuItem separatorItem]];
    [helpMenu addItemWithTitle:@"About FBNeo" action:@selector(showAbout:) keyEquivalent:@""];
    
    [helpMenuItem setSubmenu:helpMenu];
    [mainMenu addItem:helpMenuItem];
    
    // Set as application menu
    [NSApp setMainMenu:mainMenu];
}

#pragma mark - Menu Actions

- (void)loadRom:(id)sender {
    NSOpenPanel *openPanel = [NSOpenPanel openPanel];
    [openPanel setTitle:@"Select ROM File"];
    [openPanel setCanChooseFiles:YES];
    [openPanel setCanChooseDirectories:NO];
    [openPanel setAllowsMultipleSelection:NO];
    
    // Set the default directory to the ROMs directory
    NSString *romsPath = @"/Users/plasx/dev/ROMs";
    if ([[NSFileManager defaultManager] fileExistsAtPath:romsPath]) {
        [openPanel setDirectoryURL:[NSURL fileURLWithPath:romsPath]];
    }
    
    // Set allowed file types
    if (@available(macOS 11.0, *)) {
        NSArray *allowedTypes = @[UTTypeZipArchive];
        openPanel.allowedContentTypes = allowedTypes;
    } else {
        // For older macOS versions
        openPanel.allowedFileTypes = @[@"zip"];
    }
    
    [openPanel beginSheetModalForWindow:self.window completionHandler:^(NSInteger result) {
        if (result == NSModalResponseOK) {
            NSURL *selectedFile = openPanel.URLs.firstObject;
            NSString *romPath = selectedFile.path;
            
            // Try to load the ROM directly
            if (![self loadROMFile:romPath]) {
                // If direct load fails, show the ROM browser instead
                [self showRomBrowserDialog];
            }
        }
    }];
}

- (void)togglePause:(id)sender {
    self.isPaused = !self.isPaused;
    
    // Update menu item text
    NSMenuItem *menuItem = (NSMenuItem *)sender;
    [menuItem setTitle:(self.isPaused ? @"Resume" : @"Pause")];
}

- (void)resetEmulation:(id)sender {
    if (!self.gameLoaded) {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"No Game Running"];
        [alert setInformativeText:@"Please start a game before attempting to reset."];
        [alert runModal];
        return;
    }
    
    NSLog(@"Resetting game");
    
    // Reset the loaded game
    [self resetLoadedGame];
}

- (void)toggleFullscreen:(id)sender {
    // Toggle fullscreen state
    [self setFullscreen:!self.settings.fullscreen];
    
    // Update menu item
    NSMenuItem *menuItem = (NSMenuItem *)sender;
    [menuItem setState:self.settings.fullscreen ? NSControlStateValueOn : NSControlStateValueOff];
    
    // Toggle fullscreen mode
    [self.window toggleFullScreen:nil];
}

- (void)setScalingOption:(id)sender {
    if ([sender isKindOfClass:[NSMenuItem class]]) {
        NSMenuItem *menuItem = (NSMenuItem *)sender;
        [self setScaling:(int)menuItem.tag];
        
        // Update menu state
        NSMenu *menu = [menuItem menu];
        for (NSMenuItem *item in menu.itemArray) {
            [item setState:(item.tag == self.settings.scaling) ? NSControlStateValueOn : NSControlStateValueOff];
        }
        
        // Re-create pipeline with appropriate filtering
        [self setupRenderPipeline];
    }
}

- (void)toggleFPS:(id)sender {
    [self setShowFPS:!self.settings.showFPS];
    
    // Update menu item
    NSMenuItem *menuItem = (NSMenuItem *)sender;
    [menuItem setState:self.settings.showFPS ? NSControlStateValueOn : NSControlStateValueOff];
}

- (void)showAbout:(id)sender {
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:@"About FBNeo Metal"];
    [alert setInformativeText:@"FinalBurn Neo Metal Implementation\nA Metal-based arcade emulator for macOS"];
    [alert runModal];
}

- (void)showPreferences:(id)sender {
    // Create a preferences window
    NSWindow *prefsWindow = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 500, 400)
                                                       styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
                                                         backing:NSBackingStoreBuffered
                                                           defer:NO];
    [prefsWindow setTitle:@"FBNeo Preferences"];
    
    // Create a tab view for different preference categories
    NSTabView *tabView = [[NSTabView alloc] initWithFrame:NSMakeRect(10, 10, 480, 380)];
    
    // Video tab
    NSTabViewItem *videoTab = [[NSTabViewItem alloc] initWithIdentifier:@"video"];
    [videoTab setLabel:@"Video"];
    NSView *videoView = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 480, 350)];
    
    // Add some controls to the video tab
    NSButton *vsyncCheck = [[NSButton alloc] initWithFrame:NSMakeRect(20, 310, 200, 20)];
    [vsyncCheck setButtonType:NSButtonTypeSwitch];
    [vsyncCheck setTitle:@"Enable VSync"];
    [vsyncCheck setState:NSControlStateValueOn];
    [videoView addSubview:vsyncCheck];
    
    NSButton *fullscreenCheck = [[NSButton alloc] initWithFrame:NSMakeRect(20, 280, 200, 20)];
    [fullscreenCheck setButtonType:NSButtonTypeSwitch];
    [fullscreenCheck setTitle:@"Start in Fullscreen"];
    [fullscreenCheck setState:NSControlStateValueOff];
    [videoView addSubview:fullscreenCheck];
    
    NSTextField *scalingLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, 250, 100, 20)];
    [scalingLabel setStringValue:@"Scaling Mode:"];
    [scalingLabel setBezeled:NO];
    [scalingLabel setDrawsBackground:NO];
    [scalingLabel setEditable:NO];
    [scalingLabel setSelectable:NO];
    [videoView addSubview:scalingLabel];
    
    NSPopUpButton *scalingPopup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(130, 250, 150, 20) pullsDown:NO];
    [scalingPopup addItemWithTitle:@"Nearest Neighbor"];
    [scalingPopup addItemWithTitle:@"Bilinear"];
    [scalingPopup addItemWithTitle:@"CRT Effect"];
    [videoView addSubview:scalingPopup];
    
    [videoTab setView:videoView];
    [tabView addTabViewItem:videoTab];
    
    // Audio tab
    NSTabViewItem *audioTab = [[NSTabViewItem alloc] initWithIdentifier:@"audio"];
    [audioTab setLabel:@"Audio"];
    NSView *audioView = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 480, 350)];
    
    // Add some controls to the audio tab
    NSButton *audioCheck = [[NSButton alloc] initWithFrame:NSMakeRect(20, 310, 200, 20)];
    [audioCheck setButtonType:NSButtonTypeSwitch];
    [audioCheck setTitle:@"Enable Audio"];
    [audioCheck setState:NSControlStateValueOn];
    [audioView addSubview:audioCheck];
    
    NSTextField *volumeLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, 280, 100, 20)];
    [volumeLabel setStringValue:@"Volume:"];
    [volumeLabel setBezeled:NO];
    [volumeLabel setDrawsBackground:NO];
    [volumeLabel setEditable:NO];
    [volumeLabel setSelectable:NO];
    [audioView addSubview:volumeLabel];
    
    NSSlider *volumeSlider = [[NSSlider alloc] initWithFrame:NSMakeRect(130, 280, 200, 20)];
    [volumeSlider setMinValue:0.0];
    [volumeSlider setMaxValue:100.0];
    [volumeSlider setDoubleValue:80.0];
    [audioView addSubview:volumeSlider];
    
    NSTextField *sampleRateLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, 250, 100, 20)];
    [sampleRateLabel setStringValue:@"Sample Rate:"];
    [sampleRateLabel setBezeled:NO];
    [sampleRateLabel setDrawsBackground:NO];
    [sampleRateLabel setEditable:NO];
    [sampleRateLabel setSelectable:NO];
    [audioView addSubview:sampleRateLabel];
    
    NSPopUpButton *sampleRatePopup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(130, 250, 150, 20) pullsDown:NO];
    [sampleRatePopup addItemWithTitle:@"22050 Hz"];
    [sampleRatePopup addItemWithTitle:@"44100 Hz"];
    [sampleRatePopup addItemWithTitle:@"48000 Hz"];
    [audioView addSubview:sampleRatePopup];
    
    [audioTab setView:audioView];
    [tabView addTabViewItem:audioTab];
    
    // Input tab
    NSTabViewItem *inputTab = [[NSTabViewItem alloc] initWithIdentifier:@"input"];
    [inputTab setLabel:@"Input"];
    NSView *inputView = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 480, 350)];
    
    // Add some controls to the input tab
    NSButton *configureP1Button = [[NSButton alloc] initWithFrame:NSMakeRect(20, 310, 200, 30)];
    [configureP1Button setTitle:@"Configure Player 1"];
    [configureP1Button setBezelStyle:NSBezelStyleRounded];
    [inputView addSubview:configureP1Button];
    
    NSButton *configureP2Button = [[NSButton alloc] initWithFrame:NSMakeRect(20, 270, 200, 30)];
    [configureP2Button setTitle:@"Configure Player 2"];
    [configureP2Button setBezelStyle:NSBezelStyleRounded];
    [inputView addSubview:configureP2Button];
    
    NSButton *autofireCheck = [[NSButton alloc] initWithFrame:NSMakeRect(20, 230, 200, 20)];
    [autofireCheck setButtonType:NSButtonTypeSwitch];
    [autofireCheck setTitle:@"Enable Autofire"];
    [autofireCheck setState:NSControlStateValueOff];
    [inputView addSubview:autofireCheck];
    
    [inputTab setView:inputView];
    [tabView addTabViewItem:inputTab];
    
    // AI tab
    NSTabViewItem *aiTab = [[NSTabViewItem alloc] initWithIdentifier:@"ai"];
    [aiTab setLabel:@"AI"];
    NSView *aiView = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 480, 350)];
    
    // Add some controls to the AI tab
    NSButton *aiEnabledCheck = [[NSButton alloc] initWithFrame:NSMakeRect(20, 310, 200, 20)];
    [aiEnabledCheck setButtonType:NSButtonTypeSwitch];
    [aiEnabledCheck setTitle:@"Enable AI"];
    [aiEnabledCheck setState:NSControlStateValueOn];
    [aiView addSubview:aiEnabledCheck];
    
    NSTextField *difficultyLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, 280, 100, 20)];
    [difficultyLabel setStringValue:@"AI Difficulty:"];
    [difficultyLabel setBezeled:NO];
    [difficultyLabel setDrawsBackground:NO];
    [difficultyLabel setEditable:NO];
    [difficultyLabel setSelectable:NO];
    [aiView addSubview:difficultyLabel];
    
    NSSlider *difficultySlider = [[NSSlider alloc] initWithFrame:NSMakeRect(130, 280, 200, 20)];
    [difficultySlider setMinValue:1.0];
    [difficultySlider setMaxValue:10.0];
    [difficultySlider setDoubleValue:5.0];
    [aiView addSubview:difficultySlider];
    
    NSTextField *playerControlLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, 250, 100, 20)];
    [playerControlLabel setStringValue:@"AI Controls:"];
    [playerControlLabel setBezeled:NO];
    [playerControlLabel setDrawsBackground:NO];
    [playerControlLabel setEditable:NO];
    [playerControlLabel setSelectable:NO];
    [aiView addSubview:playerControlLabel];
    
    NSPopUpButton *playerControlPopup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(130, 250, 150, 20) pullsDown:NO];
    [playerControlPopup addItemWithTitle:@"None"];
    [playerControlPopup addItemWithTitle:@"Player 1"];
    [playerControlPopup addItemWithTitle:@"Player 2"];
    [playerControlPopup addItemWithTitle:@"Both Players"];
    [aiView addSubview:playerControlPopup];
    
    NSButton *trainingModeCheck = [[NSButton alloc] initWithFrame:NSMakeRect(20, 220, 200, 20)];
    [trainingModeCheck setButtonType:NSButtonTypeSwitch];
    [trainingModeCheck setTitle:@"Training Mode"];
    [trainingModeCheck setState:NSControlStateValueOff];
    [aiView addSubview:trainingModeCheck];
    
    NSButton *debugOverlayCheck = [[NSButton alloc] initWithFrame:NSMakeRect(20, 190, 200, 20)];
    [debugOverlayCheck setButtonType:NSButtonTypeSwitch];
    [debugOverlayCheck setTitle:@"Show Debug Overlay"];
    [debugOverlayCheck setState:NSControlStateValueOff];
    [aiView addSubview:debugOverlayCheck];
    
    NSButton *loadModelButton = [[NSButton alloc] initWithFrame:NSMakeRect(20, 150, 200, 30)];
    [loadModelButton setTitle:@"Load AI Model..."];
    [loadModelButton setBezelStyle:NSBezelStyleRounded];
    [aiView addSubview:loadModelButton];
    
    [aiTab setView:aiView];
    [tabView addTabViewItem:aiTab];
    
    // Add the tab view to the window
    [[prefsWindow contentView] addSubview:tabView];
    
    // Add Save and Cancel buttons
    NSButton *cancelButton = [[NSButton alloc] initWithFrame:NSMakeRect(320, 10, 80, 30)];
    [cancelButton setTitle:@"Cancel"];
    [cancelButton setBezelStyle:NSBezelStyleRounded];
    [cancelButton setTarget:self];
    [cancelButton setAction:@selector(closePreferences:)];
    [[prefsWindow contentView] addSubview:cancelButton];
    
    NSButton *saveButton = [[NSButton alloc] initWithFrame:NSMakeRect(410, 10, 80, 30)];
    [saveButton setTitle:@"Save"];
    [saveButton setBezelStyle:NSBezelStyleRounded];
    [saveButton setTarget:self];
    [saveButton setAction:@selector(savePreferences:)];
    [[prefsWindow contentView] addSubview:saveButton];
    
    // Show the window
    [prefsWindow center];
    [prefsWindow makeKeyAndOrderFront:nil];
}

- (void)closePreferences:(id)sender {
    [sender.window close];
}

- (void)savePreferences:(id)sender {
    // Save preferences logic would go here
    
    // Show a confirmation
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:@"Preferences Saved"];
    [alert setInformativeText:@"Your preferences have been saved."];
    [alert runModal];
    
    [sender.window close];
}

#pragma mark - AI Menu Actions

- (void)toggleAI:(id)sender {
    [self setAIEnabled:!self.settings.aiEnabled];
    
    // Update menu item
    NSMenuItem *menuItem = (NSMenuItem *)sender;
    [menuItem setState:self.settings.aiEnabled ? NSControlStateValueOn : NSControlStateValueOff];
}

- (void)setAIPlayerControl:(id)sender {
    if ([sender isKindOfClass:[NSMenuItem class]]) {
        NSMenuItem *menuItem = (NSMenuItem *)sender;
        [self setAIControlledPlayer:(int)menuItem.tag];
        
        // Update menu state
        NSMenu *menu = [menuItem menu];
        for (NSMenuItem *item in menu.itemArray) {
            [item setState:(item.tag == self.settings.aiControlledPlayer) ? NSControlStateValueOn : NSControlStateValueOff];
        }
    }
}

- (void)setAIDifficultyLevel:(id)sender {
    if ([sender isKindOfClass:[NSMenuItem class]]) {
        NSMenuItem *menuItem = (NSMenuItem *)sender;
        [self setAIDifficulty:(int)menuItem.tag];
        
        // Update menu state
        NSMenu *menu = [menuItem menu];
        for (NSMenuItem *item in menu.itemArray) {
            [item setState:(item.tag == self.settings.aiDifficulty) ? NSControlStateValueOn : NSControlStateValueOff];
        }
    }
}

- (void)toggleTrainingMode:(id)sender {
    [self setAITrainingMode:!self.settings.aiTrainingMode];
    
    // Update menu item
    NSMenuItem *menuItem = (NSMenuItem *)sender;
    [menuItem setState:self.settings.aiTrainingMode ? NSControlStateValueOn : NSControlStateValueOff];
}

- (void)toggleAIDebugOverlay:(id)sender {
    [self setAIDebugOverlay:!self.settings.aiDebugOverlay];
    
    // Update menu item
    NSMenuItem *menuItem = (NSMenuItem *)sender;
    [menuItem setState:self.settings.aiDebugOverlay ? NSControlStateValueOn : NSControlStateValueOff];
}

- (void)loadAIModelFile:(id)sender {
    NSOpenPanel *openPanel = [NSOpenPanel openPanel];
    [openPanel setTitle:@"Load AI Model"];
    [openPanel setAllowedFileTypes:@[@"pt"]]; // TorchScript serialized models
    [openPanel setAllowsMultipleSelection:NO];
    
    [openPanel beginSheetModalForWindow:self.window completionHandler:^(NSInteger result) {
        if (result == NSModalResponseOK) {
            NSURL *selectedFile = [openPanel URLs][0];
            [self loadAIModel:[selectedFile path]];
        }
    }];
}

- (void)selectAIModel:(id)sender {
    // Load a default model for simplicity
    [self loadAIModel:@"models/default_model.pt"];
}

- (void)showMemoryViewer:(id)sender {
    // Create a memory viewer window
    NSRect frame = NSMakeRect(0, 0, 600, 500);
    NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable;
    NSWindow *memoryWindow = [[NSWindow alloc] initWithContentRect:frame
                                                         styleMask:style
                                                           backing:NSBackingStoreBuffered
                                                             defer:NO];
    [memoryWindow setTitle:@"FBNeo - Memory Viewer"];
    
    // Create a split view
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
    [hexTextView setString:@"00000000: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................\n00000010: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................\n00000020: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................\n00000030: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................\n00000040: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................\n"];
    [hexTextView setEditable:NO];
    [hexScrollView setDocumentView:hexTextView];
    [hexScrollView setBorderType:NSBezelBorder];
    [hexScrollView setHasVerticalScroller:YES];
    [hexScrollView setHasHorizontalScroller:YES];
    
    // Add views to split view
    [splitView addSubview:regionsScrollView];
    [splitView addSubview:hexScrollView];
    
    // Set initial positions
    [splitView setPosition:150 ofDividerAtIndex:0];
    
    // Create controls
    NSTextField *addressField = [[NSTextField alloc] initWithFrame:NSMakeRect(10, 10, 120, 20)];
    [addressField setPlaceholderString:@"Address (hex)"];
    
    NSButton *goButton = [[NSButton alloc] initWithFrame:NSMakeRect(140, 10, 50, 20)];
    [goButton setTitle:@"Go"];
    [goButton setBezelStyle:NSBezelStyleRounded];
    
    NSButton *refreshButton = [[NSButton alloc] initWithFrame:NSMakeRect(200, 10, 80, 20)];
    [refreshButton setTitle:@"Refresh"];
    [refreshButton setBezelStyle:NSBezelStyleRounded];
    
    // Create data source for regions
    MemoryViewerDataSource *dataSource = [[MemoryViewerDataSource alloc] init];
    [regionsTableView setDataSource:dataSource];
    [regionsTableView setDelegate:dataSource];
    
    // Add everything to the window
    [[memoryWindow contentView] addSubview:splitView];
    [[memoryWindow contentView] addSubview:addressField];
    [[memoryWindow contentView] addSubview:goButton];
    [[memoryWindow contentView] addSubview:refreshButton];
    
    // Add memory region data
    [dataSource setHexTextView:hexTextView];
    
    // Show the window
    [memoryWindow center];
    [memoryWindow makeKeyAndOrderFront:nil];
}

// Memory viewer data source
@interface MemoryViewerDataSource : NSObject <NSTableViewDataSource, NSTableViewDelegate>
@property (nonatomic, strong) NSArray *memoryRegions;
@property (weak) NSTextView *hexTextView;
@end

@implementation MemoryViewerDataSource

- (instancetype)init {
    self = [super init];
    if (self) {
        // Sample memory regions
        _memoryRegions = @[
            @"Main CPU RAM (0x000000)",
            @"Video RAM (0x100000)",
            @"Palette RAM (0x200000)",
            @"Sound RAM (0x300000)",
            @"ROM (0x400000)",
            @"Sprite RAM (0x500000)",
            @"Input Ports (0x600000)",
            @"EEPROM (0x700000)"
        ];
    }
    return self;
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    return self.memoryRegions.count;
}

- (id)tableView:(NSTableView *)tableView objectValueForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
    if (row < 0 || row >= self.memoryRegions.count) {
        return nil;
    }
    
    return self.memoryRegions[row];
}

- (void)tableViewSelectionDidChange:(NSNotification *)notification {
    NSTableView *tableView = notification.object;
    NSInteger selectedRow = tableView.selectedRow;
    
    if (selectedRow >= 0 && selectedRow < self.memoryRegions.count) {
        // Get the selected memory region
        NSString *regionName = self.memoryRegions[selectedRow];
        NSString *baseAddress = @"00000000";
        
        // Extract the base address from the region name if possible
        NSRange range = [regionName rangeOfString:@"(0x" options:NSBackwardsSearch];
        if (range.location != NSNotFound) {
            NSRange addressRange = NSMakeRange(range.location + 2, 6);
            if (addressRange.location + addressRange.length <= regionName.length) {
                baseAddress = [regionName substringWithRange:addressRange];
            }
        }
        
        // Generate sample memory data for the selected region
        NSMutableString *hexData = [NSMutableString string];
        
        for (int i = 0; i < 32; i++) {
            // Address column
            [hexData appendString:[NSString stringWithFormat:@"%06X: ", (unsigned int)strtoul([baseAddress UTF8String], NULL, 16) + i * 16]];
            
            // Hex data
            NSMutableString *lineBytes = [NSMutableString string];
            NSMutableString *lineChars = [NSMutableString string];
            
            for (int j = 0; j < 16; j++) {
                uint8_t randomByte = arc4random_uniform(256);
                [lineBytes appendString:[NSString stringWithFormat:@"%02X ", randomByte]];
                
                // Generate ASCII representation
                char c = (randomByte >= 32 && randomByte < 127) ? (char)randomByte : '.';
                [lineChars appendString:[NSString stringWithFormat:@"%c", c]];
            }
            
            [hexData appendString:lineBytes];
            [hexData appendString:@" "];
            [hexData appendString:lineChars];
            [hexData appendString:@"\n"];
        }
        
        // Update the hex view
        [self.hexTextView setString:hexData];
    }
}

- (void)setHexTextView:(NSTextView *)hexTextView {
    _hexTextView = hexTextView;
}

@end

- (void)showRegisterViewer:(id)sender {
    // Create a register viewer window
    NSWindow *registerWindow = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 500, 400)
                                                          styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                                                            backing:NSBackingStoreBuffered
                                                              defer:NO];
    [registerWindow setTitle:@"FBNeo - Register Viewer"];
    
    // Create a tab view for different CPUs
    NSTabView *tabView = [[NSTabView alloc] initWithFrame:NSMakeRect(10, 10, 480, 380)];
    
    // M68000 CPU tab
    NSTabViewItem *m68kTab = [[NSTabViewItem alloc] initWithIdentifier:@"m68k"];
    [m68kTab setLabel:@"M68000"];
    NSView *m68kView = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 480, 380)];
    
    // Create a table view for the registers
    NSScrollView *scrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(10, 10, 460, 360)];
    NSTableView *tableView = [[NSTableView alloc] initWithFrame:NSMakeRect(0, 0, 460, 360)];
    
    NSTableColumn *regNameColumn = [[NSTableColumn alloc] initWithIdentifier:@"regName"];
    [regNameColumn setWidth:100];
    [regNameColumn setTitle:@"Register"];
    [tableView addTableColumn:regNameColumn];
    
    NSTableColumn *regValueColumn = [[NSTableColumn alloc] initWithIdentifier:@"regValue"];
    [regValueColumn setWidth:100];
    [regValueColumn setTitle:@"Value (Hex)"];
    [tableView addTableColumn:regValueColumn];
    
    NSTableColumn *regDecColumn = [[NSTableColumn alloc] initWithIdentifier:@"regDec"];
    [regDecColumn setWidth:100];
    [regDecColumn setTitle:@"Value (Dec)"];
    [tableView addTableColumn:regDecColumn];
    
    NSTableColumn *regBinColumn = [[NSTableColumn alloc] initWithIdentifier:@"regBin"];
    [regBinColumn setWidth:150];
    [regBinColumn setTitle:@"Value (Bin)"];
    [tableView addTableColumn:regBinColumn];
    
    [scrollView setDocumentView:tableView];
    [scrollView setBorderType:NSBezelBorder];
    [scrollView setHasVerticalScroller:YES];
    [scrollView setHasHorizontalScroller:YES];
    
    [m68kView addSubview:scrollView];
    [m68kTab setView:m68kView];
    [tabView addTabViewItem:m68kTab];
    
    // Z80 CPU tab
    NSTabViewItem *z80Tab = [[NSTabViewItem alloc] initWithIdentifier:@"z80"];
    [z80Tab setLabel:@"Z80"];
    NSView *z80View = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 480, 380)];
    
    // Create a table view for the Z80 registers
    NSScrollView *z80ScrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(10, 10, 460, 360)];
    NSTableView *z80TableView = [[NSTableView alloc] initWithFrame:NSMakeRect(0, 0, 460, 360)];
    
    NSTableColumn *z80RegNameColumn = [[NSTableColumn alloc] initWithIdentifier:@"regName"];
    [z80RegNameColumn setWidth:100];
    [z80RegNameColumn setTitle:@"Register"];
    [z80TableView addTableColumn:z80RegNameColumn];
    
    NSTableColumn *z80RegValueColumn = [[NSTableColumn alloc] initWithIdentifier:@"regValue"];
    [z80RegValueColumn setWidth:100];
    [z80RegValueColumn setTitle:@"Value (Hex)"];
    [z80TableView addTableColumn:z80RegValueColumn];
    
    NSTableColumn *z80RegDecColumn = [[NSTableColumn alloc] initWithIdentifier:@"regDec"];
    [z80RegDecColumn setWidth:100];
    [z80RegDecColumn setTitle:@"Value (Dec)"];
    [z80TableView addTableColumn:z80RegDecColumn];
    
    NSTableColumn *z80RegBinColumn = [[NSTableColumn alloc] initWithIdentifier:@"regBin"];
    [z80RegBinColumn setWidth:150];
    [z80RegBinColumn setTitle:@"Value (Bin)"];
    [z80TableView addTableColumn:z80RegBinColumn];
    
    [z80ScrollView setDocumentView:z80TableView];
    [z80ScrollView setBorderType:NSBezelBorder];
    [z80ScrollView setHasVerticalScroller:YES];
    [z80ScrollView setHasHorizontalScroller:YES];
    
    [z80View addSubview:z80ScrollView];
    [z80Tab setView:z80View];
    [tabView addTabViewItem:z80Tab];
    
    // Set up data sources
    RegisterViewerDataSource *m68kDataSource = [[RegisterViewerDataSource alloc] initWithCPUType:@"M68000"];
    [tableView setDataSource:m68kDataSource];
    
    RegisterViewerDataSource *z80DataSource = [[RegisterViewerDataSource alloc] initWithCPUType:@"Z80"];
    [z80TableView setDataSource:z80DataSource];
    
    // Add refresh button
    NSButton *refreshButton = [[NSButton alloc] initWithFrame:NSMakeRect(400, 10, 80, 25)];
    [refreshButton setTitle:@"Refresh"];
    [refreshButton setBezelStyle:NSBezelStyleRounded];
    
    // Add the tabview to the window
    [[registerWindow contentView] addSubview:tabView];
    
    // Show the window
    [registerWindow center];
    [registerWindow makeKeyAndOrderFront:nil];
}

// Register viewer data source
@interface RegisterViewerDataSource : NSObject <NSTableViewDataSource>
@property (nonatomic, strong) NSString *cpuType;
@property (nonatomic, strong) NSArray *registerNames;
@property (nonatomic, strong) NSArray *registerValues;
@end

@implementation RegisterViewerDataSource

- (instancetype)initWithCPUType:(NSString *)cpuType {
    self = [super init];
    if (self) {
        _cpuType = cpuType;
        
        if ([cpuType isEqualToString:@"M68000"]) {
            _registerNames = @[
                @"D0", @"D1", @"D2", @"D3", @"D4", @"D5", @"D6", @"D7",
                @"A0", @"A1", @"A2", @"A3", @"A4", @"A5", @"A6", @"A7/SP",
                @"PC", @"SR", @"USP", @"ISP", @"MSP"
            ];
            
            // Generate some random hex values for demo
            NSMutableArray *values = [NSMutableArray array];
            for (int i = 0; i < _registerNames.count; i++) {
                uint32_t randomValue;
                if (i < 16) { // D and A registers are 32-bit
                    randomValue = arc4random_uniform(0xFFFFFFFF);
                } else if (i == 16) { // PC
                    randomValue = 0x400000 + arc4random_uniform(0x10000);
                } else { // Status and stack pointers
                    randomValue = arc4random_uniform(0xFFFF);
                }
                [values addObject:@(randomValue)];
            }
            _registerValues = values;
        } else if ([cpuType isEqualToString:@"Z80"]) {
            _registerNames = @[
                @"AF", @"BC", @"DE", @"HL",
                @"IX", @"IY", @"SP", @"PC",
                @"AF'", @"BC'", @"DE'", @"HL'",
                @"I", @"R", @"IM", @"IFF1", @"IFF2"
            ];
            
            // Generate some random hex values for demo
            NSMutableArray *values = [NSMutableArray array];
            for (int i = 0; i < _registerNames.count; i++) {
                uint32_t randomValue;
                if (i < 8) { // Main register pairs
                    randomValue = arc4random_uniform(0xFFFF);
                } else if (i < 12) { // Shadow registers
                    randomValue = arc4random_uniform(0xFFFF);
                } else { // Special registers
                    randomValue = arc4random_uniform(0xFF);
                }
                [values addObject:@(randomValue)];
            }
            _registerValues = values;
        }
    }
    return self;
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    return self.registerNames.count;
}

- (id)tableView:(NSTableView *)tableView objectValueForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
    if (row < 0 || row >= self.registerNames.count) {
        return nil;
    }
    
    NSString *identifier = tableColumn.identifier;
    
    if ([identifier isEqualToString:@"regName"]) {
        return self.registerNames[row];
    } else if ([identifier isEqualToString:@"regValue"]) {
        uint32_t value = [self.registerValues[row] unsignedIntValue];
        
        // Format differently based on register type
        if ([self.cpuType isEqualToString:@"M68000"]) {
            if (row < 16) { // D and A registers are 32-bit
                return [NSString stringWithFormat:@"$%08X", value];
            } else if (row == 16) { // PC
                return [NSString stringWithFormat:@"$%08X", value];
            } else { // Status and stack pointers
                return [NSString stringWithFormat:@"$%04X", value];
            }
        } else { // Z80
            if (row < 12) { // Main and shadow register pairs
                return [NSString stringWithFormat:@"$%04X", value];
            } else { // Special registers
                return [NSString stringWithFormat:@"$%02X", value];
            }
        }
    } else if ([identifier isEqualToString:@"regDec"]) {
        uint32_t value = [self.registerValues[row] unsignedIntValue];
        return [NSString stringWithFormat:@"%u", value];
    } else if ([identifier isEqualToString:@"regBin"]) {
        uint32_t value = [self.registerValues[row] unsignedIntValue];
        
        // Format binary representation with different lengths based on register
        if ([self.cpuType isEqualToString:@"M68000"]) {
            if (row < 16) { // D and A registers are 32-bit
                return [self formatBinary:value bits:32];
            } else if (row == 16) { // PC
                return [self formatBinary:value bits:32];
            } else { // Status and stack pointers
                return [self formatBinary:value bits:16];
            }
        } else { // Z80
            if (row < 12) { // Main and shadow register pairs
                return [self formatBinary:value bits:16];
            } else { // Special registers
                return [self formatBinary:value bits:8];
            }
        }
    }
    
    return nil;
}

// Helper to format binary values with proper spacing
- (NSString *)formatBinary:(uint32_t)value bits:(int)bits {
    NSMutableString *binary = [NSMutableString string];
    
    for (int i = bits - 1; i >= 0; i--) {
        [binary appendString:(value & (1 << i)) ? @"1" : @"0"];
        if (i % 4 == 0 && i > 0) {
            [binary appendString:@" "];
        }
    }
    
    return binary;
}

@end

- (void)showDisassembly:(id)sender {
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
    [disasmTableView setDelegate:disasmDataSource];
    
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

// Disassembler data source
@interface DisassemblerDataSource : NSObject <NSTableViewDataSource, NSTableViewDelegate>
@property (nonatomic, strong) NSArray *codeRegions;
@property (nonatomic, strong) NSArray *addresses;
@property (nonatomic, strong) NSArray *bytes;
@property (nonatomic, strong) NSArray *instructions;
@property (nonatomic, strong) NSTableView *disasmTableView;
@end

@implementation DisassemblerDataSource

- (instancetype)init {
    self = [super init];
    if (self) {
        // Sample code regions
        _codeRegions = @[
            @"Main CPU (0x000000)",
            @"Sound CPU (0x100000)",
            @"MCU (0x200000)",
            @"ROM (0x400000)"
        ];
        
        // Initialize with sample M68000 disassembly
        [self generateSampleDisassembly];
    }
    return self;
}

- (void)generateSampleDisassembly {
    // Sample disassembly for M68000
    NSMutableArray *addrs = [NSMutableArray array];
    NSMutableArray *bytesList = [NSMutableArray array];
    NSMutableArray *insts = [NSMutableArray array];
    
    uint32_t baseAddress = 0x000000;
    
    // Create some sample 68000 instructions
    NSArray *sampleInstructions = @[
        @[@"4E75", @"RTS"],
        @[@"4E71", @"NOP"],
        @[@"4E72", @"STOP #$2000"],
        @[@"4EAE FFF8", @"JSR -8(A6)"],
        @[@"4EF9 0001 2000", @"JMP $12000"],
        @[@"4A40", @"TST.W D0"],
        @[@"4A79 0010 0000", @"TST.W $100000"],
        @[@"4CDF 7FFF", @"MOVEM.L (SP)+,D0-D7/A0-A6"],
        @[@"4E66", @"MOVE USP,A6"],
        @[@"600C", @"BRA.S $+14"],
        @[@"6100 1234", @"BSR $+4660"],
        @[@"6700 0010", @"BEQ $+18"],
        @[@"7001", @"MOVEQ #1,D0"],
        @[@"7200", @"MOVEQ #0,D1"],
        @[@"7404", @"MOVEQ #4,D2"],
        @[@"7A0A", @"MOVEQ #10,D5"],
        @[@"4BF9 0001 0000", @"LEA $10000,A5"],
        @[@"43F9 0000 8000", @"LEA $8000,A1"],
        @[@"203C 0000 00FF", @"MOVE.L #$FF,D0"],
        @[@"3039 0001 0004", @"MOVE.W $10004,D0"],
        @[@"3239 0001 0006", @"MOVE.W $10006,D1"],
        @[@"0C40 00FF", @"CMPI.W #$FF,D0"],
        @[@"0C41 00FF", @"CMPI.W #$FF,D1"],
        @[@"6600 0014", @"BNE $+22"],
        @[@"4880", @"EXT.W D0"],
        @[@"4881", @"EXT.W D1"],
        @[@"C041", @"AND.W D1,D0"],
        @[@"4A40", @"TST.W D0"],
        @[@"6700 0008", @"BEQ $+10"],
        @[@"7001", @"MOVEQ #1,D0"],
        @[@"4E75", @"RTS"]
    ];
    
    for (int i = 0; i < sampleInstructions.count; i++) {
        NSArray *inst = sampleInstructions[i];
        [addrs addObject:[NSString stringWithFormat:@"$%06X", baseAddress]];
        [bytesList addObject:inst[0]];
        [insts addObject:inst[1]];
        
        // Increment address based on instruction length
        NSString *bytes = inst[0];
        baseAddress += (bytes.length / 2);
    }
    
    _addresses = addrs;
    _bytes = bytesList;
    _instructions = insts;
}

#pragma mark - NSTableViewDataSource

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    // Store disassembly table view reference
    if ([tableView.tableColumns count] > 2) {
        self.disasmTableView = tableView;
    }
    
    if ([tableView.tableColumns count] == 1) {
        // Region list
        return self.codeRegions.count;
    } else {
        // Disassembly view
        return self.addresses.count;
    }
}

- (id)tableView:(NSTableView *)tableView objectValueForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
    if ([tableView.tableColumns count] == 1) {
        // Region list
        if (row < 0 || row >= self.codeRegions.count) {
            return nil;
        }
        return self.codeRegions[row];
    } else {
        // Disassembly view
        if (row < 0 || row >= self.addresses.count) {
            return nil;
        }
        
        if ([tableColumn.identifier isEqualToString:@"address"]) {
            return self.addresses[row];
        } else if ([tableColumn.identifier isEqualToString:@"bytes"]) {
            return self.bytes[row];
        } else if ([tableColumn.identifier isEqualToString:@"instruction"]) {
            return self.instructions[row];
        }
    }
    return nil;
}

@end

- (void)showRomBrowserDialog {
    // Create a game browser window
    NSRect frame = NSMakeRect(0, 0, 800, 600);
    NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable;
    NSWindow *browserWindow = [[NSWindow alloc] initWithContentRect:frame
                                                         styleMask:style
                                                           backing:NSBackingStoreBuffered
                                                             defer:NO];
    [browserWindow setTitle:@"FBNeo - ROM Browser"];
    
    // Create a proper browser UI
    NSScrollView *scrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(20, 60, 760, 500)];
    [scrollView setBorderType:NSBezelBorder];
    [scrollView setHasVerticalScroller:YES];
    [scrollView setHasHorizontalScroller:YES];
    
    NSTableView *tableView = [[NSTableView alloc] initWithFrame:NSMakeRect(0, 0, 740, 500)];
    
    NSTableColumn *gameColumn = [[NSTableColumn alloc] initWithIdentifier:@"game"];
    [gameColumn setWidth:400];
    [gameColumn setTitle:@"Game"];
    [tableView addTableColumn:gameColumn];
    
    NSTableColumn *manufacturerColumn = [[NSTableColumn alloc] initWithIdentifier:@"manufacturer"];
    [manufacturerColumn setWidth:170];
    [manufacturerColumn setTitle:@"Manufacturer"];
    [tableView addTableColumn:manufacturerColumn];
    
    NSTableColumn *yearColumn = [[NSTableColumn alloc] initWithIdentifier:@"year"];
    [yearColumn setWidth:80];
    [yearColumn setTitle:@"Year"];
    [tableView addTableColumn:yearColumn];
    
    [scrollView setDocumentView:tableView];
    
    // Create cancel button
    NSButton *cancelButton = [[NSButton alloc] initWithFrame:NSMakeRect(600, 20, 80, 32)];
    [cancelButton setTitle:@"Cancel"];
    [cancelButton setBezelStyle:NSBezelStyleRounded];
    [cancelButton setTarget:self];
    [cancelButton setAction:@selector(closeRomBrowser:)];
    
    // Create play button
    NSButton *playButton = [[NSButton alloc] initWithFrame:NSMakeRect(700, 20, 80, 32)];
    [playButton setTitle:@"Play"];
    [playButton setBezelStyle:NSBezelStyleRounded];
    [playButton setTarget:self];
    [playButton setAction:@selector(loadSelectedRom:)];
    
    // Add all UI elements to window
    NSView *contentView = [browserWindow contentView];
    [contentView addSubview:scrollView];
    [contentView addSubview:cancelButton];
    [contentView addSubview:playButton];
    
    // Add data to the table
    NSMutableArray *gameData = [NSMutableArray array];
    [gameData addObject:@[@"Marvel vs Capcom: Clash of Super Heroes", @"Capcom", @"1998"]];
    [gameData addObject:@[@"Street Fighter II: World Warrior", @"Capcom", @"1991"]];
    [gameData addObject:@[@"Street Fighter Alpha 3", @"Capcom", @"1998"]];
    [gameData addObject:@[@"Metal Slug 3", @"SNK", @"2000"]];
    [gameData addObject:@[@"The King of Fighters '98", @"SNK", @"1998"]];
    [gameData addObject:@[@"Samurai Shodown II", @"SNK", @"1994"]];
    [gameData addObject:@[@"Mortal Kombat II", @"Midway", @"1993"]];
    [gameData addObject:@[@"X-Men vs. Street Fighter", @"Capcom", @"1996"]];
    [gameData addObject:@[@"Darkstalkers", @"Capcom", @"1994"]];
    [gameData addObject:@[@"Aliens vs. Predator", @"Capcom", @"1994"]];
    
    // Store the games data in the window's user data
    [tableView setTag:100]; // Tag for tableView
    objc_setAssociatedObject(browserWindow, "romBrowserData", gameData, OBJC_ASSOCIATION_RETAIN);
    
    // Set up data source and delegate
    [tableView setDataSource:self];
    [tableView setDelegate:self];
    
    // Center and show the window
    [browserWindow center];
    [browserWindow makeKeyAndOrderFront:nil];
}

// Close ROM browser
- (void)closeRomBrowser:(id)sender {
    NSButton *button = sender;
    [button.window close];
}

// Load selected ROM
- (void)loadSelectedRom:(id)sender {
    NSButton *button = sender;
    NSWindow *window = button.window;
    NSView *contentView = [window contentView];
    
    // Find the tableView with tag 100
    NSTableView *tableView = nil;
    for (NSView *view in [contentView subviews]) {
        if ([view isKindOfClass:[NSScrollView class]]) {
            NSView *docView = [(NSScrollView *)view documentView];
            if ([docView isKindOfClass:[NSTableView class]] && docView.tag == 100) {
                tableView = (NSTableView *)docView;
                break;
            }
        }
    }
    
    if (!tableView) {
        NSLog(@"Error: Could not find table view");
        return;
    }
    
    // Get selected row
    NSInteger selectedRow = [tableView selectedRow];
    if (selectedRow < 0) {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"No Game Selected"];
        [alert setInformativeText:@"Please select a game from the list."];
        [alert runModal];
        return;
    }
    
    // Get game data
    NSArray *gameData = objc_getAssociatedObject(window, "romBrowserData");
    NSArray *gameInfo = gameData[selectedRow];
    NSString *gameName = gameInfo[0];
    
    // Close ROM browser
    [window close];
    
    // Look for the ROM file
    NSString *romPath = nil;
    
    // Try to find the ROM file in the default ROM path
    NSString *defaultRomPath = @"/Users/plasx/dev/ROMs";
    
    // Try different ROM name formats
    NSArray *possibleFilenames = @[
        [NSString stringWithFormat:@"%@.zip", gameName],
        [NSString stringWithFormat:@"%@.zip", [gameName stringByReplacingOccurrencesOfString:@" " withString:@""]],
        [NSString stringWithFormat:@"%@.zip", [gameName stringByReplacingOccurrencesOfString:@": " withString:@""]],
        [NSString stringWithFormat:@"%@.zip", [[gameName componentsSeparatedByString:@":"][0] stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]]],
        [NSString stringWithFormat:@"%@.zip", [[gameName componentsSeparatedByString:@" - "][0] stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]]]
    ];
    
    for (NSString *filename in possibleFilenames) {
        NSString *testPath = [defaultRomPath stringByAppendingPathComponent:filename];
        if ([[NSFileManager defaultManager] fileExistsAtPath:testPath]) {
            romPath = testPath;
            break;
        }
    }
    
    // If ROM not found in the default path, try to extract a short name
    if (!romPath) {
        // Get the short name for Marvel vs Capcom: mvsc.zip
        NSString *shortName = [self getShortNameForGame:gameName];
        if (shortName) {
            NSString *testPath = [defaultRomPath stringByAppendingPathComponent:[NSString stringWithFormat:@"%@.zip", shortName]];
            if ([[NSFileManager defaultManager] fileExistsAtPath:testPath]) {
                romPath = testPath;
            }
        }
    }
    
    // If we found the ROM, load it
    if (romPath) {
        // Load the ROM
        if ([self loadROMFile:romPath]) {
            NSLog(@"Game loaded successfully: %@", gameName);
        } else {
            NSLog(@"Failed to load game: %@", gameName);
        }
    } else {
        // ROM not found
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"ROM Not Found"];
        [alert setInformativeText:[NSString stringWithFormat:@"Could not find ROM file for: %@\n\nPlease make sure the ROM file is in the ROM directory.", gameName]];
        [alert runModal];
    }
}

// Helper method to get short name for common games
- (NSString *)getShortNameForGame:(NSString *)gameName {
    NSDictionary *gameNameToShortName = @{
        @"Marvel vs Capcom: Clash of Super Heroes": @"mvsc",
        @"Street Fighter II: World Warrior": @"sf2",
        @"Street Fighter Alpha 3": @"sfa3",
        @"Metal Slug 3": @"mslug3",
        @"The King of Fighters '98": @"kof98",
        @"Samurai Shodown II": @"samsho2",
        @"Mortal Kombat II": @"mk2",
        @"X-Men vs. Street Fighter": @"xmvsf",
        @"Darkstalkers": @"dstlk",
        @"Aliens vs. Predator": @"avsp"
    };
    
    return gameNameToShortName[gameName];
}

#pragma mark - NSTableViewDataSource

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    NSWindow *window = [tableView window];
    if (!window) return 0;
    
    NSArray *gameData = objc_getAssociatedObject(window, "romBrowserData");
    return gameData ? gameData.count : 0;
}

- (id)tableView:(NSTableView *)tableView objectValueForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
    NSWindow *window = [tableView window];
    if (!window) return nil;
    
    NSArray *gameData = objc_getAssociatedObject(window, "romBrowserData");
    if (!gameData || row >= gameData.count) return nil;
    
    NSArray *gameInfo = gameData[row];
    NSString *identifier = [tableColumn identifier];
    
    if ([identifier isEqualToString:@"game"]) {
        return gameInfo[0];
    } else if ([identifier isEqualToString:@"manufacturer"]) {
        return gameInfo[1];
    } else if ([identifier isEqualToString:@"year"]) {
        return gameInfo[2];
    }
    
    return nil;
}

#pragma mark - ROM Loading and Game Management

- (BOOL)loadROMFile:(NSString *)path {
    NSLog(@"Loading ROM: %@", path);
    
    // Store the ROM path
    self.currentROMPath = path;
    
    // Check if the file exists
    if (![[NSFileManager defaultManager] fileExistsAtPath:path]) {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"ROM File Not Found"];
        [alert setInformativeText:[NSString stringWithFormat:@"Could not find ROM file at path: %@", path]];
        [alert runModal];
        return NO;
    }
    
    // Extract the filename without the extension
    NSString *romName = [[path lastPathComponent] stringByDeletingPathExtension];
    
    // For multiple ROM sets in a zip, the filename might correspond to the game name
    // Try to find a matching ROM set
    BOOL romFound = NO;
    for (int i = 0; i < nBurnDrvCount; i++) {
        nBurnDrvActive = i;
        
        // Check for exact match on ROM name
        if ([romName caseInsensitiveCompare:@(BurnDrvGetTextA(DRV_NAME))] == NSOrderedSame) {
            romFound = YES;
            break;
        }
        
        // Check if the ROM name contains the driver name
        if ([romName rangeOfString:@(BurnDrvGetTextA(DRV_NAME)) options:NSCaseInsensitiveSearch].location != NSNotFound) {
            romFound = YES;
            break;
        }
        
        // Additional search by description
        if ([romName rangeOfString:@(BurnDrvGetTextA(DRV_FULLNAME)) options:NSCaseInsensitiveSearch].location != NSNotFound) {
            romFound = YES;
            break;
        }
    }
    
    if (!romFound) {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"ROM Not Supported"];
        [alert setInformativeText:[NSString stringWithFormat:@"Could not find a matching ROM set for: %@", romName]];
        [alert runModal];
        return NO;
    }
    
    // Now actually load the ROM
    // Call the Metal_LoadROM function which is defined in the C++ code
    extern int Metal_LoadROM(const char* romPath);
    int result = Metal_LoadROM([path UTF8String]);
    
    if (result != 0) {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"ROM Loading Error"];
        [alert setInformativeText:@"Failed to load the selected ROM. It may be missing files or be an unsupported variant."];
        [alert runModal];
        return NO;
    }
    
    // Update window title with game name
    [self.window setTitle:[NSString stringWithFormat:@"FBNeo - %s", BurnDrvGetTextA(DRV_FULLNAME)]];
    
    // Set game loaded state
    self.gameLoaded = YES;
    self.isPaused = NO;
    
    // Start the emulation
    [self runLoadedGame];
    
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
    // TODO: Call appropriate Metal bridge function
    // For now, the game will run as part of Metal_LoadROM
}

- (void)resetLoadedGame {
    if (!self.gameLoaded) {
        NSLog(@"No game loaded to reset");
        return;
    }
    
    NSLog(@"Resetting game: %@", self.currentROMPath);
    
    // Call into C++ code to reset the game
    extern int Metal_ResetGame();
    int result = Metal_ResetGame();
    
    if (result != 0) {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"Reset Error"];
        [alert setInformativeText:@"Failed to reset the game."];
        [alert runModal];
    } else {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"Game Reset"];
        [alert setInformativeText:@"The game has been reset successfully."];
        [alert runModal];
    }
}

// Main function
int main(int argc, const char * argv[]) {
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];
        
        FBNeoMetalApp *delegate = [[FBNeoMetalApp alloc] init];
        [app setDelegate:delegate];
        
        [app finishLaunching];
        [app run];
    }
    return 0;
} 