// metal_cps2_renderer.cpp
// Metal-specific CPS2 rendering implementation

#include "metal_declarations.h"
#include "metal_bridge.h"
#include "burnint.h"
#include "cps.h"
#include "cps2_rom_loader.h"

// External references to FBNeo core variables
extern UINT8* pBurnDraw;
extern INT32 nBurnPitch;
extern INT32 nBurnBpp;
extern INT32 nBurnDrvActive;

// External references to CPS2 variables
extern UINT32* CpsPal;           // CPS Palette (6bpp)
extern UINT16* CpsrBase;         // Tile data base
extern INT32 nCpsGfxScroll[4];   // Scroll values
extern INT32 nCpstType;          // Type of tiles to draw
extern INT32 nCpstX;             // X coordinate to draw tiles at
extern INT32 nCpstY;             // Y coordinate to draw tiles at
extern INT32 nCpstTile;          // First tile to draw
extern INT32 nCpstFlip;          // Direction to draw tiles
extern UINT8* CpstOneDoX;        // Function to draw one line of tile
extern UINT8* CpsZRamF;          // Graphics buffer

// Metal-specific rendering variables
static UINT32* g_Metal_CPS2PaletteBuffer = NULL;     // Metal-friendly palette buffer
static int g_Metal_CPS2PaletteSize = 0;             // Size of the palette buffer
static bool g_Metal_CPS2PaletteUpdated = false;     // Flag to indicate when palette needs update

// Previous rendering functions to hook
static INT32 (*PrevCps2rRender)() = NULL;

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
    
    // Ensure values are clamped to valid range
    r = (r > 255) ? 255 : r;
    g = (g > 255) ? 255 : g;
    b = (b > 255) ? 255 : b;
    
    // Return as Metal BGRA format (Alpha = 255)
    return (255 << 24) | (r << 16) | (g << 8) | b;
}

// Initialize the CPS2 Metal renderer
int Metal_CPS2_InitRenderer() {
    printf("Metal_CPS2_InitRenderer: Initializing CPS2 renderer for Metal\n");
    
    // Clean up any previous allocation
    if (g_Metal_CPS2PaletteBuffer != NULL) {
        free(g_Metal_CPS2PaletteBuffer);
        g_Metal_CPS2PaletteBuffer = NULL;
    }
    
    // Allocate palette buffer (CPS2 games use a 1024-entry palette)
    g_Metal_CPS2PaletteSize = 1024;
    g_Metal_CPS2PaletteBuffer = (UINT32*)malloc(g_Metal_CPS2PaletteSize * sizeof(UINT32));
    
    if (g_Metal_CPS2PaletteBuffer == NULL) {
        printf("Metal_CPS2_InitRenderer: Failed to allocate palette buffer\n");
        g_Metal_CPS2PaletteSize = 0;
        return 1;
    }
    
    // Initialize palette with black
    memset(g_Metal_CPS2PaletteBuffer, 0, g_Metal_CPS2PaletteSize * sizeof(UINT32));
    
    // Store previous render function to chain-call
    if (Cps2rRender != NULL) {
        PrevCps2rRender = Cps2rRender;
    } else {
        printf("Metal_CPS2_InitRenderer: Warning - Cps2rRender function not found\n");
        PrevCps2rRender = NULL;
    }
    
    printf("Metal_CPS2_InitRenderer: CPS2 Metal renderer initialized\n");
    return 0;
}

// Clean up the CPS2 Metal renderer
void Metal_CPS2_ExitRenderer() {
    printf("Metal_CPS2_ExitRenderer: Shutting down CPS2 renderer\n");
    
    // Free palette buffer
    if (g_Metal_CPS2PaletteBuffer) {
        free(g_Metal_CPS2PaletteBuffer);
        g_Metal_CPS2PaletteBuffer = NULL;
        g_Metal_CPS2PaletteSize = 0;
    }
    
    // Reset function hooks
    PrevCps2rRender = NULL;
}

// Update the Metal-friendly palette buffer from CPS2 palette
void Metal_CPS2_UpdatePalette() {
    if (!CpsPal || !g_Metal_CPS2PaletteBuffer || g_Metal_CPS2PaletteSize <= 0) {
        return;
    }
    
    // Convert all palette entries to Metal format
    for (int i = 0; i < g_Metal_CPS2PaletteSize; i++) {
        g_Metal_CPS2PaletteBuffer[i] = CPS2_ConvertPaletteEntry(CpsPal[i]);
    }
    
    g_Metal_CPS2PaletteUpdated = true;
}

