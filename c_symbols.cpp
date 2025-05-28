#ifdef __cplusplus
extern "C" {
#endif

#include "src/burner/metal/fixes/c_symbols_header.h"

// We need to add the proper visibility attribute to ensure the symbols are exported
#define EXPORT __attribute__((visibility("default")))

// No duplicate stubs here; all symbols implemented in c_linkage_bridge.cpp have been removed from this file.

#ifdef __cplusplus
}
#endif 