#!/bin/bash

# Colors for terminal output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[0;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Set ROM path and name
ROM_PATH="/Users/plasx/dev/ROMs"
ROM_NAME="mvsc"
FULL_ROM_PATH="${ROM_PATH}/${ROM_NAME}.zip"

echo -e "${BLUE}========================================================${NC}"
echo -e "${BLUE}   FBNeo Metal - Marvel vs. Capcom: Clash of Super Heroes   ${NC}"
echo -e "${BLUE}========================================================${NC}"

# Check if ROM exists
if [ ! -f "$FULL_ROM_PATH" ]; then
  echo -e "${RED}ERROR: ROM file not found: ${FULL_ROM_PATH}${NC}"
  echo -e "${YELLOW}Please ensure the ROM file exists at this location.${NC}"
  exit 1
fi

# Set executable path
EXECUTABLE="./bin/metal/fbneo_metal_app"

# Check if the executable exists
if [ ! -f "$EXECUTABLE" ]; then
  echo -e "${RED}Error: FBNeo Metal executable not found at ${EXECUTABLE}${NC}"
  exit 1
fi

# Display system information
echo -e "${GREEN}=== System Information ===${NC}"
echo -e "${GREEN}OS: $(uname -s) $(uname -r) $(uname -m)${NC}"
echo -e "${GREEN}=== Game Information ===${NC}"
echo -e "${GREEN}ROM: Marvel vs. Capcom: Clash of Super Heroes (${ROM_NAME}.zip)${NC}"
echo -e "${GREEN}Publisher: Capcom${NC}"
echo -e "${GREEN}Year: 1998${NC}"
echo -e "${GREEN}Hardware: CPS-2${NC}"

echo -e "${BLUE}========================================================${NC}"
echo -e "${GREEN}Starting Marvel vs. Capcom...${NC}"

# Run the executable with the ROM
"$EXECUTABLE" "$FULL_ROM_PATH"

# Check exit status
EXIT_STATUS=$?
if [ $EXIT_STATUS -ne 0 ]; then
  echo -e "${RED}Game exited with error code: $EXIT_STATUS${NC}"
else
  echo -e "${GREEN}Game session ended successfully.${NC}"
fi 