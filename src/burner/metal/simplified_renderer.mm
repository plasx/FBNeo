#import <Cocoa/Cocoa.h>
#import <MetalKit/MetalKit.h>

// Simple Metal renderer for FBNeo
// This handles just the frame buffer updating

// Global frame buffer and dimensions
static unsigned char* g_pFrameBuffer = NULL;
static int g_nWidth = 320;
static int g_nHeight = 240;
static int g_nBPP = 32;
static int g_nPitch = 0;

// Forward declaration for external function
extern void UpdateMetalFrameTexture(const void *frameData, unsigned int width, unsigned int height);

// Initialize the video subsystem
int VidInit() {
    NSLog(@"VidInit() called");
    
    // Initialize with default dimensions 
    g_nWidth = 320;
    g_nHeight = 240;
    g_nBPP = 32;
    g_nPitch = g_nWidth * (g_nBPP / 8);
    
    // Allocate frame buffer memory
    size_t bufferSize = g_nWidth * g_nHeight * (g_nBPP / 8);
    g_pFrameBuffer = (unsigned char*)malloc(bufferSize);
    
    if (!g_pFrameBuffer) {
        NSLog(@"Error: Failed to allocate frame buffer");
        return 1;
    }
    
    // Clear to black
    memset(g_pFrameBuffer, 0, bufferSize);
    
    NSLog(@"Video system initialized: %dx%d, %d bpp", g_nWidth, g_nHeight, g_nBPP);
    
    return 0;
}

// Shutdown the video subsystem
int VidExit() {
    NSLog(@"VidExit() called");
    
    // Free frame buffer
    if (g_pFrameBuffer) {
        free(g_pFrameBuffer);
        g_pFrameBuffer = NULL;
    }
    
    NSLog(@"Video system shutdown");
    
    return 0;
}

// Set the size of the frame buffer
int VidSetSize(int width, int height, int bpp) {
    NSLog(@"VidSetSize(%d, %d, %d) called", width, height, bpp);
    
    if (width <= 0 || height <= 0 || (bpp != 16 && bpp != 24 && bpp != 32)) {
        NSLog(@"Error: Invalid dimensions or bit depth");
        return 1;
    }
    
    // If dimensions haven't changed, nothing to do
    if (g_pFrameBuffer && g_nWidth == width && g_nHeight == height && g_nBPP == bpp) {
        return 0;
    }
    
    // Free old buffer
    if (g_pFrameBuffer) {
        free(g_pFrameBuffer);
        g_pFrameBuffer = NULL;
    }
    
    // Update dimensions
    g_nWidth = width;
    g_nHeight = height;
    g_nBPP = bpp;
    g_nPitch = g_nWidth * (g_nBPP / 8);
    
    // Allocate new buffer
    size_t bufferSize = g_nWidth * g_nHeight * (g_nBPP / 8);
    g_pFrameBuffer = (unsigned char*)malloc(bufferSize);
    
    if (!g_pFrameBuffer) {
        NSLog(@"Error: Failed to allocate frame buffer");
        return 1;
    }
    
    // Clear to black
    memset(g_pFrameBuffer, 0, bufferSize);
    
    NSLog(@"Frame buffer resized to %dx%d, %d bpp", g_nWidth, g_nHeight, g_nBPP);
    
    return 0;
}

// This is called by the emulator core to render a frame
int VidFrame() {
    // This function would be called during emulation to prepare the frame
    // We don't need to do anything here as we update in VidPaint
    return 0;
}

// Actually paint the frame to the Metal surface
int VidPaint(int bRedraw) {
    // Use the global frame buffer
    if (!g_pFrameBuffer) {
        printf("VidPaint: Frame buffer is null\n");
        return 1;
    }
    
    // Update texture on Metal side with the frame buffer
    UpdateMetalFrameTexture(g_pFrameBuffer, g_nWidth, g_nHeight);
    
    return 0;
}

// Used by the emulator core to clear the screen
int VidClear() {
    if (g_pFrameBuffer) {
        memset(g_pFrameBuffer, 0, g_nPitch * g_nHeight);
    }
    return 0;
}

// Get access to the frame buffer
unsigned char* VidGetFrameBuffer() {
    return g_pFrameBuffer;
}

// Get the width of the frame buffer
int VidGetWidth() {
    return g_nWidth;
}

// Get the height of the frame buffer
int VidGetHeight() {
    return g_nHeight;
}

// Test pattern to verify Metal is working
void RenderTestPattern() {
    static int frameCount = 0;
    frameCount++;
    
    if (!g_pFrameBuffer) {
        return;
    }
    
    int tileSize = 32;
    int offset = (frameCount / 2) % tileSize; // Animate the pattern
    
    // Fill buffer with a test pattern
    for (int y = 0; y < g_nHeight; y++) {
        for (int x = 0; x < g_nWidth; x++) {
            int tileX = (x + offset) / tileSize;
            int tileY = (y + offset) / tileSize;
            bool isEvenTile = (tileX + tileY) % 2 == 0;
            
            uint32_t color;
            if (isEvenTile) {
                // Calculate a gradient color based on position
                uint8_t r = (x * 255) / g_nWidth;
                uint8_t g = (y * 255) / g_nHeight;
                uint8_t b = ((x + y) * 127) / (g_nWidth + g_nHeight);
                color = (255 << 24) | (r << 16) | (g << 8) | b; // RGBA
            } else {
                // Black color for odd tiles
                color = 0xFF000000; // Black with full alpha
            }
            
            // Write the pixel to the frame buffer
            uint32_t* pixel = (uint32_t*)(g_pFrameBuffer + y * g_nPitch + x * 4);
            *pixel = color;
        }
    }
    
    // Update the texture
    VidPaint(0);
} 