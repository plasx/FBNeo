#!/bin/bash
# Run script for FBNeo Metal frontend with CPS2 ROMs

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
RESET='\033[0m'

# Default ROM directory - user can override this
ROM_DIR="${1:-$HOME/ROMs/CPS2}"

# Check if binary exists
if [ ! -f "./fbneo_metal" ]; then
    echo -e "${RED}Error: fbneo_metal binary not found!${RESET}"
    echo -e "Run the build_metal_final.sh script first."
    exit 1
fi

# Ensure binary is executable
chmod +x ./fbneo_metal

# List available ROMs
list_roms() {
    echo -e "${BLUE}Available ROMs in $ROM_DIR:${RESET}"
    
    # Check if directory exists
    if [ ! -d "$ROM_DIR" ]; then
        echo -e "${YELLOW}ROM directory not found: $ROM_DIR${RESET}"
        echo -e "Please specify a valid ROM directory as parameter."
        echo -e "Example: $0 /path/to/roms"
        return 1
    fi
    
    # List zip files
    rom_count=0
    for rom in "$ROM_DIR"/*.zip; do
        if [ -f "$rom" ]; then
            filename=$(basename "$rom")
            echo -e " ${GREEN}→${RESET} ${filename}"
            rom_count=$((rom_count + 1))
        fi
    done
    
    if [ $rom_count -eq 0 ]; then
        echo -e "${YELLOW}No .zip ROM files found in $ROM_DIR${RESET}"
        echo -e "Make sure your ROMs are in .zip format."
        return 1
    fi
    
    return 0
}

# Select a ROM interactively
select_rom() {
    echo -e "${BLUE}Popular CPS2 ROMs:${RESET}"
    echo -e " ${GREEN}1.${RESET} Marvel vs. Capcom (mvsc.zip)"
    echo -e " ${GREEN}2.${RESET} Street Fighter Alpha 3 (sfa3.zip)"
    echo -e " ${GREEN}3.${RESET} X-Men vs. Street Fighter (xmvsf.zip)"
    echo -e " ${GREEN}4.${RESET} Super Street Fighter II Turbo (ssf2t.zip)"
    echo -e " ${GREEN}5.${RESET} Darkstalkers (dstlk.zip)"
    echo -e " ${GREEN}6.${RESET} Vampire Savior (vsav.zip)"
    echo -e " ${GREEN}7.${RESET} Custom ROM (enter path)"
    echo -e " ${GREEN}8.${RESET} Exit"
    
    read -p "Enter your choice (1-8): " choice
    
    case $choice in
        1) rom_path="$ROM_DIR/mvsc.zip" ;;
        2) rom_path="$ROM_DIR/sfa3.zip" ;;
        3) rom_path="$ROM_DIR/xmvsf.zip" ;;
        4) rom_path="$ROM_DIR/ssf2t.zip" ;;
        5) rom_path="$ROM_DIR/dstlk.zip" ;;
        6) rom_path="$ROM_DIR/vsav.zip" ;;
        7) 
            read -p "Enter ROM path: " custom_path
            rom_path="$custom_path"
            ;;
        8) exit 0 ;;
        *) 
            echo -e "${RED}Invalid choice.${RESET}"
            return 1
            ;;
    esac
    
    # Check if ROM exists
    if [ ! -f "$rom_path" ]; then
        echo -e "${RED}ROM not found: $rom_path${RESET}"
        return 1
    fi
    
    echo -e "${GREEN}Selected ROM: $(basename "$rom_path")${RESET}"
    return 0
}

# Main execution
echo -e "${BLUE}FBNeo Metal Frontend Launcher${RESET}"
echo -e "${YELLOW}==============================${RESET}"

# List available ROMs
list_roms

# Select a ROM
while true; do
    if select_rom; then
        break
    fi
done

# Run the emulator with the selected ROM
echo -e "${BLUE}Running FBNeo Metal with $(basename "$rom_path")...${RESET}"
./fbneo_metal "$rom_path"

# Capture exit code
EXIT_CODE=$?

# Report result
if [ $EXIT_CODE -eq 0 ]; then
    echo -e "${GREEN}Emulator session ended normally.${RESET}"
else
    echo -e "${RED}Emulator exited with error code: $EXIT_CODE${RESET}"
fi
