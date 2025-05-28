#pragma once

// Metal implementation defines
// These settings enable special handling of C/C++ compatibility issues

// Mark this file as a Metal implementation file
#define METAL_IMPLEMENTATION_FILE 1

// Tell the build system we're using Metal
#define BUILD_METAL 1 

// Avoid using the unsafe sprintf macro in header files
#ifndef UNSAFE_SPRINTF
#define UNSAFE_SPRINTF 0
#endif

// Define a more reasonable MAX_PATH
#ifndef MAX_PATH
#define MAX_PATH 512
#endif

// Additional defines for compatible builds
#define NO_FORCE_ASCII 1
#define DONT_INCLUDE_WINUSER 1
#define NO_WINDOWS_HEADERS 1 