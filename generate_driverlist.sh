#!/bin/bash

# Create the src/dep/generated directory if it doesn't exist
mkdir -p src/dep/generated

# Generate the driver list header
cat > src/dep/generated/driverlist.h << 'EOL'
#include "burnint.h"
#include "../../burn/drv/capcom/cps.h"
#define DRV_CAPCOM
extern struct BurnDriver BurnDrvCpsMvsc;
static struct BurnDriver* pDriver[] = {
  &BurnDrvCpsMvsc,     // Marvel vs. Capcom Clash of Super Heroes (USA 980123)
  NULL
};
EOL

# Make the script executable
chmod +x generate_driverlist.sh 