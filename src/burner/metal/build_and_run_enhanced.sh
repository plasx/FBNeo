#!/bin/bash
# Build and run script for FBNeo with Metal renderer on macOS
# This script builds and runs the enhanced version of FBNeo with Metal renderer

# Set up colors for terminal output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print header
echo -e "${BLUE}==============================================================${NC}"
echo -e "${BLUE}  FBNeo Metal Renderer - Build and Run Script (Enhanced)      ${NC}"
echo -e "${BLUE}==============================================================${NC}"

# Set up environment variables
FBNEO_ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BUILD_DIR="${FBNEO_ROOT}/build/metal"
ROM_DIR=""
SELECTED_ROM=""

# Function to check if tools are installed
check_prerequisites() {
    echo -e "${YELLOW}Checking prerequisites...${NC}"
    
    # Check for Clang
    if ! command -v clang &> /dev/null; then
        echo -e "${RED}Error: Clang compiler not found.${NC}"
        echo "Please install Xcode Command Line Tools by running 'xcode-select --install'"
        exit 1
    fi
    
    # Check for Metal compiler tools
    if ! command -v xcrun &> /dev/null; then
        echo -e "${RED}Error: Xcode tools not found.${NC}"
        echo "Please install Xcode and Xcode Command Line Tools"
        exit 1
    fi
    
    echo -e "${GREEN}All prerequisites satisfied.${NC}"
}

# Function to build the enhanced version
build_enhanced() {
    echo -e "${YELLOW}Building enhanced Metal renderer...${NC}"
    
    # Navigate to root directory
    cd "$FBNEO_ROOT" || { echo -e "${RED}Error: Failed to navigate to FBNeo root directory${NC}"; exit 1; }
    
    # Clean previous build if requested
    if [ "$1" == "clean" ]; then
        echo "Cleaning previous build..."
        make -f makefile.metal clean
    fi
    
    # Build enhanced version
    make -f makefile.metal enhanced
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}Build failed!${NC}"
        exit 1
    fi
    
    echo -e "${GREEN}Build completed successfully.${NC}"
}

# Function to select ROM directory
select_rom_directory() {
    echo -e "${YELLOW}Select ROM directory:${NC}"
    
    # Check if ROM directory is already set
    if [ -n "$ROM_DIR" ] && [ -d "$ROM_DIR" ]; then
        echo -e "Current ROM directory: ${GREEN}$ROM_DIR${NC}"
        echo -e "1) Use current directory"
        echo -e "2) Select a different directory"
        read -p "Select option (1-2): " option
        
        if [ "$option" != "2" ]; then
            return
        fi
    fi
    
    # Ask user for ROM directory path
    read -p "Enter path to ROM directory: " ROM_DIR
    
    # Validate ROM directory
    if [ ! -d "$ROM_DIR" ]; then
        echo -e "${RED}Error: Directory not found.${NC}"
        ROM_DIR=""
        select_rom_directory
    else
        echo -e "${GREEN}ROM directory set to: $ROM_DIR${NC}"
    fi
}

# Function to select ROM
select_rom() {
    echo -e "${YELLOW}Selecting ROM:${NC}"
    
    # Check if ROM directory is set
    if [ -z "$ROM_DIR" ] || [ ! -d "$ROM_DIR" ]; then
        echo -e "${RED}Error: ROM directory not set or invalid.${NC}"
        select_rom_directory
    fi
    
    # Create a temporary file to store ROM filenames
    TMP_FILE=$(mktemp)
    
    # Find all potential ROM files (zip, bin, etc.)
    find "$ROM_DIR" -type f \( -name "*.zip" -o -name "*.7z" -o -name "*.bin" \) | sort > "$TMP_FILE"
    
    # Check if any ROMs found
    if [ ! -s "$TMP_FILE" ]; then
        echo -e "${RED}Error: No ROM files found in the directory.${NC}"
        ROM_DIR=""
        select_rom_directory
        select_rom
        return
    fi
    
    # List ROMs
    echo -e "${BLUE}Available ROMs:${NC}"
    cat -n "$TMP_FILE"
    
    # Select ROM
    read -p "Enter ROM number: " ROM_NUM
    
    # Validate selection
    if ! [[ "$ROM_NUM" =~ ^[0-9]+$ ]]; then
        echo -e "${RED}Error: Invalid input.${NC}"
        select_rom
    else
        # Get selected ROM path
        SELECTED_ROM=$(sed -n "${ROM_NUM}p" "$TMP_FILE")
        
        if [ -z "$SELECTED_ROM" ]; then
            echo -e "${RED}Error: Invalid selection.${NC}"
            select_rom
        else
            echo -e "${GREEN}Selected ROM: $(basename "$SELECTED_ROM")${NC}"
        fi
    fi
    
    # Clean up
    rm "$TMP_FILE"
}

# Function to run the emulator
run_emulator() {
    echo -e "${YELLOW}Running FBNeo with Metal renderer...${NC}"
    
    # Check if build exists
    if [ ! -f "$FBNEO_ROOT/fbneo_metal_enhanced" ]; then
        echo -e "${RED}Error: Executable not found. Please build first.${NC}"
        return
    fi
    
    # Run the emulator
    cd "$FBNEO_ROOT" || { echo -e "${RED}Error: Failed to navigate to FBNeo root directory${NC}"; exit 1; }
    
    echo -e "${BLUE}Starting emulator with ROM: $(basename "$SELECTED_ROM")${NC}"
    echo -e "${BLUE}Press Ctrl+C to terminate.${NC}"
    
    # Run with ROM
    ./fbneo_metal_enhanced "$SELECTED_ROM"
}

# Main execution
main() {
    check_prerequisites
    
    # Show menu
    while true; do
        echo -e "\n${BLUE}=== FBNeo Metal Renderer Menu ===${NC}"
        echo -e "1) Build (normal)"
        echo -e "2) Build (clean)"
        echo -e "3) Select ROM directory"
        echo -e "4) Select ROM"
        echo -e "5) Run emulator"
        echo -e "6) Build and run"
        echo -e "7) Exit"
        
        read -p "Select option (1-7): " option
        
        case $option in
            1) build_enhanced ;;
            2) build_enhanced clean ;;
            3) select_rom_directory ;;
            4) select_rom ;;
            5) 
                if [ -z "$SELECTED_ROM" ]; then
                    echo -e "${RED}Error: No ROM selected.${NC}"
                    select_rom
                fi
                run_emulator
                ;;
            6)
                build_enhanced
                if [ -z "$SELECTED_ROM" ]; then
                    select_rom_directory
                    select_rom
                fi
                run_emulator
                ;;
            7) 
                echo -e "${GREEN}Exiting...${NC}"
                exit 0
                ;;
            *)
                echo -e "${RED}Invalid option. Please try again.${NC}"
                ;;
        esac
    done
}

# Run main function
main 