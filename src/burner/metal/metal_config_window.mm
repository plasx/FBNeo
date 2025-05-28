#import <Cocoa/Cocoa.h>
#import <GameController/GameController.h>
#include "metal_renderer_defines.h"

// External C functions
extern "C" {
    int Metal_GetKeyMapping(int* mapping, int maxMappings);
    int Metal_SetKeyMapping(int* mapping, int mappingSize);
    int Metal_SaveKeyMappingForGame(const char* gameName);
    int Metal_LoadKeyMappingForGame(const char* gameName);
    
    int Metal_GetControllerCount();
    const char* Metal_GetControllerName(int controllerIndex);
    int Metal_GetControllerMapping(int controllerIndex, int* mapping, int maxMappings);
    int Metal_SetControllerMapping(int controllerIndex, int* mapping, int mappingSize);
    int Metal_SaveControllerMappingForGame(const char* gameName, int controllerIndex);
    int Metal_LoadControllerMappingForGame(const char* gameName, int controllerIndex);
    
    int Metal_SaveGlobalConfig();
    int Metal_LoadGlobalConfig();
    
    void Metal_SetShaderType(int shaderType);
    int Metal_GetShaderType();
}

// Key name lookup
static NSString* GetKeyName(int keyCode) {
    static NSDictionary *keyNames = nil;
    
    if (keyNames == nil) {
        keyNames = @{
            @(kVK_Return): @"Return",
            @(kVK_Tab): @"Tab",
            @(kVK_Space): @"Space",
            @(kVK_Delete): @"Delete",
            @(kVK_Escape): @"Escape",
            @(kVK_Command): @"Command",
            @(kVK_Shift): @"Shift",
            @(kVK_CapsLock): @"Caps Lock",
            @(kVK_Option): @"Option",
            @(kVK_Control): @"Control",
            @(kVK_RightCommand): @"Right Command",
            @(kVK_RightShift): @"Right Shift",
            @(kVK_RightOption): @"Right Option",
            @(kVK_RightControl): @"Right Control",
            @(kVK_LeftArrow): @"Left Arrow",
            @(kVK_RightArrow): @"Right Arrow", 
            @(kVK_DownArrow): @"Down Arrow",
            @(kVK_UpArrow): @"Up Arrow",
            @(kVK_ANSI_A): @"A",
            @(kVK_ANSI_B): @"B",
            @(kVK_ANSI_C): @"C",
            @(kVK_ANSI_D): @"D",
            @(kVK_ANSI_E): @"E",
            @(kVK_ANSI_F): @"F",
            @(kVK_ANSI_G): @"G",
            @(kVK_ANSI_H): @"H",
            @(kVK_ANSI_I): @"I",
            @(kVK_ANSI_J): @"J",
            @(kVK_ANSI_K): @"K",
            @(kVK_ANSI_L): @"L",
            @(kVK_ANSI_M): @"M",
            @(kVK_ANSI_N): @"N",
            @(kVK_ANSI_O): @"O",
            @(kVK_ANSI_P): @"P",
            @(kVK_ANSI_Q): @"Q",
            @(kVK_ANSI_R): @"R",
            @(kVK_ANSI_S): @"S",
            @(kVK_ANSI_T): @"T",
            @(kVK_ANSI_U): @"U",
            @(kVK_ANSI_V): @"V",
            @(kVK_ANSI_W): @"W",
            @(kVK_ANSI_X): @"X",
            @(kVK_ANSI_Y): @"Y",
            @(kVK_ANSI_Z): @"Z",
            @(kVK_ANSI_0): @"0",
            @(kVK_ANSI_1): @"1",
            @(kVK_ANSI_2): @"2",
            @(kVK_ANSI_3): @"3",
            @(kVK_ANSI_4): @"4",
            @(kVK_ANSI_5): @"5",
            @(kVK_ANSI_6): @"6",
            @(kVK_ANSI_7): @"7",
            @(kVK_ANSI_8): @"8",
            @(kVK_ANSI_9): @"9",
            @(kVK_F1): @"F1",
            @(kVK_F2): @"F2",
            @(kVK_F3): @"F3",
            @(kVK_F4): @"F4",
            @(kVK_F5): @"F5",
            @(kVK_F6): @"F6",
            @(kVK_F7): @"F7",
            @(kVK_F8): @"F8",
            @(kVK_F9): @"F9",
            @(kVK_F10): @"F10",
            @(kVK_F11): @"F11",
            @(kVK_F12): @"F12"
        };
    }
    
    return keyNames[@(keyCode)] ?: [NSString stringWithFormat:@"Key %d", keyCode];
}

