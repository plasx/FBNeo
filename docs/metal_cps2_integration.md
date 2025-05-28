# FBNeo Metal CPS2 Integration

This document provides technical details about the integration between the CPS2 driver and the Metal rendering pipeline in FBNeo for macOS. It covers the architecture, key components, and how the implementation handles various aspects of CPS2 emulation.

## Architecture Overview

The Metal CPS2 integration consists of several key components:

1. **CPS2 ROM Loader** - Handles loading and verification of CPS2 ROM files
2. **Metal CPS2 Renderer** - Manages rendering of CPS2 graphics using Metal
3. **Metal Bridge** - Connects the FBNeo core with Metal-specific implementations
4. **CPS2 Hooks** - Intercepts and redirects CPS2 driver calls to Metal implementations

```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│ FBNeo Core  │───►│  CPS Hooks  │───►│ Metal Bridge│
└─────────────┘    └─────────────┘    └─────────────┘
                         │                   │
                         ▼                   ▼
                  ┌─────────────┐    ┌─────────────┐
                  │ CPS2 Loader │    │CPS2 Renderer│
                  └─────────────┘    └─────────────┘
```

## Key Components

### CPS2 ROM Loader

The CPS2 ROM loader is responsible for:

- Loading CPS2 ROM sets from ZIP archives or directories
- Verifying ROM checksums to ensure integrity
- Managing the decryption of encrypted CPS2 ROMs
- Supporting various CPS2 ROM formats and region variants

**Key Files:**
- `src/burner/metal/cps2_rom_loader.h`
- `src/burner/metal/cps2_rom_loader.cpp`
- `src/burner/metal/rom_verify.cpp`

### Metal CPS2 Renderer

The Metal CPS2 renderer handles:

- Converting CPS2's 4-bit per channel palette (RRRRGGGGBBBB0000) to Metal's 8-bit RGBA format
- Managing texture creation and updates for CPS2 tile data
- Handling CPS2-specific rendering quirks and features
- Supporting various scaling options and filters

**Key Files:**
- `src/burner/metal/metal_cps2_renderer.h`
- `src/burner/metal/metal_cps2_renderer.cpp`

### Metal Bridge

The Metal bridge serves as an intermediary between:

- The FBNeo core's rendering calls and Metal
- Input handling between macOS and the emulator
- Frame timing and synchronization
- Audio processing and output

**Key Files:**
- `src/burner/metal/metal_bridge.h`
- `src/burner/metal/metal_bridge.cpp`

### CPS2 Hooks

The CPS2 hooks component:

- Intercepts calls to CPS2 driver functions
- Redirects drawing operations to the Metal renderer
- Provides custom implementations of CPS functions
- Handles CPS2-specific memory management

**Key Files:**
- `src/burner/metal/metal_cps2_hooks.cpp`

## Technical Implementation Details

### Palette Handling

CPS2 games use a specific palette format that needs conversion for Metal:

```cpp
// CPS2 colors use a 4-bit per channel format (RRRRGGGGBBBB0000)
// Metal requires ARGB 8-bit per channel
static UINT32 CPS2_ConvertPaletteEntry(UINT32 palEntry) {
    // Extract R, G, B components (4 bits each) from CPS 15-bit color
    int r = (palEntry >> 8) & 0xF;
    int g = (palEntry >> 4) & 0xF;
    int b = (palEntry >> 0) & 0xF;
    
    // Scale up to 8 bits per channel (multiply by 17 to get 0-255 range)
    r = (r * 17);
    g = (g * 17);
    b = (b * 17);
    
    // Return as Metal BGRA format (Alpha = 255)
    return (255 << 24) | (r << 16) | (g << 8) | b;
}
```

The palette conversion system monitors changes to the CPS2 palette and only updates the Metal texture when changes are detected, improving performance.

### Tile Rendering

CPS2 uses tile-based rendering with multiple layers:

1. The CPS2 driver decodes tile data into the scroll layers
2. The Metal renderer creates and maintains textures for these layers
3. When the driver calls drawing functions, Metal composites these layers

```cpp
// Example of Metal_CPS2_Render function (simplified)
INT32 Metal_CPS2_Render() {
    // Update palette if needed
    if (CpsPalUpdate) {
        Metal_CPS2_UpdatePalette();
    }
    
    // Process scroll layers
    for (int i = 0; i < 4; i++) {
        Metal_CPS2_ProcessScrollLayer(i, CpsrBase, nCpsGfxScroll[i]);
    }
    
    // Render sprites
    Metal_CPS2_ProcessSprites();
    
    // Composite all layers
    Metal_CPS2_CompositeLayers();
    
    return 0;
}
```

### ROM Decryption

