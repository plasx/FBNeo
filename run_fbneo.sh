#!/bin/bash

# Colors for terminal output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

echo -e "${BLUE}========================================================${NC}"
echo -e "${BLUE}        FBNeo Metal AI Enhanced Launcher                ${NC}"
echo -e "${BLUE}========================================================${NC}"

FBNEO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EXECUTABLE="${FBNEO_DIR}/bin/metal/fbneo_metal_app"
ROM_DIR="${HOME}/Documents/ROMs"

# Create ROM directory if it doesn't exist
mkdir -p "${ROM_DIR}"

if [ ! -f "$EXECUTABLE" ]; then
    echo -e "${YELLOW}Error: FBNeo executable not found at ${EXECUTABLE}${NC}"
    echo -e "${YELLOW}Please build the application first.${NC}"
    exit 1
fi

echo -e "${GREEN}FBNeo executable: ${EXECUTABLE}${NC}"
echo -e "${GREEN}ROM directory: ${ROM_DIR}${NC}"
echo -e "${GREEN}Starting FBNeo Metal with AI enhancements...${NC}"

# Run FBNeo with the ROM directory path
"$EXECUTABLE" 