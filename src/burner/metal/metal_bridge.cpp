#include "metal_declarations.h"
#include "burnint.h"
#include "metal_bridge.h"
#include "metal_exports.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

// ROM path management
char g_szROMPath[MAX_PATH] = {0};

// ROM path getter function
const char* GetROMPathString() {
    return g_szROMPath[0] ? g_szROMPath : NULL;
}

// ROM path setter function
int SetCurrentROMPath(const char* path) {
    if (!path) {
        return 1;
    }
    
    strncpy(g_szROMPath, path, MAX_PATH - 1);
    g_szROMPath[MAX_PATH - 1] = '\0';
    
    printf("ROM path set to: %s\n", g_szROMPath);
    return 0;
}

// Externs for accessing FBNeo globals
extern UINT8* pBurnDraw;
extern INT32 nBurnPitch;
extern INT32 nBurnBpp;
extern INT16* pBurnSoundOut;

// Our own FBNeo globals for Metal
UINT8* pBurnDraw_Metal = NULL;
INT32 nBurnPitch_Metal = 0;
INT32 nBurnBpp_Metal = 0;

// Flag to track if the FBNeo system is initialized
static bool g_bFBNeoInitialized = false;

// Local frame buffer
static unsigned char* g_pFrameBuffer = NULL;
static int g_nFrameWidth = 384;  // Default width
static int g_nFrameHeight = 224; // Default height
static int g_nFrameDepth = 32;   // Default depth (32bpp)
static int g_nFrameSize = 0;     // Size in bytes
static bool g_bFrameBufferUpdated = false;

// Initialize the frame buffer
static void InitFrameBuffer(int width, int height, int bpp) {
    // Free existing buffer if any
    if (g_pFrameBuffer) {
        free(g_pFrameBuffer);
        g_pFrameBuffer = NULL;
    }
    
    // Calculate new size
    int bytesPerPixel = bpp / 8;
    g_nFrameSize = width * height * bytesPerPixel;
    
    // Allocate new buffer
    g_pFrameBuffer = (unsigned char*)malloc(g_nFrameSize);
    if (!g_pFrameBuffer) {
        printf("ERROR: Failed to allocate frame buffer of size %d bytes!\n", g_nFrameSize);
        return;
    }
    
    // Clear the buffer
    memset(g_pFrameBuffer, 0, g_nFrameSize);
    
    // Update dimensions
    g_nFrameWidth = width;
    g_nFrameHeight = height;
    g_nFrameDepth = bpp;
    
    printf("Initialized frame buffer: %dx%d, %d bpp (%d bytes)\n", width, height, bpp, g_nFrameSize);
    
    // Set up FBNeo core pointers
    pBurnDraw_Metal = g_pFrameBuffer;
    nBurnPitch_Metal = width * bytesPerPixel;
    nBurnBpp_Metal = bpp;
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
    if (!pBurnDraw_Metal) {
        // Get the game dimensions
        int width = 0, height = 0;
        BurnDrvGetVisibleSize(&width, &height);
        
        if (width <= 0 || height <= 0) {
            width = 384;  // Default for CPS2
            height = 224; // Default for CPS2
        }
        
        printf("[InitFrameBufferSettings] Initializing frame buffer: %dx%d\n", width, height);
        
        // Initialize frame buffer with game dimensions
        InitFrameBuffer(width, height, 32);
        
        if (!pBurnDraw_Metal) {
            printf("[InitFrameBufferSettings] ERROR: Failed to initialize frame buffer\n");
            return 1;
        }
    }
    
    // CRITICAL: Set up FBNeo drawing pointers
    pBurnDraw = pBurnDraw_Metal;
    nBurnPitch = nBurnPitch_Metal;
    nBurnBpp = nBurnBpp_Metal;
    
    return 0;
}