// Apply special effects to the palette (fading, flashing, etc)
void Metal_CPS2_ApplyPaletteEffects() {
    // Get the current game
    const CPS2ROMInfo* romInfo = CPS2_GetROMInfo();
    if (!romInfo) {
        return;
    }
    
    // Apply game-specific palette effects
    switch (romInfo->hardwareType) {
        case CPS2_HW_MARVEL:
            // Marvel games often use palette rotation for special effects
            // Add implementation for Marvel-specific effects
            break;
            
        case CPS2_HW_VAMPIRE:
            // Vampire/Darkstalkers games use palette fading for special moves
            // Add implementation for Vampire-specific effects
            break;
            
        default:
            // Standard CPS2 palette handling
            break;
    }
}

// CPS2 rendering hook for Metal
INT32 Metal_CPS2_Render() {
    // Call the original rendering function first
    INT32 result = 0;
    if (PrevCps2rRender) {
        result = PrevCps2rRender();
        if (result != 0) {
            printf("Metal_CPS2_Render: Original render function returned error %d\n", result);
            // Continue anyway to try our rendering
        }
    }
    
    // Update the Metal palette from CPS2 palette
    Metal_CPS2_UpdatePalette();
    
    // Apply any special palette effects
    Metal_CPS2_ApplyPaletteEffects();
    
    // Set the flag to indicate that rendering is complete
    // and the Metal texture should be updated
    extern void Metal_SetFrameRendered(bool rendered);
    Metal_SetFrameRendered(true);
    
    return result;
}

// Get the Metal-friendly palette buffer
UINT32* Metal_CPS2_GetPaletteBuffer() {
    return g_Metal_CPS2PaletteBuffer;
}

// Check if the palette has been updated
bool Metal_CPS2_IsPaletteUpdated() {
    bool result = g_Metal_CPS2PaletteUpdated;
    g_Metal_CPS2PaletteUpdated = false;
    return result;
}

// Handle screen rotation for vertical CPS2 games
bool Metal_CPS2_IsScreenRotated() {
    // Get the current game
    const CPS2ROMInfo* romInfo = CPS2_GetROMInfo();
    if (!romInfo) {
        return false;
    }
    
    // Check if this is a vertical game
    // CPS2 games are generally horizontal (384x224)
    // but some games are designed for vertical screens
    return (romInfo->width < romInfo->height);
}

// Get the scaled dimensions for the current CPS2 game
void Metal_CPS2_GetDimensions(int* width, int* height, float* scale) {
    if (!width || !height || !scale) {
        return;
    }
    
    // Default safe values
    *width = 384;
    *height = 224;
    *scale = 1.0f;
    
    // Get the current game
    const CPS2ROMInfo* romInfo = CPS2_GetROMInfo();
    if (!romInfo) {
        return;
    }
    
    // Validate width and height
    if (romInfo->width <= 0 || romInfo->height <= 0) {
        printf("Metal_CPS2_GetDimensions: Invalid dimensions in ROM info: %dx%d\n", 
              romInfo->width, romInfo->height);
        return;
    }
    
    // Set dimensions from ROM info
    *width = romInfo->width;
    *height = romInfo->height;
    
    // Calculate scaling factor (maintain aspect ratio)
    // Default to 2x scaling
    *scale = 2.0f;
}

// Set up the CPS2 Metal rendering hooks
void Metal_CPS2_SetupRenderHooks() {
    printf("Metal_CPS2_SetupRenderHooks: Setting up CPS2 rendering hooks\n");
    
    // Initialize renderer
    int result = Metal_CPS2_InitRenderer();
    if (result != 0) {
        printf("Metal_CPS2_SetupRenderHooks: Failed to initialize renderer, error %d\n", result);
        return;
    }
    
    // Set up function hooks
    extern INT32 (*pCps2RenderCallback)();
    if (pCps2RenderCallback != NULL) {
        // Store existing callback if any
        printf("Metal_CPS2_SetupRenderHooks: Replacing existing render callback\n");
    }
    pCps2RenderCallback = Metal_CPS2_Render;
    
    printf("Metal_CPS2_SetupRenderHooks: CPS2 rendering hooks installed successfully\n");
} 