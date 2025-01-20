# Makefile.metal for FBNeo with Metal and SDL2

TARGET = fbneo_metal
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
DARWIN = 1
endif

# Explicitly list sources for cross-platform compatibility
SOURCES = \
    src/intf/video/sdl/sdl2_video_metal.mm \
    src/intf/video/metal_renderer.mm \
    src/burn/burn.cpp \
    src/burn/cpuexec.cpp \
    src/burn/devices/joyprocess.cpp \
    src/burn/intf/burnint.cpp \
    src/burn/devices/input/input_manager.cpp \
    src/intf/audio/audio_sdl2.cpp \
    src/intf/video/scalers/scaler_common.cpp
    # Add additional source files here if needed

OBJS = $(SOURCES:.cpp=.o)
OBJS := $(OBJS:.mm=.o)

CXX = clang++
CXXFLAGS = -std=c++17 -stdlib=libc++ -Wall -Wextra -Wno-unused-parameter -fobjc-arc
CXXFLAGS += -ObjC++ -fobjc-arc -DTCHAR_H_INCLUDE -DLSB_FIRST -DUSE_SPEEDHACKS
INCLUDES = \
    -I. \
    -Isrc \
    -Isrc/burn \
    -Isrc/burn/devices \
    -Isrc/burn/intf \
    -Isrc/intf/audio \
    -Isrc/intf/video \
    -Isrc/intf/input \
    -Isrc/intf/video/scalers \
    -Icompat \
    -I/opt/homebrew/include \
    `sdl2-config --cflags`

LDFLAGS = \
    -framework Metal \
    -framework MetalKit \
    -framework Cocoa \
    -framework QuartzCore \
    -framework IOKit \
    -framework CoreVideo \
    -L/opt/homebrew/lib \
    `sdl2-config --libs`

ifdef DARWIN
LDFLAGS += -framework OpenGL
endif

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "Linking $(TARGET)..."
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.cpp
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

%.o: %.mm
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	@echo "Cleaning up..."
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean