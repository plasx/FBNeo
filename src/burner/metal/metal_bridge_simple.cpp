// Simplified Metal bridge for Phase 3
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Add forward declaration for the verification function
extern "C" {
    void Metal_VerifyCps2Emulation(int frameCount);
}

// Basic type definitions
typedef int32_t INT32;
typedef uint8_t UINT8;
typedef uint16_t UINT16;
typedef uint32_t UINT32;
typedef uint64_t UINT64;
typedef int16_t INT16;

// Forward declarations from FBNeo core
extern "C" {
    // Basic FBNeo variables
    extern UINT32 nBurnDrvCount;
    extern int nBurnDrvActive;
    extern UINT8* pBurnDraw;
    extern INT32 nBurnPitch;
    extern INT32 nBurnBpp;
    
    // Core functions
    INT32 BurnLibInit();
    INT32 BurnLibExit();
    INT32 BurnDrvInit();
    INT32 BurnDrvExit();
    INT32 BurnDrvFrame();
    INT32 BurnDrvFind(const char* szName);
    INT32 BurnDrvSelect(INT32 nDrvNum);
    const char* BurnDrvGetTextA(UINT32 i);
    INT32 BurnDrvGetVisibleSize(INT32* pnWidth, INT32* pnHeight);
    INT32 BurnDrvGetHardwareCode();
    INT32 BurnDrvGetFlags();
    bool BurnDrvIsWorking();
    INT32 BurnDrvGetMaxPlayers();
    
    // CPS2 specific declarations
    INT32 Cps2Init();
    INT32 Cps2Frame();
    INT32 DrvExit();
    void CpsRedraw();
    void BurnRecalcPal();
    
    // Additional FBNeo functions for proper rendering
    INT32 BurnDrvRedraw();
    INT32 BurnTransferCopy(UINT32* pDest);
    
    // CPS variables we need to access
    extern INT32 Cps;
    extern INT32 nCpsGfxScroll[4];
    extern INT32 nCpsGfxMask;
    extern UINT8 *CpsGfx;
    extern UINT8 *CpsRom;
    extern UINT8 *CpsZRom;
    extern UINT8* CpsQSam;
    extern INT32 nCpsGfxLen;
    extern INT32 nCpsRomLen;
    extern INT32 nCpsCodeLen;
    extern INT32 nCpsZRomLen;
    extern INT32 nCpsQSamLen;
    extern INT32 nCpsAdLen;
}

// Frame buffer management
static UINT8* g_pFrameBuffer = NULL;
static UINT8* g_pConvertedBuffer = NULL; // For format conversion
static int g_nFrameWidth = 384;
static int g_nFrameHeight = 224;
static int g_nFrameDepth = 32;
static int g_nFrameSize = 0;
static bool g_bFrameBufferUpdated = false;
static bool g_bFBNeoInitialized = false;

// Memory management for CPS2
static UINT8* g_pCpsGfx = NULL;
static UINT8* g_pCpsRom = NULL;
static UINT8* g_pCpsZRom = NULL;
static UINT8* g_pCpsQSam = NULL;
static INT32 g_nCpsGfxLen = 0;
static INT32 g_nCpsRomLen = 0;
static INT32 g_nCpsZRomLen = 0;
static INT32 g_nCpsQSamLen = 0;