// Controller button name lookup
static NSString* GetControllerButtonName(int buttonCode) {
    static NSDictionary *buttonNames = nil;
    
    if (buttonNames == nil) {
        buttonNames = @{
            @(FBNEO_KEY_UP): @"D-Pad Up",
            @(FBNEO_KEY_DOWN): @"D-Pad Down",
            @(FBNEO_KEY_LEFT): @"D-Pad Left",
            @(FBNEO_KEY_RIGHT): @"D-Pad Right",
            @(FBNEO_KEY_BUTTON1): @"Button 1",
            @(FBNEO_KEY_BUTTON2): @"Button 2",
            @(FBNEO_KEY_BUTTON3): @"Button 3",
            @(FBNEO_KEY_BUTTON4): @"Button 4",
            @(FBNEO_KEY_BUTTON5): @"Button 5",
            @(FBNEO_KEY_BUTTON6): @"Button 6",
            @(FBNEO_KEY_COIN): @"Coin",
            @(FBNEO_KEY_START): @"Start",
            @(FBNEO_KEY_SERVICE): @"Service",
            @(FBNEO_KEY_RESET): @"Reset",
            @(FBNEO_KEY_PAUSE): @"Pause",
            @(FBNEO_KEY_DIAGNOSTIC): @"Diagnostic",
            @(FBNEO_KEY_MENU): @"Menu",
            @(FBNEO_KEY_SAVE_STATE): @"Save State",
            @(FBNEO_KEY_LOAD_STATE): @"Load State",
            @(FBNEO_KEY_FAST_FORWARD): @"Fast Forward",
            @(FBNEO_KEY_FULLSCREEN): @"Fullscreen",
            @(FBNEO_KEY_SCREENSHOT): @"Screenshot",
            @(FBNEO_KEY_QUIT): @"Quit"
        };
    }
    
    return buttonNames[@(buttonCode)] ?: [NSString stringWithFormat:@"Button %d", buttonCode];
}

// Input Config Window Controller
@interface FBNeoInputConfigWindowController : NSWindowController <NSTableViewDataSource, NSTableViewDelegate, NSComboBoxDataSource>

// UI Elements
@property (nonatomic, weak) IBOutlet NSTabView *tabView;
@property (nonatomic, weak) IBOutlet NSTableView *keyboardTableView;
@property (nonatomic, weak) IBOutlet NSTableView *controllerTableView;
@property (nonatomic, weak) IBOutlet NSComboBox *controllerSelectionBox;
@property (nonatomic, weak) IBOutlet NSPopUpButton *shaderPopupButton;
@property (nonatomic, weak) IBOutlet NSButton *saveButton;
@property (nonatomic, weak) IBOutlet NSButton *cancelButton;
@property (nonatomic, weak) IBOutlet NSButton *resetButton;
@property (nonatomic, weak) IBOutlet NSButton *saveAsGameConfigButton;
@property (nonatomic, weak) IBOutlet NSTextField *gameNameField;

// Data
@property (nonatomic, strong) NSMutableArray *keyboardMappings;
@property (nonatomic, strong) NSMutableArray *controllerMappings;
@property (nonatomic, strong) NSMutableArray *availableControllers;
@property (nonatomic, assign) int selectedControllerIndex;
@property (nonatomic, strong) NSString *currentGameName;
@property (nonatomic, assign) BOOL isWaitingForKeyInput;
@property (nonatomic, assign) int waitingForKeyRow;
@property (nonatomic, assign) BOOL isGameSpecificConfig;

// Actions
- (IBAction)saveButtonClicked:(id)sender;
- (IBAction)cancelButtonClicked:(id)sender;
- (IBAction)resetButtonClicked:(id)sender;
- (IBAction)saveAsGameConfigButtonClicked:(id)sender;
- (IBAction)shaderSelectionChanged:(id)sender;
- (IBAction)controllerSelectionChanged:(id)sender;

// Initialize with game name (optional)
- (id)initWithGameName:(NSString*)gameName;

@end

@implementation FBNeoInputConfigWindowController

- (id)init {
    return [self initWithGameName:nil];
}

- (id)initWithGameName:(NSString*)gameName {
    self = [super initWithWindowNibName:@"InputConfigWindow"];
    if (self) {
        _keyboardMappings = [NSMutableArray array];
        _controllerMappings = [NSMutableArray array];
        _availableControllers = [NSMutableArray array];
        _selectedControllerIndex = -1;
        _currentGameName = [gameName copy];
        _isWaitingForKeyInput = NO;
        _waitingForKeyRow = -1;
        _isGameSpecificConfig = (gameName != nil);
        
        // Load available controllers
        [self loadAvailableControllers];
        
        // Load keyboard mappings
        [self loadKeyboardMappings];
        
        // Load controller mappings if a controller is connected
        if (_selectedControllerIndex >= 0) {
            [self loadControllerMappings];
        }
    }
    return self;
}

- (void)windowDidLoad {
    [super windowDidLoad];
    
    // Set window title based on game config or global config
    if (_isGameSpecificConfig) {
        [self.window setTitle:[NSString stringWithFormat:@"Input Configuration for %@", _currentGameName]];
        self.gameNameField.stringValue = _currentGameName;
    } else {
        [self.window setTitle:@"Global Input Configuration"];
        self.gameNameField.hidden = YES;
        self.saveAsGameConfigButton.hidden = YES;
    }
    
    // Set up shader selection popup
    [self.shaderPopupButton removeAllItems];
    [self.shaderPopupButton addItemWithTitle:@"Basic"];
    [self.shaderPopupButton addItemWithTitle:@"CRT"];
    [self.shaderPopupButton addItemWithTitle:@"Scanlines"];
    [self.shaderPopupButton addItemWithTitle:@"HQ2X"];
    
    // Select current shader
    int currentShader = Metal_GetShaderType();
    [self.shaderPopupButton selectItemAtIndex:currentShader];
    
    // Register for keyboard events
    [self.window makeFirstResponder:self];
}

- (void)loadAvailableControllers {
    [_availableControllers removeAllObjects];
    
    int controllerCount = Metal_GetControllerCount();
    
    // Add all available controllers
    for (int i = 0; i < controllerCount; i++) {
        const char* name = Metal_GetControllerName(i);
        if (name) {
            NSString *controllerName = [NSString stringWithUTF8String:name];
            [_availableControllers addObject:@{
                @"name": controllerName,
                @"index": @(i)
            }];
        }
    }
    
    // Set initial selection
    if (_availableControllers.count > 0) {
        _selectedControllerIndex = [_availableControllers[0][@"index"] intValue];
    } else {
        _selectedControllerIndex = -1;
    }
}

- (void)loadKeyboardMappings {
    [_keyboardMappings removeAllObjects];
    
    // Define the keys we want to map
    NSArray *keysToConfigure = @[
        @{@"code": @(FBNEO_KEY_UP), @"name": @"D-Pad Up"},
        @{@"code": @(FBNEO_KEY_DOWN), @"name": @"D-Pad Down"},
        @{@"code": @(FBNEO_KEY_LEFT), @"name": @"D-Pad Left"},
        @{@"code": @(FBNEO_KEY_RIGHT), @"name": @"D-Pad Right"},
        @{@"code": @(FBNEO_KEY_BUTTON1), @"name": @"Button 1"},
        @{@"code": @(FBNEO_KEY_BUTTON2), @"name": @"Button 2"},
        @{@"code": @(FBNEO_KEY_BUTTON3), @"name": @"Button 3"},
        @{@"code": @(FBNEO_KEY_BUTTON4), @"name": @"Button 4"},
        @{@"code": @(FBNEO_KEY_BUTTON5), @"name": @"Button 5"},
        @{@"code": @(FBNEO_KEY_BUTTON6), @"name": @"Button 6"},
        @{@"code": @(FBNEO_KEY_COIN), @"name": @"Coin"},
        @{@"code": @(FBNEO_KEY_START), @"name": @"Start"},
        @{@"code": @(FBNEO_KEY_PAUSE), @"name": @"Pause"},
        @{@"code": @(FBNEO_KEY_FULLSCREEN), @"name": @"Fullscreen"},
        @{@"code": @(FBNEO_KEY_SCREENSHOT), @"name": @"Screenshot"},
        @{@"code": @(FBNEO_KEY_QUIT), @"name": @"Quit"}
    ];
    
    // Get current keyboard mapping
    int maxMappings = (int)keysToConfigure.count * 2; // Each mapping is a pair
    int *mappings = (int*)malloc(maxMappings * sizeof(int));
    memset(mappings, 0, maxMappings * sizeof(int));
    
    int numMappings = Metal_GetKeyMapping(mappings, maxMappings);
    
    // Create mapping dictionary
    NSMutableDictionary *mappingDict = [NSMutableDictionary dictionary];
    for (int i = 0; i < numMappings; i += 2) {
        int fbKey = mappings[i];
        int macKey = mappings[i+1];
        mappingDict[@(fbKey)] = @(macKey);
    }
    
    // Create entries for each key
    for (NSDictionary *keyInfo in keysToConfigure) {
        NSNumber *keyCode = keyInfo[@"code"];
        NSNumber *mappedKey = mappingDict[keyCode];
        int macKeyCode = mappedKey ? [mappedKey intValue] : 0;
        
        [_keyboardMappings addObject:@{
            @"code": keyCode,
            @"name": keyInfo[@"name"],
            @"mappedKey": @(macKeyCode),
            @"mappedKeyName": GetKeyName(macKeyCode)
        }];
    }
    
    free(mappings);
}

