#import <Cocoa/Cocoa.h>
#import <MetalKit/MetalKit.h>

// Forward declarations
@class FBNeoMenuManager;
@class FBNeoMetalDelegate;

// Settings structure
#ifndef _FBNEO_SETTINGS_DEFINED
#define _FBNEO_SETTINGS_DEFINED

typedef struct {
    // Video settings
    BOOL fullscreen;
    int scalingMode;   // 0 = Normal, 1 = Integer Scale, 2 = Fill Screen
    int aspectRatio;   // 0 = Original, 1 = 4:3, 2 = 16:9
    BOOL scanlines;
    BOOL smoothing;    // Bilinear filtering
    int frameSkip;     // 0 = None, 1-10 = Skip N frames
    BOOL vsync;
    BOOL showFPS;
    
    // Audio settings
    BOOL audioEnabled;
    int volume;       // 0-100
    int sampleRate;   // 44100, 48000, etc.
    
    // Input settings
    BOOL autoFire;
    int autoFireRate; // Frames between autofire
    
    // Debug settings
    BOOL hitboxViewer;
    BOOL frameCounter;
    BOOL inputDisplay;
    
    // AI settings
    BOOL aiEnabled;
    int aiControlledPlayer; // 0 = None, 1 = P1, 2 = P2, 3 = Both
    int aiDifficulty; // 1-10 scale (1=Beginner, 10=Expert)
    BOOL aiTrainingMode;
    BOOL aiDebugOverlay;
    
    // Display mode (for test displays and debugging)
    int displayMode;  // 0 = Normal, 1 = RAM, 2 = VRAM
    
    // For backwards compatibility
    BOOL autoRun;
    int scanlineIntensity;
    int controllerType;
    BOOL enableSpeedHacks;
} FBNeoSettings;

#endif // _FBNEO_SETTINGS_DEFINED

// Menu manager class
@interface FBNeoMenuManager : NSObject

@property (nonatomic, assign) FBNeoSettings settings;
@property (nonatomic, strong) NSMutableArray *recentRoms;
@property (nonatomic, strong) NSMenuItem *gameMenuItem;
@property (nonatomic, strong) NSMenu *gameMenu;
@property (nonatomic, strong) NSMenuItem *recentRomsMenuItem;
@property (nonatomic, strong) NSMenu *recentRomsMenu;

// Initialization
- (instancetype)initWithAppDelegate:(FBNeoMetalDelegate *)delegate;

// Menu methods
- (void)createMainMenu;
- (void)updateRecentRomsMenu;
- (void)addRecentRom:(NSString *)romPath;
- (void)saveRecentRomsList;
- (void)loadRecentRomsList;
- (void)saveSettings;
- (void)loadSettings;
- (void)updateSettingsMenuItems;
- (void)setDisplayMode:(int)mode;
- (void)setVSync:(BOOL)enabled;
- (void)setScanlineIntensity:(int)intensity;
- (void)setControllerType:(int)type;

@end

// Main delegate class for the Metal implementation
@interface FBNeoMetalDelegate : NSObject <NSApplicationDelegate, MTKViewDelegate>
@property (strong) NSWindow *window;
@property (strong) MTKView *metalView;
@property (strong) id<MTLDevice> device;
@property (strong) id<MTLCommandQueue> commandQueue;
@property (strong) id<MTLRenderPipelineState> pipelineState;
@property (strong) id<MTLTexture> frameTexture;
@property (strong) id<MTLBuffer> vertexBuffer;
@property (strong) FBNeoMenuManager *menuManager;
@property (assign) BOOL isRunning;
@property (assign) BOOL isPaused;
@property (strong) NSString *currentROMPath;
@property (assign) BOOL gameLoaded;

// Initialize the Metal application
- (void)setupApplication;

// Menu action handlers
- (void)loadRom:(id)sender;
- (void)openRecentRom:(id)sender;
- (void)clearRecentRoms:(id)sender;
- (void)runGame:(id)sender;
- (void)pauseEmulation:(id)sender;
- (void)resetEmulation:(id)sender;
- (void)showRomInfo:(id)sender;
- (void)showPreferences:(id)sender;
- (void)showAbout:(id)sender;
- (void)setDisplayMode:(id)sender;
- (void)setScalingMode:(id)sender;
- (void)setAspectRatio:(id)sender;
- (void)toggleVSync:(id)sender;
- (void)showScanlineSettings:(id)sender;
- (void)toggleFPS:(id)sender;
- (void)toggleAudio:(id)sender;
- (void)setSampleRate:(id)sender;
- (void)setVolume:(id)sender;
- (void)configureControls:(id)sender;
- (void)setControllerType:(id)sender;
- (void)toggleAutofire:(id)sender;
- (void)showDipSwitches:(id)sender;
- (void)captureScreen:(id)sender;
- (void)recordVideo:(id)sender;
- (void)showCheats:(id)sender;
- (void)showGameGenie:(id)sender;
- (void)toggleSpeedHacks:(id)sender;
- (void)setFrameSkip:(id)sender;
- (void)showHelp:(id)sender;
- (void)showCompatibility:(id)sender;
- (void)showMemoryViewer:(id)sender;
- (void)showDisassembly:(id)sender;

// ROM loading and game management
- (BOOL)loadROMFile:(NSString *)path;
- (void)runLoadedGame;
- (void)resetLoadedGame;

// Rendering methods
- (void)setupRenderPipeline;
- (void)createFrameTexture;
- (void)createVertexBuffer;
- (void)updateFrameTexture:(const void *)frameData width:(NSUInteger)width height:(NSUInteger)height;

// ROM browser methods
- (void)showRomBrowserDialog;
- (void)closeRomBrowser:(NSButton *)sender;
- (void)playSelectedRom:(NSButton *)sender;
- (void)loadSelectedGame:(NSString *)gameName;

@end