// Initialize frame buffer
static void InitFrameBuffer(int width, int height, int bpp) {
    if (g_pFrameBuffer) {
        free(g_pFrameBuffer);
        g_pFrameBuffer = NULL;
    }
    
    if (g_pConvertedBuffer) {
        free(g_pConvertedBuffer);
        g_pConvertedBuffer = NULL;
    }
    
    int bytesPerPixel = bpp / 8;
    g_nFrameSize = width * height * bytesPerPixel;
    
    // Allocate main frame buffer (for FBNeo core)
    g_pFrameBuffer = (UINT8*)malloc(g_nFrameSize);
    if (!g_pFrameBuffer) {
        printf("ERROR: Failed to allocate frame buffer of size %d bytes!\n", g_nFrameSize);
        return;
    }
    
    // Allocate converted buffer for Metal (always BGRA8888)
    int metalBufferSize = width * height * 4; // Always 32-bit for Metal
    g_pConvertedBuffer = (UINT8*)malloc(metalBufferSize);
    if (!g_pConvertedBuffer) {
        printf("ERROR: Failed to allocate converted buffer of size %d bytes!\n", metalBufferSize);
        free(g_pFrameBuffer);
        g_pFrameBuffer = NULL;
        return;
    }
    
    memset(g_pFrameBuffer, 0, g_nFrameSize);
    memset(g_pConvertedBuffer, 0, metalBufferSize);
    
    g_nFrameWidth = width;
    g_nFrameHeight = height;
    g_nFrameDepth = bpp;
    
    // Set up FBNeo core pointers
    pBurnDraw = g_pFrameBuffer;
    nBurnPitch = width * bytesPerPixel;
    nBurnBpp = bpp;
    
    printf("Initialized frame buffer: %dx%d, %d bpp (%d bytes)\n", width, height, bpp, g_nFrameSize);
    printf("Initialized converted buffer: %dx%d, 32 bpp (%d bytes)\n", width, height, metalBufferSize);
}

// Add a function to dump the frame buffer for debugging
static void DumpFrameBufferToFile(const char* filename, void* buffer, int width, int height) {
    if (!buffer) {
        printf("[DumpFrameBufferToFile] ERROR: Buffer is NULL\n");
        return;
    }
    
    FILE* f = fopen(filename, "wb");
    if (!f) {
        printf("[DumpFrameBufferToFile] ERROR: Could not open file %s for writing\n", filename);
        return;
    }
    
    // Write PPM format (simple uncompressed image)
    fprintf(f, "P6\n%d %d\n255\n", width, height);
    
    // Convert BGRA to RGB for PPM
    unsigned char* bgra = (unsigned char*)buffer;
    for (int i = 0; i < width * height; i++) {
        fputc(bgra[i*4+2], f); // R
        fputc(bgra[i*4+1], f); // G
        fputc(bgra[i*4+0], f); // B
    }
    
    fclose(f);
    printf("[DumpFrameBufferToFile] Dumped frame to %s\n", filename);
}

