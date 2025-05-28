# Training Overlay Design

## Overview
The Training Overlay system provides real-time visual feedback during gameplay, displaying hitboxes, frame data, input history, and state information. It's designed to integrate with the FBNeo Metal renderer and AIMemoryMapping system to provide a comprehensive training experience for both human players and AI agents.

## Architecture

### Components
1. **TrainingOverlay** - Central class managing all overlay components
2. **HitboxDisplay** - Handles visualization of attack and defense hitboxes
3. **FrameDataDisplay** - Shows frame advantage/disadvantage and move properties
4. **InputHistoryDisplay** - Displays recent inputs for both players
5. **StateInfoDisplay** - Shows current game state, player states, and values

### Integration Points
- **MetalRenderer** - For drawing primitives (rectangles, text)
- **AIMemoryMapping** - For accessing game state and hitbox data
- **Input System** - For capturing and recording input history
- **Game Loop** - For update and render timing

## Rendering Pipeline

### Approach
The overlay uses a deferred rendering approach where game data is collected during the update phase and rendered as a post-processing step after the main game render.

1. **Data Collection**
   - Poll AIMemoryMapping for current values
   - Process input events
   - Calculate derived metrics (frame advantage, etc.)

2. **Rendering**
   - Draw hitboxes with appropriate colors (attack=red, hurt=blue, etc.)
   - Render text for frame data and state information
   - Display input history with standardized icons

### Performance Considerations
- Use batched rendering for hitboxes
- Cache text rendering where possible
- Enable/disable individual overlay components
- Skip rendering for off-screen elements

## Hitbox Visualization

### Detection Methods
1. **Memory Mapping** - Direct access to hitbox coordinates
2. **Pattern Detection** - Identify hitboxes from sprite data when direct mapping unavailable
3. **Coordinate Transformation** - Convert game coordinates to screen space

### Visual Style
- Attack hitboxes: Red with 50% transparency
- Hurt/vulnerable hitboxes: Blue with 50% transparency
- Throw hitboxes: Green with 50% transparency
- Collision boxes: Yellow with 30% transparency

## Frame Data Display

### Data Sources
- Memory mapping for current animation state
- Input history for move detection
- Frame counting for advantage calculation

### Display Format
```
Move Name: [name]
Startup: X | Active: Y | Recovery: Z
On Hit: +/- N | On Block: +/- M
Properties: [Overhead/Low/Throw/Armor/Invincible]
```

## Input History Display

### Features
- Last 10 inputs per player
- Timestamp or frame counter
- Standard notation (↑, ↓, ←, →, LP, MP, HP, LK, MK, HK)
- Color coding for special moves and supers

### Layout
- Player 1: Left side, bottom-up
- Player 2: Right side, bottom-up
- Input icons sized appropriately for visibility

## State Information Display

### Categories
1. **Player State**
   - Health
   - Meter
   - Stun/Dizzy
   - Status effects
   
2. **Game State**
   - Round timer
   - Round count
   - Match state

3. **Character-Specific**
   - V-Trigger (SF5)
   - X-Factor (MvC3)
   - Groove selection (CvS2)

## Memory Mapping Integration

### Requirements
- Memory mappings must include hitbox location data
- Frame data requires animation state mappings
- Position and state mappings for coordinate transformation

### Example Mapping Extension
```json
{
  "hitboxes": {
    "p1_attack_box": {
      "address": "0xFF8400",
      "type": "hitbox_array",
      "count": 4,
      "format": "x,y,width,height"
    },
    "p1_hurt_box": {
      "address": "0xFF8500",
      "type": "hitbox_array",
      "count": 8,
      "format": "x,y,width,height"
    }
  },
  "frame_data": {
    "p1_move_startup": {
      "address": "0xFF9000",
      "type": "uint8"
    },
    "p1_move_active": {
      "address": "0xFF9001",
      "type": "uint8"
    },
    "p1_move_recovery": {
      "address": "0xFF9002",
      "type": "uint8"
    }
  }
}
```

## Configuration Options

### User Preferences
- Toggle individual overlays (hitboxes, frame data, input history, state info)
- Hitbox opacity and color customization
- Text size and position adjustment
- Input history length and notation style

### Game-Specific Settings
- Hitbox detection method
- Coordinate system adjustments
- Frame data display format

## Implementation Plan

### Phase 1: Core Framework
- Create TrainingOverlay class
- Implement basic rendering pipeline
- Setup configuration system

### Phase 2: Hitbox Visualization
- Implement hitbox detection and rendering
- Add support for multiple hitbox types
- Create coordinate transformation system

### Phase 3: Frame Data Display
- Build frame counter and advantage calculator
- Implement move recognition system
- Design frame data visualization

### Phase 4: Input History
- Create input capture and history tracking
- Design input visualization system
- Implement special move recognition

### Phase 5: State Information
- Implement state data collection
- Design flexible state display system
- Add game-specific state visualizations

## Usage Example

```cpp
// Initialize the training overlay
TrainingOverlay::getInstance()->initialize(metalRenderer, memoryMapping);

// Enable specific displays
TrainingOverlay::getInstance()->setHitboxDisplay(true);
TrainingOverlay::getInstance()->setFrameDataDisplay(true);
TrainingOverlay::getInstance()->setInputHistoryDisplay(true);
TrainingOverlay::getInstance()->setStateInfoDisplay(true);

// Update cycle
void gameLoop() {
    // Game logic
    game.update();
    
    // Update overlay data
    TrainingOverlay::getInstance()->update();
    
    // Render game
    renderer.render();
    
    // Render overlays
    TrainingOverlay::getInstance()->render();
}

// Input handling
void onInputEvent(int player, uint32_t inputs) {
    // Process for game
    game.handleInput(player, inputs);
    
    // Add to overlay history
    TrainingOverlay::getInstance()->addInputEvent(player, inputs);
}
``` 