// Bridge function for BurnLibInit
INT32 BurnLibInit_Metal() {
    printf("[BurnLibInit_Metal] === STARTING FBNEO LIBRARY INITIALIZATION ===\n");
    fflush(stdout);
    
    // Null pointer safety checks
    if (g_bFBNeoInitialized) {
        printf("[BurnLibInit_Metal] Already initialized, skipping\n");
        fflush(stdout);
        return 0;
    }
    
    printf("[BurnLibInit_Metal] Step 1: Initializing global pointers\n");
    fflush(stdout);
    
    // Initialize global pointers to safe defaults
    pBurnDraw = NULL;
    pBurnDraw_Metal = NULL;
    nBurnPitch = 0;
    nBurnPitch_Metal = 0;
    nBurnBpp = 0;
    nBurnBpp_Metal = 0;
    
    printf("[BurnLibInit_Metal] Step 2: Setting up frame buffer dimensions\n");
    fflush(stdout);
    
    // Create a properly sized frame buffer
    int width = 384;  // Standard FBNeo frame width for CPS2
    int height = 224; // Standard FBNeo frame height for CPS2
    
    printf("[BurnLibInit_Metal] Step 3: Allocating frame buffer: %dx%d\n", width, height);
    fflush(stdout);
    
    // Initialize frame buffer with default size
    InitFrameBuffer(width, height, 32);
    
    // Verify frame buffer allocation
    if (!g_pFrameBuffer || !pBurnDraw_Metal) {
        printf("[BurnLibInit_Metal] ERROR: Frame buffer allocation failed\n");
        printf("[BurnLibInit_Metal] g_pFrameBuffer = %p, pBurnDraw_Metal = %p\n", g_pFrameBuffer, pBurnDraw_Metal);
        fflush(stdout);
        return 1;
    }
    
    printf("[BurnLibInit_Metal] Step 4: Setting up core pointers\n");
    fflush(stdout);
    
    // Set up core pointers to our frame buffer
    pBurnDraw = pBurnDraw_Metal;
    nBurnPitch = nBurnPitch_Metal;
    nBurnBpp = nBurnBpp_Metal;
    
    printf("[BurnLibInit_Metal] Frame buffer setup: %dx%d, %d bytes at %p\n", 
           width, height, width * height * 4, pBurnDraw_Metal);
    printf("[BurnLibInit_Metal] Pitch: %d, BPP: %d\n", nBurnPitch, nBurnBpp);
    fflush(stdout);
    
    // Initialize FBNeo library with error checking
    printf("[BurnLibInit_Metal] Step 5: Calling BurnLibInit...\n");
    fflush(stdout);
    
    INT32 nRet = BurnLibInit();
    if (nRet != 0) {
        printf("[BurnLibInit_Metal] ERROR: BurnLibInit failed: %d\n", nRet);
        fflush(stdout);
        // Clean up on failure
        if (g_pFrameBuffer) {
            free(g_pFrameBuffer);
            g_pFrameBuffer = NULL;
        }
        pBurnDraw = NULL;
        pBurnDraw_Metal = NULL;
        return nRet;
    }
    
    printf("[BurnLibInit_Metal] Step 6: Verifying driver system\n");
    fflush(stdout);
    
    // Verify driver count
    printf("[BurnLibInit_Metal] Driver count: %d\n", nBurnDrvCount);
    if (nBurnDrvCount == 0) {
        printf("[BurnLibInit_Metal] WARNING: No drivers available\n");
    }
    
    // Verify pDriver array
    if (!pDriver) {
        printf("[BurnLibInit_Metal] ERROR: pDriver array is NULL\n");
        fflush(stdout);
        return 1;
    }
    
    printf("[BurnLibInit_Metal] pDriver array: %p\n", pDriver);
    fflush(stdout);
    
    // Check first few drivers for validity
    for (int i = 0; i < nBurnDrvCount && i < 3; i++) {
        if (!pDriver[i]) {
            printf("[BurnLibInit_Metal] WARNING: pDriver[%d] is NULL\n", i);
        } else {
            printf("[BurnLibInit_Metal] Driver %d: %p -> %s\n", i, pDriver[i],
                   pDriver[i]->szShortName ? pDriver[i]->szShortName : "NULL");
        }
        fflush(stdout);
    }
    
    g_bFBNeoInitialized = true;
    
    printf("[BurnLibInit_Metal] === INITIALIZATION SUCCESS ===\n");
    fflush(stdout);
    return 0;
}

// Bridge function for BurnLibExit
INT32 BurnLibExit_Metal() {
    printf("Metal BurnLibExit_Metal() called\n");
    
    // Free frame buffer
    if (g_pFrameBuffer) {
        free(g_pFrameBuffer);
        g_pFrameBuffer = NULL;
    }
    
    // Reset FBNeo core pointers
    pBurnDraw = NULL;
    nBurnPitch = 0;
    nBurnBpp = 0;
    
    // Reset Metal pointers
    pBurnDraw_Metal = NULL;
    nBurnPitch_Metal = 0;
    nBurnBpp_Metal = 0;
    
    // Exit FBNeo library
    INT32 nRet = BurnLibExit();
    
    g_bFBNeoInitialized = false;
    
    printf("Metal BurnLibExit_Metal() returned: %d\n", nRet);
    return nRet;
}

