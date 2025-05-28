#include "burnint.h"

// Minimal implementation of interface_main.cpp for Metal build

// Function prototypes
INT32 VidSSetCapture(INT32 nFlags);
INT32 VidSGetToggle();
INT32 CheatUpdate();

// Use existing definition from burn.h
static INT32 DummyBprintf(INT32 nStatus, TCHAR* szFormat, ...)
{
    return 0;
}

// Instead of defining bprintf, reference the existing one
// Comment out the original definition
// INT32 (*bprintf)(INT32 nStatus, TCHAR* szFormat, ...) = DummyBprintf;
extern INT32 (*bprintf)(INT32 nStatus, TCHAR* szFormat, ...);

// Minimal implementation of video-related functions
INT32 VidSSetCapture(INT32 nFlags)
{
    return 0;
}

INT32 VidSGetToggle()
{
    return 0;
}

// Minimal implementation of cheat-related function
INT32 CheatUpdate()
{
    return 0;
}