// Convert framebuffer format to BGRA8888 for Metal
static void ConvertFrameBufferToMetal() {
    if (!g_pFrameBuffer || !g_pConvertedBuffer) {
        printf("[ConvertFrameBufferToMetal] ERROR: Buffers not initialized\n");
        return;
    }
    
    // CRITICAL: Always use pBurnDraw as the source when it's valid
    // This ensures we're using the actual frame rendered by the emulation core
    if (!pBurnDraw) {
        printf("[ConvertFrameBufferToMetal] ERROR: pBurnDraw is NULL, cannot convert frame\n");
        return;
    }
    
    // Log once per second
    static int frameCount = 0;
    frameCount++;
    bool logFrame = (frameCount % 60 == 0);
    
    UINT8* sourceBuffer = (UINT8*)pBurnDraw;
    UINT8* destBuffer = (UINT8*)g_pConvertedBuffer;
    
    // Metal uses BGRA format (B=0, G=1, R=2, A=3)
    // Most rendering engines use RGBA format (R=0, G=1, B=2, A=3)
    // We need to ensure our buffer matches the Metal format
    
    UINT32* src32 = (UINT32*)sourceBuffer;
    UINT32* dst32 = (UINT32*)destBuffer;
    
    const int pixelCount = g_nFrameWidth * g_nFrameHeight;
    
    // Count non-zero pixels for validation
    int nonZeroPixels = 0;
    
    // Convert to BGRA (which is what Metal expects)
    for (int i = 0; i < pixelCount; i++) {
        UINT32 pixel = src32[i];
        
        // Extract components assuming RGBA format
        UINT8 r = (pixel >> 0) & 0xFF;   // Red (bits 0-7)
        UINT8 g = (pixel >> 8) & 0xFF;   // Green (bits 8-15)
        UINT8 b = (pixel >> 16) & 0xFF;  // Blue (bits 16-23)
        UINT8 a = (pixel >> 24) & 0xFF;  // Alpha (bits 24-31)
        
        // Force alpha to 255 (fully opaque) for better visibility
        a = 0xFF;
        
        // Rearrange to BGRA for Metal
        dst32[i] = (a << 24) | (r << 16) | (g << 8) | b;
        
        // Count non-zero pixels for debugging
        if (dst32[i] != 0) {
            nonZeroPixels++;
        }
    }
    
    // Debugging information
    if (logFrame) {
        // Sample some pixels for debugging
        printf("[ConvertFrameBufferToMetal] Frame %d - Sample pixels:\n", frameCount);
        for (int i = 0; i < 5 && i < pixelCount; i++) {
            UINT32 srcPixel = src32[i];
            UINT32 dstPixel = dst32[i];
            printf("  Pixel %d: Source=0x%08X, Converted=0x%08X\n", i, srcPixel, dstPixel);
        }
        
        // Print stats about buffer content
        printf("[ConvertFrameBufferToMetal] Frame %d contains %d/%d non-zero pixels (%.1f%%)\n", 
               frameCount, nonZeroPixels, pixelCount, 
               (float)nonZeroPixels * 100.0f / (float)pixelCount);
        
        // Dump frame buffer for debugging on specific frames
        if (frameCount == 60 || frameCount == 120) {
            char filename[100];
            snprintf(filename, sizeof(filename), "frame_buffer_%d.ppm", frameCount);
            DumpFrameBufferToFile(filename, g_pConvertedBuffer, g_nFrameWidth, g_nFrameHeight);
        }
        
        // CRITICAL: Add test pattern periodically to verify rendering
        if (frameCount % 240 == 0) {
            printf("[ConvertFrameBufferToMetal] Generating test pattern border...\n");
            
            // Add a colorful border to make the frame more visible
            for (int y = 0; y < g_nFrameHeight; y++) {
                for (int x = 0; x < g_nFrameWidth; x++) {
                    if (x < 10 || x >= g_nFrameWidth - 10 || 
                        y < 10 || y >= g_nFrameHeight - 10) {
                        // Create a rainbow pattern in the border
                        UINT8 r = (UINT8)((x * 255) / g_nFrameWidth);
                        UINT8 g = (UINT8)((y * 255) / g_nFrameHeight);
                        UINT8 b = (UINT8)(((x+y) * 255) / (g_nFrameWidth+g_nFrameHeight));
                        
                        // Set pixel in BGRA format
                        dst32[y * g_nFrameWidth + x] = (0xFF << 24) | (r << 16) | (g << 8) | b;
                    }
                }
            }
        }
    }
}

// Calculate checksum for debugging
static UINT32 CalculateFrameChecksum(void* buffer, int size) {
    if (!buffer || size <= 0) return 0;
    
    UINT32 checksum = 0;
    UINT8* data = (UINT8*)buffer;
    
    for (int i = 0; i < size; i++) {
        checksum ^= data[i];
        checksum = (checksum << 1) | (checksum >> 31); // Rotate left
    }
    
    return checksum;
}

// Initialize frame buffer and emulation settings for the current frame
INT32 InitFrameBufferAndEmulationSettings() {
    if (!g_bFBNeoInitialized) {
        printf("[InitFrameBufferSettings] ERROR: FBNeo is not initialized\n");
        return 1;
    }
    
    if (nBurnDrvActive >= nBurnDrvCount) {
        printf("[InitFrameBufferSettings] ERROR: No active driver\n");
        return 1;
    }
    
    // Check if frame buffer is initialized
    if (!g_pFrameBuffer) {
        // Get the game dimensions
        int width = 0, height = 0;
        BurnDrvGetVisibleSize(&width, &height);
        
        if (width <= 0 || height <= 0) {
            width = 384;  // Default to CPS2 resolution
            height = 224;
        }
        
        // Initialize frame buffer with the correct dimensions using 32-bit BGRA
        InitFrameBuffer(width, height, 32);
        
        if (!g_pFrameBuffer) {
            printf("[InitFrameBufferSettings] ERROR: Failed to initialize frame buffer\n");
            return 1;
        }
        
        printf("[InitFrameBufferSettings] Initialized frame buffer: %dx%d, 32 bpp\n", 
               width, height);
    }
    
    // Set up FBNeo rendering settings for 32-bit color (BGRA8888)
    pBurnDraw = g_pFrameBuffer;
    nBurnPitch = g_nFrameWidth * 4;  // 4 bytes per pixel
    nBurnBpp = 32;  // 32 bits per pixel
    
    return 0;
}