// Bridge function for BurnDrvInit
INT32 BurnDrvInit_Metal(INT32 nDrvNum) {
    printf("[BurnDrvInit_Metal] === STARTING DRIVER INITIALIZATION ===\n");
    printf("[BurnDrvInit_Metal] Called for driver #%d\n", nDrvNum);
    fflush(stdout);
    
    // Validate driver number
    if (nDrvNum < 0 || nDrvNum >= nBurnDrvCount) {
        printf("[BurnDrvInit_Metal] ERROR: Invalid driver number %d (max: %d)\n", nDrvNum, nBurnDrvCount - 1);
        fflush(stdout);
        return 1;
    }
    
    printf("[BurnDrvInit_Metal] Step 1: Driver number validation passed\n");
    fflush(stdout);
    
    // Ensure FBNeo library is initialized
    if (!g_bFBNeoInitialized) {
        printf("[BurnDrvInit_Metal] ERROR: FBNeo library not initialized! Calling BurnLibInit_Metal first\n");
        fflush(stdout);
        INT32 nRet = BurnLibInit_Metal();
        if (nRet != 0) {
            printf("[BurnDrvInit_Metal] ERROR: BurnLibInit_Metal failed: %d\n", nRet);
            fflush(stdout);
            return nRet;
        }
    }
    
    printf("[BurnDrvInit_Metal] Step 2: FBNeo library initialization verified\n");
    fflush(stdout);
    
    // Validate pDriver array and selected driver
    if (!pDriver) {
        printf("[BurnDrvInit_Metal] ERROR: pDriver array is NULL\n");
        fflush(stdout);
        return 1;
    }
    
    printf("[BurnDrvInit_Metal] Step 3: pDriver array validation passed (%p)\n", pDriver);
    fflush(stdout);
    
    if (!pDriver[nDrvNum]) {
        printf("[BurnDrvInit_Metal] ERROR: pDriver[%d] is NULL\n", nDrvNum);
        fflush(stdout);
        return 1;
    }
    
    printf("[BurnDrvInit_Metal] Step 4: Driver pointer validation passed (%p)\n", pDriver[nDrvNum]);
    fflush(stdout);
    
    struct BurnDriver* pDrv = pDriver[nDrvNum];
    printf("[BurnDrvInit_Metal] Step 5: Driver info validation:\n");
    printf("  Short name: %s\n", pDrv->szShortName ? pDrv->szShortName : "NULL");
    printf("  Full name: %s\n", pDrv->szFullNameA ? pDrv->szFullNameA : "NULL");
    printf("  Hardware: 0x%08X\n", pDrv->nHardwareCode);
    printf("  Init function: %p\n", pDrv->Init);
    printf("  Exit function: %p\n", pDrv->Exit);
    printf("  Frame function: %p\n", pDrv->Frame);
    fflush(stdout);
    
    // Select driver with validation
    printf("[BurnDrvInit_Metal] Step 6: Selecting driver...\n");
    fflush(stdout);
    
    INT32 nRet = BurnDrvSelect(nDrvNum);
    if (nRet != 0) {
        printf("[BurnDrvInit_Metal] ERROR: BurnDrvSelect failed: %d\n", nRet);
        fflush(stdout);
        return nRet;
    }
    
    printf("[BurnDrvInit_Metal] Step 7: Driver selection successful\n");
    fflush(stdout);
    
    // Verify driver selection
    if (nBurnDrvActive != nDrvNum) {
        printf("[BurnDrvInit_Metal] WARNING: Active driver mismatch: expected %d, got %d\n", nDrvNum, nBurnDrvActive);
    }
    
    printf("[BurnDrvInit_Metal] Step 8: Active driver verification (nBurnDrvActive = %d)\n", nBurnDrvActive);
    fflush(stdout);
    
    // === ROM LOADING AND DEBUGGING SECTION ===
    printf("[BurnDrvInit_Metal] Step 9: ROM Loading and Analysis\n");
    fflush(stdout);
    
    // Initialize ROM loading system
    BurnROMInit();
    
    // Set ROM path if available
    const char* romPath = GetROMPathString();
    if (romPath) {
        printf("[BurnDrvInit_Metal] Setting ROM path: %s\n", romPath);
        BurnSetROMPath(romPath);
    }
    
    // Enumerate and log all ROM regions
    printf("[BurnDrvInit_Metal] === ROM ENUMERATION ===\n");
    for (INT32 i = 0; i < 64; i++) { // Check up to 64 ROM regions
        struct BurnRomInfo ri;
        if (BurnDrvGetRomInfo(&ri, i) != 0) {
            printf("[BurnDrvInit_Metal] End of ROM list at index %d\n", i);
            break; // End of ROM list
        }
        
        if (!ri.szName || ri.nLen == 0) {
            printf("[BurnDrvInit_Metal] Empty ROM entry at index %d\n", i);
            continue;
        }
        
        // Decode ROM type
        const char* romTypeStr = "Unknown";
        const char* romRegionStr = "Unknown";
        
        switch (ri.nType & 0xFF) {
            case 0x01: // CPS2_PRG_68K
                romTypeStr = "68K Program";
                romRegionStr = "CPU";
                break;
            case 0x02: // CPS2_GFX
                romTypeStr = "Graphics";
                romRegionStr = "GFX";
                break;
            case 0x03: // CPS2_PRG_Z80
                romTypeStr = "Z80 Program";
                romRegionStr = "Sound CPU";
                break;
            case 0x04: // CPS2_QSND
                romTypeStr = "QSound Samples";
                romRegionStr = "Audio";
                break;
            case 0x05: // CPS2_ENCRYPTION_KEY
                romTypeStr = "Encryption Key";
                romRegionStr = "Security";
                break;
            default:
                romTypeStr = "Other";
                romRegionStr = "Misc";
                break;
        }
        
        printf("[BurnDrvInit_Metal] ROM %d: %s\n", i, ri.szName);
        printf("  Size: 0x%08X (%d KB)\n", ri.nLen, ri.nLen / 1024);
        printf("  CRC: 0x%08X\n", ri.nCrc);
        printf("  Type: 0x%08X (%s)\n", ri.nType, romTypeStr);
        printf("  Region: %s\n", romRegionStr);
        
        // Attempt to load ROM data
        UINT8* romData = (UINT8*)malloc(ri.nLen);
        if (romData) {
            INT32 bytesLoaded = 0;
            INT32 loadResult = BurnLoadRom(romData, &bytesLoaded, i);
            
            if (loadResult == 0 && bytesLoaded > 0) {
                printf("  Status: Loaded successfully (%d bytes)\n", bytesLoaded);
                
                // Calculate checksum of loaded data
                UINT32 checksum = 0;
                for (UINT32 j = 0; j < ri.nLen; j++) {
                    checksum ^= romData[j];
                    checksum = (checksum << 1) | (checksum >> 31); // Rotate left
                }
                printf("  Data checksum: 0x%08X\n", checksum);
                
                // Dump first ROM region to file for debugging
                if (i == 0) {
                    FILE* dumpFile = fopen("/tmp/mvsc_rom0.bin", "wb");
                    if (dumpFile) {
                        fwrite(romData, 1, ri.nLen, dumpFile);
                        fclose(dumpFile);
                        printf("  Debug: Dumped to /tmp/mvsc_rom0.bin\n");
                    }
                }
            } else {
                printf("  Status: Load failed (result: %d, bytes: %d)\n", loadResult, bytesLoaded);
            }
            
            free(romData);
        } else {
            printf("  Status: Memory allocation failed\n");
        }
        
        fflush(stdout);
    }
    printf("[BurnDrvInit_Metal] === END ROM ENUMERATION ===\n");
    fflush(stdout);
    
    // Get game dimensions with validation
    printf("[BurnDrvInit_Metal] Step 10: Getting game dimensions...\n");
    fflush(stdout);
    
    INT32 width = 0, height = 0;
    nRet = BurnDrvGetVisibleSize(&width, &height);
    if (nRet != 0) {
        printf("[BurnDrvInit_Metal] WARNING: BurnDrvGetVisibleSize failed: %d\n", nRet);
        width = 384;  // Default for CPS2
        height = 224; // Default for CPS2
    }
    
    printf("[BurnDrvInit_Metal] Step 11: Game dimensions retrieved: %dx%d\n", width, height);
    fflush(stdout);
    
    // Validate dimensions
    if (width <= 0 || height <= 0 || width > 2048 || height > 2048) {
        printf("[BurnDrvInit_Metal] WARNING: Invalid game dimensions: %dx%d, using defaults\n", width, height);
        width = 384;  // Standard for CPS2
        height = 224; // Standard for CPS2
    }
    
    printf("[BurnDrvInit_Metal] Step 12: Final dimensions: %dx%d\n", width, height);
    fflush(stdout);
    
    // Reinitialize frame buffer with correct dimensions before driver init
    printf("[BurnDrvInit_Metal] Step 13: Reinitializing frame buffer...\n");
    fflush(stdout);
    
    InitFrameBuffer(width, height, 32);  // Always use 32bpp for Metal
    
    // Verify frame buffer after reinitialization
    if (!pBurnDraw_Metal || !g_pFrameBuffer) {
        printf("[BurnDrvInit_Metal] ERROR: Frame buffer reinitialization failed\n");
        printf("[BurnDrvInit_Metal] pBurnDraw_Metal = %p, g_pFrameBuffer = %p\n", pBurnDraw_Metal, g_pFrameBuffer);
        fflush(stdout);
        return 1;
    }
    
    printf("[BurnDrvInit_Metal] Step 14: Frame buffer reinitialization successful\n");
    fflush(stdout);
    
    // CRITICAL: Force connection between FBNeo and our frame buffer
    pBurnDraw = pBurnDraw_Metal;
    nBurnPitch = nBurnPitch_Metal;
    nBurnBpp = nBurnBpp_Metal;
    
    // Print frame buffer information for debugging
    printf("[BurnDrvInit_Metal] Step 15: Frame buffer connection:\n");
    printf("  pBurnDraw = %p (pBurnDraw_Metal = %p)\n", pBurnDraw, pBurnDraw_Metal);
    printf("  nBurnPitch = %d, nBurnBpp = %d\n", nBurnPitch, nBurnBpp);
    printf("  Dimensions = %dx%d, Size = %d bytes\n", width, height, width * height * (nBurnBpp/8));
    fflush(stdout);
    
    // Clear the frame buffer to avoid garbage data
    if (pBurnDraw_Metal) {
        printf("[BurnDrvInit_Metal] Step 16: Clearing frame buffer...\n");
        fflush(stdout);
        memset(pBurnDraw_Metal, 0, width * height * (nBurnBpp/8));
        printf("[BurnDrvInit_Metal] Frame buffer cleared\n");
        fflush(stdout);
    }
    
    // Validate driver Init function before calling
    if (!pDrv->Init) {
        printf("[BurnDrvInit_Metal] ERROR: Driver Init function is NULL\n");
        fflush(stdout);
        return 1;
    }
    
    printf("[BurnDrvInit_Metal] Step 17: About to call driver Init function (%p)...\n", pDrv->Init);
    fflush(stdout);
    
    // Add memory debugging around the critical Init call
    printf("[BurnDrvInit_Metal] === CALLING DRIVER INIT ===\n");
    printf("[BurnDrvInit_Metal] Memory state before Init:\n");
    printf("  pBurnDraw = %p\n", pBurnDraw);
    printf("  nBurnPitch = %d\n", nBurnPitch);
    printf("  nBurnBpp = %d\n", nBurnBpp);
    printf("  nBurnDrvActive = %d\n", nBurnDrvActive);
    fflush(stdout);
    
    // Initialize the driver
    nRet = pDrv->Init();
    
    printf("[BurnDrvInit_Metal] === DRIVER INIT RETURNED ===\n");
    printf("[BurnDrvInit_Metal] Init function returned: %d\n", nRet);
    fflush(stdout);
    
    if (nRet != 0) {
        printf("[BurnDrvInit_Metal] ERROR: Driver Init failed: %d\n", nRet);
        fflush(stdout);
        return nRet;
    }
    
    printf("[BurnDrvInit_Metal] Step 18: Driver initialization successful!\n");
    printf("[BurnDrvInit_Metal] === DRIVER INITIALIZATION COMPLETE ===\n");
    fflush(stdout);
    return 0;
}