- (void)loadControllerMappings {
    [_controllerMappings removeAllObjects];
    
    if (_selectedControllerIndex < 0) {
        return;
    }
    
    // Define the buttons we want to map
    NSArray *buttonsToConfigure = @[
        @{@"code": @(FBNEO_KEY_UP), @"name": @"D-Pad Up"},
        @{@"code": @(FBNEO_KEY_DOWN), @"name": @"D-Pad Down"},
        @{@"code": @(FBNEO_KEY_LEFT), @"name": @"D-Pad Left"},
        @{@"code": @(FBNEO_KEY_RIGHT), @"name": @"D-Pad Right"},
        @{@"code": @(FBNEO_KEY_BUTTON1), @"name": @"Button 1"},
        @{@"code": @(FBNEO_KEY_BUTTON2), @"name": @"Button 2"},
        @{@"code": @(FBNEO_KEY_BUTTON3), @"name": @"Button 3"},
        @{@"code": @(FBNEO_KEY_BUTTON4), @"name": @"Button 4"},
        @{@"code": @(FBNEO_KEY_BUTTON5), @"name": @"Button 5"},
        @{@"code": @(FBNEO_KEY_BUTTON6), @"name": @"Button 6"},
        @{@"code": @(FBNEO_KEY_COIN), @"name": @"Coin"},
        @{@"code": @(FBNEO_KEY_START), @"name": @"Start"}
    ];
    
    // Get current controller mapping
    int maxMappings = (int)buttonsToConfigure.count * 2; // Each mapping is a pair
    int *mappings = (int*)malloc(maxMappings * sizeof(int));
    memset(mappings, 0, maxMappings * sizeof(int));
    
    int numMappings = Metal_GetControllerMapping(_selectedControllerIndex, mappings, maxMappings);
    
    // Create mapping dictionary
    NSMutableDictionary *mappingDict = [NSMutableDictionary dictionary];
    for (int i = 0; i < numMappings; i += 2) {
        int fbButton = mappings[i];
        int controllerButton = mappings[i+1];
        mappingDict[@(fbButton)] = @(controllerButton);
    }
    
    // Create entries for each button
    for (NSDictionary *buttonInfo in buttonsToConfigure) {
        NSNumber *buttonCode = buttonInfo[@"code"];
        NSNumber *mappedButton = mappingDict[buttonCode];
        int controllerButtonCode = mappedButton ? [mappedButton intValue] : 0;
        
        [_controllerMappings addObject:@{
            @"code": buttonCode,
            @"name": buttonInfo[@"name"],
            @"mappedButton": @(controllerButtonCode),
            @"mappedButtonName": GetControllerButtonName(controllerButtonCode)
        }];
    }
    
    free(mappings);
}

