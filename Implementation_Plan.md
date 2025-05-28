# FBNeo Metal Implementation Plan

## Current Status

Based on the code analysis, the FBNeo Metal implementation is partially complete with some key components working but others still missing or broken:

1. **Working Components**:
   - Basic Metal renderer setup with shaders and texture display
   - Window management and UI integration
   - Test pattern generation
   - Frame buffer management
   - Basic keyboard input handling structure

2. **Problematic Areas**:
   - ROM loading/verification has compilation errors
   - External function linkage issues (missing extern "C" declarations)
   - Memory tracking system inconsistencies
   - Audio implementation is incomplete
   - Integration with FBNeo core is not fully connected

## Implementation Plan

### 1. Fix Build System Issues

1. **Fix Linkage Problems**:
   - Add extern "C" declarations to header files for Metal_LogMessage and Metal_SetLogLevel
   - Implement stub implementations for missing ROM utility functions

2. **Fix ROM Verification**:
   - Resolve conflicts between ROMVerificationResult types
   - Create a consistent API for ROM verification

### 2. Connect Core Components

1. **Establish Clear Layer Separation**:
   - Create a clean interface between Metal-specific code and FBNeo
   - Implement proper stub functions for CPS2 emulation

2. **Initialize Core Systems**:
   - Implement proper initialization sequence
   - Connect Metal renderer to FBNeo frame buffer
   - Ensure memory management is consistent

### 3. Game Emulation

1. **ROM Loading**:
   - Implement proper ROM path handling
   - Fix ZIP file reading functionality
   - Connect ROM loader to CPS2 driver initialization

2. **Frame Buffer Integration**:
   - Ensure frame buffer from emulation core is properly rendered
   - Implement format conversion as needed
   - Connect FBNeo drawing calls to Metal texture

### 4. Audio Implementation

1. **Audio System**:
   - Complete AVAudioEngine implementation
   - Connect FBNeo sound output to CoreAudio
   - Implement proper synchronization

2. **Input System**:
   - Finalize keyboard mapping
   - Implement input event processing for the emulator

### 5. Final Integration

1. **Game Loop**:
   - Implement proper 60fps timing
   - Synchronize audio and video
   - Add performance monitoring

2. **Error Handling**:
   - Add comprehensive error logging
   - Implement graceful failure modes
   - Create debug visualization

## Implementation Approach

The implementation will follow these key principles:

1. **Isolation**: Keep Metal-specific code separate from the core emulator
2. **Stub First**: Create functional stubs before full implementation
3. **Incremental Testing**: Test each component separately
4. **Compatibility**: Ensure compatibility with macOS conventions

## Next Steps

1. Build and run the simplified implementation first
2. Fix ROM loading and verification
3. Connect the renderer to real game data
4. Implement audio and input
5. Test with Marvel vs Capcom ROM

This approach will ensure a working implementation that can properly run Marvel vs Capcom and other CPS2 games on macOS using Metal. 