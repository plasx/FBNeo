# FBNeo Metal Save State System

## Overview
This document describes the implementation of the save state system for the FBNeo Metal port on macOS, focusing on the ability to save and restore the emulation state for Marvel vs Capcom (CPS2).

## Features

- Save and load emulation states with multiple slots (1-10)
- Quick save/load functionality
- Keyboard shortcuts for saving and loading
- Automatic state verification with checksums
- Detailed save file metadata

## Save State Format

Each save state file contains:

1. **Header**:
   - Magic identifier: "FBNS" (FBNeo Save)
   - Version number
   - Data size
   - Checksum
   - Driver name
   - Timestamp

2. **State Data**:
   - Complete machine state compressed by the FBNeo core

## Key Functions

### Metal_SaveState(int slot)
- Saves the current emulation state to the specified slot (1-10)
- Uses BurnStateCompress to gather all state data
- Adds header with metadata
- Calculates checksum for verification
- Returns 0 on success, non-zero on failure

### Metal_LoadState(int slot)
- Loads an emulation state from the specified slot
- Verifies header and checksum
- Uses BurnStateDecompress to restore the state
- Refreshes the palette after loading
- Returns 0 on success, non-zero on failure

### Metal_QuickSave() / Metal_QuickLoad()
- Convenience functions that save/load to the current slot
- Default slot is 1

## Keyboard Shortcuts

- **F5**: Quick save to current slot
- **F8**: Quick load from current slot
- **⌘S**: Same as F5
- **⌘L**: Same as F8

## Save File Location

Save states are stored in:
```
~/Documents/FBNeoSaves/
```

Files are named using the format:
```
[game_name].state[slot_number]
```

For example:
```
~/Documents/FBNeoSaves/mvsc.state1
```

## Implementation Details

1. **BurnStateCompress/BurnStateDecompress**: These core FBNeo functions handle the gathering and restoration of all emulation state data. They use BurnAreaScan to enumerate all volatile memory regions that need saving.

2. **Checksum Verification**: Each save file includes a checksum calculated from the state data, which is verified on load to ensure the file hasn't been corrupted.

3. **Driver Compatibility**: The save system checks that states are loaded into the same driver they were saved from, preventing mismatched states.

4. **Error Handling**: Comprehensive error checking at each step of the save/load process with detailed error messages.

## Future Enhancements

1. User interface for slot selection and state management

2. State previews with thumbnails

3. Support for state compression to reduce file size

4. Auto-save functionality

5. Cross-platform save state compatibility

## How to Use

1. **To save a state**:
   - Press F5 or Command+S during gameplay
   - The state will be saved to the current slot (default: slot 1)

2. **To load a state**:
   - Press F8 or Command+L during gameplay
   - The state will be loaded from the current slot

3. **To change slots**:
   - Currently only possible through code (Metal_SetSaveSlot function)
   - Future updates will add UI for slot selection 