// Enhanced debug function to trace frame data
void Metal_DebugTraceFrame(const char* stage, void* buffer, int width, int height) {
    if (!buffer) {
        printf("[%s] ERROR: Buffer is NULL\n", stage);
        return;
    }
    
    // Calculate checksum for first portion of buffer
    UINT32 checksum = CalculateFrameChecksum(buffer, width * height * 4);
    
    // Count non-zero pixels
    UINT32* pixels = (UINT32*)buffer;
    int nonZeroPixels = 0;
    int uniqueColors = 0;
    UINT32 lastColor = 0;
    
    for (int i = 0; i < 1000 && i < (width * height); i++) {
        if (pixels[i] != 0) nonZeroPixels++;
        if (pixels[i] != lastColor) {
            uniqueColors++;
            lastColor = pixels[i];
        }
    }
    
    printf("[%s] Buffer=%p, size=%dx%d, checksum=0x%08X, non-zero=%d/1000, unique colors=%d\n", 
           stage, buffer, width, height, checksum, nonZeroPixels, uniqueColors);
    
    // Print sample pixels
    printf("[%s] Samples: [0]=0x%08X [1]=0x%08X [100]=0x%08X [200]=0x%08X\n", 
           stage, pixels[0], pixels[1], pixels[100], pixels[200]);
}

