#ifndef _METAL_RENDERER_DEFINES_H_
#define _METAL_RENDERER_DEFINES_H_

// This file contains definitions needed for the Metal renderer

// Metal-specific renderer constants
#define METAL_MAX_TEXTURE_SIZE     8192
#define METAL_MAX_UNIFORM_BUFFER   65536
#define METAL_MAX_FRAMES_IN_FLIGHT 3

// Error codes
#define METAL_ERROR_NONE 0
#define METAL_ERROR_NOT_INITIALIZED 1
#define METAL_ERROR_NO_VIEW 2
#define METAL_ERROR_NO_DEVICE 3
#define METAL_ERROR_TEXTURE_CREATION 4

// Shader types
#define METAL_SHADER_BASIC 0
#define METAL_SHADER_CRT 1
#define METAL_SHADER_SCANLINES 2
#define METAL_SHADER_HQ2X 3

// FBNeo key codes (for input mapping)
#define FBNEO_KEY_UP           0x01
#define FBNEO_KEY_DOWN         0x02
#define FBNEO_KEY_LEFT         0x03
#define FBNEO_KEY_RIGHT        0x04
#define FBNEO_KEY_BUTTON1      0x05
#define FBNEO_KEY_BUTTON2      0x06
#define FBNEO_KEY_BUTTON3      0x07
#define FBNEO_KEY_BUTTON4      0x08
#define FBNEO_KEY_BUTTON5      0x09
#define FBNEO_KEY_BUTTON6      0x0A
#define FBNEO_KEY_COIN         0x0B
#define FBNEO_KEY_START        0x0C
#define FBNEO_KEY_SERVICE      0x0D
#define FBNEO_KEY_RESET        0x0E
#define FBNEO_KEY_PAUSE        0x0F
#define FBNEO_KEY_DIAGNOSTIC   0x10
#define FBNEO_KEY_MENU         0x11
#define FBNEO_KEY_SAVE_STATE   0x12
#define FBNEO_KEY_LOAD_STATE   0x13
#define FBNEO_KEY_FAST_FORWARD 0x14
#define FBNEO_KEY_FULLSCREEN   0x15
#define FBNEO_KEY_SCREENSHOT   0x16
#define FBNEO_KEY_QUIT         0x17

// Definitions for M68K disassembler (from m68kdasm.c)
// These are needed for proper disassembly of 68020+ addressing modes

// Extension word formats
#define EXT_8BIT_DISPLACEMENT(A)          ((A)&0xff)
#define EXT_FULL(A)                       ((A)&0x100)
#define EXT_EFFECTIVE_ZERO(A)             (((A)&0xe4) == 0xc4 || ((A)&0xe2) == 0xc0)
#define EXT_BASE_REGISTER_PRESENT(A)      (!((A)&0x80))
#define EXT_INDEX_REGISTER_PRESENT(A)     (!((A)&0x40))
#define EXT_INDEX_REGISTER(A)             (((A)>>12)&7)
#define EXT_INDEX_SCALE(A)                (((A)>>9)&3)
#define EXT_INDEX_LONG(A)                 ((A)&0x800)
#define EXT_INDEX_AR(A)                   ((A)&0x8000)
#define EXT_BASE_DISPLACEMENT_PRESENT(A)  (((A)&0x30) > 0x10)
#define EXT_BASE_DISPLACEMENT_WORD(A)     (((A)&0x30) == 0x20)
#define EXT_BASE_DISPLACEMENT_LONG(A)     (((A)&0x30) == 0x30)

// Additional EXT macros for 68020+ addressing modes
#define EXT_BD_SIZE(A)                    (((A)>>4)&0x3)
#define EXT_BR_NULL(A)                    ((A)&0x80)
#define EXT_INDEX_REG(A)                  (((A)>>12)&7)
#define EXT_PRE_INDEX(A)                  (((A)&0x4) == 0x0)
#define EXT_INDEX_SUPPRESS(A)             ((A)&0x40)
#define EXT_BASE_SUPPRESS(A)              ((A)&0x80)
#define EXT_OUTER_DISP(A)                 ((A)&0x3)

// Metal renderer options
#define METAL_OPTION_VSYNC            0
#define METAL_OPTION_BILINEAR         1
#define METAL_OPTION_SHADER_TYPE      2
#define METAL_OPTION_TEXTURE_FORMAT   3
#define METAL_OPTION_THREADING_MODE   4
#define METAL_OPTION_DEBUG_OVERLAY    5

// Metal renderer threading modes
#define METAL_THREADING_NONE          0  // No threading
#define METAL_THREADING_FRAME         1  // One thread per frame
#define METAL_THREADING_TILE          2  // Tiled multi-threading

// Metal texture formats
#define METAL_TEXTURE_FORMAT_RGBA8    0  // RGBA 8-bit per channel
#define METAL_TEXTURE_FORMAT_BGRA8    1  // BGRA 8-bit per channel
#define METAL_TEXTURE_FORMAT_RGB10A2  2  // RGB 10-bit, Alpha 2-bit
#define METAL_TEXTURE_FORMAT_RGB16F   3  // RGB 16-bit float

// Metal frame flags
#define METAL_FRAME_FLAG_NONE         0
#define METAL_FRAME_FLAG_CLEAR        (1 << 0)  // Clear before drawing
#define METAL_FRAME_FLAG_SYNC         (1 << 1)  // Wait for vsync
#define METAL_FRAME_FLAG_AI_OVERLAY   (1 << 2)  // Enable AI overlay
#define METAL_FRAME_FLAG_DEBUG        (1 << 3)  // Enable debug info

// Maximum path length for configuration and ROM paths
#define MAX_PATH 512

// Driver settings structure for Metal renderer
typedef struct {
    int width;
    int height;
    int bpp;
    int fullscreen;
    int vsync;
    int shaderType;
    int aspectRatioX;
    int aspectRatioY;
    int preserveAspectRatio;
    int bilinearFiltering;
    char shaderPath[1024];
} MetalDriverSettings;

// Default global shader type
#ifndef g_metalShaderType
extern int g_metalShaderType;
#endif

#endif /* _METAL_RENDERER_DEFINES_H_ */ 