// Bridge function for BurnDrvExit
INT32 BurnDrvExit_Metal() {
    printf("[BurnDrvExit_Metal] Called\n");
    
    // Exit the driver
    INT32 nRet = BurnDrvExit();
    
    // Free frame buffer
    if (g_pFrameBuffer) {
        free(g_pFrameBuffer);
        g_pFrameBuffer = NULL;
    }
    
    // Reset pointers
    pBurnDraw = NULL;
    pBurnDraw_Metal = NULL;
    nBurnPitch = 0;
    nBurnPitch_Metal = 0;
    nBurnBpp = 0;
    nBurnBpp_Metal = 0;
    
    printf("[BurnDrvExit_Metal] Exit complete, returned: %d\n", nRet);
    return nRet;
}

// Get frame buffer
void* Metal_GetFrameBuffer() {
    return g_pFrameBuffer;
}

// Set frame buffer updated flag
void SetFrameBufferUpdated(bool updated) {
    g_bFrameBufferUpdated = updated;
}

// Check if frame buffer is updated
bool IsFrameBufferUpdated() {
    return g_bFrameBufferUpdated;
}

// Update Metal frame texture
void UpdateMetalFrameTexture(void* data, int width, int height) {
    if (!data || width <= 0 || height <= 0) {
        return;
    }
    
    // Update the frame buffer
    if (g_pFrameBuffer && width == g_nFrameWidth && height == g_nFrameHeight) {
        memcpy(g_pFrameBuffer, data, width * height * (g_nFrameDepth/8));
        g_bFrameBufferUpdated = true;
    }
}

