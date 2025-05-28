#!/bin/bash
cat > src/burner/metal/minimal/metal_minimal.cpp << "EOL"
#include "burnint.h"
#include "burner.h"
#include <stdio.h>
// Simplified Metal implementation for minimal build
int main() {
    printf("FBNeo Metal Runtime - Minimal Build\n");
    BurnLibInit();
    printf("Loaded %d drivers\n", nBurnDrvCount);
    DrvInit(0, false);
    printf("Driver initialized\n");
    DrvExit();
    BurnLibExit();
    return 0;
}
EOL
