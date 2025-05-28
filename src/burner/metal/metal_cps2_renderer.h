#pragma once

#include "metal_declarations.h"

// Initialize the CPS2 Metal renderer
int Metal_CPS2_InitRenderer();

// Clean up the CPS2 Metal renderer
void Metal_CPS2_ExitRenderer();

// Update the Metal-friendly palette buffer from CPS2 palette
void Metal_CPS2_UpdatePalette();

// Apply special effects to the palette (fading, flashing, etc)
void Metal_CPS2_ApplyPaletteEffects();

// CPS2 rendering hook for Metal
INT32 Metal_CPS2_Render();

// Get the Metal-friendly palette buffer
UINT32* Metal_CPS2_GetPaletteBuffer();

// Check if the palette has been updated
bool Metal_CPS2_IsPaletteUpdated();

// Handle screen rotation for vertical CPS2 games
bool Metal_CPS2_IsScreenRotated();

// Get the scaled dimensions for the current CPS2 game
void Metal_CPS2_GetDimensions(int* width, int* height, float* scale);

// Set up the CPS2 Metal rendering hooks
void Metal_CPS2_SetupRenderHooks(); 