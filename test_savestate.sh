#!/bin/bash
# Test script for FBNeo save state functionality

echo "=== Testing FBNeo save state functionality ==="

# Create test directories
mkdir -p test/savestates
mkdir -p ~/Documents/FBNeoSaves

# Compile a simple test program
cat > test_savestate.mm << 'EOF'
#import <Foundation/Foundation.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "src/burner/metal/metal_savestate_stubs.h"

// Stub implementations for testing
extern "C" {
    // Global to control test behavior
    static bool g_simulateGameRunning = false;
    
    // Stub for BurnDrvIsWorking (used in Metal_SaveState/Metal_LoadState)
    bool BurnDrvIsWorking() {
        return g_simulateGameRunning; // Controlled by command line arg
    }
    
    // Stub for Metal_ListSaveStates
    INT32 Metal_ListSaveStates_Stub(int* slots, int maxSlots) {
        if (!slots || maxSlots <= 0) {
            return 0;
        }
        
        // For testing purposes, simulate some existing save slots
        if (g_simulateGameRunning) {
            if (maxSlots >= 1) slots[0] = 1;
            if (maxSlots >= 2) slots[1] = 3;
            return g_simulateGameRunning ? 2 : 0;
        }
        
        return 0;
    }
    
    // Stub for Metal_ShowSaveStateStatus (used in Metal_QuickSave/Metal_QuickLoad)
    void Metal_ShowSaveStateStatus(bool isSave) {
        printf("[Metal_ShowSaveStateStatus] %s slot %d\n", 
              isSave ? "Saved to" : "Loaded from", 
              Metal_GetCurrentSaveSlot());
    }
}

int main(int argc, char* argv[]) {
    @autoreleasepool {
        printf("=== SAVE STATE TEST PROGRAM ===\n");
        
        // Check if we should simulate a running game
        if (argc > 1 && strcmp(argv[1], "--simulate-game") == 0) {
            printf("Simulating a running game with valid ROMs loaded.\n");
            g_simulateGameRunning = true;
        } else {
            printf("Simulating no game running. Add --simulate-game to test successful save/load.\n");
        }
        
        // Initialize the save state system
        printf("\nInitializing save state system...\n");
        INT32 result = Metal_InitSaveState();
        if (result != 0) {
            printf("ERROR: Failed to initialize save state system\n");
            return 1;
        }
        
        // Test saving a state
        printf("\nTesting quick save (slot 1)...\n");
        result = Metal_QuickSave();
        if (result != 0) {
            printf("NOTE: Quick save failed with code %d %s\n", 
                  result, g_simulateGameRunning ? "(unexpected failure)" : "(expected if no ROM is loaded)");
        } else {
            printf("Quick save succeeded\n");
        }
        
        // Test loading a state
        printf("\nTesting quick load (slot 1)...\n");
        result = Metal_QuickLoad();
        if (result != 0) {
            printf("NOTE: Quick load failed with code %d %s\n", 
                  result, g_simulateGameRunning ? "(might be expected if no save exists)" : "(expected if no ROM is loaded)");
        } else {
            printf("Quick load succeeded\n");
        }
        
        // Test slot selection
        printf("\nTesting slot selection (slot 2)...\n");
        result = Metal_SetSaveSlot(2);
        if (result != 0) {
            printf("ERROR: Failed to set save slot\n");
        } else {
            int currentSlot = Metal_GetCurrentSaveSlot();
            const char* status = Metal_GetSaveStateStatus();
            printf("Current slot: %d, Status: %s\n", currentSlot, status);
        }
        
        // Test listing save states (using stub)
        printf("\nTesting list save states...\n");
        int slots[10];
        int count = Metal_ListSaveStates_Stub(slots, 10);
        printf("Found %d save states\n", count);
        for (int i = 0; i < count; i++) {
            printf("  - Slot %d\n", slots[i]);
        }
        
        // Clean up
        printf("\nShutting down save state system...\n");
        Metal_ExitSaveState();
        
        printf("\n=== TEST COMPLETED ===\n");
        return 0;
    }
}
EOF

# Compile the test program
echo "Compiling test program..."
clang++ -o test_savestate test_savestate.mm \
    src/burner/metal/metal_savestate.mm \
    src/burner/metal/metal_state_helpers.cpp \
    -framework Foundation -framework Cocoa \
    -I. -I./src/burner/metal -arch arm64 -O0 -g

# Check if compilation was successful
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to compile test program"
    exit 1
fi

# Run the test program without simulation first
echo -e "\nRunning test program (no game simulation)...\n"
./test_savestate

# Then run with simulation
echo -e "\nRunning test program (with game simulation)...\n"
./test_savestate --simulate-game

# Clean up
echo -e "\nCleaning up..."
rm -f test_savestate
rm -f test_savestate.mm

echo -e "\n=== Save state test completed ===" 