// Add export declarations at the beginning of the file
#ifdef __cplusplus
extern "C" {
#endif

// Add these functions with extern "C" linkage
INT32 BurnLibInit_Metal() {
    printf("BurnLibInit_Metal: Initializing FBNeo core with CPS2 support\n");
    
    // Initialize the frame buffer
    InitFrameBuffer(384, 224, 32);
    
    // Allocate memory for CPS2
    // For now, set reasonable defaults for Marvel vs. Capcom
    g_nCpsGfxLen = 16 * 1024 * 1024;  // 16MB for graphics
    g_nCpsRomLen = 2 * 1024 * 1024;   // 2MB for program ROM
    g_nCpsZRomLen = 64 * 1024;        // 64KB for Z80 ROM
    g_nCpsQSamLen = 8 * 1024 * 1024;  // 8MB for QSound samples
    
    // Allocate memory for CPS2 components
    g_pCpsGfx = (UINT8*)malloc(g_nCpsGfxLen);
    g_pCpsRom = (UINT8*)malloc(g_nCpsRomLen);
    g_pCpsZRom = (UINT8*)malloc(g_nCpsZRomLen);
    g_pCpsQSam = (UINT8*)malloc(g_nCpsQSamLen);
    
    if (!g_pCpsGfx || !g_pCpsRom || !g_pCpsZRom || !g_pCpsQSam) {
        printf("BurnLibInit_Metal: ERROR: Failed to allocate CPS2 memory\n");
        
        // Free any allocated memory
        if (g_pCpsGfx) free(g_pCpsGfx);
        if (g_pCpsRom) free(g_pCpsRom);
        if (g_pCpsZRom) free(g_pCpsZRom);
        if (g_pCpsQSam) free(g_pCpsQSam);
        
        g_pCpsGfx = NULL;
        g_pCpsRom = NULL;
        g_pCpsZRom = NULL;
        g_pCpsQSam = NULL;
        
        return 1;
    }
    
    // Initialize memory
    memset(g_pCpsGfx, 0, g_nCpsGfxLen);
    memset(g_pCpsRom, 0, g_nCpsRomLen);
    memset(g_pCpsZRom, 0, g_nCpsZRomLen);
    memset(g_pCpsQSam, 0, g_nCpsQSamLen);
    
    // Set global CPS pointers to point to our allocations
    CpsGfx = g_pCpsGfx;
    CpsRom = g_pCpsRom;
    CpsZRom = g_pCpsZRom;
    CpsQSam = g_pCpsQSam;
    
    // Set global CPS lengths
    nCpsGfxLen = g_nCpsGfxLen;
    nCpsRomLen = g_nCpsRomLen;
    nCpsZRomLen = g_nCpsZRomLen;
    nCpsQSamLen = g_nCpsQSamLen;
    
    // Set the system type to CPS2
    Cps = 2;
    
    // Set the initialization flag
    g_bFBNeoInitialized = true;
    
    printf("BurnLibInit_Metal: Initialization complete, allocated memory:\n");
    printf("  CpsGfx: %p (%d bytes)\n", g_pCpsGfx, g_nCpsGfxLen);
    printf("  CpsRom: %p (%d bytes)\n", g_pCpsRom, g_nCpsRomLen);
    printf("  CpsZRom: %p (%d bytes)\n", g_pCpsZRom, g_nCpsZRomLen);
    printf("  CpsQSam: %p (%d bytes)\n", g_pCpsQSam, g_nCpsQSamLen);
    
    return 0;
}

INT32 BurnLibExit_Metal() {
    printf("BurnLibExit_Metal: Shutting down FBNeo core\n");
    
    // Free CPS2 memory
    if (g_pCpsGfx) {
        free(g_pCpsGfx);
        g_pCpsGfx = NULL;
    }
    
    if (g_pCpsRom) {
        free(g_pCpsRom);
        g_pCpsRom = NULL;
    }
    
    if (g_pCpsZRom) {
        free(g_pCpsZRom);
        g_pCpsZRom = NULL;
    }
    
    if (g_pCpsQSam) {
        free(g_pCpsQSam);
        g_pCpsQSam = NULL;
    }
    
    // Clear global CPS pointers
    CpsGfx = NULL;
    CpsRom = NULL;
    CpsZRom = NULL;
    CpsQSam = NULL;
    
    // Clear the initialization flag
    g_bFBNeoInitialized = false;
    
    return 0;
}

// Update the BurnDrvInit_Metal function to properly set up CPS2 driver
INT32 BurnDrvInit_Metal(INT32 nDrvNum) {
    printf("[BurnDrvInit_Metal] Initializing driver %d\n", nDrvNum);
    
    if (!g_bFBNeoInitialized) {
        printf("[BurnDrvInit_Metal] ERROR: FBNeo not initialized\n");
        return 1;
    }
    
    // Select driver
    INT32 nRet = BurnDrvSelect(nDrvNum);
    if (nRet != 0) {
        printf("[BurnDrvInit_Metal] ERROR: BurnDrvSelect failed: %d\n", nRet);
        return nRet;
    }
    
    // Get game dimensions
    INT32 width = 0, height = 0;
    BurnDrvGetVisibleSize(&width, &height);
    if (width <= 0 || height <= 0) {
        width = 384;
        height = 224;
    }
    
    printf("[BurnDrvInit_Metal] Game dimensions: %dx%d\n", width, height);
    
    // Reinitialize frame buffer with correct dimensions
    InitFrameBuffer(width, height, 32);
    
    // Initialize driver
    nRet = BurnDrvInit();
    if (nRet != 0) {
        printf("[BurnDrvInit_Metal] ERROR: BurnDrvInit failed: %d\n", nRet);
        return nRet;
    }
    
    // Force palette recalculation
    BurnRecalcPal();
    
    printf("[BurnDrvInit_Metal] Driver initialization successful\n");
    return 0;
}

#ifdef __cplusplus
}
#endif

