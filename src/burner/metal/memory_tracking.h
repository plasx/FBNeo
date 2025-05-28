#ifndef MEMORY_TRACKING_H
#define MEMORY_TRACKING_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Memory component types
typedef enum {
    MEM_COMPONENT_GRAPHICS,
    MEM_COMPONENT_SOUND,
    MEM_COMPONENT_OTHER,
    MEM_COMPONENT_Z80,
    MEM_COMPONENT_68K,
    MEM_COMPONENT_MAIN_CPU,   // Alias for primary CPU
    MEM_COMPONENT_SOUND_CPU,  // Alias for sound CPU
    MEM_COMPONENT_AUDIO,      // Alias for sound
    MEM_COMPONENT_COUNT
} MemoryComponentType;

// Initialize memory tracking
void Memory_Init(void);

// Generate memory initialization report
void Memory_GenerateReport(void);

// Clean up memory tracking
void Memory_Exit(void);

// Track memory allocation for a specific component
void* Memory_Alloc(size_t size, MemoryComponentType component, const char* description);

// Free tracked memory
void Memory_Free(void* address);

// Convenience functions for component-specific allocations
void* Memory_AllocMainCPU(size_t size, const char* description);
void* Memory_AllocSoundCPU(size_t size, const char* description);
void* Memory_AllocGraphics(size_t size, const char* description);
void* Memory_AllocAudio(size_t size, const char* description);

// Get total allocated for a component
size_t Memory_GetComponentTotal(MemoryComponentType component);

// Get total allocated memory
size_t Memory_GetTotalAllocated(void);

// Print memory usage statistics
void Memory_PrintStats(void);

// Initialize memory components for emulation
void Memory_InitComponents(void);

// Report memory initialization status for all components
void Memory_ReportInitStatus(void);

// --- Alias functions for compatibility with older code ---
static inline void MemoryTracker_Init(void) { Memory_Init(); }
static inline void* MemoryTracker_Allocate(size_t size, const char* description) { 
    return Memory_Alloc(size, MEM_COMPONENT_OTHER, description); 
}
static inline void MemoryTracker_Free(void* address) { Memory_Free(address); }
static inline void MemoryTracker_LogStats(void) { Memory_PrintStats(); }

#ifdef __cplusplus
}
#endif

#endif // MEMORY_TRACKING_H 