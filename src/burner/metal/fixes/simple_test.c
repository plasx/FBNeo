#include <stdio.h>
#include <stdint.h>

// Define basic types
typedef int32_t INT32;
typedef uint32_t UINT32;
typedef int8_t INT8;
typedef uint8_t UINT8;
typedef int16_t INT16;
typedef uint16_t UINT16;

// Define the problematic macros directly
#define EXT_BD_SIZE(A)           (((A)>>4)&0x3)
#define EXT_INDEX_SUPPRESS(A)    ((A)&0x40)
#define EXT_BASE_SUPPRESS(A)     ((A)&0x80)

// Define __fastcall for ARM64
#define __fastcall

// Mock CPS variables
UINT8 CpsInp001[16] = {0};
INT32 CpsrLineInfo[16][16] = {{0}};

// Simple test function with __fastcall
INT32 __fastcall TestFunction(INT32 a, INT32 b) {
    return a + b;
}

// Test SekGetPC function
UINT32 SekGetPC(INT32 n) {
    return (UINT32)0x12345678;
}

int main() {
    // Test EXT macros
    UINT16 test_word = 0x4080;
    printf("EXT_BD_SIZE: %d\n", EXT_BD_SIZE(test_word));
    printf("EXT_INDEX_SUPPRESS: %d\n", EXT_INDEX_SUPPRESS(test_word));
    printf("EXT_BASE_SUPPRESS: %d\n", EXT_BASE_SUPPRESS(test_word));
    
    // Test fastcall function
    printf("TestFunction(10, 20): %d\n", TestFunction(10, 20));
    
    // Test SekGetPC
    printf("SekGetPC(-1): 0x%08X\n", SekGetPC(-1));
    
    // Test CPS variables
    CpsInp001[0] = 0xFF;
    printf("CpsInp001[0]: 0x%02X\n", CpsInp001[0]);
    
    CpsrLineInfo[0][0] = 42;
    printf("CpsrLineInfo[0][0]: %d\n", CpsrLineInfo[0][0]);
    
    return 0;
} 