// Run a single frame of emulation
INT32 Metal_RunFrame(int bDraw) {
    static int frameCount = 0;
    bool logFrame = (++frameCount % 600) == 0;  // Only log every 600 frames to reduce spam
    
    if (logFrame) {
        printf("[Metal_RunFrame] Frame %d (bDraw=%d)\n", frameCount, bDraw);
    }
    
    if (!g_bFBNeoInitialized) {
        printf("[Metal_RunFrame] ERROR: FBNeo is not initialized\n");
        return 1;
    }
    
    if (nBurnDrvActive >= nBurnDrvCount) {
        printf("[Metal_RunFrame] ERROR: No active driver\n");
        return 1;
    }
    
    // Initialize the frame buffer and emulation settings
    INT32 initResult = InitFrameBufferAndEmulationSettings();
    if (initResult != 0) {
        printf("[Metal_RunFrame] ERROR: Failed to initialize frame buffer settings\n");
        return initResult;
    }
    
    // Process input before running the frame
    extern void Metal_ProcessInput();
    Metal_ProcessInput();
    
    // If not drawing this frame, temporarily clear pBurnDraw
    if (!bDraw) {
        pBurnDraw = NULL;  // No drawing this frame
    }
    
    // Run a frame of emulation with the FBNeo core
    INT32 nRet = BurnDrvFrame();
    
    if (nRet != 0) {
        printf("[Metal_RunFrame] ERROR: BurnDrvFrame failed: %d\n", nRet);
        return nRet;
    }
    
    // Process the rendered frame
    if (bDraw && pBurnDraw_Metal) {
        // Update the Metal texture with our frame buffer
        UpdateMetalFrameTexture(pBurnDraw_Metal, g_nFrameWidth, g_nFrameHeight);
        
        // Set the updated flag
        SetFrameBufferUpdated(true);
        
        if (logFrame) {
            // Check for content in the buffer (first 1000 bytes)
            bool hasContent = false;
            unsigned int checksum = 0;
            for (int i = 0; i < 1000 && i < g_nFrameSize; i++) {
                if (pBurnDraw_Metal[i] != 0) {
                    hasContent = true;
                    checksum += pBurnDraw_Metal[i];
                }
            }
            
            if (!hasContent) {
                printf("[Metal_RunFrame] WARNING: Frame buffer appears empty (checksum: 0x%08X)\n", checksum);
            } else {
                printf("[Metal_RunFrame] Frame buffer has content (checksum: 0x%08X)\n", checksum);
            }
        }
    }
    
    // Update audio
    extern void Metal_UpdateAudio();
    Metal_UpdateAudio();
    
    return 0;
}

// Callback function pointers
static MetalInitCallback g_pInitCallback = NULL;
static MetalRenderFrameCallback g_pRenderCallback = NULL;
static MetalShutdownCallback g_pShutdownCallback = NULL;
static void* g_pCallbackContext = NULL;

// Metal view pointer
static void* g_pMetalView = NULL;

// Metal initialization
int Metal_Init(void* viewPtr, MetalDriverSettings* settings) {
    printf("[Metal_Init] Initializing Metal renderer\n");
    
    if (!viewPtr) {
        printf("ERROR: Metal view pointer is NULL\n");
        return 1;
    }
    
    g_pMetalView = viewPtr;
    
    // Initialize frame buffer with default size
    int width = settings ? settings->width : 384;
    int height = settings ? settings->height : 224;
    
    InitFrameBuffer(width, height, 32);
    
    // Initialize FBNeo core if not already done
    if (!g_bFBNeoInitialized) {
        INT32 nRet = BurnLibInit_Metal();
        if (nRet != 0) {
            printf("ERROR: Failed to initialize FBNeo core: %d\n", nRet);
            return nRet;
        }
    }
    
    // Call init callback if registered
    if (g_pInitCallback) {
        g_pInitCallback(g_pCallbackContext);
    }
    
    printf("[Metal_Init] Initialization complete\n");
    return 0;
}

// Metal exit
int Metal_Exit() {
    printf("[Metal_Exit] Shutting down Metal renderer\n");
    
    // Call shutdown callback if registered
    if (g_pShutdownCallback) {
        g_pShutdownCallback(g_pCallbackContext);
    }
    
    // Free frame buffer
    if (g_pFrameBuffer) {
        free(g_pFrameBuffer);
        g_pFrameBuffer = NULL;
    }
    
    // Reset pointers
    g_pMetalView = NULL;
    g_pCallbackContext = NULL;
    
    // Exit FBNeo core if initialized
    if (g_bFBNeoInitialized) {
        BurnLibExit_Metal();
    }
    
    printf("[Metal_Exit] Shutdown complete\n");
    return 0;
}

// Register callbacks
void Metal_RegisterCallbacks(MetalInitCallback initFunc,
                           MetalRenderFrameCallback renderFunc,
                           MetalShutdownCallback shutdownFunc) {
    printf("[Metal_RegisterCallbacks] Registering callbacks\n");
    
    g_pInitCallback = initFunc;
    g_pRenderCallback = renderFunc;
    g_pShutdownCallback = shutdownFunc;
}

// Render a frame (used by the Metal frontend)
INT32 Metal_RenderFrame(void* frameData, int width, int height) {
    // Safety check
    if (!frameData || width <= 0 || height <= 0) {
        printf("[Metal_RenderFrame] Invalid parameters: frameData=%p, width=%d, height=%d\n",
               frameData, width, height);
        return 1;
    }
    
    // Ensure frame buffer is initialized
    if (!pBurnDraw_Metal) {
        // Initialize if needed
        InitFrameBuffer(width, height, 32);
        if (!pBurnDraw_Metal) {
            printf("[Metal_RenderFrame] Failed to initialize frame buffer\n");
            return 1;
        }
    }
    
    // If frame data was provided, copy it to our buffer
    if (frameData) {
        // Copy the provided frame data to our frame buffer
        memcpy(pBurnDraw_Metal, frameData, width * height * 4);
    }
    
    // Update the Metal texture with our frame buffer
    UpdateMetalFrameTexture(pBurnDraw_Metal, width, height);
    
    // Set frame buffer updated flag
    SetFrameBufferUpdated(true);
    
    return 0;
}

// Update texture
int Metal_UpdateTexture(void* data, int width, int height, int pitch) {
    if (!data || width <= 0 || height <= 0) {
        return 1;
    }
    
    // Ensure frame buffer is properly sized
    if (width != g_nFrameWidth || height != g_nFrameHeight) {
        InitFrameBuffer(width, height, 32);
    }
    
    // Copy data to frame buffer
    if (pitch == width * 4) {
        // Direct copy if pitch matches
        memcpy(g_pFrameBuffer, data, g_nFrameSize);
    } else {
        // Line-by-line copy if pitch differs
        unsigned char* src = (unsigned char*)data;
        unsigned char* dst = g_pFrameBuffer;
        for (int y = 0; y < height; y++) {
            memcpy(dst, src, width * 4);
            src += pitch;
            dst += width * 4;
        }
    }
    
    g_bFrameBufferUpdated = true;
    return 0;
}

// Stub implementation for Metal_IsActive
int Metal_IsActive(void) {
    return 1;
}

// Stub implementation for Metal_GetRendererInfo
const char* Metal_GetRendererInfo(void) {
    return "FBNeo Metal Renderer (Minimal)";
}

// Show a test pattern on the screen
int Metal_ShowTestPattern(int width, int height) {
    printf("Metal_ShowTestPattern(%d, %d) called\n", width, height);
    
    // If dimensions are invalid, use defaults
    if (width <= 0 || height <= 0) {
        width = g_nFrameWidth;
        height = g_nFrameHeight;
    }
    
    // Ensure the frame buffer exists and has the right size
    if (!g_pFrameBuffer || width != g_nFrameWidth || height != g_nFrameHeight) {
        InitFrameBuffer(width, height, 32);
    }
    
    // Generate a colorful test pattern
    if (g_pFrameBuffer) {
        unsigned int* pixels = (unsigned int*)g_pFrameBuffer;
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                unsigned char r = (unsigned char)((x * 255) / width);
                unsigned char g = (unsigned char)((y * 255) / height);
                unsigned char b = (unsigned char)(((x + y) * 255) / (width + height));
                
                // BGRA format for Metal
                pixels[y * width + x] = (0xFF << 24) | (r << 16) | (g << 8) | b;
            }
        }
        
        // Draw a frame counter reference
        static int animFrame = 0;
        animFrame = (animFrame + 1) % 60;
        
        // Checkerboard pattern that animates
        int blockSize = 16;
        for (int y = 0; y < height; y += blockSize) {
            for (int x = 0; x < width; x += blockSize) {
                if (((x / blockSize) + (y / blockSize) + (animFrame / 15)) % 2 == 0) {
                    // Fill a block with white
                    for (int by = 0; by < blockSize && (y + by) < height; by++) {
                        for (int bx = 0; bx < blockSize && (x + bx) < width; bx++) {
                            pixels[(y + by) * width + (x + bx)] = 0xFFFFFFFF;
                        }
                    }
                }
            }
        }
        
        // Update the Metal texture
        UpdateMetalFrameTexture(g_pFrameBuffer, width, height);
    }
    
    return 0;
}

