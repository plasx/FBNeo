# FBNeo 2025 AI Integration - Next Steps

## Build System Fixes Priority

### 1. C/C++ Compatibility Layer Improvement

- [ ] Create an enhanced `c_cpp_compatibility.h` header that:
  - [ ] Properly forward declares all required structs and enums
  - [ ] Adds proper `struct` and `enum` tags for C-mode compilation
  - [ ] Creates wrapper functions for C++ functions with default arguments
  - [ ] Provides type-safe alternatives to problematic macros

- [ ] Implement in `src/burner/metal/fixes/`:
  ```c
  // Example forward declarations
  struct CheatInfo;
  struct RomDataInfo;
  struct BurnRomInfo;
  struct cpu_core_config;
  enum BurnCartrigeCommand;
  
  // Example wrapper function for default arguments
  void IpsApplyPatchesWrapper(unsigned char* base, char* rom_name, 
                             unsigned int rom_crc, bool readonly);
  ```

### 2. Game Genre Macro Fix

- [ ] Replace direct use of game genre macros with proper variable definitions:
  - [ ] Create a separate `.c` file with proper variable declarations
  - [ ] Define variables using values instead of macros
  - [ ] Update all references to use the variables

- [ ] Implement in `src/burner/metal/fixes/genre_variables.c`:
  ```c
  /* Game genre variables for Metal build */
  const unsigned int GENRE_HORSHOOT = 1 << 0;
  const unsigned int GENRE_VERSHOOT = 1 << 1;
  const unsigned int GENRE_SCRFIGHT = 1 << 2;
  // etc.
  
  /* Provide variables for use in stub implementations */
  void* GBF_HORSHOOT_VALUE = (void*)(uintptr_t)(1 << 0);
  void* GBF_VERSHOOT_VALUE = (void*)(uintptr_t)(1 << 1);
  // etc.
  ```

### 3. Build System Enhancement

- [ ] Modify `makefile.metal` to better handle mixed C/C++ compilation:
  - [ ] Add separate compilation rules for C and C++ files
  - [ ] Use specific compiler flags based on file type
  - [ ] Create proper dependencies between generated headers

- [ ] Implement build script improvements:
  ```makefile
  # Separate flags for C and C++ files
  CFLAGS_COMMON = -g -O2 -Wall -Isrc/dep/generated -Isrc -Isrc/burn...
  CFLAGS = $(CFLAGS_COMMON) -std=c11
  CXXFLAGS = $(CFLAGS_COMMON) -std=c++11
  
  # Special rule for Metal-specific files
  $(OBJDIR)/src/burner/metal/%.o: src/burner/metal/%.c
      $(CC) $(CFLAGS) -DMETAL_BUILD -include src/burner/metal/fixes/c_cpp_compatibility.h -c $< -o $@
  ```

### 4. Stub Implementation Improvement

- [ ] Create cleaned-up stub implementations that avoid depending on problematic headers:
  - [ ] Simplify the CPU implementation stubs
  - [ ] Use opaque pointers for complex data structures
  - [ ] Provide simplified implementations of core functions

- [ ] Implement in new files:
  ```c
  // metal_clean_stubs.c
  #include <stdint.h>
  #include <stdbool.h>
  
  // Simplified CPU interface for Metal build
  int32_t SekTotalCycles() { return 0; }
  void SekSetRESETLine(int32_t cpu, int32_t state) {}
  
  // Avoid including full header by providing simplified structure
  struct metal_cpu_config {
      char name[32];
      void* callbacks;
  };
  
  // Global variables with clean initialization
  struct metal_cpu_config MegadriveCPU = {0};
  struct metal_cpu_config FD1094CPU = {0};
  ```

## AI Integration Tasks

Once the build system is fixed, focus can return to the core AI integration:

### 1. Model Loader Enhancement

- [ ] Complete the CoreML model loading and conversion system
- [ ] Add robust error handling for model availability
- [ ] Implement model verification and validation

### 2. AI Feature Implementation

- [ ] Integrate the AI inference loop with the emulation core
- [ ] Add frame capture mechanism for AI processing
- [ ] Implement action synthesis from AI predictions

### 3. Performance Optimization

- [ ] Optimize tensor operations on Metal
- [ ] Add batching support for improved throughput
- [ ] Implement hardware acceleration detection and fallbacks

### 4. User Interface Integration

- [ ] Add AI feature configuration to the settings UI
- [ ] Create visualization overlays for AI predictions
- [ ] Implement model selection and management interface

## Timeline and Priority

### Immediate (1-2 Days)
- Fix C/C++ compatibility layer
- Address game genre macro issues
- Create enhanced stub implementations

### Short-term (3-7 Days)
- Improve build system for better C/C++ handling
- Complete initial AI model loading functionality
- Fix any remaining build errors

### Medium-term (1-2 Weeks)
- Integrate AI inference with emulation core
- Implement performance optimizations
- Add basic UI controls for AI features

### Long-term (2-4 Weeks)
- Complete comprehensive AI feature set
- Optimize for different hardware capabilities
- Create full documentation and user guides 