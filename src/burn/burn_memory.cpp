// FB Neo memory management module

// The purpose of this module is to offer replacement functions for standard C/C++ ones 
// that allocate and free memory.  This should help deal with the problem of memory
// leaks and non-null pointers on game exit.

#include "burnint.h"
#include "burn_memory.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define LOG_MEMORY_USAGE    0
#define OOB_CHECKER         0  // Disable for simplicity
#define OOB_CHECK           0x200

#define MAX_MEM_PTR         0x400

// Memory tracking
static UINT8* memptr[1024];
static INT32 memsize[1024];
static INT32 mem_allocated = 0;

// FBNeo memory management implementation
// Used for tracking allocations and properly freeing memory

// Memory allocation tracker
static INT32 nTotalMemory = 0;
static INT32 nMemoryAllocated = 0;

// Initialize memory manager
void BurnInitMemoryManager()
{
	memset(memptr, 0, sizeof(memptr));
	memset(memsize, 0, sizeof(memsize));
	mem_allocated = 0;
	nTotalMemory = 0;
	nMemoryAllocated = 0;
}

// Clean up memory manager
void BurnExitMemoryManager()
{
	for (INT32 i = 0; i < 1024; i++) {
		if (memptr[i] != NULL) {
			free(memptr[i]);
			memptr[i] = NULL;
			mem_allocated -= memsize[i];
			memsize[i] = 0;
		}
	}
	mem_allocated = 0;
	if (nMemoryAllocated > 0) {
		printf("Warning! %d memory allocations still active\n", nMemoryAllocated);
	}
}

// Memory allocation functions
UINT8* _BurnMalloc(INT32 size, const char* file, INT32 line)
{
	if (size <= 0) {
		return NULL;
	}

	void* ptr = malloc(size);
	
	if (ptr) {
		nMemoryAllocated++;
		nTotalMemory += size;
	} else {
		printf("Error! BurnMalloc failed to allocate %d bytes [%s:%d]\n", size, file, line);
	}
	
	return (UINT8*)ptr;
}

// Free memory
void _BurnFree(void* ptr)
{
	if (!ptr) {
		return;
	}
	
	free(ptr);
	
	if (nMemoryAllocated > 0) {
		nMemoryAllocated--;
	}
}

// Reallocate memory
void* BurnRealloc(void* ptr, UINT32 size)
{
	void* newPtr = realloc(ptr, size);
	if (!newPtr && size) {
		printf("Error! BurnRealloc failed to allocate %d bytes\n", size);
	}
	return newPtr;
}

// Memory block swapping
void BurnSwapMemBlock(UINT8* src, UINT8* dst, INT32 size)
{
	UINT8* temp = (UINT8*)malloc(size);
	
	if (temp) {
		memcpy(temp, src, size);
		memcpy(src, dst, size);
		memcpy(dst, temp, size);
		free(temp);
	}
}

// Simple power of 2 rounding
UINT32 BurnRoundPowerOf2(UINT32 in)
{
	UINT32 t = 1;
	while (in > t) {
		t <<= 1;
	}
	return t;
}

// Return total memory allocated
INT32 BurnGetMemoryUsage()
{
	return nTotalMemory;
}