// Metal_LoadROM implementation
int Metal_LoadROM(const char* romPath) {
    printf("[Metal_LoadROM] Loading ROM from: %s\n", romPath ? romPath : "NULL");
    
    if (!romPath) {
        printf("[Metal_LoadROM] ERROR: ROM path is NULL\n");
        return 1;
    }
    
    // Verify the ROM path exists
    struct stat dirStat;
    if (stat(romPath, &dirStat) != 0) {
        printf("[Metal_LoadROM] ERROR: ROM path does not exist: %s\n", romPath);
        return 1;
    }
    
    if (!S_ISDIR(dirStat.st_mode)) {
        printf("[Metal_LoadROM] ERROR: ROM path is not a directory: %s\n", romPath);
        return 1;
    }
    
    // Set ROM path
    if (SetCurrentROMPath(romPath) != 0) {
        printf("[Metal_LoadROM] ERROR: Failed to set ROM path\n");
        return 1;
    }
    
    // Initialize FBNeo core if not already done
    if (!g_bFBNeoInitialized) {
        INT32 nRet = BurnLibInit_Metal();
        if (nRet != 0) {
            printf("[Metal_LoadROM] ERROR: Failed to initialize FBNeo core: %d\n", nRet);
            return nRet;
        }
    }
    
    // Find the correct driver for Marvel vs Capcom
    INT32 nDrvSelect = -1;
    for (INT32 i = 0; i < nBurnDrvCount; i++) {
        BurnDrvSelect(i);
        if (BurnDrvGetTextA(DRV_NAME) && strcmp(BurnDrvGetTextA(DRV_NAME), "mvsc") == 0) {
            nDrvSelect = i;
            printf("[Metal_LoadROM] Found Marvel vs Capcom driver at index %d\n", i);
            break;
        }
    }
    
    // If exact match not found, try substring match
    if (nDrvSelect == -1) {
        for (INT32 i = 0; i < nBurnDrvCount; i++) {
            BurnDrvSelect(i);
            if (BurnDrvGetTextA(DRV_NAME) && strstr(BurnDrvGetTextA(DRV_NAME), "mvsc")) {
                nDrvSelect = i;
                printf("[Metal_LoadROM] Found Marvel vs Capcom variant at index %d: %s\n", 
                       i, BurnDrvGetTextA(DRV_NAME));
                break;
            }
        }
    }
    
    if (nDrvSelect == -1) {
        printf("[Metal_LoadROM] ERROR: Could not find Marvel vs Capcom driver\n");
        
        // For debugging, show available drivers
        printf("[Metal_LoadROM] Available drivers (%d total):\n", nBurnDrvCount);
        for (INT32 i = 0; i < nBurnDrvCount && i < 10; i++) {
            BurnDrvSelect(i);
            printf("  %d: %s (%s)\n", 
                   i, 
                   BurnDrvGetTextA(DRV_NAME) ? BurnDrvGetTextA(DRV_NAME) : "NULL",
                   BurnDrvGetTextA(DRV_FULLNAME) ? BurnDrvGetTextA(DRV_FULLNAME) : "NULL");
        }
        
        return 1;
    }
    
    // Verify ROMs exist before attempting to initialize driver
    BurnDrvSelect(nDrvSelect);
    bool missingRoms = false;
    
    printf("[Metal_LoadROM] Verifying ROM files for %s...\n", BurnDrvGetTextA(DRV_NAME));
    
    // Check for ROM files
    for (INT32 i = 0; i < 64; i++) { // Check up to 64 ROM files
        struct BurnRomInfo ri;
        if (BurnDrvGetRomInfo(&ri, i) != 0) {
            break; // End of ROM list
        }
        
        if (!ri.szName || ri.nLen == 0) {
            continue; // Skip empty ROM entries
        }
        
        // Build full path to ROM file
        char romFilePath[MAX_PATH];
        snprintf(romFilePath, MAX_PATH, "%s/%s", romPath, ri.szName);
        
        // Check if file exists
        struct stat fileStat;
        if (stat(romFilePath, &fileStat) != 0) {
            printf("[Metal_LoadROM] ERROR: Missing ROM file: %s\n", ri.szName);
            missingRoms = true;
            continue;
        }
        
        // Check file size
        if ((UINT32)fileStat.st_size < ri.nLen) {
            printf("[Metal_LoadROM] ERROR: ROM file size mismatch for %s\n", ri.szName);
            printf("[Metal_LoadROM] Expected: %u bytes, Found: %lld bytes\n", 
                   ri.nLen, (long long)fileStat.st_size);
            missingRoms = true;
        } else {
            printf("[Metal_LoadROM] Found ROM: %s (size: %lld bytes)\n", 
                   ri.szName, (long long)fileStat.st_size);
        }
    }
    
    if (missingRoms) {
        printf("[Metal_LoadROM] ERROR: One or more ROM files are missing or invalid\n");
        printf("[Metal_LoadROM] Please ensure all ROMs are extracted from mvsc.zip to: %s\n", romPath);
        return 1;
    }
    
    // Initialize the driver
    printf("[Metal_LoadROM] Initializing driver for %s...\n", BurnDrvGetTextA(DRV_FULLNAME));
    INT32 nRet = BurnDrvInit_Metal(nDrvSelect);
    if (nRet != 0) {
        printf("[Metal_LoadROM] ERROR: Failed to initialize driver: %d\n", nRet);
        return nRet;
    }
    
    printf("[Metal_LoadROM] ROM loaded successfully\n");
    return 0;
}

// Metal_RunGame implementation
int Metal_RunGame() {
    printf("[Metal_RunGame] Starting game execution\n");
    
    if (!g_bFBNeoInitialized) {
        printf("ERROR: FBNeo core not initialized\n");
        return 1;
    }
    
    if (nBurnDrvActive >= nBurnDrvCount) {
        printf("ERROR: No active driver selected\n");
        return 1;
    }
    
    // Initialize input system
    BurnInputInit();
    
    // Initialize sound system
    BurnSoundInit();
    
    // Set up frame buffer
    int width = 0, height = 0;
    BurnDrvGetVisibleSize(&width, &height);
    
    if (width <= 0 || height <= 0) {
        width = 384;  // Default for CPS2
        height = 224; // Default for CPS2
    }
    
    InitFrameBuffer(width, height, 32);
    
    // Connect frame buffer to FBNeo
    pBurnDraw = pBurnDraw_Metal;
    nBurnPitch = nBurnPitch_Metal;
    nBurnBpp = nBurnBpp_Metal;
    
    printf("[Metal_RunGame] Game started successfully\n");
    return 0;
}

int Metal_ResetGame() {
    printf("Metal_ResetGame called\n");
    return 0;
}

int Metal_PauseGame(int pause) {
    printf("Metal_PauseGame called with pause=%d\n", pause);
    return 0;
}

// Minimal input functions
int Metal_HandleKeyDown(int keyCode) {
    printf("Metal_HandleKeyDown called with keyCode=%d\n", keyCode);
    return 0;
}

int Metal_HandleKeyUp(int keyCode) {
    printf("Metal_HandleKeyUp called with keyCode=%d\n", keyCode);
    return 0;
}

int Metal_InitInput() {
    printf("Metal_InitInput called\n");
    return 0;
}

// Minimal CPS support
void Cps2_SetupMetalLinkage() {
    printf("Cps2_SetupMetalLinkage called\n");
}

// Metal core functions
int Metal_InitFBNeo() {
    printf("Metal_InitFBNeo called\n");
    return 0;
}

