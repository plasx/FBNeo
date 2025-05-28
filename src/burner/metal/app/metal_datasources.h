#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

// Data source for the ROM browser outline view
@interface RomBrowserDataSource : NSObject <NSOutlineViewDataSource, NSOutlineViewDelegate>

// Game list data
@property (nonatomic, strong, readonly) NSMutableArray *availableRoms;
@property (nonatomic, strong, readonly) NSMutableArray *filteredRoms;
@property (nonatomic, strong) NSString *searchFilter;

// Initialize the data source with game data
- (instancetype)init;

// Get a game name at the specified index
- (NSString *)gameNameAtIndex:(NSInteger)index;

// Filter games by search text
- (void)filterGamesBySearchText:(NSString *)searchText;

@end

// Data source for memory viewer
@interface MemoryViewerDataSource : NSObject <NSTableViewDataSource, NSTableViewDelegate>
@property (nonatomic, strong) NSArray *memoryRegions;
@property (weak) NSTextView *hexTextView;
@end

// Data source for disassembler
@interface DisassemblerDataSource : NSObject <NSTableViewDataSource, NSTableViewDelegate>
@property (nonatomic, strong) NSArray *codeRegions;
@property (nonatomic, strong) NSArray *addresses;
@property (nonatomic, strong) NSArray *bytes;
@property (nonatomic, strong) NSArray *instructions;
@end 