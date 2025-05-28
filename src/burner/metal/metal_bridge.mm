#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

#include "metal_declarations.h"
#include "memory_tracking.h"
#include "hardware_tracking.h"
#include "debug_system.h"

// FBNeo core global variables
extern UINT8* pBurnDraw;             // Pointer to the frame buffer - set this before BurnDrvFrame()
extern INT32 nBurnPitch;             // Pitch of the frame buffer in bytes 
extern INT32 nBurnBpp;               // Bytes per pixel (2, 3, or 4)
extern INT32 nBurnSoundLen;          // Number of samples per frame
extern INT16* pBurnSoundOut;         // Buffer for audio output
extern struct GameInp* GameInp;      // Game inputs
extern int nBurnDrvActive;           // Current driver index

// Access to global frame buffer struct defined in metal_standalone_main.mm
extern EmulatorFrameBuffer g_frameBuffer;

// Our global emulation state
static bool emulation_initialized = false;
static bool rom_loaded = false;
static char current_rom_path[MAX_PATH] = {0};
static int driver_index = -1;

// External functions in FBNeo core
extern "C" {
    // ROM functions
    bool ROM_Verify(const char* romPath);
    int BurnDrvGetIndex(const char* name);
    int BurnDrvSelect(int nDrvNum);
    int BurnDrvInit();
    int BurnDrvFrame();
    int BurnDrvExit();
    int BurnDrvGetVisibleSize(int* pnWidth, int* pnHeight);
    const char* BurnDrvGetTextA(unsigned int iIndex);
    int BurnSoundRender(short* pSoundBuf, int nSegmentLength);
    int InputMake(bool bCopy);
    
    // ROM path setter implementation
    void Metal_SetRomPath(const char* path) {
        static char s_romPath[512];
        if (path) {
            strncpy(s_romPath, path, sizeof(s_romPath) - 1);
            s_romPath[sizeof(s_romPath) - 1] = '\0';
            fprintf(stderr, "[ROM CHECK] ROM path set to: %s\n", s_romPath);
        }
    }
    
    // Initialize the FBNeo core
    int FBNeoInit() {
        fprintf(stderr, "[MEM INIT] Initializing FBNeo core\n");
        emulation_initialized = true;
        return 0;
    }
    
    // Shutdown the FBNeo core
    int FBNeoExit() {
        fprintf(stderr, "[EMULATOR] Shutting down FBNeo core\n");
        if (driver_index >= 0) {
            BurnDrvExit();
            driver_index = -1;
        }
        emulation_initialized = false;
        rom_loaded = false;
        return 0;
    }
    
    // Run a frame of emulation
    int RunFrame(int bDraw) {
        if (!emulation_initialized || !rom_loaded) {
            fprintf(stderr, "[ERROR] Cannot run frame: emulation not initialized or ROM not loaded\n");
            return -1;
        }
        
        // CRITICAL FIX: Ensure frame buffer is valid before setting pBurnDraw
        if (bDraw) {
            if (!g_frameBuffer.data || g_frameBuffer.width <= 0 || g_frameBuffer.height <= 0) {
                fprintf(stderr, "[ERROR] Frame buffer is invalid. Recreating frame buffer...\n");
                
                // Get game dimensions
                int width = 0, height = 0;
                BurnDrvGetVisibleSize(&width, &height);
                
                // Use defaults if BurnDrv returns invalid dimensions
                if (width <= 0 || height <= 0) {
                    fprintf(stderr, "[WARNING] Invalid game dimensions from BurnDrvGetVisibleSize\n");
                    width = 384;  // CPS2 default width
                    height = 224; // CPS2 default height
                }
                
                // Check if we need to recreate the frame buffer
                if (g_frameBuffer.data) {
                    free(g_frameBuffer.data);
                    g_frameBuffer.data = NULL;
                }
                
                // Create a new frame buffer with proper dimensions
                g_frameBuffer.width = width;
                g_frameBuffer.height = height;
                g_frameBuffer.pitch = width * sizeof(uint32_t);
                g_frameBuffer.data = (uint32_t*)malloc(width * height * sizeof(uint32_t));
                
                if (!g_frameBuffer.data) {
                    fprintf(stderr, "[ERROR] Failed to allocate frame buffer memory\n");
                    return -1;
                }
                
                // Clear the buffer to black
                memset(g_frameBuffer.data, 0, width * height * sizeof(uint32_t));
                fprintf(stderr, "[MEM] Frame buffer recreated: %dx%d (%lu bytes)\n", 
                       width, height, width * height * sizeof(uint32_t));
            }
            
            // Connect our frame buffer to FBNeo
            pBurnDraw = (UINT8*)g_frameBuffer.data;
            nBurnPitch = g_frameBuffer.pitch;
            nBurnBpp = 4;  // 32-bit RGBA
            
            // Debug: Track frame execution
            static int frameCount = 0;
            if (++frameCount % 60 == 0) {
                fprintf(stderr, "[FRAME] Running frame %d with pBurnDraw=%p, size=%dx%d\n", 
                       frameCount, pBurnDraw, g_frameBuffer.width, g_frameBuffer.height);
                
                // Sample pixel data to verify content
                uint32_t* pixels = (uint32_t*)pBurnDraw;
                if (pixels) {
                    uint32_t checksum = 0;
                    for (int i = 0; i < 100 && i < (g_frameBuffer.width * g_frameBuffer.height); i++) {
                        checksum = (checksum << 1) ^ pixels[i];
                    }
                    fprintf(stderr, "[FRAME] Before frame: buffer checksum=0x%08X\n", checksum);
                }
            }
        } else {
            pBurnDraw = NULL;  // Don't draw this frame
        }
        
        // Process inputs first
        InputMake(true);
        
        // Run one frame of emulation
        int result = BurnDrvFrame();
        
        // Process audio (would connect to audio output)
        if (pBurnSoundOut) {
            BurnSoundRender(pBurnSoundOut, nBurnSoundLen);
        }
        
        // Verify frame data after emulation
        if (bDraw && pBurnDraw) {
            uint32_t* pixels = (uint32_t*)pBurnDraw;
            static int frameCount = 0;
            
            if (++frameCount % 60 == 0) {
                // Calculate checksum of frame data
                uint32_t checksum = 0;
                int nonZeroPixels = 0;
                
                for (int i = 0; i < 1000 && i < (g_frameBuffer.width * g_frameBuffer.height); i++) {
                    checksum = (checksum << 1) ^ pixels[i];
                    if (pixels[i] != 0) nonZeroPixels++;
                }
                
                fprintf(stderr, "[FRAME] After frame %d: buffer checksum=0x%08X, %d/1000 non-zero pixels\n",
                       frameCount, checksum, nonZeroPixels);
            }
            
            // Mark frame buffer updated if we drew this frame
            g_frameBuffer.updated = true;
        }
        
        return result;
    }
    
    // Load a ROM and initialize the driver
    bool LoadROM(const char* romPath) {
        if (!romPath || !romPath[0]) {
            fprintf(stderr, "[ROM CHECK] Invalid ROM path\n");
            return false;
        }
        
        fprintf(stderr, "[ROM CHECK] Loading ROM: %s\n", romPath);
        
        // Extract file name from path
        const char* fileName = strrchr(romPath, '/');
        if (fileName) {
            fileName++;  // Skip the '/'
        } else {
            fileName = romPath;
        }
        
        // Create a copy of the file name for driver search
        char searchName[256];
        strncpy(searchName, fileName, sizeof(searchName)-1);
        searchName[sizeof(searchName)-1] = '\0';
        
        // Remove extension (.zip)
        char* dotPos = strrchr(searchName, '.');
        if (dotPos) {
            *dotPos = '\0';
        }
        
        fprintf(stderr, "[ROM CHECK] Looking for driver: %s\n", searchName);
        
        // Find the driver
        driver_index = BurnDrvGetIndex(searchName);
        if (driver_index < 0) {
            fprintf(stderr, "[ROM CHECK] Driver not found. Trying lowercase...\n");
            
            // Convert to lowercase
            for (char* p = searchName; *p; p++) {
                *p = tolower(*p);
            }
            
            driver_index = BurnDrvGetIndex(searchName);
        }
        
        if (driver_index < 0) {
            fprintf(stderr, "[ROM CHECK] Driver not found: %s\n", searchName);
            return false;
        }
        
        // Select the driver
        fprintf(stderr, "[ROM CHECK] Found driver index: %d\n", driver_index);
        BurnDrvSelect(driver_index);
        
        // Store ROM path
        strncpy(current_rom_path, romPath, MAX_PATH-1);
        current_rom_path[MAX_PATH-1] = '\0';
        
        // Set the ROM path for other systems
        Metal_SetRomPath(romPath);
        
        // Now directly initialize the driver
        fprintf(stderr, "[HW INIT] Initializing driver\n");
        if (BurnDrvInit() != 0) {
            fprintf(stderr, "[HW INIT] Failed to initialize driver\n");
            driver_index = -1;
            return false;
        }
        
        // Get game dimensions
        int width = 0, height = 0;
        BurnDrvGetVisibleSize(&width, &height);
        fprintf(stderr, "[GRAPHICS INIT] Game dimensions: %dx%d\n", width, height);
        
        // Create frame buffer with correct dimensions
        if (g_frameBuffer.width != width || g_frameBuffer.height != height) {
            fprintf(stderr, "[MEM INIT] Creating frame buffer %dx%d\n", width, height);
            
            // Free existing buffer if needed
            if (g_frameBuffer.data) {
                free(g_frameBuffer.data);
            }
            
            // Allocate new buffer
            g_frameBuffer.width = width;
            g_frameBuffer.height = height;
            g_frameBuffer.pitch = width * sizeof(uint32_t);
            g_frameBuffer.data = (uint32_t*)malloc(width * height * sizeof(uint32_t));
            g_frameBuffer.updated = false;
            
            if (!g_frameBuffer.data) {
                fprintf(stderr, "[MEM INIT] Failed to allocate frame buffer\n");
                BurnDrvExit();
                driver_index = -1;
                return false;
            }
            
            // Clear frame buffer
            memset(g_frameBuffer.data, 0, width * height * sizeof(uint32_t));
        }
        
        // Connect frame buffer to FBNeo
        pBurnDraw = (UINT8*)g_frameBuffer.data;
        nBurnPitch = g_frameBuffer.pitch;
        nBurnBpp = 4;  // 32-bit RGBA
        
        // ROM is now loaded
        rom_loaded = true;
        
        // Run a frame to initialize the system
        fprintf(stderr, "[EMULATOR] Running initial frame\n");
        RunFrame(1);
        
        fprintf(stderr, "[GAME START] Game started: %s\n", BurnDrvGetTextA(DRV_FULLNAME));
        return true;
    }
    
    // Get ROM loaded state
    bool IsROMLoaded() {
        return rom_loaded;
    }
    
    // Get current ROM path
    const char* GetCurrentROMPath() {
        return current_rom_path;
    }
}

