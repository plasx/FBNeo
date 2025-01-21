############################################################
# makefile.metal
#
# A single-pass build for FBNeo on macOS + Metal.
# Includes .mm files in burner/metal for the new front-end.
############################################################

unexport

# 1. Detect platform
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  DARWIN = 1
endif

# 2. Basic config
NAME       = fbneo_metal
TARGET     = $(NAME)
srcdir     = src/
objdir     = obj/

# 3. Subdirectories
#    Now we add burner/metal as well:
alldir = \
  burner \
  burner/sdl \
  burner/metal \
  dep/libs/libspng \
  dep/libs/lib7z \
  dep/libs/zlib \
  intf \
  intf/video \
  intf/video/scalers \
  intf/video/sdl \
  intf/audio \
  intf/audio/sdl \
  intf/input \
  intf/input/sdl \
  intf/cd \
  intf/cd/sdl \
  intf/perfcount \
  intf/perfcount/sdl \
  dep/generated \
  burn \
  burn/drv \
  burn/drv/capcom \
  burn/drv/neo

# 4. vpath to locate .c/.cpp/.mm/.asm
vpath %.c   $(foreach dir,$(alldir),$(srcdir)$(dir))
vpath %.cpp $(foreach dir,$(alldir),$(srcdir)$(dir))
vpath %.mm  $(foreach dir,$(alldir),$(srcdir)$(dir))  # NEW for Objective-C++ code
vpath %.asm $(foreach dir,$(alldir),$(srcdir)$(dir))
vpath %.h   $(foreach dir,$(alldir),$(srcdir)$(dir))

# 5. Collect all .c/.cpp/.mm/.asm
SRC_C   := $(foreach dir,$(alldir),$(wildcard $(srcdir)$(dir)/*.c))
SRC_CPP := $(foreach dir,$(alldir),$(wildcard $(srcdir)$(dir)/*.cpp))
SRC_MM  := $(foreach dir,$(alldir),$(wildcard $(srcdir)$(dir)/*.mm))  # NEW
SRC_ASM := $(foreach dir,$(alldir),$(wildcard $(srcdir)$(dir)/*.asm))

OBJ_C   := $(SRC_C:%.c=%.o)
OBJ_CPP := $(SRC_CPP:%.cpp=%.o)
OBJ_MM  := $(SRC_MM:%.mm=%.o)    # NEW
OBJ_ASM := $(SRC_ASM:%.asm=%.o)

ALL_OBJS = $(OBJ_C) $(OBJ_CPP) $(OBJ_MM) $(OBJ_ASM)

# 6. Compiler & Linker
CC      = clang
CXX     = clang++
LD      = clang

# 7. Base flags
CFLAGS   = -O2 -fomit-frame-pointer -Wall -Wno-write-strings \
           -Wno-long-long -Wno-sign-compare -Wno-conversion -Wno-unused-parameter \
           -std=c99

CXXFLAGS = -O2 -fomit-frame-pointer -Wall -Wno-write-strings \
           -Wno-long-long -Wno-sign-compare -Wno-conversion -Wno-unused-parameter \
           -std=c++17 -stdlib=libc++

# Add some ObjC++ flags so .mm compiles properly
OBJCPPFLAGS = -fobjc-arc -ObjC++

# Possibly help zlib find <unistd.h>:
CFLAGS   += -DHAVE_UNISTD_H
CXXFLAGS += -DHAVE_UNISTD_H

# 8. Macros to skip Windows references or SDL references
DEF      = -D__APPLE__ -DDARWIN -DMETAL_STANDALONE
DEF     += -U_WIN32 -UWIN32 -DNO_WINDOWS
DEF     += -DNO_SDL_BUILD

CFLAGS   += $(DEF)
CXXFLAGS += $(DEF)

# 9. Add subdirectories to include path
incdir   = $(foreach dir,$(alldir),-I$(srcdir)$(dir))
incdir  += -I$(objdir)dep/generated 
incdir  += -I/opt/homebrew/include

CFLAGS   += $(incdir)
CXXFLAGS += $(incdir)

# 10. Libraries (Metal frameworks)
lib := -lstdc++ -lm -lpthread
ifeq ($(DARWIN),1)
lib += -framework Metal -framework Cocoa -framework QuartzCore
endif

LDFLAGS = -s

# 11. Default target
.PHONY: all init clean
all: init $(TARGET)

init:
	@echo "Building FBNeo (Metal) on macOS..."
	mkdir -p $(objdir)
	mkdir -p $(srcdir)dep/generated

# 12. Link final binary
$(TARGET): $(ALL_OBJS)
	@echo "Linking $(TARGET)..."
	$(LD) $(CFLAGS) $(LDFLAGS) -o $@ $(ALL_OBJS) $(lib)

# 13. Pattern rules
%.o: %.c
	@echo "Compiling C $<..."
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	@echo "Compiling C++ $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.mm
	@echo "Compiling ObjC++ $<..."
	$(CXX) $(CXXFLAGS) $(OBJCPPFLAGS) -c $< -o $@

%.o: %.asm
	@echo "Assembling $<..."
	nasm -f macho64 $< -o $@

clean:
	rm -f $(TARGET) $(OBJ_C) $(OBJ_CPP) $(OBJ_MM) $(OBJ_ASM)
	rm -rf $(objdir)

############################################################
# Explanation:
# - We add burner/metal folder to alldir so we get .mm files.
# - We add a pattern rule for .mm -> .o with Objective-C++ flags.
# - We link with -framework Metal -framework Cocoa -framework QuartzCore.
# - We define -DNO_WINDOWS and -DNO_SDL_BUILD to avoid Windows/SDL code paths.
# - We STILL must ifdef out leftover references to Windows/SDL in .cpp code.
############################################################