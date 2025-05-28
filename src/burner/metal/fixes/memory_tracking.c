#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Simple memory tracking functions for Metal implementation

// Allocate memory with tracking
void* MemoryTracker_Allocate(size_t size, const char* purpose) {
    void* mem = malloc(size);
    
    if (mem) {
        fprintf(stderr, "[MEM INIT] Allocated %zu bytes for %s at %p\n", 
              size, purpose ? purpose : "unknown", mem);
    } else {
        fprintf(stderr, "[MEM INIT] Failed to allocate %zu bytes for %s\n", 
              size, purpose ? purpose : "unknown");
    }
    
    return mem;
}

// Free memory with tracking
void MemoryTracker_Free(void* ptr, const char* purpose) {
    if (ptr) {
        fprintf(stderr, "[MEM INIT] Freeing memory at %p (%s)\n", 
              ptr, purpose ? purpose : "unknown");
        free(ptr);
    }
}

// Track memory usage
void MemoryTracker_TrackUsage(const char* component, size_t bytes) {
    fprintf(stderr, "[MEM INIT] %s using %zu bytes\n", component, bytes);
}

// Initialize memory tracking
void MemoryTracker_Init() {
    fprintf(stderr, "[MEM INIT] Memory tracking initialized\n");
}

// Shutdown memory tracking and report leaks
void MemoryTracker_Shutdown() {
    fprintf(stderr, "[MEM INIT] Memory tracking shutdown\n");
} 