#include "burnint.h"
#include <stdio.h>

// Basic minimal implementation for Metal target
int main() {
    printf("FBNeo Metal Minimal Implementation\n");
    // Initialize burn library
    BurnLibInit();
    printf("FBNeo initialized with %d drivers\n", nBurnDrvCount);
    
    printf("Exiting FBNeo Metal\n");
    BurnLibExit();
    return 0;
}
