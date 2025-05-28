#ifdef BUILD_METAL
// For Metal build only

// Include our fixes first
#include "burn_fixes.h"
#include "burn_sourcefile_stubs.h"

// Include the proper declarations before including burn.cpp
extern struct BurnDriver* const pDriverList[];

// Then include the original burn.cpp (let's use a less confusing approach)
#define ORIGINAL_BURN
#include "../../../burn/burn.cpp"

#ifdef BUILD_METAL
// For Metal build only

// Include our fixes first
#include "burn_fixes.h"

// Then include the original burn.cpp (let's use a less confusing approach)
#define ORIGINAL_BURN
#include "../../../burn/burn.cpp"

#endif // BUILD_METAL 