#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// Define basic types
typedef int32_t INT32;
typedef uint8_t UINT8;
typedef uint16_t UINT16;
typedef uint32_t UINT32;
typedef char TCHAR;

// Forward declarations
extern "C" {
    INT32 BurnLibInit_Metal();
    INT32 BurnDrvInit_Metal(INT32 nDrvNum);
    INT32 BurnDrvFind(const char* szName);
    void Metal_RunFrame(int bDraw);
}

// Minimal driver structure
struct BurnDriver {
    char* szShortName;
    char* szFullNameA;
    // ... other fields would go here
};

// Global variables
UINT32 nBurnDrvCount = 1;
int nBurnDrvActive = 0;
struct BurnDriver* pDriver[1];

// Minimal driver for testing
struct BurnDriver MvscDriver = {
    (char*)"mvsc",
    (char*)"Marvel vs. Capcom: Clash of Super Heroes (Euro 980112)"
};

int main(int argc, char* argv[]) {
    printf("=== FBNeo Metal Phase 2 Test ===\n");
    
    // Initialize driver list
    pDriver[0] = &MvscDriver;
    nBurnDrvCount = 1;
    
    // Test basic functionality
    printf("Step 1: Initialize FBNeo library\n");
    if (BurnLibInit_Metal() != 0) {
        printf("ERROR: Failed to initialize FBNeo library\n");
        return 1;
    }
    
    printf("Step 2: Find Marvel vs. Capcom driver\n");
    INT32 drvIndex = BurnDrvFind("mvsc");
    if (drvIndex < 0) {
        printf("ERROR: Could not find mvsc driver\n");
        return 1;
    }
    printf("Found driver at index: %d\n", drvIndex);
    
    printf("Step 3: Initialize driver\n");
    if (BurnDrvInit_Metal(drvIndex) != 0) {
        printf("ERROR: Failed to initialize driver\n");
        return 1;
    }
    
    printf("Step 4: Run a few test frames\n");
    for (int i = 0; i < 5; i++) {
        printf("Running frame %d\n", i + 1);
        Metal_RunFrame(1);
    }
    
    printf("=== Test completed successfully! ===\n");
    return 0;
} 