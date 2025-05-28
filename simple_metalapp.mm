#import <Cocoa/Cocoa.h>
#import <MetalKit/MetalKit.h>
#import <objc/runtime.h>

// Simple app to demonstrate ROM browser functionality
@interface FBNeoSimpleApp : NSObject <NSApplicationDelegate, NSTableViewDataSource, NSTableViewDelegate>
@property (strong) NSWindow *window;
@property (strong) NSMutableArray *games;
@end

@implementation FBNeoSimpleApp

- (instancetype)init {
    self = [super init];
    if (self) {
        // Initialize games list
        _games = [NSMutableArray array];
        [_games addObject:@[@"Marvel vs Capcom: Clash of Super Heroes", @"Capcom", @"1998"]];
        [_games addObject:@[@"Street Fighter II: World Warrior", @"Capcom", @"1991"]];
        [_games addObject:@[@"Street Fighter Alpha 3", @"Capcom", @"1998"]];
        [_games addObject:@[@"Metal Slug 3", @"SNK", @"2000"]];
        [_games addObject:@[@"The King of Fighters '98", @"SNK", @"1998"]];
        [_games addObject:@[@"Samurai Shodown II", @"SNK", @"1994"]];
        [_games addObject:@[@"Mortal Kombat II", @"Midway", @"1993"]];
        [_games addObject:@[@"X-Men vs. Street Fighter", @"Capcom", @"1996"]];
        [_games addObject:@[@"Darkstalkers", @"Capcom", @"1994"]];
        [_games addObject:@[@"Aliens vs. Predator", @"Capcom", @"1994"]];
    }
    return self;
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    // Create main window
    NSRect frame = NSMakeRect(0, 0, 800, 600);
    NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable;
    self.window = [[NSWindow alloc] initWithContentRect:frame
                                              styleMask:style
                                                backing:NSBackingStoreBuffered
                                                  defer:NO];
    [self.window setTitle:@"FBNeo - Metal Implementation"];
    [self.window center];
    
    // Create a button to show ROM browser
    NSButton *openButton = [[NSButton alloc] initWithFrame:NSMakeRect(300, 280, 200, 40)];
    [openButton setTitle:@"Open ROM Browser"];
    [openButton setBezelStyle:NSBezelStyleRounded];
    [openButton setTarget:self];
    [openButton setAction:@selector(showRomBrowser:)];
    
    // Add button to window
    [[self.window contentView] addSubview:openButton];
    
    // Show window
    [self.window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
}

// Show ROM browser dialog
- (void)showRomBrowser:(id)sender {
    // Create browser window
    NSRect frame = NSMakeRect(0, 0, 800, 600);
    NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable;
    NSWindow *browserWindow = [[NSWindow alloc] initWithContentRect:frame
                                                         styleMask:style
                                                           backing:NSBackingStoreBuffered
                                                             defer:NO];
    [browserWindow setTitle:@"FBNeo - ROM Browser"];
    
    // Create split view
    NSSplitView *splitView = [[NSSplitView alloc] initWithFrame:NSMakeRect(0, 60, 800, 540)];
    [splitView setVertical:YES];
    splitView.dividerStyle = NSSplitViewDividerStyleThin;
    
    // Left panel - Hardware filters
    NSScrollView *filtersScrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 0, 200, 540)];
    NSTableView *filtersTableView = [[NSTableView alloc] initWithFrame:NSMakeRect(0, 0, 200, 540)];
    NSTableColumn *filterColumn = [[NSTableColumn alloc] initWithIdentifier:@"filter"];
    [filterColumn setWidth:180];
    [filterColumn setTitle:@"Hardware"];
    [filtersTableView addTableColumn:filterColumn];
    [filtersScrollView setDocumentView:filtersTableView];
    [filtersScrollView setBorderType:NSBezelBorder];
    [filtersScrollView setHasVerticalScroller:YES];
    [filtersScrollView setHasHorizontalScroller:NO];
    
    // Right panel - Game list
    NSScrollView *gamesScrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 0, 600, 540)];
    NSTableView *gamesTableView = [[NSTableView alloc] initWithFrame:NSMakeRect(0, 0, 600, 540)];
    
    NSTableColumn *gameColumn = [[NSTableColumn alloc] initWithIdentifier:@"game"];
    [gameColumn setWidth:300];
    [gameColumn setTitle:@"Game"];
    [gamesTableView addTableColumn:gameColumn];
    
    NSTableColumn *manufacturerColumn = [[NSTableColumn alloc] initWithIdentifier:@"manufacturer"];
    [manufacturerColumn setWidth:150];
    [manufacturerColumn setTitle:@"Manufacturer"];
    [gamesTableView addTableColumn:manufacturerColumn];
    
    NSTableColumn *yearColumn = [[NSTableColumn alloc] initWithIdentifier:@"year"];
    [yearColumn setWidth:80];
    [yearColumn setTitle:@"Year"];
    [gamesTableView addTableColumn:yearColumn];
    
    [gamesScrollView setDocumentView:gamesTableView];
    [gamesScrollView setBorderType:NSBezelBorder];
    [gamesScrollView setHasVerticalScroller:YES];
    [gamesScrollView setHasHorizontalScroller:YES];
    
    // Add views to split view
    [splitView addSubview:filtersScrollView];
    [splitView addSubview:gamesScrollView];
    [splitView setPosition:200 ofDividerAtIndex:0];
    
    // Hardware types for filter list
    NSArray *hardwareTypes = @[
        @"All Games",
        @"Capcom CPS-1", 
        @"Capcom CPS-2", 
        @"Capcom CPS-3", 
        @"Neo Geo", 
        @"Sega", 
        @"Taito", 
        @"Konami",
        @"Midway"
    ];
    
    // Store data in window
    objc_setAssociatedObject(browserWindow, "hardwareTypes", hardwareTypes, OBJC_ASSOCIATION_RETAIN);
    objc_setAssociatedObject(browserWindow, "games", self.games, OBJC_ASSOCIATION_RETAIN);
    objc_setAssociatedObject(browserWindow, "filteredGames", self.games, OBJC_ASSOCIATION_RETAIN);
    
    // Create buttons
    NSButton *cancelButton = [[NSButton alloc] initWithFrame:NSMakeRect(600, 15, 80, 30)];
    [cancelButton setTitle:@"Cancel"];
    [cancelButton setBezelStyle:NSBezelStyleRounded];
    [cancelButton setTarget:self];
    [cancelButton setAction:@selector(closeRomBrowser:)];
    
    NSButton *playButton = [[NSButton alloc] initWithFrame:NSMakeRect(700, 15, 80, 30)];
    [playButton setTitle:@"Play"];
    [playButton setBezelStyle:NSBezelStyleRounded];
    [playButton setTarget:self];
    [playButton setAction:@selector(loadSelectedRom:)];
    
    // Add to content view
    NSView *contentView = [browserWindow contentView];
    [contentView addSubview:splitView];
    [contentView addSubview:cancelButton];
    [contentView addSubview:playButton];
    
    // Set data sources
    filtersTableView.tag = 100; // Hardware filter table
    gamesTableView.tag = 200;   // Games table
    [filtersTableView setDataSource:self];
    [filtersTableView setDelegate:self];
    [gamesTableView setDataSource:self];
    [gamesTableView setDelegate:self];
    
    // Show browser window
    [browserWindow center];
    [browserWindow makeKeyAndOrderFront:nil];
}

