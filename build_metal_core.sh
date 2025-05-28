#!/bin/bash
# Build script for FBNeo Metal with core integration

# Colors for better output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;36m'
NC='\033[0m' # No Color

echo -e "${BLUE}FBNeo Metal Build Script${NC}"
echo "=============================="

# Create necessary directories
echo -e "${YELLOW}Creating necessary directories...${NC}"
mkdir -p src/dep
mkdir -p src/dep/generated
mkdir -p obj/metal/burner/metal/debug
mkdir -p obj/metal/burner/metal/ai
mkdir -p obj/metal/burner/metal/replay
mkdir -p build/metal/obj/src/cpu/m68k

# Create empty files for symlinks
echo -e "${YELLOW}Creating symlinks for missing headers...${NC}"
touch src/dep/generated/tiles_generic.h
touch src/dep/generated/burnint.h

# Create symlinks to actual header files
ln -sf ../../../../burn/tiles_generic.h src/dep/generated/tiles_generic.h
ln -sf ../../../../burn/burnint.h src/dep/generated/burnint.h

# Check for tchar.h issues
if grep -q "#define _T(s) s" src/burner/metal/tchar.h; then
    echo -e "${YELLOW}Fixing tchar.h...${NC}"
    sed -i '' 's/#define _T(s) s/#ifndef _T\n#define _T(s) s\n#endif/' src/burner/metal/tchar.h
fi

# Check for _DIRS_MAX issues in burner.h
if grep -q "DIRS_MAX" src/burner/burner.h; then
    echo -e "${YELLOW}Checking DIRS_MAX definition...${NC}"
    # This is more complex to fix automatically - warn the user
    echo -e "${RED}Warning: Check DIRS_MAX definition in src/burner/burner.h${NC}"
fi

# Add missing standard headers to m68kfpu.c if needed
if [ -f "src/cpu/m68k/m68kfpu.c" ]; then
    echo -e "${YELLOW}Checking for missing headers in m68kfpu.c...${NC}"
    
    # Check if stdio.h is already included
    if ! grep -q "#include <stdio.h>" src/cpu/m68k/m68kfpu.c; then
        echo -e "${YELLOW}Adding stdio.h to m68kfpu.c...${NC}"
        sed -i '' '1i\
#include <stdio.h>
' src/cpu/m68k/m68kfpu.c
    fi
    
    # Check if m68kcpu.h is already included
    if ! grep -q "#include \"m68kcpu.h\"" src/cpu/m68k/m68kfpu.c; then
        echo -e "${YELLOW}Adding m68kcpu.h to m68kfpu.c...${NC}"
        sed -i '' '1i\
#include "m68kcpu.h"
' src/cpu/m68k/m68kfpu.c
    fi
fi

# Build with makefile.metal
echo -e "${YELLOW}Building with makefile.metal...${NC}"
make -f makefile.metal

# Check build result
if [ -f "fbneo_metal" ]; then
    echo -e "${GREEN}Build successful!${NC}"
    
    # Create ROMs directory if it doesn't exist
    mkdir -p roms
    
    # Create a symlink to the user's ROMs directory if available
    USER_ROMS_DIR="$HOME/ROMs"
    if [ -d "$USER_ROMS_DIR" ]; then
        ln -sf "$USER_ROMS_DIR" roms
        echo "Created symlink to ROMs directory at $USER_ROMS_DIR"
    fi
    
    echo "You can now run the emulator with ./fbneo_metal"
else
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi 