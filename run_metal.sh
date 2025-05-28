#!/bin/bash

# Colors for terminal output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[0;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Set default ROM path
DEFAULT_ROM_PATH="/Users/plasx/dev/ROMs"
ROM_NAME=""
FULLSCREEN=0
USE_MENU=0
SHOW_FPS=0
DEBUG=0

# Parse command line arguments
while [[ $# -gt 0 ]]; do
  case $1 in
    -p|--rom-path)
      ROM_PATH="$2"
      shift 2
      ;;
    -f|--fullscreen)
      FULLSCREEN=1
      shift
      ;;
    -m|--menu)
      USE_MENU=1
      shift
      ;;
    --show-fps)
      SHOW_FPS=1
      shift
      ;;
    -d|--debug)
      DEBUG=1
      shift
      ;;
    --help)
      echo "Usage: $0 [options] [rom_name]"
      echo "Options:"
      echo "  -p, --rom-path PATH  Set ROM path (default: $DEFAULT_ROM_PATH)"
      echo "  -f, --fullscreen     Start in fullscreen mode"
      echo "  -m, --menu           Show ROM browser menu at startup"
      echo "  --show-fps           Display FPS counter"
      echo "  -d, --debug          Enable debug output"
      echo "  --help               Show this help message"
      echo ""
      echo "Example: $0 -p /path/to/roms mvsc"
      exit 0
      ;;
    *)
      ROM_NAME="$1"
      shift
      ;;
  esac
done

# Set ROM path if not specified
if [ -z "$ROM_PATH" ]; then
  ROM_PATH="$DEFAULT_ROM_PATH"
fi

# Display header
echo -e "${BLUE}========================================================${NC}"
echo -e "${BLUE}        FBNeo Metal Implementation Launcher             ${NC}"
echo -e "${BLUE}========================================================${NC}"

# Create Metal app directory if it doesn't exist
mkdir -p bin/metal

# Check if we need to build
if [ ! -f bin/metal/fbneo_metal ] || [ -n "$(find src -newer bin/metal/fbneo_metal 2>/dev/null)" ]; then
  echo -e "${YELLOW}Executable needs to be built or updated${NC}"
  echo -e "${GREEN}Building Metal implementation...${NC}"
  make -f makefile.metal clean && make -f makefile.metal -j10

  if [ $? -ne 0 ]; then
    echo -e "${RED}Build failed${NC}"
    exit 1
  fi
else
  echo -e "${GREEN}Using existing executable${NC}"
fi

# Set executable path
EXECUTABLE="./bin/metal/fbneo_metal"

# Check if the executable exists
if [ ! -f "$EXECUTABLE" ]; then
  echo -e "${RED}Error: FBNeo Metal executable not found at ${EXECUTABLE}${NC}"
  echo -e "${RED}Build failed or was incomplete${NC}"
  exit 1
fi

# Display system information
echo -e "${GREEN}=== System Information ===${NC}"
echo -e "${GREEN}OS: $(uname -s) $(uname -r) $(uname -m)${NC}"
echo -e "${GREEN}=== FBNeo Information ===${NC}"
echo -e "${GREEN}ROM Path: ${ROM_PATH}${NC}"
if [ -n "$ROM_NAME" ]; then
  echo -e "${GREEN}ROM: ${ROM_NAME}${NC}"
fi

# Build command line arguments
CMD="$EXECUTABLE --rom-path=\"$ROM_PATH\""

if [ $FULLSCREEN -eq 1 ]; then
  CMD="$CMD --fullscreen"
fi

if [ $USE_MENU -eq 1 ]; then
  CMD="$CMD --menu"
fi

if [ $SHOW_FPS -eq 1 ]; then
  CMD="$CMD --show-fps"
fi

if [ -n "$ROM_NAME" ]; then
  CMD="$CMD $ROM_NAME"
fi

# Display command in debug mode
if [ $DEBUG -eq 1 ]; then
  echo -e "${YELLOW}Running command: ${CMD}${NC}"
fi

echo -e "${BLUE}========================================================${NC}"
echo -e "${GREEN}Starting FBNeo Metal...${NC}"

# Run the executable
eval $CMD

# Check exit status
EXIT_STATUS=$?
if [ $EXIT_STATUS -ne 0 ]; then
  echo -e "${RED}Application exited with error code: $EXIT_STATUS${NC}"
else
  echo -e "${GREEN}Application exited successfully.${NC}"
fi 