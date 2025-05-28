# FBNeo Metal C/C++ Interoperability

This document explains the approach to C/C++ interoperability in the FBNeo Metal port.

## Overview

The FBNeo Metal port uses a combination of:
- **C** code from the original FBNeo codebase
- **C++** code for some emulation components
- **Objective-C++** code for the Metal implementation on macOS

This creates a complex interoperability challenge that is addressed by the files in the `fixes` directory.

## Core Principles

1. **Clear Language Boundaries**: Each file should clearly delineate where C, C++, and Objective-C++ code interact.
2. **Consistent Linkage**: All functions that cross language boundaries must have proper `extern "C"` declarations.
3. **Standardized Approach**: Use consistent macros and patterns for interoperability.
4. **Bridge Headers**: Use dedicated bridge headers to handle interoperability concerns.

## Key Files

- `metal_interop.h`: Core interoperability macros and basic types
- `metal_bridge.h`: Universal bridge header that includes all necessary interfaces
- `metal_core_bridge.h`: Bridge for core FBNeo functions
- `metal_audio_bridge.h`: Bridge for audio subsystem
- `cps_input_bridge.h`: Bridge for CPS input system
- `metal_bridge_impl.cpp`: Implementation of the bridge functions

## Recommended Practices

### 1. Include the Right Headers

For C++ or Objective-C++ files that need to call C functions:

```cpp
// Include the unified bridge header
#include "fixes/metal_bridge.h"

// Now you can call C functions with proper linkage
void MyFunction() {
    // Call a C function from the core
    BurnSoundInit();
}
```

### 2. Define New Functions with Proper Linkage

When creating new functions that will be called across language boundaries:

```cpp
// In the header file
METAL_BEGIN_C_DECL

// Function declaration
INT32 MyNewFunction(INT32 parameter);

METAL_END_C_DECL

// In the implementation file
METAL_EXPORT_TO_C INT32 MyNewFunction(INT32 parameter) {
    // Implementation
    return 0;
}
```

### 3. Bridge Pattern for Complex Interactions

For complex interactions between C++ and C code, use a bridge pattern:

```cpp
// C++ implementation with C interface
class ComplexImplementation {
private:
    // C++ internals
    std::vector<int> data;
    
public:
    void process() {
        // Complex C++ logic
    }
};

// Bridge function with C linkage
METAL_EXPORT_TO_C INT32 Bridge_Process() {
    // Get C++ instance and call method
    static ComplexImplementation impl;
    impl.process();
    return 0;
}
```

### 4. Objective-C++ Integration

When working with Objective-C++ files:

```objc
// Import both Objective-C frameworks and C/C++ headers
#import <Metal/Metal.h>
#include "fixes/metal_bridge.h"

// Implementation that bridges between Objective-C and C
@implementation MyMetalImplementation

- (void)processFrame {
    // Call C function with proper linkage
    Metal_ProcessFrame();
}

@end
```

## Common Issues and Solutions

### Problem: Function not found during linking

**Solution**: Ensure the function has proper `extern "C"` linkage in both the declaration and implementation.

### Problem: Name mangling errors

**Solution**: Use `METAL_EXPORT_TO_C` for functions exported from C++ to C, and include bridge headers that use `METAL_BEGIN_C_DECL` and `METAL_END_C_DECL`.

### Problem: Type compatibility issues

**Solution**: Use the common types defined in `metal_interop.h` for data exchanged across language boundaries.

### Problem: Mixing C and C++ code in the same file

**Solution**: Use `#ifdef __cplusplus` guards or the provided macros to ensure proper behavior in both C and C++ contexts.

## Testing Interoperability

To test if your interoperability changes are working correctly:

1. Build the project with `make -f makefile.metal`
2. If there are linkage errors, check that all functions crossing language boundaries have proper linkage declarations
3. Test both on Intel and ARM64 Macs to ensure platform compatibility

## Adding New Bridges

When adding new subsystems that need to cross language boundaries:

1. Create a new bridge header in the `fixes` directory
2. Use the existing bridge headers as templates
3. Add your new bridge header to `metal_bridge.h`
4. Implement the bridge functions in a corresponding `.cpp` file
5. Update this documentation to include your new bridge 