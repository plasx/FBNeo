#!/bin/bash
# FBNeo Metal build script for macOS
# Builds FBNeo Metal port for Apple Silicon (ARM64) and Intel (x86_64)

set -e  # Exit on error

# Directories
FBNEO_ROOT_DIR="$(pwd)"
METAL_DIR="${FBNEO_ROOT_DIR}/src/burner/metal"
BUILD_DIR="${METAL_DIR}/build"
BIN_DIR="${METAL_DIR}/bin"
OBJ_DIR="${METAL_DIR}/obj"
SHADER_DIR="${METAL_DIR}"

# Metal shader compiler settings
METAL_COMPILER="/usr/bin/xcrun metal"
METAL_COMPILER_FLAGS="-std=macos-metal2.0 -Wall -O3"
METALLIB_TOOL="/usr/bin/xcrun metallib"

# Build settings
CXX="clang++"
CXXFLAGS="-std=c++17 -O3 -Wall -Wno-deprecated-declarations"
LDFLAGS=""
INCLUDES="-I${FBNEO_ROOT_DIR}/src/burn -I${FBNEO_ROOT_DIR}/src -I${FBNEO_ROOT_DIR}/src/burner -I${FBNEO_ROOT_DIR}/src/burner/metal -I${FBNEO_ROOT_DIR}/src/dep/libs/zlib"
DEFINES="-DUSE_METAL -DUSE_METAL_FIXES -DMACOS_X -DINCLUDE_LIB_PNGH"

# macOS frameworks
FRAMEWORKS="-framework Cocoa -framework Metal -framework MetalKit -framework QuartzCore -framework CoreVideo -framework CoreAudio -framework AudioToolbox -framework CoreGraphics"

# Architecture settings
ARCHS="arm64 x86_64"
TARGET_MACROS="MACOS=1 USE_METAL=1 LIBRETRO=0 BUILD_X64_EX=0 INCLUDE_7Z_SUPPORT=0"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Create directories if they don't exist
mkdir -p "${BUILD_DIR}"
mkdir -p "${BIN_DIR}"
mkdir -p "${OBJ_DIR}"

echo -e "${BLUE}Building FBNeo Metal port for macOS...${NC}"

# Compile Metal shaders
echo -e "${YELLOW}Compiling Metal shaders...${NC}"
SHADER_FILES=("${SHADER_DIR}/Shaders.metal" "${SHADER_DIR}/enhanced_metal_shaders.metal")
SHADER_OUTPUTS=()

for shader in "${SHADER_FILES[@]}"; do
    if [ -f "$shader" ]; then
        BASENAME=$(basename "$shader" .metal)
        AIR_FILE="${BUILD_DIR}/${BASENAME}.air"
        echo "  Compiling shader: $shader -> $AIR_FILE"
        
        $METAL_COMPILER $METAL_COMPILER_FLAGS -c "$shader" -o "$AIR_FILE"
        SHADER_OUTPUTS+=("$AIR_FILE")
    fi
done

# Create metallib from compiled shaders
if [ ${#SHADER_OUTPUTS[@]} -gt 0 ]; then
    echo "  Creating metallib..."
    $METALLIB_TOOL -o "${BIN_DIR}/fbneo_shaders.metallib" "${SHADER_OUTPUTS[@]}"
    echo -e "${GREEN}  Shader compilation complete!${NC}"
else
    echo -e "${RED}  No shaders found to compile!${NC}"
fi

# Build FBNeo Metal for multiple architectures
for arch in $ARCHS; do
    echo -e "${YELLOW}Compiling for $arch architecture...${NC}"
    
    # Architecture-specific flags
    ARCH_CXXFLAGS="$CXXFLAGS -arch $arch -target $arch-apple-macos11.0"
    ARCH_LDFLAGS="$LDFLAGS -arch $arch"
    ARCH_OBJ_DIR="${OBJ_DIR}/$arch"
    
    mkdir -p "$ARCH_OBJ_DIR"
    
    # Build FBNeo Metal using makefile
    make -f makefile.metal $TARGET_MACROS MACOSX_USE_LIBSDL=1 CXXFLAGS="$ARCH_CXXFLAGS $INCLUDES $DEFINES" LDFLAGS="$ARCH_LDFLAGS $FRAMEWORKS" OBJDIR="$ARCH_OBJ_DIR"
    
    # Copy the binary to a temporary architecture-specific name
    mv "${BIN_DIR}/fbneo" "${BIN_DIR}/fbneo_$arch"
    
    echo -e "${GREEN}Compilation for $arch complete!${NC}"
done

# Create universal binary
echo -e "${YELLOW}Creating universal binary...${NC}"
lipo -create -output "${BIN_DIR}/fbneo" "${BIN_DIR}/fbneo_arm64" "${BIN_DIR}/fbneo_x86_64"

# Clean up temporary architecture-specific binaries
rm "${BIN_DIR}/fbneo_arm64" "${BIN_DIR}/fbneo_x86_64"

echo -e "${GREEN}Build complete! Universal binary created at ${BIN_DIR}/fbneo${NC}"
echo -e "${BLUE}To run: ${BIN_DIR}/fbneo${NC}"

# Create app bundle
echo -e "${YELLOW}Creating app bundle...${NC}"
APP_DIR="${BIN_DIR}/FBNeo.app"
CONTENTS_DIR="${APP_DIR}/Contents"
MACOS_DIR="${CONTENTS_DIR}/MacOS"
RESOURCES_DIR="${CONTENTS_DIR}/Resources"

mkdir -p "${MACOS_DIR}"
mkdir -p "${RESOURCES_DIR}"

# Copy binary and resources
cp "${BIN_DIR}/fbneo" "${MACOS_DIR}/"
cp "${BIN_DIR}/fbneo_shaders.metallib" "${RESOURCES_DIR}/"

# Create Info.plist
cat > "${CONTENTS_DIR}/Info.plist" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>fbneo</string>
    <key>CFBundleIconFile</key>
    <string>AppIcon</string>
    <key>CFBundleIdentifier</key>
    <string>com.fbneo.metal</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundleName</key>
    <string>FBNeo</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0</string>
    <key>CFBundleVersion</key>
    <string>1</string>
    <key>LSMinimumSystemVersion</key>
    <string>11.0</string>
    <key>NSHighResolutionCapable</key>
    <true/>
</dict>
</plist>
EOF

echo -e "${GREEN}App bundle created at ${APP_DIR}${NC}"
echo -e "${BLUE}To run the app: open ${APP_DIR}${NC}" 