#import "metal_datasources.h"
#include "metal_wrappers.h"
#include "metal_bridge.h"
#include "burner_metal.h"
#include <stdio.h>
#import <Cocoa/Cocoa.h>
#include "../metal_driver_constants.h"  // Include this for DRV_* constants

// Replace DEBUG_LOG macro with printf
#define DEBUG_LOG printf

@implementation MemoryViewerDataSource

- (instancetype)init {
    DEBUG_LOG("MemoryViewerDataSource init called\n");
    self = [super init];
    if (self) {
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

@end

@implementation DisassemblerDataSource

- (instancetype)init {
    DEBUG_LOG("DisassemblerDataSource init called\n");
    self = [super init];
    if (self) {
        _codeRegions = @[
            @"Main CPU (0x000000)",
            @"Sound CPU (0x100000)",
            @"MCU (0x200000)",
            @"ROM (0x400000)"
        ];
        
        // Initialize with sample data
        NSMutableArray *addrs = [NSMutableArray array];
        NSMutableArray *bytesList = [NSMutableArray array];
        NSMutableArray *insts = [NSMutableArray array];
        
        [addrs addObject:@"0x000000"];
        [bytesList addObject:@"4E71"];
        [insts addObject:@"NOP"];
        
        _addresses = addrs;
        _bytes = bytesList;
        _instructions = insts;
    }
    return self;
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    if ([tableView.identifier isEqualToString:@"region"]) {
        return self.codeRegions.count;
    } else {
        return self.addresses.count;
    }
}

- (id)tableView:(NSTableView *)tableView objectValueForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
    if ([tableView.identifier isEqualToString:@"region"]) {
        if (row < 0 || row >= self.codeRegions.count) {
            return nil;
        }
        return self.codeRegions[row];
    } else {
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

@implementation RomBrowserDataSource {
    NSMutableArray *hardwareCategories;
    NSMutableArray *availableRoms;
    NSMutableArray *filteredRoms;
    NSMutableDictionary *expandedCategories;
    NSString *searchFilter;
}

@synthesize availableRoms = availableRoms;
@synthesize filteredRoms = filteredRoms;
@synthesize searchFilter = searchFilter;

- (instancetype)init {
    DEBUG_LOG("RomBrowserDataSource init called\n");
    self = [super init];
    if (self) {
        hardwareCategories = [NSMutableArray array];
        availableRoms = [NSMutableArray array];
        filteredRoms = [NSMutableArray array];
        expandedCategories = [NSMutableDictionary dictionary];
        searchFilter = @"";
        
        [self populateHardwareCategories];
        [self populateGameList];
        
        // Initialize filtered roms with all roms
        [filteredRoms addObjectsFromArray:availableRoms];
        
        DEBUG_LOG("RomBrowserDataSource initialization complete with %lu categories and %lu games\n", 
                 (unsigned long)hardwareCategories.count, (unsigned long)availableRoms.count);
    }
    return self;
}

- (void)populateHardwareCategories {
    DEBUG_LOG("populateHardwareCategories called\n");
    // Add hardware categories
    [hardwareCategories addObject:@"Arcade"];
    [hardwareCategories addObject:@"Neo Geo"];
    [hardwareCategories addObject:@"Capcom CPS-1"];
    [hardwareCategories addObject:@"Capcom CPS-2"];
    [hardwareCategories addObject:@"Capcom CPS-3"];
    [hardwareCategories addObject:@"Cave"];
    [hardwareCategories addObject:@"Data East"];
    [hardwareCategories addObject:@"Konami"];
    [hardwareCategories addObject:@"Sega"];
    
    // All categories start expanded by default
    for (NSString *category in hardwareCategories) {
        [expandedCategories setObject:@YES forKey:category];
    }
    
    DEBUG_LOG("Added %lu hardware categories\n", (unsigned long)hardwareCategories.count);
}

- (void)populateGameList {
    DEBUG_LOG("populateGameList called\n");
    
    // Get number of available drivers
    extern UINT32 nBurnDrvCount;
    
    DEBUG_LOG("Number of available drivers: %d\n", nBurnDrvCount);
    
    try {
        // Create game entries
        for (UINT32 i = 0; i < nBurnDrvCount; i++) {
            DEBUG_LOG("Processing driver %d\n", i);
            
            NSString *gameName = nil;
            NSString *manufacturer = nil;
            NSString *year = nil;
            
            @try {
                // Get game name
                extern UINT32 nBurnDrvActive;
                nBurnDrvActive = i;
                
                DEBUG_LOG("Setting active driver to %d\n", nBurnDrvActive);
                
                char *name = BurnDrvGetTextA_Metal(DRV_FULLNAME);
                if (name) {
                    gameName = [NSString stringWithUTF8String:name];
                    DEBUG_LOG("Game name: %s\n", name);
                } else {
                    gameName = [NSString stringWithFormat:@"Game %d", i+1];
                    DEBUG_LOG("No name found, using fallback: %s\n", [gameName UTF8String]);
                }
                
                // Get manufacturer
                char *mfg = BurnDrvGetTextA_Metal(DRV_MANUFACTURER);
                if (mfg) {
                    manufacturer = [NSString stringWithUTF8String:mfg];
                    DEBUG_LOG("Manufacturer: %s\n", mfg);
                } else {
                    manufacturer = @"Unknown";
                }
                
                // Get year
                char *yearText = BurnDrvGetTextA_Metal(DRV_DATE);
                if (yearText) {
                    year = [NSString stringWithUTF8String:yearText];
                    DEBUG_LOG("Year: %s\n", yearText);
                } else {
                    year = @"2022";
                }
                
                DEBUG_LOG("Creating game entry for %s\n", [gameName UTF8String]);
                
                // Add the game to the list with more metadata
                [availableRoms addObject:@{
                    @"name": gameName,
                    @"manufacturer": manufacturer,
                    @"year": year,
                    @"index": @(i),
                    @"shortName": [NSString stringWithUTF8String:BurnDrvGetTextA_Metal(DRV_NAME) ?: "unknown"],
                    @"comment": [NSString stringWithUTF8String:BurnDrvGetTextA_Metal(DRV_COMMENT) ?: ""]
                }];
            } @catch (NSException *exception) {
                DEBUG_LOG("Exception while processing driver %d: %s\n", i, [exception.reason UTF8String]);
            }
        }
    } catch (...) {
        DEBUG_LOG("C++ exception in populateGameList\n");
    }
    
    // Sort the games alphabetically
    [availableRoms sortUsingComparator:^NSComparisonResult(id obj1, id obj2) {
        return [obj1[@"name"] compare:obj2[@"name"]];
    }];
    
    DEBUG_LOG("populateGameList finished, found %ld games\n", availableRoms.count);
}

- (void)filterGamesBySearchText:(NSString *)searchText {
    DEBUG_LOG("Filtering games by search text: %s\n", [searchText UTF8String]);
    
    // Store the search filter
    searchFilter = searchText;
    
    // Clear filtered roms
    [filteredRoms removeAllObjects];
    
    // If search text is empty, show all roms
    if (!searchText || [searchText length] == 0) {
        [filteredRoms addObjectsFromArray:availableRoms];
        DEBUG_LOG("Empty search text, showing all %lu games\n", (unsigned long)filteredRoms.count);
        return;
    }
    
    // Convert search text to lowercase for case-insensitive search
    NSString *lowerSearch = [searchText lowercaseString];
    
    // Filter games by search text
    for (NSDictionary *game in availableRoms) {
        NSString *name = [game[@"name"] lowercaseString];
        NSString *manufacturer = [game[@"manufacturer"] lowercaseString];
        NSString *year = game[@"year"];
        NSString *shortName = [game[@"shortName"] lowercaseString];
        
        // Check if any field contains the search text
        if ([name containsString:lowerSearch] ||
            [manufacturer containsString:lowerSearch] ||
            [year containsString:lowerSearch] ||
            [shortName containsString:lowerSearch]) {
            [filteredRoms addObject:game];
        }
    }
    
    DEBUG_LOG("Found %lu games matching search text\n", (unsigned long)filteredRoms.count);
}

- (NSString *)gameNameAtIndex:(NSInteger)index {
    if (index >= 0 && index < filteredRoms.count) {
        return filteredRoms[index][@"name"];
    }
    return nil;
}

#pragma mark - NSOutlineViewDataSource

- (NSInteger)outlineView:(NSOutlineView *)outlineView numberOfChildrenOfItem:(id)item {
    DEBUG_LOG("outlineView:numberOfChildrenOfItem: called\n");
    
    if (item == nil) {
        // Root level - return the number of hardware categories
        return hardwareCategories.count;
    } else {
        // Category level - return the number of filtered ROMs
        // In a real implementation, you would filter by hardware type
        return filteredRoms.count;
    }
}

- (BOOL)outlineView:(NSOutlineView *)outlineView isItemExpandable:(id)item {
    DEBUG_LOG("outlineView:isItemExpandable: called\n");
    
    // Only hardware categories are expandable
    return [hardwareCategories containsObject:item];
}

- (id)outlineView:(NSOutlineView *)outlineView child:(NSInteger)index ofItem:(id)item {
    DEBUG_LOG("outlineView:child:ofItem: called\n");
    
    if (item == nil) {
        // Root level - return the hardware category
        return hardwareCategories[index];
    } else {
        // Category level - return the ROM
        return filteredRoms[index];
    }
}

- (id)outlineView:(NSOutlineView *)outlineView objectValueForTableColumn:(NSTableColumn *)tableColumn byItem:(id)item {
    DEBUG_LOG("outlineView:objectValueForTableColumn:byItem: called\n");
    
    if ([hardwareCategories containsObject:item]) {
        // It's a hardware category
        return item;
    } else {
        // It's a ROM
        return item[@"name"];
    }
}

#pragma mark - NSOutlineViewDelegate

- (BOOL)outlineView:(NSOutlineView *)outlineView isGroupItem:(id)item {
    // Return YES for hardware categories to show them as group items
    return [hardwareCategories containsObject:item];
}

- (BOOL)outlineView:(NSOutlineView *)outlineView shouldExpandItem:(id)item {
    // Check if this category should be expanded
    if ([hardwareCategories containsObject:item]) {
        return [expandedCategories[item] boolValue];
    }
    return NO;
}

- (void)outlineViewItemDidExpand:(NSNotification *)notification {
    // Record that this category is expanded
    id item = notification.userInfo[@"NSObject"];
    if ([hardwareCategories containsObject:item]) {
        [expandedCategories setObject:@YES forKey:item];
    }
}

- (void)outlineViewItemDidCollapse:(NSNotification *)notification {
    // Record that this category is collapsed
    id item = notification.userInfo[@"NSObject"];
    if ([hardwareCategories containsObject:item]) {
        [expandedCategories setObject:@NO forKey:item];
    }
}

#pragma mark - NSTableViewDataSource

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    DEBUG_LOG("numberOfRowsInTableView: called\n");
    return filteredRoms.count;
}

- (id)tableView:(NSTableView *)tableView objectValueForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
    DEBUG_LOG("tableView:objectValueForTableColumn:row: called for row %ld\n", row);
    
    if (row < 0 || row >= filteredRoms.count) {
        return nil;
    }
    
    NSDictionary *game = filteredRoms[row];
    NSString *columnIdentifier = tableColumn.identifier;
    
    if ([columnIdentifier isEqualToString:@"nameColumn"]) {
        return game[@"name"];
    } else if ([columnIdentifier isEqualToString:@"yearColumn"]) {
        return game[@"year"];
    } else if ([columnIdentifier isEqualToString:@"manufacturerColumn"]) {
        return game[@"manufacturer"];
    }
    
    return nil;
}

@end 