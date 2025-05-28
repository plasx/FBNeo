#!/bin/bash
# Install necessary debug tools for FBNeo Metal

# Colors for better visualization
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${GREEN}Installing debug tools for FBNeo Metal...${NC}"

# Check if Homebrew is installed
if ! command -v brew &> /dev/null; then
    echo -e "${RED}Error: Homebrew is not installed. Please install it first:${NC}"
    echo -e "${YELLOW}/bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\"${NC}"
    exit 1
fi

# Install dtach
echo -e "${BLUE}Installing dtach...${NC}"
brew install dtach || { echo -e "${RED}Failed to install dtach${NC}"; exit 1; }

# Install socat
echo -e "${BLUE}Installing socat...${NC}"
brew install socat || { echo -e "${RED}Failed to install socat${NC}"; exit 1; }

# Install unbuffer via expect
echo -e "${BLUE}Installing expect (for unbuffer command)...${NC}"
brew install expect || { echo -e "${RED}Failed to install expect${NC}"; exit 1; }

# Create a script to run with dtach
cat > run_with_dtach.sh << 'EOF'
#!/bin/bash
# Run FBNeo Metal with dtach to force unbuffered output

# Colors for better visualization
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Show usage information if no arguments provided
if [ "$#" -eq 0 ]; then
    echo -e "${YELLOW}Usage: $0 /path/to/rom.zip${NC}"
    echo -e "${YELLOW}Example: $0 /Users/plasx/dev/ROMs/mvsc.zip${NC}"
    exit 1
fi

# Get ROM path from arguments
ROM_PATH="$1"

# Create a temporary socket
SOCKET="/tmp/fbneo_dtach_socket"

# Create a temporary log directory if it doesn't exist
LOG_DIR="/tmp/fbneo_logs"
mkdir -p $LOG_DIR

# Generate a timestamp for the log file
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
LOG_FILE="$LOG_DIR/fbneo_metal_$TIMESTAMP.log"

echo -e "${GREEN}Running FBNeo Metal with debug output forced to terminal${NC}"
echo -e "${GREEN}ROM: $ROM_PATH${NC}"
echo -e "${GREEN}Debug log will also be saved to: $LOG_FILE${NC}"
echo -e "${GREEN}=======================================================${NC}"

# Run in dtach with no controlling terminal, redirecting output to tee
# The -n flag creates a new session without detaching
# The -E flag makes dtach not interpret Ctrl+\ as detach sequence
dtach -n "$SOCKET" -E ./fbneo_metal "$ROM_PATH" 2>&1 | tee "$LOG_FILE"

echo -e "${BLUE}Emulation ended. Debug log saved to: $LOG_FILE${NC}"

# Offer to view the log file
echo -e "${YELLOW}Would you like to view the log file? (y/n)${NC}"
read -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    less "$LOG_FILE"
fi
EOF

# Create a script to run with unbuffer
cat > run_with_unbuffer.sh << 'EOF'
#!/bin/bash
# Run FBNeo Metal with unbuffer to force unbuffered output

# Colors for better visualization
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Show usage information if no arguments provided
if [ "$#" -eq 0 ]; then
    echo -e "${YELLOW}Usage: $0 /path/to/rom.zip${NC}"
    echo -e "${YELLOW}Example: $0 /Users/plasx/dev/ROMs/mvsc.zip${NC}"
    exit 1
fi

# Get ROM path from arguments
ROM_PATH="$1"

# Create a temporary log directory if it doesn't exist
LOG_DIR="/tmp/fbneo_logs"
mkdir -p $LOG_DIR

# Generate a timestamp for the log file
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
LOG_FILE="$LOG_DIR/fbneo_metal_$TIMESTAMP.log"

echo -e "${GREEN}Running FBNeo Metal with debug output forced to terminal${NC}"
echo -e "${GREEN}ROM: $ROM_PATH${NC}"
echo -e "${GREEN}Debug log will also be saved to: $LOG_FILE${NC}"
echo -e "${GREEN}=======================================================${NC}"

# Run with unbuffer to force line-buffered output
unbuffer ./fbneo_metal "$ROM_PATH" 2>&1 | tee "$LOG_FILE"

echo -e "${BLUE}Emulation ended. Debug log saved to: $LOG_FILE${NC}"

# Offer to view the log file
echo -e "${YELLOW}Would you like to view the log file? (y/n)${NC}"
read -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    less "$LOG_FILE"
fi
EOF

# Make scripts executable
chmod +x run_with_dtach.sh run_with_unbuffer.sh

echo -e "${GREEN}Debug tools installed successfully!${NC}"
echo -e "${BLUE}You can now run FBNeo Metal with debug output using:${NC}"
echo -e "${YELLOW}./run_with_dtach.sh /path/to/rom.zip${NC}"
echo -e "${YELLOW}./run_with_unbuffer.sh /path/to/rom.zip${NC}"
echo -e "${YELLOW}./run_with_socat.sh /path/to/rom.zip${NC}"
echo -e "${YELLOW}./run_terminal_debug.sh /path/to/rom.zip${NC}"
echo -e "${YELLOW}./run_with_debug.py /path/to/rom.zip${NC}" 