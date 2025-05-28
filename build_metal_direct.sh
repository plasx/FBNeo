#!/bin/bash

# FBNeo Metal Direct Build Script
# This script builds the Metal backend using a simplified makefile

set -e  # Exit on error

echo "=== FBNeo Metal Direct Build Script ==="
echo "Starting build process..."

# Ensure the metal_declarations.c file exists
if [ ! -f "src/burner/metal/metal_declarations.c" ]; then
    echo "Creating metal_declarations.c..."
    cat > src/burner/metal/metal_declarations.c << 'EOF'
#include "metal_declarations.h"

// Define global variables declared in metal_declarations.h
char szAppRomPaths[DIRS_MAX][MAX_PATH] = {{0}};
char szAppDirPath[MAX_PATH] = {0};
struct BurnDrvMeta BurnDrvInfo = {0};

// Frame buffer variables
UINT8* pBurnDraw_Metal = NULL;
INT32 nBurnPitch_Metal = 0;
INT32 nBurnBpp_Metal = 0;
EOF
fi

# Ensure the renderer stubs file exists
if [ ! -f "src/burner/metal/fixes/metal_renderer_stubs.c" ]; then
    echo "Creating renderer stubs..."
    mkdir -p src/burner/metal/fixes
    cat > src/burner/metal/fixes/metal_renderer_stubs.c << 'EOF'
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../metal_renderer_c.h"

// Frame buffer variables
static void* g_frameBuffer = NULL;
static int g_frameWidth = 0;
static int g_frameHeight = 0;
static int g_framePitch = 0;
static int g_frameBpp = 0;

// Initialize Metal renderer
int Metal_InitRenderer(int width, int height, int bpp) {
    g_frameWidth = width;
    g_frameHeight = height;
    g_frameBpp = bpp;
    g_framePitch = width * (bpp / 8);
    
    // Allocate frame buffer
    if (g_frameBuffer) {
        free(g_frameBuffer);
    }
    
    g_frameBuffer = malloc(g_framePitch * height);
    if (!g_frameBuffer) {
        return 0;
    }
    
    memset(g_frameBuffer, 0, g_framePitch * height);
    return 1;
}

// Shutdown Metal renderer
void Metal_ShutdownRenderer() {
    if (g_frameBuffer) {
        free(g_frameBuffer);
        g_frameBuffer = NULL;
    }
    
    g_frameWidth = 0;
    g_frameHeight = 0;
    g_framePitch = 0;
    g_frameBpp = 0;
}

// Get frame buffer pointer
void* Metal_GetFrameBuffer() {
    return g_frameBuffer;
}

// Set a pixel in the frame buffer
void Metal_SetPixel(int x, int y, uint32_t color) {
    if (!g_frameBuffer || x < 0 || y < 0 || x >= g_frameWidth || y >= g_frameHeight) {
        return;
    }
    
    if (g_frameBpp == 32) {
        uint32_t* buffer = (uint32_t*)g_frameBuffer;
        buffer[y * g_frameWidth + x] = color;
    } else if (g_frameBpp == 16) {
        uint16_t* buffer = (uint16_t*)g_frameBuffer;
        buffer[y * g_frameWidth + x] = (uint16_t)color;
    }
}

// Clear the frame buffer
void Metal_ClearFrameBuffer(uint32_t color) {
    if (!g_frameBuffer) {
        return;
    }
    
    if (g_frameBpp == 32) {
        uint32_t* buffer = (uint32_t*)g_frameBuffer;
        for (int i = 0; i < g_frameWidth * g_frameHeight; i++) {
            buffer[i] = color;
        }
    } else if (g_frameBpp == 16) {
        uint16_t* buffer = (uint16_t*)g_frameBuffer;
        for (int i = 0; i < g_frameWidth * g_frameHeight; i++) {
            buffer[i] = (uint16_t)color;
        }
    } else {
        memset(g_frameBuffer, 0, g_framePitch * g_frameHeight);
    }
}

// Render a frame
int Metal_RenderFrame(void* frameData, int width, int height) {
    // Copy the frame data to our buffer if provided
    if (frameData && g_frameBuffer) {
        if (width == g_frameWidth && height == g_frameHeight) {
            // Direct copy since dimensions match
            size_t size = g_framePitch * height;
            memcpy(g_frameBuffer, frameData, size);
        } else {
            // Dimensions don't match - need to copy line by line
            int minWidth = (width < g_frameWidth) ? width : g_frameWidth;
            int minHeight = (height < g_frameHeight) ? height : g_frameHeight;
            int srcPitch = width * (g_frameBpp / 8);
            
            uint8_t* src = (uint8_t*)frameData;
            uint8_t* dst = (uint8_t*)g_frameBuffer;
            
            for (int y = 0; y < minHeight; y++) {
                memcpy(dst + (y * g_framePitch), src + (y * srcPitch), minWidth * (g_frameBpp / 8));
            }
        }
    }
    
    return 1;
}

// Update texture with frame buffer data
void Metal_UpdateTexture(void* data, int width, int height) {
    // Bridge to Objective-C++ implementation
    // Empty stub since this is handled in the Objective-C++ code
}