// Close ROM browser
- (void)closeRomBrowser:(id)sender {
    NSWindow *window = [(NSButton *)sender window];
    [window close];
}

// Load selected ROM
- (void)loadSelectedRom:(id)sender {
    NSWindow *window = [(NSButton *)sender window];
    NSView *contentView = [window contentView];
    
    // Find the games table view
    NSTableView *tableView = nil;
    for (NSView *view in [contentView subviews]) {
        if ([view isKindOfClass:[NSSplitView class]]) {
            NSSplitView *splitView = (NSSplitView *)view;
            if (splitView.subviews.count >= 2) {
                NSScrollView *scrollView = splitView.subviews[1];
                if ([scrollView isKindOfClass:[NSScrollView class]]) {
                    NSView *docView = [scrollView documentView];
                    if ([docView isKindOfClass:[NSTableView class]] && [docView tag] == 200) {
                        tableView = (NSTableView *)docView;
                        break;
                    }
                }
            }
        }
    }
    
    if (!tableView) {
        NSLog(@"Error: Could not find games table");
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
    NSArray *filteredGames = objc_getAssociatedObject(window, "filteredGames");
    NSArray *gameInfo = filteredGames[selectedRow];
    NSString *gameName = gameInfo[0];
    
    // Close ROM browser
    [window close];
    
    // Simulate starting the game
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:@"Loading Game"];
    [alert setInformativeText:[NSString stringWithFormat:@"Starting emulation for: %@", gameName]];
    [alert runModal];
    
    // Update main window title
    [self.window setTitle:[NSString stringWithFormat:@"FBNeo - %@", gameName]];
}

