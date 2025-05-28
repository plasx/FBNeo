#!/bin/bash
# Build and run the FBNeo Metal test app

# Colors for terminal output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[0;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${BLUE}========================================================${NC}"
echo -e "${GREEN}Building FBNeo Metal Test Application...${NC}"
echo -e "${BLUE}========================================================${NC}"

# Clean and build
make -f makefile.metal.test clean
if ! make -f makefile.metal.test -j10; then
  echo -e "${RED}Build failed!${NC}"
  exit 1
fi

# Check if the executable exists
if [ ! -f "./metal_test" ]; then
  echo -e "${RED}Executable not found at ./metal_test${NC}"
  exit 1
fi

echo -e "${BLUE}========================================================${NC}"
echo -e "${GREEN}Running Metal Test Application...${NC}"
echo -e "${BLUE}========================================================${NC}"

# Run the test application
./metal_test

# Check exit status
EXIT_STATUS=$?
if [ $EXIT_STATUS -ne 0 ]; then
  echo -e "${RED}Application exited with error code: $EXIT_STATUS${NC}"
  exit $EXIT_STATUS
else
  echo -e "${GREEN}Application exited successfully.${NC}"
fi 