// Set render states
void Metal_SetRenderState(int state, int value) {
    // Empty stub - implemented in Objective-C++ code
}

// Get renderer info
const char* Metal_GetRendererInfo() {
    return "Metal Renderer (C Stub)";
}

// Export frame buffer as RGBA8 data
void* Metal_GetFrameBufferData(int* width, int* height) {
    if (!g_frameBuffer || !width || !height) {
        return NULL;
    }
    
    *width = g_frameWidth;
    *height = g_frameHeight;
    
    // If we already have RGBA8 format, just return the pointer
    if (g_frameBpp == 32) {
        return g_frameBuffer;
    }
    
    // Otherwise, we need to convert from 16-bit to 32-bit
    uint32_t* convertedBuffer = (uint32_t*)malloc(g_frameWidth * g_frameHeight * 4);
    if (!convertedBuffer) {
        return NULL;
    }
    
    if (g_frameBpp == 16) {
        uint16_t* srcBuffer = (uint16_t*)g_frameBuffer;
        for (int i = 0; i < g_frameWidth * g_frameHeight; i++) {
            uint16_t pixel = srcBuffer[i];
            
            // Convert RGB565 to RGBA8888
            uint8_t r = ((pixel >> 11) & 0x1F) << 3;
            uint8_t g = ((pixel >> 5) & 0x3F) << 2;
            uint8_t b = (pixel & 0x1F) << 3;
            
            convertedBuffer[i] = (r << 24) | (g << 16) | (b << 8) | 0xFF;
        }
    }
    
    return convertedBuffer;
}

// Free frame buffer data allocated by Metal_GetFrameBufferData
void Metal_FreeFrameBufferData(void* data) {
    if (data && data != g_frameBuffer) {
        free(data);
    }
}
EOF
fi

# Ensure the metal_stubs.c file in fixes directory exists
if [ ! -f "src/burner/metal/fixes/metal_stubs.c" ]; then
    echo "Creating metal_stubs.c in fixes directory..."
    mkdir -p src/burner/metal/fixes
    cat > src/burner/metal/fixes/metal_stubs.c << 'EOF'
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Define debug variables
int Debug_BurnGunInitted = 0;

// Additional Metal-specific stubs
void BurnSetMouseDivider(int divider) {
    // Stub implementation
}

int AnalogDeadZone(int input) {
    // Simple deadzone implementation
    const int DEADZONE = 10;
    if (input > -DEADZONE && input < DEADZONE) {
        return 0;
    }
    return input;
}

int ProcessAnalog(int value, int min, int deadzone, int center, int max) {
    // Basic analog processing
    if (value < center) {
        if (value < center - deadzone) {
            return min + (center - value) * (center - min) / (center - deadzone);
        }
        return center;
    } else {
        if (value > center + deadzone) {
            return center + (value - center) * (max - center) / (deadzone);
        }
        return center;
    }
}

// Sound initialization
void BurnSoundInit() {
    // Stub implementation
}

// Define other required globals
#define INPUT_DEADZONE 10
EOF
fi

# Fix the shader issue
echo "Fixing shader file..."
sed -i '' 's/color = (frac.x < 0.5) ? colorL : colorR;/color = (frac.x < 0.5) ? colorTL : colorTR;/g' src/burner/metal/Shaders.metal

# Add include guards to problematic header files
echo "Adding include guards to problematic header files..."

# Create a list of header files that need guards
HEADERS_TO_GUARD=(
  "src/burn/tiles_generic.h"
  "src/burn/tilemap_generic.h"
)

for header in "${HEADERS_TO_GUARD[@]}"; do
  if [ -f "$header" ]; then
    # Check if the header already has include guards
    if ! grep -q "#ifndef" "$header" || ! grep -q "#define" "$header"; then
      # Get the header name for the guard
      GUARD_NAME=$(basename "$header" | tr '[:lower:].' '[:upper:]_')
      
      # Create a temporary file with guards
      echo "#ifndef ${GUARD_NAME}_INCLUDE" > "${header}.tmp"
      echo "#define ${GUARD_NAME}_INCLUDE" >> "${header}.tmp"
      echo "" >> "${header}.tmp"
      cat "$header" >> "${header}.tmp"
      echo "" >> "${header}.tmp"
      echo "#endif // ${GUARD_NAME}_INCLUDE" >> "${header}.tmp"
      
      # Replace the original file
      mv "${header}.tmp" "$header"
      echo "Added include guards to $header"
    else
      echo "$header already has include guards"
    fi
  else
    echo "Warning: $header not found"
  fi
done

# Clean any previous build files
echo "Cleaning previous build files..."
make -f makefile.metal.direct clean

# Build using our direct makefile
echo "Building using direct makefile..."
make -f makefile.metal.direct -j10

# Check if build was successful
if [ -f "fbneo_metal" ]; then
    echo "=== Build successful! ==="
    echo "Binary created: fbneo_metal"
    echo "Metal library created: bin/metal/Shaders.metallib"
    
    # Make sure the binary is executable
    chmod +x fbneo_metal
    chmod +x bin/metal/fbneo_metal
    
    echo "Build process completed."
    exit 0
else
    echo "=== Build failed! ==="
    exit 1
fi 