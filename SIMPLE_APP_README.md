# FBNeo Metal ROM Browser Demo

This is a simplified implementation of the FBNeo (Final Burn Neo) ROM browser UI for macOS using the Metal graphics API. The demo showcases a functional ROM browser interface similar to the one found in the Windows version of FBNeo.

## Features

- Split-view interface with hardware filter categories
- Game list with details (name, manufacturer, year)
- Filter functionality to show games by hardware type
- Simulated game loading

## Getting Started

### Prerequisites
- macOS 10.14 or higher
- Xcode command line tools for compilation

### Building and Running

1. Build the application:
   ```
   ./build_simple_app.sh
   ```

2. Launch the application:
   ```
   ./run_simple_app.sh
   ```

## Using the ROM Browser

1. Click the "Open ROM Browser" button in the main window
2. The ROM browser window will open with a split view:
   - Left panel: Hardware filter categories
   - Right panel: Game list with name, manufacturer, and year
3. Select a hardware category to filter the game list
4. Select a game and click "Play" to simulate loading the game
5. Click "Cancel" to close the browser without selecting a game

## Implementation Details

This demo implements a simplified version of the Metal ROM browser UI with:

- NSTableViews for hardware filters and game list
- NSSplitView for the split-panel interface
- Hardware filter functionality that updates the game list dynamically
- Associated object storage for managing window data

## Next Steps

This demo provides the foundation for further development of the FBNeo Metal implementation:

1. Integration with the full FBNeo Metal application
2. Actual game loading and emulation
3. Additional UI features like search functionality
4. Preferences dialog implementation
5. Debug tools implementation

## Notes

The demo uses a predefined list of games for demonstration purposes. In a complete implementation, the ROM browser would scan directories for ROM files and display them based on an internal database or romsets. 