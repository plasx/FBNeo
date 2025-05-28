#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "memory_tracking.h"

// Simple memory tracker implementation

// Global memory tracking state
static int g_memoryInitialized = 0;
static size_t g_totalAllocated = 0;
static size_t g_totalFreed = 0;
static size_t g_peakMemoryUsage = 0;
static size_t g_currentMemoryUsage = 0;

// Memory component usage tracking
static size_t g_componentAllocated[MEM_COMPONENT_COUNT] = {0};
static size_t g_componentPeak[MEM_COMPONENT_COUNT] = {0};

// Initialize the memory system
void Memory_Init() {
    g_memoryInitialized = 1;
    g_totalAllocated = 0;
    g_totalFreed = 0;
    g_peakMemoryUsage = 0;
    g_currentMemoryUsage = 0;
    
    for (int i = 0; i < MEM_COMPONENT_COUNT; i++) {
        g_componentAllocated[i] = 0;
        g_componentPeak[i] = 0;
    }
    
    printf("[MEMORY] Memory tracking system initialized\n");
}

// Allocate memory for a specific component
void* Memory_Allocate(size_t size, MemoryComponentType type) {
    if (!g_memoryInitialized) {
        Memory_Init();
    }
    
    void* ptr = malloc(size);
    if (ptr) {
        g_totalAllocated += size;
        g_currentMemoryUsage += size;
        
        if (g_currentMemoryUsage > g_peakMemoryUsage) {
            g_peakMemoryUsage = g_currentMemoryUsage;
        }
        
        if (type < MEM_COMPONENT_COUNT) {
            g_componentAllocated[type] += size;
            if (g_componentAllocated[type] > g_componentPeak[type]) {
                g_componentPeak[type] = g_componentAllocated[type];
            }
        }
    }
    
    return ptr;
}

// Free memory
void Memory_Free(void* address) {
    if (address) {
        free(address);
        // Note: Since we don't track individual allocations, we can't accurately
        // adjust g_currentMemoryUsage or g_componentAllocated here.
    }
}

// Log memory statistics
void Memory_PrintStats() {
    printf("\n[MEMORY] Memory Usage Statistics:\n");
    printf("---------------------------------\n");
    printf("Total allocated:    %zu bytes\n", g_totalAllocated);
    printf("Total freed:        %zu bytes\n", g_totalFreed);
    printf("Current usage:      %zu bytes\n", g_currentMemoryUsage);
    printf("Peak usage:         %zu bytes\n", g_peakMemoryUsage);
    printf("\nUsage by component:\n");
    
    const char* componentNames[MEM_COMPONENT_COUNT] = {
        "Graphics",
        "Sound",
        "Game Data",
        "Z80 ROM",
        "68K ROM",
        "Other"
    };
    
    for (int i = 0; i < MEM_COMPONENT_COUNT; i++) {
        printf("  %-12s: %8zu bytes (peak: %8zu bytes)\n", 
               componentNames[i], 
               g_componentAllocated[i], 
               g_componentPeak[i]);
    }
    printf("\n");
}

// Allocate CPS2 graphics memory
void* CPS2_AllocateGraphics(size_t size) {
    printf("[MEMORY] Allocating %zu bytes for CPS2 graphics\n", size);
    return Memory_Allocate(size, MEM_COMPONENT_GRAPHICS);
}

// Allocate CPS2 sound memory
void* CPS2_AllocateSound(size_t size) {
    printf("[MEMORY] Allocating %zu bytes for CPS2 sound\n", size);
    return Memory_Allocate(size, MEM_COMPONENT_SOUND);
}

// Allocate general CPS2 memory
void* CPS2_AllocateGeneral(size_t size) {
    printf("[MEMORY] Allocating %zu bytes for CPS2 general data\n", size);
    return Memory_Allocate(size, MEM_COMPONENT_OTHER);
}

// Alloc Z80 ROM
void* CPS2_AllocateZ80Rom(size_t size) {
    printf("[MEMORY] Allocating %zu bytes for Z80 ROM\n", size);
    return Memory_Allocate(size, MEM_COMPONENT_Z80);
}

// Alloc 68K ROM
void* CPS2_Allocate68KRom(size_t size) {
    printf("[MEMORY] Allocating %zu bytes for 68K ROM\n", size);
    return Memory_Allocate(size, MEM_COMPONENT_68K);
}

// Implementation of BurnMalloc that uses our memory tracker
unsigned char* BurnMalloc(int size) {
    return (unsigned char*)Memory_Allocate(size, MEM_COMPONENT_OTHER);
}

// Implementation of BurnFree that uses our memory tracker
void _BurnFree(void* ptr) {
    Memory_Free(ptr);
} 