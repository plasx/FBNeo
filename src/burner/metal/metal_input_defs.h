#ifndef METAL_INPUT_DEFS_H
#define METAL_INPUT_DEFS_H

#ifdef __cplusplus
extern "C" {
#endif

// macOS Virtual Key Codes
enum {
    kVK_ANSI_A                    = 0x00,
    kVK_ANSI_S                    = 0x01,
    kVK_ANSI_D                    = 0x02,
    kVK_ANSI_F                    = 0x03,
    kVK_ANSI_H                    = 0x04,
    kVK_ANSI_G                    = 0x05,
    kVK_ANSI_Z                    = 0x06,
    kVK_ANSI_X                    = 0x07,
    kVK_ANSI_C                    = 0x08,
    kVK_ANSI_V                    = 0x09,
    kVK_ANSI_B                    = 0x0B,
    kVK_ANSI_Q                    = 0x0C,
    kVK_ANSI_W                    = 0x0D,
    kVK_ANSI_E                    = 0x0E,
    kVK_ANSI_R                    = 0x0F,
    kVK_ANSI_Y                    = 0x10,
    kVK_ANSI_T                    = 0x11,
    kVK_ANSI_1                    = 0x12,
    kVK_ANSI_2                    = 0x13,
    kVK_ANSI_3                    = 0x14,
    kVK_ANSI_4                    = 0x15,
    kVK_ANSI_6                    = 0x16,
    kVK_ANSI_5                    = 0x17,
    kVK_ANSI_Equal                = 0x18,
    kVK_ANSI_9                    = 0x19,
    kVK_ANSI_7                    = 0x1A,
    kVK_ANSI_Minus                = 0x1B,
    kVK_ANSI_8                    = 0x1C,
    kVK_ANSI_0                    = 0x1D,
    kVK_ANSI_RightBracket         = 0x1E,
    kVK_ANSI_O                    = 0x1F,
    kVK_ANSI_U                    = 0x20,
    kVK_ANSI_LeftBracket          = 0x21,
    kVK_ANSI_I                    = 0x22,
    kVK_ANSI_P                    = 0x23,
    kVK_Return                    = 0x24,
    kVK_ANSI_L                    = 0x25,
    kVK_ANSI_J                    = 0x26,
    kVK_ANSI_Quote                = 0x27,
    kVK_ANSI_K                    = 0x28,
    kVK_ANSI_Semicolon            = 0x29,
    kVK_ANSI_Backslash            = 0x2A,
    kVK_ANSI_Comma                = 0x2B,
    kVK_ANSI_Slash                = 0x2C,
    kVK_ANSI_N                    = 0x2D,
    kVK_ANSI_M                    = 0x2E,
    kVK_ANSI_Period               = 0x2F,
    kVK_Tab                       = 0x30,
    kVK_Space                     = 0x31,
    kVK_ANSI_Grave                = 0x32,
    kVK_Delete                    = 0x33,
    kVK_Escape                    = 0x35,
    kVK_Command                   = 0x37,
    kVK_Shift                     = 0x38,
    kVK_CapsLock                  = 0x39,
    kVK_Option                    = 0x3A,
    kVK_Control                   = 0x3B,
    kVK_RightShift                = 0x3C,
    kVK_RightOption               = 0x3D,
    kVK_RightControl              = 0x3E,
    kVK_F5                        = 0x60,
    kVK_F6                        = 0x61,
    kVK_F7                        = 0x62,
    kVK_F3                        = 0x63,
    kVK_F8                        = 0x64,
    kVK_F9                        = 0x65,
    kVK_F11                       = 0x67,
    kVK_F10                       = 0x6D,
    kVK_F12                       = 0x6F,
    kVK_Help                      = 0x72,
    kVK_Home                      = 0x73,
    kVK_PageUp                    = 0x74,
    kVK_ForwardDelete             = 0x75,
    kVK_F4                        = 0x76,
    kVK_End                       = 0x77,
    kVK_F2                        = 0x78,
    kVK_PageDown                  = 0x79,
    kVK_F1                        = 0x7A,
    kVK_LeftArrow                 = 0x7B,
    kVK_RightArrow                = 0x7C,
    kVK_DownArrow                 = 0x7D,
    kVK_UpArrow                   = 0x7E
};

// CPS2 Input Indices for Marvel vs Capcom
// These are the indices to use with BurnDrvSetInput
enum {
    // Player 1
    P1_UP          = 0,  // CpsInp001+3
    P1_DOWN        = 1,  // CpsInp001+2
    P1_LEFT        = 2,  // CpsInp001+1
    P1_RIGHT       = 3,  // CpsInp001+0
    P1_WEAK_PUNCH  = 4,  // CpsInp001+4
    P1_MED_PUNCH   = 5,  // CpsInp001+5
    P1_STRONG_PUNCH = 6, // CpsInp001+6
    P1_WEAK_KICK   = 7,  // CpsInp011+0
    P1_MED_KICK    = 8,  // CpsInp011+1
    P1_STRONG_KICK = 9,  // CpsInp011+2
    P1_START       = 10, // CpsInp020+0
    P1_COIN        = 11, // CpsInp020+4
    
    // Player 2
    P2_UP          = 12, // CpsInp000+3
    P2_DOWN        = 13, // CpsInp000+2
    P2_LEFT        = 14, // CpsInp000+1
    P2_RIGHT       = 15, // CpsInp000+0
    P2_WEAK_PUNCH  = 16, // CpsInp000+4
    P2_MED_PUNCH   = 17, // CpsInp000+5
    P2_STRONG_PUNCH = 18, // CpsInp000+6
    P2_WEAK_KICK   = 19, // CpsInp011+4
    P2_MED_KICK    = 20, // CpsInp011+5
    P2_STRONG_KICK = 21, // CpsInp020+6
    P2_START       = 22, // CpsInp020+1
    P2_COIN        = 23, // CpsInp020+5
    
    // System
    RESET          = 24, // CpsReset
    DIAGNOSTIC     = 25, // CpsInp021+1
    SERVICE        = 26, // CpsInp021+2
    
    // Special
    QUIT           = 27,
    SAVE_STATE     = 28,
    LOAD_STATE     = 29,
    TOGGLE_OVERLAY = 30
};

#ifdef __cplusplus
}
#endif

#endif // METAL_INPUT_DEFS_H 