CPS2 ROMs use encryption that must be handled during loading:

1. The ROM loader identifies the encryption type from the game info
2. Appropriate decryption keys are applied based on game region and version
3. Decrypted data is verified against expected checksums

```cpp
// Example of how decryption is handled (simplified)
bool CPS2_DecryptROM(const CPS2ROMInfo* info, UINT8* data, size_t size) {
    if (!info || !info->needsDecryption) {
        return true; // No decryption needed
    }
    
    switch (info->encryptionType) {
        case CPS2_ENCRYPTION_TYPE_A:
            return Decrypt_CPS2_TypeA(data, size, info->key);
        case CPS2_ENCRYPTION_TYPE_B:
            return Decrypt_CPS2_TypeB(data, size, info->key);
        // Other encryption types...
        default:
            return false;
    }
}
```

### Frame Buffer Processing

The Metal renderer needs to efficiently transfer frame data from the FBNeo core:

1. The core renders to an internal buffer (`pBurnDraw`)
2. The Metal renderer processes this data and creates a Metal texture
3. The texture is then rendered to the screen with appropriate filtering

```cpp
// Example of frame processing (simplified)
void Metal_CPS2_ProcessFrameBuffer() {
    if (!pBurnDraw) return;
    
    // Get dimensions
    int width = g_frameWidth;
    int height = g_frameHeight;
    
    // Process the frame buffer
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Get pixel from CPS2 buffer
            UINT16* src = (UINT16*)pBurnDraw;
            UINT16 pixel = src[y * (nBurnPitch / 2) + x];
            
            // Convert to RGBA for Metal
            UINT32 rgba = GetRGBAFromCPS2Format(pixel);
            
            // Write to Metal texture buffer
            g_frameBuffer[y * width + x] = rgba;
        }
    }
    
    // Update Metal texture with new frame data
    UpdateMetalTexture(g_frameBuffer, width, height);
}
```

## CPS2 Game Compatibility

The Metal implementation supports all standard CPS2 games, with specific handling for:

1. **Region Variants** - Supporting Japanese, USA, European, Asian versions
2. **Special Cases** - Games with unique rendering requirements
3. **ROM Format Variations** - Supporting different dump formats and naming conventions

### Special Cases

Some CPS2 games require custom handling:

```cpp
// Example of special case handling for specific games
void Metal_CPS2_ApplyGameSpecificFixes(const char* gameId) {
    if (strcmp(gameId, "ssf2t") == 0) {
        // Super Street Fighter II Turbo specific fix
        g_useAlternativePaletteMode = true;
    } else if (strcmp(gameId, "progear") == 0) {
        // Progear specific rendering fix
        g_enableExtraAlphaBlending = true;
    }
}
```

## Performance Considerations

The Metal CPS2 implementation includes several optimizations:

1. **Palette Tracking** - Only updating palette textures when changes are detected
2. **Layer Caching** - Avoiding re-rendering layers that haven't changed
3. **Async Texture Updates** - Using Metal's async compute capabilities
4. **Specialized Shaders** - Using optimized Metal shaders for CPS2 scaling and filtering

### Apple Silicon Optimizations

For Apple Silicon devices, further optimizations include:

1. Using the unified memory architecture to reduce texture transfer overhead
2. Leveraging Metal GPU Family 2+ features
3. Optimizing memory access patterns for the M1's cache design

## Testing and Validation

The Metal CPS2 integration is thoroughly tested with:

1. **Unit Tests** - For individual components like palette conversion
2. **Integration Tests** - To ensure proper interaction between components
3. **Performance Tests** - To identify and address bottlenecks
4. **Compatibility Tests** - With various CPS2 games to ensure correct rendering

## Debugging Tools

For developers working on the Metal CPS2 implementation, several debugging tools are available:

```cpp
// Enable CPS2 debug rendering (shows layer boundaries)
Metal_CPS2_EnableDebugRendering(true);

// Dump CPS2 palette to file
Metal_CPS2_DumpPalette("palette_dump.txt");

// Log CPS2 renderer performance metrics
Metal_CPS2_LogPerformanceMetrics(true);
```

## Future Improvements

Planned enhancements to the Metal CPS2 implementation include:

1. **Advanced Scaling Filters** - Adding CRT simulation and other advanced filters
2. **Performance Optimization** - Further tuning for M2/M3 chips
3. **Extended Compatibility** - Support for bootlegs and rare CPS2 variants

## References

- CPS2 Technical Reference: [CapcomCPS2Technical.pdf](https://github.com/finalburnneo/FBNeo/wiki/documentation/CapcomCPS2Technical.pdf)
- Metal Programming Guide: [Apple Developer Documentation](https://developer.apple.com/documentation/metal) 