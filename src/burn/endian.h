// Endianness handling for the FBNeo Metal port
// This file provides proper endian detection and byte swapping functions
// for cross-platform compatibility, with special handling for macOS/ARM64

#ifndef _ENDIAN_H
#define _ENDIAN_H

#include <stdint.h>

// Endian detection

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  #define FBNEO_BIG_ENDIAN
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  #define FBNEO_LITTLE_ENDIAN
#elif defined(_MSC_VER) || defined(__x86_64__) || defined(__i386__) || \
      (defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN) || \
      defined(__ARMEL__) || defined(__THUMBEL__) || defined(__AARCH64EL__)
  #define FBNEO_LITTLE_ENDIAN
#elif defined(__MIPSEB__) || defined(__ARMEB__) || defined(__THUMBEB__) || \
      defined(__AARCH64EB__) || \
      (defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN)
  #define FBNEO_BIG_ENDIAN
#else
  #error "Unknown endianness - please define FBNEO_BIG_ENDIAN or FBNEO_LITTLE_ENDIAN"
#endif

// Platform-specific byte swapping functions
#if defined(__APPLE__)
  #include <libkern/OSByteOrder.h>
  
  #define BURN_ENDIAN_SWAP_INT16(x) OSSwapInt16(x)
  #define BURN_ENDIAN_SWAP_INT32(x) OSSwapInt32(x)
  #define BURN_ENDIAN_SWAP_INT64(x) OSSwapInt64(x)
  
#elif defined(_MSC_VER)
  #include <stdlib.h>
  
  #define BURN_ENDIAN_SWAP_INT16(x) _byteswap_ushort(x)
  #define BURN_ENDIAN_SWAP_INT32(x) _byteswap_ulong(x)
  #define BURN_ENDIAN_SWAP_INT64(x) _byteswap_uint64(x)
  
#elif defined(__GNUC__)
  #define BURN_ENDIAN_SWAP_INT16(x) __builtin_bswap16(x)
  #define BURN_ENDIAN_SWAP_INT32(x) __builtin_bswap32(x)
  #define BURN_ENDIAN_SWAP_INT64(x) __builtin_bswap64(x)
  
#else
  // Generic implementation as fallback
  static inline uint16_t BURN_ENDIAN_SWAP_INT16(uint16_t x) {
    return (x >> 8) | (x << 8);
  }
  
  static inline uint32_t BURN_ENDIAN_SWAP_INT32(uint32_t x) {
    return ((x >> 24) & 0xff) | 
           ((x >> 8)  & 0xff00) | 
           ((x << 8)  & 0xff0000) | 
           ((x << 24) & 0xff000000);
  }
  
  static inline uint64_t BURN_ENDIAN_SWAP_INT64(uint64_t x) {
    return ((x >> 56) & 0xffULL) | 
           ((x >> 40) & 0xff00ULL) | 
           ((x >> 24) & 0xff0000ULL) | 
           ((x >> 8)  & 0xff000000ULL) |
           ((x << 8)  & 0xff00000000ULL) |
           ((x << 24) & 0xff0000000000ULL) |
           ((x << 40) & 0xff000000000000ULL) |
           ((x << 56) & 0xff00000000000000ULL);
  }
#endif

// Define endian macros based on system endianness
#ifdef FBNEO_LITTLE_ENDIAN
  #define LSB_FIRST 1
  
  // Little endian system - no swapping needed for little endian operations
  #define BURN_ENDIAN_SWAP_INT16_LE(x) (x)
  #define BURN_ENDIAN_SWAP_INT32_LE(x) (x)
  #define BURN_ENDIAN_SWAP_INT64_LE(x) (x)
  
  // Swap for big endian operations
  #define BURN_ENDIAN_SWAP_INT16_BE(x) BURN_ENDIAN_SWAP_INT16(x)
  #define BURN_ENDIAN_SWAP_INT32_BE(x) BURN_ENDIAN_SWAP_INT32(x)
  #define BURN_ENDIAN_SWAP_INT64_BE(x) BURN_ENDIAN_SWAP_INT64(x)
  
#else // Big endian system
  #undef LSB_FIRST
  
  // Swap for little endian operations
  #define BURN_ENDIAN_SWAP_INT16_LE(x) BURN_ENDIAN_SWAP_INT16(x)
  #define BURN_ENDIAN_SWAP_INT32_LE(x) BURN_ENDIAN_SWAP_INT32(x)
  #define BURN_ENDIAN_SWAP_INT64_LE(x) BURN_ENDIAN_SWAP_INT64(x)
  
  // No swapping needed for big endian operations
  #define BURN_ENDIAN_SWAP_INT16_BE(x) (x)
  #define BURN_ENDIAN_SWAP_INT32_BE(x) (x)
  #define BURN_ENDIAN_SWAP_INT64_BE(x) (x)
#endif

// Load/store functions for unaligned memory access
#if defined(FBNEO_LITTLE_ENDIAN)
  static inline uint16_t BURN_UNALIGNED_READ16(void* addr) {
    return *((uint16_t*)addr);
  }
  
  static inline uint32_t BURN_UNALIGNED_READ32(void* addr) {
    return *((uint32_t*)addr);
  }
  
  static inline void BURN_UNALIGNED_WRITE16(void* addr, uint16_t val) {
    *((uint16_t*)addr) = val;
  }
  
  static inline void BURN_UNALIGNED_WRITE32(void* addr, uint32_t val) {
    *((uint32_t*)addr) = val;
  }
#else
  // On big endian platforms, we need to handle unaligned access differently
  static inline uint16_t BURN_UNALIGNED_READ16(void* addr) {
    uint8_t* ptr = (uint8_t*)addr;
    return (ptr[0] << 0) | (ptr[1] << 8);
  }
  
  static inline uint32_t BURN_UNALIGNED_READ32(void* addr) {
    uint8_t* ptr = (uint8_t*)addr;
    return (ptr[0] << 0) | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
  }
  
  static inline void BURN_UNALIGNED_WRITE16(void* addr, uint16_t val) {
    uint8_t* ptr = (uint8_t*)addr;
    ptr[0] = val & 0xff;
    ptr[1] = (val >> 8) & 0xff;
  }
  
  static inline void BURN_UNALIGNED_WRITE32(void* addr, uint32_t val) {
    uint8_t* ptr = (uint8_t*)addr;
    ptr[0] = val & 0xff;
    ptr[1] = (val >> 8) & 0xff;
    ptr[2] = (val >> 16) & 0xff;
    ptr[3] = (val >> 24) & 0xff;
  }
#endif

#endif // _ENDIAN_H 