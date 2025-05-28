#include "ai/metal_ai_module.h"
#include <Cocoa/Cocoa.h>

// Add this interface definition
@interface MetalUIController : NSObject
- (void)createMenu;
- (void)addAIMenu;
- (void)toggleAI:(id)sender;
- (void)toggleAITraining:(id)sender;
- (void)saveAIModel:(id)sender;
- (void)loadAIModel:(id)sender;
- (void)exportAIModelToCoreML:(id)sender;
- (void)configureDistributedTraining:(id)sender;
- (void)createGameMemoryMapping:(id)sender;
- (void)optimizeCoreMLModel:(id)sender;
@end

@implementation MetalUIController

// Add a new menu item for AI settings
- (void)addAIMenu {
    // Create AI submenu
    NSMenu* aiMenu = [[NSMenu alloc] initWithTitle:@"AI"];
    
    // Toggle AI
    NSMenuItem* toggleAI = [[NSMenuItem alloc] initWithTitle:@"Enable AI" 
                                                      action:@selector(toggleAI:) 
                                               keyEquivalent:@"a"];
    [toggleAI setTarget:self];
    [aiMenu addItem:toggleAI];
    
    // Toggle AI training
    NSMenuItem* toggleTraining = [[NSMenuItem alloc] initWithTitle:@"Enable Training Mode" 
                                                           action:@selector(toggleAITraining:) 
                                                    keyEquivalent:@"t"];
    [toggleTraining setTarget:self];
    [aiMenu addItem:toggleTraining];
    
    [aiMenu addItem:[NSMenuItem separatorItem]];
    
    // Save model
    NSMenuItem* saveModel = [[NSMenuItem alloc] initWithTitle:@"Save AI Model" 
                                                       action:@selector(saveAIModel:) 
                                                keyEquivalent:@"s"];
    [saveModel setTarget:self];
    [aiMenu addItem:saveModel];
    
    // Load model
    NSMenuItem* loadModel = [[NSMenuItem alloc] initWithTitle:@"Load AI Model" 
                                                       action:@selector(loadAIModel:) 
                                                keyEquivalent:@"l"];
    [loadModel setTarget:self];
    [aiMenu addItem:loadModel];
    
    [aiMenu addItem:[NSMenuItem separatorItem]];
    
    // Export to CoreML
    NSMenuItem* exportCoreML = [[NSMenuItem alloc] initWithTitle:@"Export to CoreML" 
                                                         action:@selector(exportAIModelToCoreML:) 
                                                  keyEquivalent:@"e"];
    [exportCoreML setTarget:self];
    [aiMenu addItem:exportCoreML];
    
    // Optimize CoreML model
    NSMenuItem* optimizeCoreML = [[NSMenuItem alloc] initWithTitle:@"Optimize CoreML Model" 
                                                          action:@selector(optimizeCoreMLModel:) 
                                                   keyEquivalent:@"o"];
    [optimizeCoreML setTarget:self];
    [aiMenu addItem:optimizeCoreML];
    
    [aiMenu addItem:[NSMenuItem separatorItem]];
    
    // Configure distributed training
    NSMenuItem* configureTraining = [[NSMenuItem alloc] initWithTitle:@"Configure Distributed Training" 
                                                             action:@selector(configureDistributedTraining:) 
                                                      keyEquivalent:@"d"];
    [configureTraining setTarget:self];
    [aiMenu addItem:configureTraining];
    
    // Create game memory mapping
    NSMenuItem* createMapping = [[NSMenuItem alloc] initWithTitle:@"Create Game Memory Mapping" 
                                                         action:@selector(createGameMemoryMapping:) 
                                                  keyEquivalent:@"m"];
    [createMapping setTarget:self];
    [aiMenu addItem:createMapping];
    
    // Add AI menu to main menu
    NSMenuItem* aiMenuItem = [[NSMenuItem alloc] initWithTitle:@"AI" 
                                                       action:nil 
                                                keyEquivalent:@""];
    [aiMenuItem setSubmenu:aiMenu];
    [[NSApp mainMenu] addItem:aiMenuItem];
}

// Add this inside the createMenu function, just before the end
- (void)createMenu {
    // Add AI menu
    [self addAIMenu];
}

// Action handlers
- (void)toggleAI:(id)sender {
    fbneo::metal::ai::setEnabled(!fbneo::metal::ai::isEnabled());
    
    // Update menu item
    NSMenuItem* item = (NSMenuItem*)sender;
    if (fbneo::metal::ai::isEnabled()) {
        [item setTitle:@"Disable AI"];
    } else {
        [item setTitle:@"Enable AI"];
    }
}

- (void)toggleAITraining:(id)sender {
    fbneo::metal::ai::setTrainingMode(!fbneo::metal::ai::isTrainingMode());
    
    // Update menu item
    NSMenuItem* item = (NSMenuItem*)sender;
    if (fbneo::metal::ai::isTrainingMode()) {
        [item setTitle:@"Disable Training Mode"];
    } else {
        [item setTitle:@"Enable Training Mode"];
    }
}

