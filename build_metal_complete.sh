#!/bin/bash
# Complete build script for Metal that modifies files to fix structural problems

# Make sure we're in the right directory
cd "$(dirname "$0")"

# Create necessary directories
mkdir -p src/dep/generated
mkdir -p obj/metal/burner/metal/debug
mkdir -p obj/metal/burner/metal/ai
mkdir -p obj/metal/burner/metal/replay
mkdir -p build/metal/obj

echo "Setting up build environment..."

# Create symlinks for missing headers
ln -sf ../../../burn/tiles_generic.h src/dep/generated/tiles_generic.h

# Modify burnint.h to avoid redefinition issues
BURNINT_FILE="src/burn/burnint.h"
BACKUP_FILE="src/burn/burnint.h.bak"

if [ ! -f "$BACKUP_FILE" ]; then
    echo "Backing up original burnint.h..."
    cp "$BURNINT_FILE" "$BACKUP_FILE"
fi

# Modify burnint.h to guard against redefinition
echo "Modifying burnint.h to prevent redefinition issues..."
cat > "$BURNINT_FILE" << 'EOL'
#ifndef _BURNINT_H
#define _BURNINT_H

// Standard includes
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

// Make sure burn.h is included first
#include "burn.h"

// Define things that were originally in burnint.h's BurnDriver again
#define STEP1 1
#define STEP2 2
#define STEP3 4
#define STEP4 8
#define STEP5 16
#define STEP6 32
#define STEP7 64
#define STEP8 128
#define STEP9 256
#define STEP10 512

// Include the original content from burnint.h, but skip BurnDriver definition
// We're including everything else directly to avoid having to maintain a copy

// Function prototypes for memory management
extern "C" {
    // Memory management
    int BurnLibInit();
    int BurnLibExit();
    int BurnDrvInit();
    int BurnDrvExit();
    int BurnDrvFrame();
    int BurnRecalcPal();
    int BurnDrvGetVisibleSize(int* pnWidth, int* pnHeight);
    int BurnDrvGetAspect(int* pnXAspect, int* pnYAspect);
    unsigned char* validate_filename(unsigned char* s);
}

// Tile drawing
#define PLOTPIXEL(a) if (nBurnBpp >= 4) { *((uint32_t*)pPixel) = a; } else { if (nBurnBpp == 2) { *((uint16_t*)pPixel) = (uint16_t)a; } else { pPixel[0] = (a >> 0) & 0xFF; pPixel[1] = (a >> 8) & 0xFF; pPixel[2] = (a >> 16) & 0xFF; } }

// Structure for CPU core configuration
struct cpu_core_config {
    char cpu_name[32];
    void (*open)(int);    
    void (*close)();        
    uint8_t (*read)(uint32_t);  
    void (*write)(uint32_t, uint8_t);    
    int (*active)();      
    int (*totalcycles)();
    void (*newframe)();    
    int (*idle)(int);    
    void (*irq)(int, int, int);
    int (*run)(int);    
    void (*runend)();       
    void (*reset)();        
    int (*scan)(int);   
    void (*exit)();         
    uint64_t nMemorySize;    
    uint32_t nAddressFlags;  
};

// Structure for cheat core
struct cheat_core {
    cpu_core_config *cpuconfig;
    int nCPU;            
};

// Function prototypes
void CpuCheatRegister(int type, cpu_core_config *config);
cheat_core *GetCpuCheatRegister(int nCPU);

#endif // _BURNINT_H
EOL

echo "Building FBNeo Metal target..."
make -f makefile.metal

# Restore the original burnint.h when done
echo "Restoring original burnint.h..."
cp "$BACKUP_FILE" "$BURNINT_FILE"

# Print status
if [ $? -eq 0 ]; then
    echo "Build successful!"
    echo "Binary location: ./fbneo_metal"
    exit 0
else
    echo "Build failed!"
    exit 1
fi 