#pragma mark - NSTableViewDataSource

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    NSWindow *window = [tableView window];
    if (!window) return 0;
    
    if (tableView.tag == 100) {
        // Hardware filter list
        NSArray *hardwareTypes = objc_getAssociatedObject(window, "hardwareTypes");
        return hardwareTypes ? hardwareTypes.count : 0;
    } else {
        // Games list
        NSArray *filteredGames = objc_getAssociatedObject(window, "filteredGames");
        return filteredGames ? filteredGames.count : 0;
    }
}

- (id)tableView:(NSTableView *)tableView objectValueForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
    NSWindow *window = [tableView window];
    if (!window) return nil;
    
    if (tableView.tag == 100) {
        // Hardware filter list
        NSArray *hardwareTypes = objc_getAssociatedObject(window, "hardwareTypes");
        if (!hardwareTypes || row >= hardwareTypes.count) return nil;
        return hardwareTypes[row];
    } else {
        // Games list
        NSArray *filteredGames = objc_getAssociatedObject(window, "filteredGames");
        if (!filteredGames || row >= filteredGames.count) return nil;
        
        NSArray *gameInfo = filteredGames[row];
        NSString *identifier = [tableColumn identifier];
        
        if ([identifier isEqualToString:@"game"]) {
            return gameInfo[0];
        } else if ([identifier isEqualToString:@"manufacturer"]) {
            return gameInfo[1];
        } else if ([identifier isEqualToString:@"year"]) {
            return gameInfo[2];
        }
    }
    
    return nil;
}

#pragma mark - NSTableViewDelegate

- (void)tableViewSelectionDidChange:(NSNotification *)notification {
    NSTableView *tableView = notification.object;
    
    // Only handle changes to hardware filter table
    if (tableView.tag != 100) return;
    
    NSInteger selectedRow = tableView.selectedRow;
    if (selectedRow < 0) return;
    
    NSWindow *window = [tableView window];
    if (!window) return;
    
    // Get hardware types and selected type
    NSArray *hardwareTypes = objc_getAssociatedObject(window, "hardwareTypes");
    if (!hardwareTypes || selectedRow >= hardwareTypes.count) return;
    
    NSString *selectedType = hardwareTypes[selectedRow];
    NSArray *allGames = objc_getAssociatedObject(window, "games");
    NSArray *filteredGames;
    
    // Filter games by selected hardware type
    if ([selectedType isEqualToString:@"All Games"]) {
        filteredGames = allGames;
    } else {
        NSMutableArray *result = [NSMutableArray array];
        
        for (NSArray *gameInfo in allGames) {
            NSString *manufacturer = gameInfo[1];
            
            if ([selectedType isEqualToString:@"Capcom CPS-1"] ||
                [selectedType isEqualToString:@"Capcom CPS-2"] ||
                [selectedType isEqualToString:@"Capcom CPS-3"]) {
                if ([manufacturer isEqualToString:@"Capcom"]) {
                    [result addObject:gameInfo];
                }
            } else if ([selectedType isEqualToString:@"Neo Geo"]) {
                if ([manufacturer isEqualToString:@"SNK"]) {
                    [result addObject:gameInfo];
                }
            } else if ([selectedType isEqualToString:@"Midway"]) {
                if ([manufacturer isEqualToString:@"Midway"]) {
                    [result addObject:gameInfo];
                }
            }
        }
        
        filteredGames = result;
    }
    
    // Store filtered games in window
    objc_setAssociatedObject(window, "filteredGames", filteredGames, OBJC_ASSOCIATION_RETAIN);
    
    // Find and reload games table
    for (NSView *view in [window.contentView subviews]) {
        if ([view isKindOfClass:[NSSplitView class]]) {
            NSSplitView *splitView = (NSSplitView *)view;
            if (splitView.subviews.count >= 2) {
                NSScrollView *scrollView = splitView.subviews[1];
                if ([scrollView isKindOfClass:[NSScrollView class]]) {
                    NSView *docView = [scrollView documentView];
                    if ([docView isKindOfClass:[NSTableView class]] && [docView tag] == 200) {
                        NSTableView *gamesTable = (NSTableView *)docView;
                        [gamesTable reloadData];
                        break;
                    }
                }
            }
        }
    }
}

@end

// Main function
int main(int argc, const char * argv[]) {
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        FBNeoSimpleApp *delegate = [[FBNeoSimpleApp alloc] init];
        [app setDelegate:delegate];
        [app run];
    }
    return 0;
} 