- (void)saveAIModel:(id)sender {
    // Create save panel
    NSSavePanel* panel = [NSSavePanel savePanel];
    [panel setTitle:@"Save AI Model"];
    [panel setMessage:@"Choose a location to save the AI model."];
    [panel setNameFieldStringValue:[NSString stringWithFormat:@"%s.model", 
                                   fbneo::metal::ai::getGameType()]];
    [panel setAllowedFileTypes:@[@"model"]];
    
    // Show save panel
    [panel beginSheetModalForWindow:[NSApp mainWindow] completionHandler:^(NSInteger result) {
        if (result == NSModalResponseOK) {
            // Get file path
            NSString* path = [[panel URL] path];
            
            // Save model
            fbneo::metal::ai::saveModel([path UTF8String]);
        }
    }];
}

- (void)loadAIModel:(id)sender {
    // Create open panel
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    [panel setTitle:@"Load AI Model"];
    [panel setMessage:@"Choose an AI model to load."];
    [panel setAllowedFileTypes:@[@"model"]];
    
    // Show open panel
    [panel beginSheetModalForWindow:[NSApp mainWindow] completionHandler:^(NSInteger result) {
        if (result == NSModalResponseOK) {
            // Get file path
            NSString* path = [[panel URL] path];
            
            // Load model
            fbneo::metal::ai::loadModel([path UTF8String]);
        }
    }];
}

- (void)exportAIModelToCoreML:(id)sender {
    // Create save panel
    NSSavePanel* panel = [NSSavePanel savePanel];
    [panel setTitle:@"Export AI Model to CoreML"];
    [panel setMessage:@"Choose a location to export the CoreML model."];
    [panel setNameFieldStringValue:[NSString stringWithFormat:@"%s.mlmodel", 
                                   fbneo::metal::ai::getGameType()]];
    [panel setAllowedFileTypes:@[@"mlmodel"]];
    
    // Show save panel
    [panel beginSheetModalForWindow:[NSApp mainWindow] completionHandler:^(NSInteger result) {
        if (result == NSModalResponseOK) {
            // Get file path
            NSString* path = [[panel URL] path];
            
            // Export model
            fbneo::metal::ai::exportToCoreML([path UTF8String]);
        }
    }];
}

- (void)configureDistributedTraining:(id)sender {
    // Create alert with input fields
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:@"Configure Distributed Training"];
    [alert setInformativeText:@"Set parameters for distributed training:"];
    [alert addButtonWithTitle:@"OK"];
    [alert addButtonWithTitle:@"Cancel"];
    
    // Create text fields for input
    NSTextField *workersField = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 200, 24)];
    [workersField setPlaceholderString:@"Number of Workers (2-8)"];
    [workersField setStringValue:@"4"];
    
    NSTextField *syncIntervalField = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 200, 24)];
    [syncIntervalField setPlaceholderString:@"Sync Interval in Frames (10-1000)"];
    [syncIntervalField setStringValue:@"100"];
    
    NSTextField *learningRateField = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 200, 24)];
    [learningRateField setPlaceholderString:@"Learning Rate (0.0001-0.01)"];
    [learningRateField setStringValue:@"0.0003"];
    
    // Create accessory view to hold the text fields
    NSStackView *stackView = [[NSStackView alloc] initWithFrame:NSMakeRect(0, 0, 200, 72)];
    [stackView setOrientation:NSUserInterfaceLayoutOrientationVertical];
    [stackView addArrangedSubview:workersField];
    [stackView addArrangedSubview:syncIntervalField];
    [stackView addArrangedSubview:learningRateField];
    [stackView setSpacing:10.0];
    [alert setAccessoryView:stackView];
    
    // Show alert
    NSModalResponse response = [alert runModal];
    
    if (response == NSAlertFirstButtonReturn) {
        // Get values from text fields
        int numWorkers = [[workersField stringValue] intValue];
        int syncInterval = [[syncIntervalField stringValue] intValue];
        float learningRate = [[learningRateField stringValue] floatValue];
        
        // Validate input
        if (numWorkers < 2) numWorkers = 2;
        if (numWorkers > 8) numWorkers = 8;
        
        if (syncInterval < 10) syncInterval = 10;
        if (syncInterval > 1000) syncInterval = 1000;
        
        if (learningRate < 0.0001f) learningRate = 0.0001f;
        if (learningRate > 0.01f) learningRate = 0.01f;
        
        // Configure distributed training
        fbneo::metal::ai::configureDistributedTraining(numWorkers, syncInterval, learningRate);
    }
}