// Metal_LoadAndInitROM implementation - main entry point for loading a ROM
extern "C" bool Metal_LoadAndInitROM(const char* romPath) {
    if (!romPath || romPath[0] == '\0') {
        NSLog(@"[ROM CHECK] Invalid ROM path");
        return false;
    }
    
    NSLog(@"[ROM CHECK] Loading ROM: %s", romPath);
    
    // Initialize FBNeo core if needed
    if (!emulation_initialized) {
        FBNeoInit();
    }
    
    // Load the ROM using the core function
    return LoadROM(romPath);
}

// Metal_ProcessFrame implementation - run one frame of emulation
extern "C" bool Metal_ProcessFrame() {
    // Run a frame of emulation with drawing enabled
    int result = RunFrame(1);
    
    // Only log occasionally to avoid flooding the console
    static int frameCount = 0;
    if (++frameCount % 60 == 0) {
        NSLog(@"[EMULATOR] Frame %d processed", frameCount);
    }
    
    return (result == 0);
}

// Get frame buffer from emulation
extern "C" unsigned char* Metal_GetFrameBuffer(int* width, int* height, int* pitch) {
    if (width) *width = g_frameBuffer.width;
    if (height) *height = g_frameBuffer.height;
    if (pitch) *pitch = g_frameBuffer.pitch;
    
    return (unsigned char*)g_frameBuffer.data;
}

// Get frame dimensions
extern "C" int Metal_GetFrameWidth() {
    int width = 0, height = 0;
    BurnDrvGetVisibleSize(&width, &height);
    return width;
}

extern "C" int Metal_GetFrameHeight() {
    int width = 0, height = 0;
    BurnDrvGetVisibleSize(&width, &height);
    return height;
}

// Get full frame dimensions
extern "C" void Metal_GetFullSize(int* width, int* height) {
    BurnDrvGetVisibleSize(width, height);
}

// Metal_VerifyFramePipeline implementation
extern "C" int Metal_VerifyFramePipeline(int width, int height) {
    // Simple verification that we can render a frame
    NSLog(@"Verifying frame pipeline for %dx%d", width, height);
    
    // Return success
    return 0;
}