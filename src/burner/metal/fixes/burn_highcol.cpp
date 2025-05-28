//
// burn_highcol.cpp
//
// Implementation of BurnHighCol function for Metal
//

// Include our declarations first to avoid conflicts
#include "../metal_declarations.h"

// System includes
#include <stdio.h>
#include <stdlib.h>

// BurnHighCol implementations for different color depths

// BurnHighCol implementation for RGB565 (16bit)
static UINT32 HighCol16(INT32 r, INT32 g, INT32 b, INT32 /* i */)
{
	UINT32 t;
	t  = (r << 8) & 0xf800; // rrrr r... .... ....
	t |= (g << 3) & 0x07e0; // .... .ggg ggg. ....
	t |= (b >> 3) & 0x001f; // .... .... ...b bbbb
	return t;
}

// BurnHighCol implementation for RGB888 (24bit)
static UINT32 HighCol24(INT32 r, INT32 g, INT32 b, INT32 /* i */)
{
	UINT32 t;
	t  = (r << 16) & 0xff0000;
	t |= (g <<  8) & 0x00ff00;
	t |= (b <<  0) & 0x0000ff;
	return t;
}

// BurnHighCol implementation for ARGB8888 (32bit)
extern "C" UINT32 BurnHighCol32(INT32 r, INT32 g, INT32 b, INT32 /* i */) {
	// Convert r, g, b to ARGB8888 format
	// Note that the 'i' (intensity) parameter is ignored
	UINT32 color = 0xFF000000;  // Alpha = 255
	
	// Clamp colors to 0-255 range
	r = (r < 0) ? 0 : ((r > 255) ? 255 : r);
	g = (g < 0) ? 0 : ((g > 255) ? 255 : g);
	b = (b < 0) ? 0 : ((b > 255) ? 255 : b);
	
	// Set color components
	color |= (r << 16);
	color |= (g << 8);
	color |= b;
	
	return color;
}

// BurnHighCol implementation for BGRA8888 (32bit Metal format)
static UINT32 HighColBGRA(INT32 r, INT32 g, INT32 b, INT32 /* i */)
{
	UINT32 t;
	t = 0xff000000; // Alpha = 255
	t |= (b << 16) & 0x00ff0000;
	t |= (g <<  8) & 0x0000ff00;
	t |= (r <<  0) & 0x000000ff;
	return t;
}

// Set the color depth-dependent function pointers
// This is implemented in C to avoid C++ name mangling
extern "C" INT32 SetBurnHighCol(INT32 nDepth) {
	printf("SetBurnHighCol(%d) called for Metal integration\n", nDepth);
	
	// For Metal we always use 32-bit color 
	// This is important because Metal expects BGRA8Unorm format
	printf("Using 32-bit color conversion for Metal\n");
	
	// BurnHighCol is set directly in metal_integration.cpp
	
	return 0;
} 