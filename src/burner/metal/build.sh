#!/bin/bash
# Build script for FBNeo Metal

set -e

# Set directories
SRCDIR="../.."
OUTPUT_DIR="../../../bin/metal"
OBJDIR="../../obj"

# Make sure directories exist
mkdir -p $OUTPUT_DIR
mkdir -p $OBJDIR/burner/metal/app
mkdir -p $OBJDIR/burner/metal/fixes
mkdir -p $OBJDIR/burner/metal/ai
mkdir -p $OBJDIR/burn
mkdir -p $OBJDIR/intf/video/metal

# Check architecture
ARCH=$(uname -m)
if [ "$ARCH" == "arm64" ]; then
    ARCH_FLAG="-arch arm64"
else
    ARCH_FLAG="-arch x86_64"
fi

# Frameworks
FRAMEWORKS="-framework Metal -framework MetalKit -framework Cocoa -framework CoreGraphics -framework QuartzCore -framework Foundation -framework AppKit -framework CoreAudio -framework AudioToolbox -framework CoreML -lz"

# Include directories
INCLUDES="-I../../.. -I../../../burn -I../../../burn/devices -I../../../burn/sound -I../../../burn/snd \
          -I../../../burner -I../../../burner/metal -I../../../burner/metal/app -I../../../burner/metal/fixes \
          -I../../../cpu -I../../../cpu/m68k -I../../../cpu/z80 -I../../../dep/generated \
          -I../../../intf/video -I../../../intf/audio -I../../../intf/input -I/opt/homebrew/include"

# Compiler flags
CXXFLAGS="$ARCH_FLAG -std=c++17 -O2 $INCLUDES -DBUILD_METAL -DINCLUDE_METAL_IMPL -Damd64=1 -DINCLUDE_7Z_SUPPORT"

# Needed source files
METAL_SOURCES=(
    "$SRCDIR/burner/metal/app/metal_app.mm"
    "$SRCDIR/burner/metal/app/metal_menu.mm"
    "$SRCDIR/burner/metal/app/metal_datasources.mm"
    "$SRCDIR/burner/metal/metal_renderer.mm"
    "$SRCDIR/burner/metal/main_metal.mm"
    "$SRCDIR/burner/metal/game_main.mm"
    "$SRCDIR/burner/metal/game_input.mm"
    "$SRCDIR/burner/metal/game_audio.mm"
    "$SRCDIR/burner/metal/game_renderer.mm"
    "$SRCDIR/burner/metal/metal_bridge.cpp"
    "$SRCDIR/burner/metal/metal_exports.cpp"
    "$SRCDIR/burner/metal/metal_app_delegate.mm"
    "$SRCDIR/burner/metal/ai_menu_integration.mm"
    "$SRCDIR/burner/metal/rom_fixes.cpp"
    "$SRCDIR/burn/wrapper_burn.cpp"
)

# Build minimal Metal test app
build_minimal() {
    echo "Building minimal Metal test app..."
    clang++ $CXXFLAGS $INCLUDES -c minimal_metal.mm -o $OBJDIR/burner/metal/minimal_metal.o
    
    echo "Linking minimal Metal test app..."
    clang++ -o $OUTPUT_DIR/minimal_metal $OBJDIR/burner/metal/minimal_metal.o $FRAMEWORKS
    
    if [ $? -eq 0 ]; then
        echo "Minimal Metal test app build successful!"
        echo "Binary location: $OUTPUT_DIR/minimal_metal"
    else
        echo "Error: Minimal Metal test app build failed."
    fi
}