// Bridge functions
extern "C" {
    INT32 BurnDrvExit_Metal() {
        printf("[BurnDrvExit_Metal] Exiting driver\n");
        
        INT32 nRet = BurnDrvExit();
        
        if (g_pFrameBuffer) {
            free(g_pFrameBuffer);
            g_pFrameBuffer = NULL;
        }
        
        if (g_pConvertedBuffer) {
            free(g_pConvertedBuffer);
            g_pConvertedBuffer = NULL;
        }
        
        pBurnDraw = NULL;
        nBurnPitch = 0;
        nBurnBpp = 0;
        
        printf("[BurnDrvExit_Metal] Exit complete: %d\n", nRet);
        return nRet;
    }
    
    INT32 Metal_RunFrame(int bDraw) {
        static int frameCount = 0;
        frameCount++;
        
        // Debug log every 60 frames
        bool logFrame = (frameCount % 60 == 0);
        
        if (logFrame) {
            printf("[Metal_RunFrame] Frame %d: bDraw=%d\n", frameCount, bDraw);
        }
        
        if (!g_bFBNeoInitialized) {
            printf("[Metal_RunFrame] ERROR: FBNeo is not initialized\n");
            return 1;
        }
        
        if (nBurnDrvActive >= nBurnDrvCount) {
            printf("[Metal_RunFrame] ERROR: No active driver\n");
            return 1;
        }
        
        // Initialize frame buffer and emulation settings
        InitFrameBufferAndEmulationSettings();
        
        // Set up drawing
        if (bDraw) {
            // Ensure framebuffer is properly set up
            if (!g_pFrameBuffer) {
                printf("[Metal_RunFrame] ERROR: Frame buffer not allocated\n");
                return 1;
            }
            
            // CRITICAL FIX: Always set pBurnDraw to our g_pFrameBuffer
            // This ensures the emulation core renders directly to our buffer
            pBurnDraw = g_pFrameBuffer;
            nBurnPitch = g_nFrameWidth * 4; // 4 bytes per pixel (32-bit BGRA)
            nBurnBpp = 32; // 32 bits per pixel
            
            if (logFrame) {
                printf("[Metal_RunFrame] Frame %d: pBurnDraw=%p, pitch=%d, bpp=%d, size=%d bytes\n", 
                       frameCount, pBurnDraw, nBurnPitch, nBurnBpp, g_nFrameSize);
                
                // Clear part of the frame buffer periodically to verify new rendering
                if (frameCount % 240 == 0) {
                    // Only clear the top 25% to make issues more visible
                    memset(g_pFrameBuffer, 0, g_nFrameSize / 4);
                    printf("[Metal_RunFrame] Cleared top 25%% of frame buffer to verify fresh rendering\n");
                }
            }
            
            // Force recalculate palette on first frame
            static bool firstFrame = true;
            if (firstFrame) {
                BurnRecalcPal();
                firstFrame = false;
                printf("[Metal_RunFrame] Initial palette recalculation done\n");
            }
        } else {
            // Not drawing this frame
            pBurnDraw = NULL;
        }
        
        // Process input before running the frame
        extern void Metal_ProcessInput();
        Metal_ProcessInput();
        
        // Run frame using the real driver's frame function
        INT32 nRet = BurnDrvFrame();
        if (nRet != 0) {
            printf("[Metal_RunFrame] ERROR: BurnDrvFrame failed: %d\n", nRet);
            return nRet;
        }
        
        // Debug the frame buffer content after emulation
        if (logFrame && bDraw && g_pFrameBuffer) {
            UINT32* pixels = (UINT32*)g_pFrameBuffer;
            int nonZeroPixels = 0;
            UINT32 checksum = 0;
            
            // Count non-zero pixels in a sample area
            for (int i = 0; i < 1000 && i < (g_nFrameWidth * g_nFrameHeight); i++) {
                if (pixels[i] != 0) nonZeroPixels++;
                checksum ^= pixels[i];
            }
            
            printf("[Metal_RunFrame] Frame %d data: %d/1000 non-zero pixels, checksum=0x%08X\n", 
                  frameCount, nonZeroPixels, checksum);
            
            // Sample first few pixels
            printf("[Metal_RunFrame] First 5 pixels: ");
            for (int i = 0; i < 5; i++) {
                printf("0x%08X ", pixels[i]);
            }
            printf("\n");
            
            // Add error if no content detected
            if (nonZeroPixels == 0) {
                printf("[Metal_RunFrame] ⚠️ WARNING: No content detected in frame buffer!\n");
            } else {
                printf("[Metal_RunFrame] ✅ Frame buffer contains visible content\n");
            }
        }
        
        if (bDraw && g_pFrameBuffer) {
            // Convert framebuffer format for Metal
            ConvertFrameBufferToMetal();
            
            // Mark the frame buffer as updated
            g_bFrameBufferUpdated = true;
            
            // Periodically verify the pipeline
            if (frameCount == 10 || frameCount == 60 || frameCount % 300 == 0) {
                extern bool MetalRenderer_VerifyPipeline();
                MetalRenderer_VerifyPipeline();
            }
        }
        
        return 0;
    }
    
    // Frame buffer access functions
    void* Metal_GetFrameBuffer() {
        // CRITICAL FIX: Always return the converted buffer for Metal rendering
        // The converted buffer is in BGRA format which Metal expects
        if (!g_pConvertedBuffer) {
            printf("[Metal_GetFrameBuffer] WARNING: Converted buffer is NULL\n");
            return NULL;
        }
        
        // Add debug logging every 60 frames
        static int frameCount = 0;
        if (++frameCount % 60 == 0) {
            // Check if the buffer contains actual data
            UINT32* pixels = (UINT32*)g_pConvertedBuffer;
            int nonZeroPixels = 0;
            UINT32 checksum = 0;
            
            // Count non-zero pixels in a sample area
            for (int i = 0; i < 1000 && i < (g_nFrameWidth * g_nFrameHeight); i++) {
                if (pixels[i] != 0) nonZeroPixels++;
                checksum ^= pixels[i];
            }
            
            printf("[Metal_GetFrameBuffer] Frame %d: Buffer %p has %d/1000 non-zero pixels, checksum=0x%08X\n", 
                   frameCount, g_pConvertedBuffer, nonZeroPixels, checksum);
            
            // Sample a few pixels for debugging
            printf("[Metal_GetFrameBuffer] First 5 pixels: ");
            for (int i = 0; i < 5; i++) {
                printf("0x%08X ", pixels[i]);
            }
            printf("\n");
            
            if (nonZeroPixels == 0) {
                printf("[Metal_GetFrameBuffer] ⚠️ WARNING: Returning empty buffer to renderer!\n");
                
                // CRITICAL FIX: If buffer is empty, create a test pattern
                // This helps diagnose if the renderer itself is working
                if (frameCount % 120 == 0) {
                    printf("[Metal_GetFrameBuffer] Generating test pattern...\n");
                    for (int y = 0; y < g_nFrameHeight; y++) {
                        for (int x = 0; x < g_nFrameWidth; x++) {
                            UINT32 color;
                            // Create a simple checkered pattern
                            if ((x / 16 + y / 16) % 2 == 0) {
                                color = 0xFF0000FF; // Blue in BGRA format
                            } else {
                                color = 0xFFFF0000; // Red in BGRA format
                            }
                            pixels[y * g_nFrameWidth + x] = color;
                        }
                    }
                }
            }
        }
        
        return g_pConvertedBuffer;
    }
    
    // Get raw framebuffer (for debugging)
    void* Metal_GetRawFrameBuffer() {
        return pBurnDraw;
    }
    
    bool IsFrameBufferUpdated() {
        return g_bFrameBufferUpdated;
    }
    
    void SetFrameBufferUpdated(bool updated) {
        g_bFrameBufferUpdated = updated;
    }
    
    void UpdateMetalFrameTexture(void* data, int width, int height) {
        if (!data || width <= 0 || height <= 0) {
            return;
        }
        
        if (g_pConvertedBuffer && width == g_nFrameWidth && height == g_nFrameHeight) {
            memcpy(g_pConvertedBuffer, data, width * height * 4); // Always 32-bit for Metal
            g_bFrameBufferUpdated = true;
        }
    }
    
    // Debug functions
    void Metal_PrintFrameBufferInfo() {
        printf("[Metal_PrintFrameBufferInfo] Frame buffer info:\n");
        printf("  Raw buffer: %p (%dx%d, %d bpp, %d bytes)\n", 
               g_pFrameBuffer, g_nFrameWidth, g_nFrameHeight, g_nFrameDepth, g_nFrameSize);
        printf("  Converted buffer: %p (%dx%d, 32 bpp, %d bytes)\n", 
               g_pConvertedBuffer, g_nFrameWidth, g_nFrameHeight, g_nFrameWidth * g_nFrameHeight * 4);
        printf("  FBNeo pointers: pBurnDraw=%p, nBurnPitch=%d, nBurnBpp=%d\n", 
               pBurnDraw, nBurnPitch, nBurnBpp);
    }

    // Stub functions for save state system are now in metal_savestate_stub.mm
    // Remove duplicate implementations from this file 
} 