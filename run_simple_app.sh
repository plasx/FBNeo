#!/bin/bash

# Colors for terminal output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

echo -e "${BLUE}========================================================${NC}"
echo -e "${BLUE}        FBNeo Metal - Simple ROM Browser Demo          ${NC}"
echo -e "${BLUE}========================================================${NC}"

# Set paths
FBNEO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EXECUTABLE="${FBNEO_DIR}/bin/metal/simple_fbneo_app"

# Check if the executable exists
if [ ! -f "$EXECUTABLE" ]; then
    echo -e "${YELLOW}Error: FBNeo Simple app executable not found at ${EXECUTABLE}${NC}"
    echo -e "${YELLOW}Please build the application first with ./build_simple_app.sh${NC}"
    exit 1
fi

echo -e "${GREEN}=== System Information ===${NC}"
echo -e "${GREEN}OS: $(uname -s) $(uname -r) $(uname -m)${NC}"
echo -e "${GREEN}=== FBNeo Simple App Information ===${NC}"
echo -e "${GREEN}Executable: ${EXECUTABLE}${NC}"
echo -e "${GREEN}Executable size: $(du -h "$EXECUTABLE" | cut -f1)${NC}"
echo -e "${BLUE}========================================================${NC}"

# Launch the app
echo -e "${GREEN}Starting FBNeo Simple ROM Browser...${NC}"
"$EXECUTABLE"

# Check exit status
EXIT_STATUS=$?
if [ $EXIT_STATUS -ne 0 ]; then
    echo -e "${YELLOW}Application exited with error code: $EXIT_STATUS${NC}"
else
    echo -e "${GREEN}Application exited successfully.${NC}"
fi 