- (IBAction)saveButtonClicked:(id)sender {
    // Save keyboard configuration
    int numKeyMappings = (int)_keyboardMappings.count * 2;
    int *keyMappings = (int*)malloc(numKeyMappings * sizeof(int));
    
    for (int i = 0; i < _keyboardMappings.count; i++) {
        NSDictionary *mapping = _keyboardMappings[i];
        keyMappings[i*2] = [mapping[@"code"] intValue];
        keyMappings[i*2+1] = [mapping[@"mappedKey"] intValue];
    }
    
    Metal_SetKeyMapping(keyMappings, numKeyMappings);
    free(keyMappings);
    
    // Save controller configuration if available
    if (_selectedControllerIndex >= 0) {
        int numControllerMappings = (int)_controllerMappings.count * 2;
        int *controllerMappings = (int*)malloc(numControllerMappings * sizeof(int));
        
        for (int i = 0; i < _controllerMappings.count; i++) {
            NSDictionary *mapping = _controllerMappings[i];
            controllerMappings[i*2] = [mapping[@"code"] intValue];
            controllerMappings[i*2+1] = [mapping[@"mappedButton"] intValue];
        }
        
        Metal_SetControllerMapping(_selectedControllerIndex, controllerMappings, numControllerMappings);
        free(controllerMappings);
    }
    
    // Save shader setting
    Metal_SetShaderType((int)self.shaderPopupButton.indexOfSelectedItem);
    
    // Save as game-specific configuration if needed
    if (_isGameSpecificConfig && _currentGameName) {
        const char *gameName = [_currentGameName UTF8String];
        Metal_SaveKeyMappingForGame(gameName);
        
        if (_selectedControllerIndex >= 0) {
            Metal_SaveControllerMappingForGame(gameName, _selectedControllerIndex);
        }
    } else {
        // Save global configuration
        Metal_SaveGlobalConfig();
    }
    
    [self.window close];
}

- (IBAction)cancelButtonClicked:(id)sender {
    [self.window close];
}

- (IBAction)resetButtonClicked:(id)sender {
    // Reset to defaults
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:@"Reset configuration?"];
    [alert setInformativeText:@"Are you sure you want to reset to default configuration?"];
    [alert addButtonWithTitle:@"Reset"];
    [alert addButtonWithTitle:@"Cancel"];
    
    NSModalResponse response = [alert runModal];
    
    if (response == NSAlertFirstButtonReturn) {
        // Reload with default configurations
        [self loadKeyboardMappings];
        [self loadControllerMappings];
        [self.keyboardTableView reloadData];
        [self.controllerTableView reloadData];
    }
}

- (IBAction)saveAsGameConfigButtonClicked:(id)sender {
    NSString *gameName = [self.gameNameField stringValue];
    
    if ([gameName length] == 0) {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"Game name required"];
        [alert setInformativeText:@"Please enter a name for the game configuration."];
        [alert runModal];
        return;
    }
    
    _currentGameName = gameName;
    _isGameSpecificConfig = YES;
    
    [self saveButtonClicked:sender];
}

- (IBAction)shaderSelectionChanged:(id)sender {
    // Set shader type (preview will be shown when applied)
}

- (IBAction)controllerSelectionChanged:(id)sender {
    if (_controllerSelectionBox.indexOfSelectedItem >= 0 && 
        _controllerSelectionBox.indexOfSelectedItem < _availableControllers.count) {
        _selectedControllerIndex = [_availableControllers[_controllerSelectionBox.indexOfSelectedItem][@"index"] intValue];
        [self loadControllerMappings];
        [self.controllerTableView reloadData];
    }
}

#pragma mark - NSTableViewDataSource

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    if (tableView == self.keyboardTableView) {
        return _keyboardMappings.count;
    } else if (tableView == self.controllerTableView) {
        return _controllerMappings.count;
    }
    return 0;
}

- (id)tableView:(NSTableView *)tableView objectValueForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
    if (tableView == self.keyboardTableView) {
        if (row < 0 || row >= _keyboardMappings.count) {
            return nil;
        }
        
        NSDictionary *mapping = _keyboardMappings[row];
        
        if ([[tableColumn identifier] isEqualToString:@"keyName"]) {
            return mapping[@"name"];
        } else if ([[tableColumn identifier] isEqualToString:@"keyMapping"]) {
            if (_isWaitingForKeyInput && _waitingForKeyRow == row) {
                return @"Press a key...";
            }
            return mapping[@"mappedKeyName"];
        }
    } else if (tableView == self.controllerTableView) {
        if (row < 0 || row >= _controllerMappings.count) {
            return nil;
        }
        
        NSDictionary *mapping = _controllerMappings[row];
        
        if ([[tableColumn identifier] isEqualToString:@"buttonName"]) {
            return mapping[@"name"];
        } else if ([[tableColumn identifier] isEqualToString:@"buttonMapping"]) {
            return mapping[@"mappedButtonName"];
        }
    }
    
    return nil;
}

