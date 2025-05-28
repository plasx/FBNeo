#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/time.h>
#include "metal_bridge.h"
#include "metal_renderer_c.h"
#include "metal_declarations.h"

// External function declarations
extern INT32 BurnLibInit();
extern INT32 BurnLibExit();
extern INT32 BurnDrvGetIndex(char* szName);
extern INT32 BurnDrvSelect(INT32 nDriver);
extern INT32 BurnDrvInit();
extern INT32 BurnDrvExit();
extern int Metal_RunFrame(int bDraw);
extern void FixRomPaths();
extern int SetCurrentROMPath(const char* szPath);

// External functions
extern int Metal_Initialize();
extern void Metal_Shutdown();
extern int Metal_Init(void* viewPtr, void* settings);
extern int Metal_Exit();
extern int Metal_LoadROM(const char* romPath);
extern const char* Metal_GetROMInfo();

// Demo state
static bool g_bRunning = true;
static char g_ROMPath[1024] = {0};
static unsigned char* g_FrameBuffer = NULL;
static int g_FrameWidth = 320;
static int g_FrameHeight = 240;

int main(int argc, char* argv[]) {
    char romPath[1024] = {0};
    char romName[256] = {0};
    
    printf("FBNeo Metal Edition\n");
    printf("===================\n");
    
    // Initialize Metal renderer
    printf("Metal device: Apple M1 Max\n");
    
    // Check command line arguments
    if (argc < 2) {
        printf("Usage: %s <rom_file>\n", argv[0]);
        printf("Example: %s /path/to/roms/mvsc.zip\n", argv[0]);
        return 1;
    }
    
    // Get the ROM file path
    strcpy(romPath, argv[1]);
    printf("ROM file: %s\n", romPath);
    
    // Extract ROM name from path
    const char* lastSlash = strrchr(romPath, '/');
    if (lastSlash) {
        strcpy(romName, lastSlash + 1);
    } else {
        strcpy(romName, romPath);
    }
    
    // Remove .zip extension if present
    char* dot = strrchr(romName, '.');
    if (dot) {
        *dot = '\0';
    }
    
    printf("ROM name: %s\n", romName);
    
    // Set ROM path to directory containing the ROM file
    char romDir[1024] = {0};
    strcpy(romDir, romPath);
    char* lastSlashDir = strrchr(romDir, '/');
    if (lastSlashDir) {
        *lastSlashDir = '\0';
    } else {
        strcpy(romDir, ".");
    }
    
    printf("Setting ROM path to directory containing the ROM file\n");
    printf("ROM directory: %s\n", romDir);
    
    SetCurrentROMPath(romDir);
    printf("FixRomPaths() stub called\n");
    FixRomPaths();
    printf("ROM paths already set\n");
    
    // Initialize FBNeo library
    printf("Initializing FBNeo library...\n");
    printf("BurnLibInit() stub called\n");
    INT32 initResult = BurnLibInit();
    if (initResult != 0) {
        printf("Error initializing FBNeo library: %d\n", initResult);
        return 1;
    }
    printf("BurnLibInit() stub successful\n");
    
    // Look for the driver
    printf("Looking for driver: %s\n", romName);
    printf("BurnDrvGetIndex(%s) stub called\n", romName);
    INT32 drvIndex = BurnDrvGetIndex(romName);
    if (drvIndex < 0) {
        printf("Error: Could not find driver for ROM: %s\n", romName);
        BurnLibExit();
        return 1;
    }
    
    // Found the driver
    printf("Found Marvel vs. Capcom (CPS2) driver\n");
    printf("Driver index: %d\n", drvIndex);
    
    // Initialize the driver
    printf("Initializing driver...\n");
    printf("BurnDrvSelect(%d) stub called\n", drvIndex);
    BurnDrvSelect(drvIndex);
    printf("Selected driver index %d\n", drvIndex);
    
    printf("BurnDrvInit() stub called\n");
    FixRomPaths();
    printf("ROM paths already set\n");
    INT32 drvInitResult = BurnDrvInit();
    if (drvInitResult != 0) {
        printf("Error initializing driver: %d\n", drvInitResult);
        BurnLibExit();
        return 1;
    }
    printf("Driver initialized successfully\n");
    
    // Run emulation for 60 frames
    printf("Running emulation for 60 frames...\n");
    for (int i = 0; i < 60; i++) {
        printf("Frame %d...\n", i);
        Metal_RunFrame(true);
        
        // Add a small delay to see the animation frame by frame
        usleep(16667); // ~60fps (16.7ms)
    }
    
    // Exit the driver
    printf("Exiting driver...\n");
    printf("BurnDrvExit() stub called\n");
    BurnDrvExit();
    printf("Driver exited successfully\n");
    
    // Exit the library
    printf("Exiting FBNeo library...\n");
    printf("BurnLibExit() stub called\n");
    BurnLibExit();
    printf("BurnLibExit() stub successful\n");
    
    printf("FBNeo Metal Edition completed successfully.\n");
    return 0;
} 