# Build complete Metal implementation
build_metal() {
    echo "Building FBNeo Metal implementation..."
    
    # Compile stubs and essential bridge code
    clang++ $CXXFLAGS $INCLUDES -c fixes/burn_stubs.cpp -o $OBJDIR/burner/metal/fixes/burn_stubs.o
    
    # Compile our Metal implementation files
    clang++ $CXXFLAGS $INCLUDES -c metal_bridge.cpp -o $OBJDIR/burner/metal/metal_bridge.o
    clang++ $CXXFLAGS $INCLUDES -c metal_input.cpp -o $OBJDIR/burner/metal/metal_input.o
    clang++ $CXXFLAGS $INCLUDES -c metal_audio.mm -o $OBJDIR/burner/metal/metal_audio.o
    clang++ $CXXFLAGS $INCLUDES -c metal_app.mm -o $OBJDIR/burner/metal/metal_app.o
    
    # Compile Metal renderer
    clang++ $CXXFLAGS $INCLUDES -c metal_renderer.mm -o $OBJDIR/burner/metal/metal_renderer.o
    
    # Compile core FBNeo engine files
    clang++ $CXXFLAGS $INCLUDES -c $SRCDIR/burn/burn.cpp -o $OBJDIR/burn/burn.o
    clang++ $CXXFLAGS $INCLUDES -c $SRCDIR/burn/burn_memory.cpp -o $OBJDIR/burn/burn_memory.o
    clang++ $CXXFLAGS $INCLUDES -c $SRCDIR/burn/burn_sound.cpp -o $OBJDIR/burn/burn_sound.o
    clang++ $CXXFLAGS $INCLUDES -c $SRCDIR/burn/burn_gun.cpp -o $OBJDIR/burn/burn_gun.o
    
    # Compile CPS2 driver files
    clang++ $CXXFLAGS $INCLUDES -c $SRCDIR/burn/drv/capcom/cps.cpp -o $OBJDIR/burn/drv/capcom/cps.o
    clang++ $CXXFLAGS $INCLUDES -c $SRCDIR/burn/drv/capcom/cps_config.cpp -o $OBJDIR/burn/drv/capcom/cps_config.o
    clang++ $CXXFLAGS $INCLUDES -c $SRCDIR/burn/drv/capcom/cps_draw.cpp -o $OBJDIR/burn/drv/capcom/cps_draw.o
    clang++ $CXXFLAGS $INCLUDES -c $SRCDIR/burn/drv/capcom/cps_mem.cpp -o $OBJDIR/burn/drv/capcom/cps_mem.o
    clang++ $CXXFLAGS $INCLUDES -c $SRCDIR/burn/drv/capcom/cps_obj.cpp -o $OBJDIR/burn/drv/capcom/cps_obj.o
    clang++ $CXXFLAGS $INCLUDES -c $SRCDIR/burn/drv/capcom/cps_pal.cpp -o $OBJDIR/burn/drv/capcom/cps_pal.o
    clang++ $CXXFLAGS $INCLUDES -c $SRCDIR/burn/drv/capcom/cps_run.cpp -o $OBJDIR/burn/drv/capcom/cps_run.o
    clang++ $CXXFLAGS $INCLUDES -c $SRCDIR/burn/drv/capcom/cps_rw.cpp -o $OBJDIR/burn/drv/capcom/cps_rw.o
    clang++ $CXXFLAGS $INCLUDES -c $SRCDIR/burn/drv/capcom/cps_scr.cpp -o $OBJDIR/burn/drv/capcom/cps_scr.o
    clang++ $CXXFLAGS $INCLUDES -c $SRCDIR/burn/drv/capcom/cps2_crpt.cpp -o $OBJDIR/burn/drv/capcom/cps2_crpt.o
    clang++ $CXXFLAGS $INCLUDES -c $SRCDIR/burn/drv/capcom/d_cps2.cpp -o $OBJDIR/burn/drv/capcom/d_cps2.o
    
    # CPU cores
    clang++ $CXXFLAGS $INCLUDES -c $SRCDIR/cpu/m68k/m68kcpu.c -o $OBJDIR/cpu/m68k/m68kcpu.o
    clang++ $CXXFLAGS $INCLUDES -c $SRCDIR/cpu/m68k/m68kopac.c -o $OBJDIR/cpu/m68k/m68kopac.o
    clang++ $CXXFLAGS $INCLUDES -c $SRCDIR/cpu/m68k/m68kopdm.c -o $OBJDIR/cpu/m68k/m68kopdm.o
    clang++ $CXXFLAGS $INCLUDES -c $SRCDIR/cpu/m68k/m68kopnz.c -o $OBJDIR/cpu/m68k/m68kopnz.o
    clang++ $CXXFLAGS $INCLUDES -c $SRCDIR/cpu/m68k/m68kops.c -o $OBJDIR/cpu/m68k/m68kops.o
    
    # Link everything together
    echo "Linking FBNeo Metal implementation..."
    clang++ -o $OUTPUT_DIR/fbneo_metal \
        $OBJDIR/burner/metal/fixes/burn_stubs.o \
        $OBJDIR/burner/metal/metal_bridge.o \
        $OBJDIR/burner/metal/metal_input.o \
        $OBJDIR/burner/metal/metal_audio.o \
        $OBJDIR/burner/metal/metal_app.o \
        $OBJDIR/burner/metal/metal_renderer.o \
        $OBJDIR/burn/burn.o \
        $OBJDIR/burn/burn_memory.o \
        $OBJDIR/burn/burn_sound.o \
        $OBJDIR/burn/burn_gun.o \
        $OBJDIR/burn/drv/capcom/cps.o \
        $OBJDIR/burn/drv/capcom/cps_config.o \
        $OBJDIR/burn/drv/capcom/cps_draw.o \
        $OBJDIR/burn/drv/capcom/cps_mem.o \
        $OBJDIR/burn/drv/capcom/cps_obj.o \
        $OBJDIR/burn/drv/capcom/cps_pal.o \
        $OBJDIR/burn/drv/capcom/cps_run.o \
        $OBJDIR/burn/drv/capcom/cps_rw.o \
        $OBJDIR/burn/drv/capcom/cps_scr.o \
        $OBJDIR/burn/drv/capcom/cps2_crpt.o \
        $OBJDIR/burn/drv/capcom/d_cps2.o \
        $OBJDIR/cpu/m68k/m68kcpu.o \
        $OBJDIR/cpu/m68k/m68kopac.o \
        $OBJDIR/cpu/m68k/m68kopdm.o \
        $OBJDIR/cpu/m68k/m68kopnz.o \
        $OBJDIR/cpu/m68k/m68kops.o \
        $FRAMEWORKS
        
    if [ $? -eq 0 ]; then
        echo "FBNeo Metal build successful!"
        echo "Binary location: $OUTPUT_DIR/fbneo_metal"
    else
        echo "Error: FBNeo Metal build failed."
    fi
    
    # Create App bundle if it doesn't exist
    if [ ! -d "$OUTPUT_DIR/FBNeo.app" ]; then
        echo "Creating App bundle..."
        mkdir -p "$OUTPUT_DIR/FBNeo.app/Contents/MacOS"
        mkdir -p "$OUTPUT_DIR/FBNeo.app/Contents/Resources"
        
        # Copy executable
        cp "$OUTPUT_DIR/fbneo_metal" "$OUTPUT_DIR/FBNeo.app/Contents/MacOS/FBNeo"
        
        # Create Info.plist
        cat > "$OUTPUT_DIR/FBNeo.app/Contents/Info.plist" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleDevelopmentRegion</key>
    <string>English</string>
    <key>CFBundleExecutable</key>
    <string>FBNeo</string>
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
    <key>CFBundleSignature</key>
    <string>????</string>
    <key>CFBundleVersion</key>
    <string>1</string>
    <key>NSPrincipalClass</key>
    <string>NSApplication</string>
    <key>NSHighResolutionCapable</key>
    <true/>
</dict>
</plist>
EOF
        
        echo "App bundle created: $OUTPUT_DIR/FBNeo.app"
    fi
}

# Run the Metal implementation
run_metal() {
    echo "Running FBNeo Metal..."
    open "$OUTPUT_DIR/FBNeo.app"
}

# Clean build artifacts
clean() {
    echo "Cleaning build artifacts..."
    rm -rf $OBJDIR
    rm -f $OUTPUT_DIR/fbneo_metal
    rm -f $OUTPUT_DIR/minimal_metal
}

# Parse command line arguments
case "$1" in
    minimal)
        build_minimal
        ;;
    metal)
        build_metal
        ;;
    run)
        run_metal
        ;;
    clean)
        clean
        ;;
    all)
        clean
        build_minimal
        build_metal
        ;;
    *)
        echo "Usage: $0 {minimal|metal|run|clean|all}"
        exit 1
        ;;
esac

exit 0 