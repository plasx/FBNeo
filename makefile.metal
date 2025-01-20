# makefile.metal
# A minimal example for building FBNeo on macOS with Metal + SDL2

TARGET = fbneo_metal

# Detect macOS
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
DARWIN = 1
endif

# ------------------------------------------------------------------------
# SOURCE FILES
# ------------------------------------------------------------------------
# Instead of wildcarding everything, we’ll list only the needed .cpp files.
# In practice, you might add more from src/burn/drv/* if you want more systems.

SOURCES = \
    src/intf/video/sdl/sdl2_video_metal.mm \
    src/intf/video/metal_renderer.mm \
    src/intf/video/vid_interface_metal.cpp \
    src/burn/burn.cpp \
    src/burn/burnint.cpp \
    src/burn/cpuexec.cpp \
    src/burn/drv/capcom/cps1.cpp \
    src/burn/drv/capcom/cps2.cpp \
    src/burn/drv/neo/neo_geo.cpp \
    src/burn/state.cpp \
    # ... add other cross-platform drivers you need ...
    # If you add more, be sure to confirm none reference <tchar.h> or Windows-only code

# ------------------------------------------------------------------------
# OBJECT FILES
# ------------------------------------------------------------------------
OBJS = $(SOURCES:.cpp=.o)
OBJS := $(OBJS:.mm=.o)

# ------------------------------------------------------------------------
# COMPILER AND FLAGS
# ------------------------------------------------------------------------
CXX = clang++
CXXFLAGS = -std=c++17 -stdlib=libc++ -Wall -Wextra -Wno-unused-parameter
CXXFLAGS += -ObjC++ -fobjc-arc  # Use Objective-C++ with ARC for .mm files

# If you want to silence "ISO C++11 does not allow conversion from string literal to 'char *'"
CXXFLAGS += -Wno-write-strings

# ------------------------------------------------------------------------
# INCLUDE PATHS
# ------------------------------------------------------------------------
INCLUDES = \
    -I. \
    -Isrc \
    -Isrc/burn \
    -Isrc/intf/video \
    -I/opt/homebrew/include  # For SDL2 if installed via Homebrew

# ------------------------------------------------------------------------
# LINKER FLAGS
# ------------------------------------------------------------------------
LDFLAGS = \
    -framework Metal \
    -framework MetalKit \
    -framework Cocoa \
    -framework QuartzCore \
    -framework IOKit \
    -framework CoreVideo \
    -L/opt/homebrew/lib \
    -lSDL2

# ------------------------------------------------------------------------
# DEFAULT TARGET
# ------------------------------------------------------------------------
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS)

# Rules to compile .cpp/.mm
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

%.o: %.mm
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean