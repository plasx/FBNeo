// tchar_dummy.h
// A minimal stand-in for <tchar.h> on non-Windows platforms.

#pragma once

#ifndef _WIN32

// Just define TCHAR as 'char', plus macros
typedef char            TCHAR;
#define _TCHAR_DEFINED
#define _T(x)           x

#endif