#pragma mark - NSTableViewDelegate

- (void)tableView:(NSTableView *)tableView didClickTableColumn:(NSTableColumn *)tableColumn {
    // Nothing special needed here
}

- (void)tableViewSelectionDidChange:(NSNotification *)notification {
    NSTableView *tableView = notification.object;
    
    if (tableView == self.keyboardTableView) {
        NSInteger row = [tableView selectedRow];
        if (row >= 0 && row < _keyboardMappings.count) {
            _isWaitingForKeyInput = YES;
            _waitingForKeyRow = (int)row;
            [self.keyboardTableView reloadData];
        }
    }
}

#pragma mark - NSComboBoxDataSource

- (NSInteger)numberOfItemsInComboBox:(NSComboBox *)comboBox {
    return _availableControllers.count;
}

- (id)comboBox:(NSComboBox *)comboBox objectValueForItemAtIndex:(NSInteger)index {
    if (index >= 0 && index < _availableControllers.count) {
        return _availableControllers[index][@"name"];
    }
    return nil;
}

#pragma mark - Keyboard and input handling

- (BOOL)keyDown:(NSEvent *)event {
    if (_isWaitingForKeyInput && _waitingForKeyRow >= 0 && _waitingForKeyRow < _keyboardMappings.count) {
        int keyCode = (int)event.keyCode;
        
        // Update the mapping
        NSMutableDictionary *mapping = [_keyboardMappings[_waitingForKeyRow] mutableCopy];
        mapping[@"mappedKey"] = @(keyCode);
        mapping[@"mappedKeyName"] = GetKeyName(keyCode);
        _keyboardMappings[_waitingForKeyRow] = mapping;
        
        // Reset waiting state
        _isWaitingForKeyInput = NO;
        _waitingForKeyRow = -1;
        
        // Reload the table
        [self.keyboardTableView reloadData];
        
        // Prevent further key processing
        return YES;
    }
    
    return NO;
}

- (void)keyDown:(NSEvent *)theEvent {
    if (![self keyDown:theEvent]) {
        [super keyDown:theEvent];
    }
}

@end

// C interface for showing config window
extern "C" {
    void Metal_ShowInputConfigWindow(const char* gameName) {
        @autoreleasepool {
            NSString *gameNameStr = gameName ? [NSString stringWithUTF8String:gameName] : nil;
            FBNeoInputConfigWindowController *controller = [[FBNeoInputConfigWindowController alloc] initWithGameName:gameNameStr];
            [controller showWindow:nil];
            
            // Run a temporary modal session
            NSWindow *window = [controller window];
            NSModalSession session = [NSApp beginModalSessionForWindow:window];
            
            for (;;) {
                if ([NSApp runModalSession:session] != NSModalResponseContinue)
                    break;
            }
            
            [NSApp endModalSession:session];
        }
    }
    
    void Metal_ShowInputConfigWindowWithTab(const char* gameName, const char* tabName) {
        @autoreleasepool {
            NSString *gameNameStr = gameName ? [NSString stringWithUTF8String:gameName] : nil;
            NSString *tabNameStr = tabName ? [NSString stringWithUTF8String:tabName] : @"keyboard";
            
            FBNeoInputConfigWindowController *controller = [[FBNeoInputConfigWindowController alloc] initWithGameName:gameNameStr];
            [controller showWindow:nil];
            
            // Select the specified tab
            if ([tabNameStr isEqualToString:@"controller"]) {
                [controller.tabView selectTabViewItemAtIndex:1];
            } else if ([tabNameStr isEqualToString:@"display"]) {
                [controller.tabView selectTabViewItemAtIndex:2];
            } else {
                // Default to keyboard tab
                [controller.tabView selectTabViewItemAtIndex:0];
            }
            
            // Run a temporary modal session
            NSWindow *window = [controller window];
            NSModalSession session = [NSApp beginModalSessionForWindow:window];
            
            for (;;) {
                if ([NSApp runModalSession:session] != NSModalResponseContinue)
                    break;
            }
            
            [NSApp endModalSession:session];
        }
    }
} 