- (void)createGameMemoryMapping:(id)sender {
    // Create confirmation alert
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:@"Create Game Memory Mapping"];
    [alert setInformativeText:@"This will scan the game memory and create mappings for game state variables. Continue?"];
    [alert addButtonWithTitle:@"Yes"];
    [alert addButtonWithTitle:@"No"];
    
    // Show alert
    NSModalResponse response = [alert runModal];
    
    if (response == NSAlertFirstButtonReturn) {
        // Create memory mapping
        bool success = fbneo::metal::ai::createGameMemoryMapping();
        
        // Show result alert
        NSAlert *resultAlert = [[NSAlert alloc] init];
        if (success) {
            [resultAlert setMessageText:@"Memory Mapping Created"];
            [resultAlert setInformativeText:@"Game memory mapping was created successfully."];
        } else {
            [resultAlert setMessageText:@"Memory Mapping Failed"];
            [resultAlert setInformativeText:@"Failed to create game memory mapping. Check the console for details."];
        }
        [resultAlert addButtonWithTitle:@"OK"];
        [resultAlert runModal];
    }
}

- (void)optimizeCoreMLModel:(id)sender {
    // Create open panel for input model
    NSOpenPanel *inputPanel = [NSOpenPanel openPanel];
    [inputPanel setTitle:@"Select CoreML Model to Optimize"];
    [inputPanel setMessage:@"Choose a CoreML model (.mlmodel) to optimize."];
    [inputPanel setAllowedFileTypes:@[@"mlmodel"]];
    
    // Show open panel
    [inputPanel beginSheetModalForWindow:[NSApp mainWindow] completionHandler:^(NSInteger result) {
        if (result == NSModalResponseOK) {
            // Get input model path
            NSString *inputPath = [[inputPanel URL] path];
            
            // Create save panel for output model
            NSSavePanel *outputPanel = [NSSavePanel savePanel];
            [outputPanel setTitle:@"Save Optimized CoreML Model"];
            [outputPanel setMessage:@"Choose a location to save the optimized model."];
            [outputPanel setNameFieldStringValue:[[inputPath lastPathComponent] stringByReplacingOccurrencesOfString:@".mlmodel" 
                                                                                                          withString:@"_optimized.mlmodel"]];
            [outputPanel setAllowedFileTypes:@[@"mlmodel"]];
            
            // Show save panel
            [outputPanel beginSheetModalForWindow:[NSApp mainWindow] completionHandler:^(NSInteger outputResult) {
                if (outputResult == NSModalResponseOK) {
                    // Get output path
                    NSString *outputPath = [[outputPanel URL] path];
                    
                    // Create optimization options
                    NSAlert *optionsAlert = [[NSAlert alloc] init];
                    [optionsAlert setMessageText:@"Optimization Options"];
                    [optionsAlert setInformativeText:@"Select target compute unit:"];
                    [optionsAlert addButtonWithTitle:@"Optimize"];
                    [optionsAlert addButtonWithTitle:@"Cancel"];
                    
                    // Create popup button for target device
                    NSPopUpButton *targetDevicePopup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(0, 0, 200, 24)];
                    [targetDevicePopup addItemWithTitle:@"All Compute Units"];
                    [targetDevicePopup addItemWithTitle:@"CPU Only"];
                    [targetDevicePopup addItemWithTitle:@"GPU Only"];
                    [targetDevicePopup addItemWithTitle:@"Neural Engine Only"];
                    [optionsAlert setAccessoryView:targetDevicePopup];
                    
                    // Show options alert
                    NSModalResponse optionsResult = [optionsAlert runModal];
                    
                    if (optionsResult == NSAlertFirstButtonReturn) {
                        // Get selected target device
                        NSString *targetDevice;
                        switch ([targetDevicePopup indexOfSelectedItem]) {
                            case 1: targetDevice = @"CPU"; break;
                            case 2: targetDevice = @"GPU"; break;
                            case 3: targetDevice = @"ANE"; break;
                            default: targetDevice = @"ALL"; break;
                        }
                        
                        // Optimize model
                        bool success = fbneo::metal::ai::optimizeCoreMLForDevice(
                            [inputPath UTF8String],
                            [outputPath UTF8String],
                            [targetDevice UTF8String]
                        );
                        
                        // Show result alert
                        NSAlert *resultAlert = [[NSAlert alloc] init];
                        if (success) {
                            [resultAlert setMessageText:@"Model Optimized"];
                            [resultAlert setInformativeText:[NSString stringWithFormat:@"Model was optimized successfully for %@.", targetDevice]];
                        } else {
                            [resultAlert setMessageText:@"Optimization Failed"];
                            [resultAlert setInformativeText:@"Failed to optimize model. Check the console for details."];
                        }
                        [resultAlert addButtonWithTitle:@"OK"];
                        [resultAlert runModal];
                    }
                }
            }];
        }
    }];
}

@end

// Create a global instance of the UI controller to handle menu actions
static MetalUIController *gUIController = nil;

// External function to initialize the UI
extern "C" void Metal_UI_Init() {
    if (!gUIController) {
        gUIController = [[MetalUIController alloc] init];
        [gUIController createMenu];
    }
}

// External function to refresh the UI state
extern "C" void Metal_UI_Refresh() {
    // Update menu items based on current state
}

// Add this function to handle settings changes
extern "C" void Metal_UI_ApplySettings(const void* settings) {
    // Apply settings from the core
} 