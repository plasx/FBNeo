#!/bin/bash
# Comprehensive script to fix all duplicate symbols in FBNeo Metal build

set -e  # Exit on error

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
RESET='\033[0m'

echo -e "${BLUE}========================================================${RESET}"
echo -e "${BLUE}   FBNeo Metal - Fixing All Duplicate Symbols           ${RESET}"
echo -e "${BLUE}========================================================${RESET}"

# Make the fix scripts executable
echo -e "${BLUE}Making fix scripts executable...${RESET}"
chmod +x fix_duplicates_sound.sh
chmod +x fix_duplicates_error.sh
chmod +x fix_duplicates_init.sh
chmod +x fix_duplicates_driver.sh
chmod +x fix_duplicates_input.sh
chmod +x fix_duplicates_audio.sh
chmod +x fix_duplicates_lastError.sh

# Run each fix script
echo -e "${BLUE}Running fix scripts...${RESET}"
./fix_duplicates_sound.sh
./fix_duplicates_error.sh 
./fix_duplicates_init.sh
./fix_duplicates_driver.sh
./fix_duplicates_input.sh
./fix_duplicates_audio.sh
./fix_duplicates_lastError.sh

# Update the makefile to use our unified implementation files
echo -e "${BLUE}Updating makefile.metal...${RESET}"

# Create backup of makefile
cp makefile.metal makefile.metal.bak

# Add our unified implementations to STANDALONE_SRC_FILES
sed -i '' 's/build\/metal\/sound_stubs.c/build\/metal_fixed\/sound_globals.c build\/metal_fixed\/init_functions.c build\/metal_fixed\/driver_functions.c build\/metal_fixed\/error_functions.c build\/metal_fixed\/input_functions.c build\/metal_fixed\/audio_functions.c build\/metal_fixed\/rom_functions.c/' makefile.metal

# Comment out conflicting stub files to avoid duplicate symbols
sed -i '' 's/src\/burner\/metal\/fixes\/metal_cps_stubs.c/# src\/burner\/metal\/fixes\/metal_cps_stubs.c # Removed to avoid duplicate symbols/' makefile.metal
sed -i '' 's/src\/burner\/metal\/fixes\/metal_stubs.c/# src\/burner\/metal\/fixes\/metal_stubs.c # Removed to avoid duplicate symbols/' makefile.metal

echo -e "${GREEN}Makefile updated successfully!${RESET}"

# Creating a simple script to build and run Marvel vs Capcom
echo -e "${BLUE}Creating build and run script...${RESET}"

cat > build_and_run_mvsc.sh << 'EOL'
#!/bin/bash
# Build and run Marvel vs Capcom with Metal frontend

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
RESET='\033[0m'

echo -e "${BLUE}Building FBNeo Metal...${RESET}"
make -f makefile.metal clean && make -f makefile.metal -j10

if [ $? -ne 0 ]; then
    echo -e "${RED}Build failed!${RESET}"
    exit 1
fi

echo -e "${GREEN}Build succeeded!${RESET}"

# Default ROM path - adjust as needed
ROM_PATH="$HOME/dev/ROMs/mvsc.zip"

# Check if a different ROM path was provided
if [ "$1" != "" ]; then
    ROM_PATH="$1"
fi

# Verify ROM file exists
if [ ! -f "$ROM_PATH" ]; then
    echo -e "${RED}Error: ROM file not found at $ROM_PATH${RESET}"
    echo -e "${YELLOW}Usage: ./build_and_run_mvsc.sh [path/to/mvsc.zip]${RESET}"
    exit 1
fi

echo -e "${BLUE}Running Marvel vs Capcom with Metal implementation...${RESET}"
echo -e "${BLUE}ROM: ${YELLOW}$ROM_PATH${RESET}"

# Run the emulator
./fbneo_metal "$ROM_PATH"
EOL

chmod +x build_and_run_mvsc.sh

echo -e "${GREEN}All duplicate symbols have been fixed!${RESET}"
echo -e "${YELLOW}To build and run, use: ./build_and_run_mvsc.sh [path/to/mvsc.zip]${RESET}"
echo -e "${BLUE}========================================================${RESET}" 