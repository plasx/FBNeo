#include "burnint.h"

// Define the sourcefile_table structure for the Metal build
// This is referenced in burn.cpp and needed for compilation

#ifdef __cplusplus
extern "C" {
#endif

// Define the sourcefile table structure
typedef struct {
    const char* game_name;
    const char* sourcefile;
} sourcefile_entry;

// Define a minimal sourcefile table for the Metal build
// This is used to look up source file names based on game names
sourcefile_entry sourcefile_table[] = {
    { "1941",      "d_cps1.cpp" },
    { "1941j",     "d_cps1.cpp" },
    { "1941u",     "d_cps1.cpp" },
    { "area88",    "d_cps1.cpp" },
    { "area88r",   "d_cps1.cpp" },
    { "area88b",   "d_cps1.cpp" },
    { "19xx",      "d_cps2.cpp" },
    { "19xxa",     "d_cps2.cpp" },
    { "19xxj",     "d_cps2.cpp" },
    { "19xxjr1",   "d_cps2.cpp" },
    { "1944",      "d_cps2.cpp" },
    { "1944j",     "d_cps2.cpp" },
    { "armwar",    "d_cps2.cpp" },
    { "jojo",      "d_cps3.cpp" },
    { "jojon",     "d_cps3.cpp" },
    { "jojoalt",   "d_cps3.cpp" },
    { "jojoaltn",  "d_cps3.cpp" },
    { "jojoba",    "d_cps3.cpp" },
    { "jojoban",   "d_cps3.cpp" },
    { "",          "" }  // End marker
};

#ifdef __cplusplus
}
#endif 