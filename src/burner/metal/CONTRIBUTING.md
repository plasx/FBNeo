# Contributing to FBNeo Metal Implementation

Thank you for your interest in contributing to the Metal implementation of FBNeo! This guide covers important considerations for working with this codebase, particularly related to C/C++ compatibility.

## C/C++ Compatibility Considerations

The FBNeo codebase contains a mix of C and C++ code, which presents unique challenges when building for Metal. Here are key considerations:

### 1. Type Definitions

C doesn't have built-in `bool` type or `wchar_t` support. When writing C code:

- Use the provided compatibility typedefs from `fixes/c_cpp_fixes.h`
- For new C files, always include this header
- In header files that may be included from C, use conditional compilation:

```c
#ifdef __cplusplus
// C++ code
bool SomeFunction();
#else
// C code
int SomeFunction(); // Return 0/1 instead of bool
#endif
```

### 2. Struct and Enum Tags

C requires explicit struct/enum tags when referencing types:

```c
// In C++
CheatInfo* info;

// In C
struct CheatInfo* info; // Must use 'struct' tag
```

When writing headers that will be used from both C and C++:

```c
#ifdef __cplusplus
extern "C" {
#endif

// Use complete declarations with struct keywords
struct BurnDriver* GetDriver(int index);

#ifdef __cplusplus
}
#endif
```

### 3. Build System Integration

- C files should be compiled with C compiler (clang)
- C++ files should be compiled with C++ compiler (clang++)
- Objective-C++ files (.mm) should be compiled with special flags
- Include `fixes/c_cpp_fixes.h` in C files for compatibility

### 4. Adding New Files

When adding new files:

- **C++ files (.cpp)**: No special considerations needed
- **C files (.c)**: 
  - Include `fixes/c_cpp_fixes.h` at the top
  - Use struct/enum tags properly
  - Avoid C++ features (classes, templates, etc.)
- **Objective-C++ files (.mm)**:
  - Can freely mix Objective-C, C, and C++
  - Use `@import` for Apple frameworks
  - Interface with C code using extern "C" blocks

### 5. Conditional Compilation

Use preprocessor conditionals to handle differences:

```c
#ifdef METAL_BUILD
// Metal-specific code
#else
// Standard FBNeo code
#endif
```

### 6. Avoiding Common Pitfalls

- **Include order matters**: Put C++ headers after C headers
- **Don't use C++ features in C files**: No references, templates, etc.
- **Watch for name collisions**: Use proper namespaces in C++ code
- **Handle extern "C"**: Use it for all C functions called from C++

## Metal-Specific Guidelines

### 1. Metal API Usage

- Always check for device/API availability
- Use `@available` for newer API features
- Provide fallbacks for unsupported features

### 2. Performance Considerations

- Minimize CPU/GPU synchronization
- Use triple buffering when possible
- Batch similar operations together
- Avoid frequent resource creation/destruction

### 3. Memory Management

- Use ARC for Objective-C/C++ code
- Manually manage C/C++ resources
- Be mindful of ownership in mixed code

### 4. AI Integration

- Keep AI processing non-blocking
- Use CoreML when possible for hardware acceleration
- Provide fallbacks for machines without Neural Engine

## Testing

Please test your changes on:

1. Both Intel and Apple Silicon Macs
2. Multiple macOS versions if possible
3. Various game ROMs to ensure compatibility

## Submitting Changes

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Run the build and tests
5. Submit a pull request with a clear description

Thank you for contributing to FBNeo Metal! 