// DEPRECATED: Generate a test pattern
void Metal_GenerateTestPattern(int width, int height) {
    static bool warningShown = false;
    
    if (!warningShown) {
        printf("[Metal_GenerateTestPattern] WARNING: This function is deprecated and should not be used.\n");
        printf("[Metal_GenerateTestPattern] Real ROM rendering should be used instead of test patterns.\n");
        warningShown = true;
    }
    
    if (!pBurnDraw_Metal || !g_nFrameSize) {
        printf("[Metal_GenerateTestPattern] Frame buffer not initialized.\n");
        return;
    }
    
    // Fill with black
    memset(pBurnDraw_Metal, 0, g_nFrameSize);
    
    // Add warning text by drawing a red border
    uint32_t borderColor = 0xFF0000FF; // RGBA red
    
    // Draw top and bottom borders
    for (int x = 0; x < width; x++) {
        // Top border (first 4 rows)
        for (int y = 0; y < 4; y++) {
            ((uint32_t*)pBurnDraw_Metal)[y * width + x] = borderColor;
        }
        
        // Bottom border (last 4 rows)
        for (int y = height - 4; y < height; y++) {
            ((uint32_t*)pBurnDraw_Metal)[y * width + x] = borderColor;
        }
    }
    
    // Draw left and right borders
    for (int y = 0; y < height; y++) {
        // Left border (first 4 columns)
        for (int x = 0; x < 4; x++) {
            ((uint32_t*)pBurnDraw_Metal)[y * width + x] = borderColor;
        }
        
        // Right border (last 4 columns)
        for (int x = width - 4; x < width; x++) {
            ((uint32_t*)pBurnDraw_Metal)[y * width + x] = borderColor;
        }
    }
    
    // Write "TEST PATTERN - DEPRECATED" in the center
    uint32_t textColor = 0xFFFFFFFF; // RGBA white
    int centerY = height / 2;
    
    // Simply set some pixels to indicate this is a test pattern
    for (int y = centerY - 10; y < centerY + 10; y++) {
        for (int x = width / 4; x < width * 3 / 4; x++) {
            if ((x + y) % 4 == 0) {
                ((uint32_t*)pBurnDraw_Metal)[y * width + x] = textColor;
            }
        }
    }
    
    SetFrameBufferUpdated(true);
}

// Frame buffer information
static unsigned char* pFrameBuffer = NULL;
static int nFrameWidth = 384;  // Default width for CPS2 games
static int nFrameHeight = 224; // Default height for CPS2 games
static int nFramePitch = 0;
static bool bFrameUpdated = false;

// Flag to indicate if Metal renderer is active
static bool bMetalRendererActive = false;

// C interface for Metal renderer
extern "C" {
    void* GetFrameBufferPtr() {
        return pFrameBuffer;
    }
    
    int GetFrameBufferWidth() {
        return nFrameWidth;
    }
    
    int GetFrameBufferHeight() {
        return nFrameHeight;
    }
    
    int GetFrameBufferPitch() {
        return nFramePitch;
    }
    
    bool C_IsFrameBufferUpdated() {
        return bFrameUpdated;
    }
    
    void C_SetFrameBufferUpdated(bool updated) {
        bFrameUpdated = updated;
    }
    
    void MetalRenderer_SetFrameSize(int width, int height) {
        nFrameWidth = width;
        nFrameHeight = height;
        nFramePitch = width * 4; // Assume RGBA8888 format (4 bytes per pixel)
    }
    
    void MetalRenderer_SetActive(bool active) {
        bMetalRendererActive = active;
    }
    
    bool MetalRenderer_IsActive() {
        return bMetalRendererActive;
    }
}

// Video callbacks for FBNeo integration
INT32 MetalSetBurnHighCol(INT32 nDepth) {
    return 0;
}

// Initialize the renderer
INT32 MetalInit() {
    printf("MetalInit: Initializing Metal renderer\n");
    
    return 0;
}

// Exit the renderer
INT32 MetalExit() {
    printf("MetalExit: Shutting down Metal renderer\n");
    
    return 0;
}

// Set the screen size
INT32 MetalSetScreenSize(UINT32 nWidth, UINT32 nHeight) {
    printf("MetalSetScreenSize: %dx%d\n", nWidth, nHeight);
    
    MetalRenderer_SetFrameSize(nWidth, nHeight);
    
    return 0;
}

// Clear the screen
INT32 MetalClear() {
    if (pFrameBuffer) {
        memset(pFrameBuffer, 0, nFrameWidth * nFrameHeight * 4);
    }
    
    return 0;
}

// Flip buffers and present the frame
INT32 MetalPresentFrame(INT32 nDraw) {
    // Instead of implementing this function here, defer to Metal_RunFrame
    // which will coordinate with the renderer to present the frame
    return Metal_RunFrame(nDraw);
}

// Video driver interface implementation removed for Metal build
// The Metal renderer uses direct rendering instead of the VidDriver interface 

// Handle input state changes
void Metal_HandleInput(INT32 i, INT32 nState)
{
    if (nBurnDrvActive >= nBurnDrvCount) {
        return;
    }

    // Set input state through core
    BurnInputSetKey(i, nState);
}

// Stub renderer functions to prevent crashes
extern "C" {
    // Graphics buffer initialization stubs
    void BurnTransferInit() {
        printf("[BurnTransferInit] Transfer system init (Metal stub)\n");
    }
    
    void BurnTransferExit() {
        printf("[BurnTransferExit] Transfer system exit (Metal stub)\n");
    }
    
    // Palette functions
    void BurnRecalcPal() {
        printf("[BurnRecalcPal] Palette recalculation (Metal stub)\n");
    }
    
    INT32 BurnDrvGetPaletteEntries() {
        return 256;  // Standard palette size
    }
    
    // Screen clearing function
    void BurnClearScreen() {
        printf("[BurnClearScreen] Clear screen (Metal stub)\n");
        if (g_pFrameBuffer) {
            memset(g_pFrameBuffer, 0, g_nFrameSize);
        }
    }
}

// External function to update Metal texture
extern "C" void UpdateMetalFrameTexture(void* data, int width, int height) {
    // If we have a render callback, use it
    if (g_pRenderCallback) {
        g_pRenderCallback(g_pCallbackContext, data, width, height);
    } 
    // Otherwise, call the Metal renderer directly
    else if (g_bFBNeoInitialized) {
        extern int MetalRenderer_UpdateFrame(const void* frameData, int width, int height, int pitch);
        MetalRenderer_UpdateFrame(data, width, height, width * 4);
    }
}

// Set callback for rendering
void Metal_SetRenderCallback(MetalRenderCallback callback, void* context) {
    g_pRenderCallback = callback;
    g_pCallbackContext = context;
    printf("[Metal_SetRenderCallback] Render callback set: %p, context: %p\n", 
           callback, context);
} 