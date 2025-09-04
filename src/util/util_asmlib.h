#pragma once

/**
 * \file util_asmlib.h
 * \brief Zero-overhead dispatcher for asmlib optimized functions
 * 
 * Provides compile-time dispatching for memory operations with zero runtime overhead.
 * Small operations (≤64 bytes) use compiler built-ins that get inlined to direct CPU
 * instructions. Large operations use asmlib's SSE-optimized implementations.
 * 
 * Key features:
 * - Compile-time size detection using __builtin_constant_p
 * - Force inline for zero function call overhead
 * - SSE-only asmlib to prevent AVX-SSE transition penalties
 * - Threshold-based dispatch optimized for DXVK's workload
 */

// Include necessary headers for size_t
#ifdef __cplusplus
#include <cstddef>
#else
#include <stddef.h>
#endif

#ifdef DXVK_USE_ASMLIB
  // Forward declare asmlib functions (SSE-only versions)
  #ifdef __cplusplus
  extern "C" {
  #endif
    void * A_memcpy (void * dest, const void * src, size_t count);
    void * A_memmove(void * dest, const void * src, size_t count);
    void * A_memset (void * dest, int c, size_t count);
    int    A_memcmp (const void * buf1, const void * buf2, size_t count);
    size_t A_strlen (const char * str);
    char * A_strcpy (char * dest, const char * src);
    int    A_strcmp (const char * str1, const char * str2);
    char * A_strcat (char * dest, const char * src);
  #ifdef __cplusplus
  }
  #endif
  
  #ifdef __cplusplus
    #include <cstring>
    #include <type_traits>
    
    // Force inline for zero overhead - critical for performance
    #ifdef __GNUC__
      #define DXVK_ALWAYS_INLINE __attribute__((always_inline)) inline
    #elif defined(_MSC_VER)
      #define DXVK_ALWAYS_INLINE __forceinline
    #else
      #define DXVK_ALWAYS_INLINE inline
    #endif
    
    namespace dxvk::asm_dispatch {
      // Threshold: Use compiler intrinsics for ≤64 bytes, asmlib for larger
      constexpr size_t ASMLIB_THRESHOLD = 64;
      
      /**
       * \brief Initialize asmlib (no-op with SSE-only version)
       * 
       * SSE-only libraries don't need runtime CPU detection
       */
      DXVK_ALWAYS_INLINE void init() {
        // No-op: SSE-only libraries don't need runtime detection
      }
      
      /**
       * \brief Compile-time optimized memcpy
       * 
       * Uses __builtin_constant_p to detect compile-time constants
       * and dispatches to optimal implementation with zero overhead
       */
      DXVK_ALWAYS_INLINE void* memcpy_dispatch(void* dst, const void* src, size_t n) {
        // GCC/Clang: __builtin_constant_p is true if n is compile-time constant
        if (__builtin_constant_p(n)) {
          if (n <= ASMLIB_THRESHOLD) {
            // Small constant size: becomes MOV instructions
            return __builtin_memcpy(dst, src, n);
          } else {
            // Large constant size: use asmlib
            return A_memcpy(dst, src, n);
          }
        }
        // Runtime dispatch for variable sizes
        if (n <= ASMLIB_THRESHOLD) {
          // Small runtime size: compiler may still optimize
          return std::memcpy(dst, src, n);
        }
        // Large runtime size: use asmlib
        return A_memcpy(dst, src, n);
      }
      
      /**
       * \brief Compile-time optimized memmove
       */
      DXVK_ALWAYS_INLINE void* memmove_dispatch(void* dst, const void* src, size_t n) {
        if (__builtin_constant_p(n)) {
          if (n <= ASMLIB_THRESHOLD) {
            return __builtin_memmove(dst, src, n);
          } else {
            return A_memmove(dst, src, n);
          }
        }
        if (n <= ASMLIB_THRESHOLD) {
          return std::memmove(dst, src, n);
        }
        return A_memmove(dst, src, n);
      }
      
      /**
       * \brief Compile-time optimized memset
       */
      DXVK_ALWAYS_INLINE void* memset_dispatch(void* dst, int c, size_t n) {
        if (__builtin_constant_p(n)) {
          if (n <= ASMLIB_THRESHOLD) {
            return __builtin_memset(dst, c, n);
          } else {
            return A_memset(dst, c, n);
          }
        }
        if (n <= ASMLIB_THRESHOLD) {
          return std::memset(dst, c, n);
        }
        return A_memset(dst, c, n);
      }
      
      /**
       * \brief Compile-time optimized memcmp
       */
      DXVK_ALWAYS_INLINE int memcmp_dispatch(const void* p1, const void* p2, size_t n) {
        if (__builtin_constant_p(n)) {
          if (n <= ASMLIB_THRESHOLD) {
            return __builtin_memcmp(p1, p2, n);
          } else {
            return A_memcmp(p1, p2, n);
          }
        }
        if (n <= ASMLIB_THRESHOLD) {
          return std::memcmp(p1, p2, n);
        }
        return A_memcmp(p1, p2, n);
      }
    }
    
    // Zero-overhead macro dispatchers
    // These evaluate at compile time when possible
    #define dxvk_memcpy(dst, src, n) \
      ((__builtin_constant_p(n) && (n) <= 64) ? \
        __builtin_memcpy(dst, src, n) : \
        dxvk::asm_dispatch::memcpy_dispatch(dst, src, n))
    
    #define dxvk_memmove(dst, src, n) \
      ((__builtin_constant_p(n) && (n) <= 64) ? \
        __builtin_memmove(dst, src, n) : \
        dxvk::asm_dispatch::memmove_dispatch(dst, src, n))
    
    #define dxvk_memset(dst, c, n) \
      ((__builtin_constant_p(n) && (n) <= 64) ? \
        __builtin_memset(dst, c, n) : \
        dxvk::asm_dispatch::memset_dispatch(dst, c, n))
    
    #define dxvk_memcmp(p1, p2, n) \
      ((__builtin_constant_p(n) && (n) <= 64) ? \
        __builtin_memcmp(p1, p2, n) : \
        dxvk::asm_dispatch::memcmp_dispatch(p1, p2, n))
    
    // String operations always use asmlib (size not known at compile time)
    #define dxvk_strlen     A_strlen
    #define dxvk_strcpy     A_strcpy
    #define dxvk_strcmp     A_strcmp
    #define dxvk_strcat     A_strcat
    
  #else
    // C language support
    #include <string.h>
    
    static inline void dxvk_asm_dispatch_init(void) {
      // No-op for C files
    }
    
    #define dxvk_memcpy     memcpy
    #define dxvk_memmove    memmove
    #define dxvk_memset     memset
    #define dxvk_memcmp     memcmp
    #define dxvk_strlen     strlen
    #define dxvk_strcpy     strcpy
    #define dxvk_strcmp     strcmp
    #define dxvk_strcat     strcat
  #endif
  
#else
  // Fallback to standard library when asmlib is not enabled
  #ifdef __cplusplus
    #include <cstring>
    
    namespace dxvk::asm_dispatch {
      inline void init() {
        // No-op when not using asmlib
      }
    }
    
    #define dxvk_memcpy     std::memcpy
    #define dxvk_memmove    std::memmove
    #define dxvk_memset     std::memset
    #define dxvk_memcmp     std::memcmp
    #define dxvk_strlen     std::strlen
    #define dxvk_strcpy     std::strcpy
    #define dxvk_strcmp     std::strcmp
    #define dxvk_strcat     std::strcat
  #else
    #include <string.h>
    
    static inline void dxvk_asm_dispatch_init(void) {
      // No-op for C files when not using asmlib
    }
    
    #define dxvk_memcpy     memcpy
    #define dxvk_memmove    memmove
    #define dxvk_memset     memset
    #define dxvk_memcmp     memcmp
    #define dxvk_strlen     strlen
    #define dxvk_strcpy     strcpy
    #define dxvk_strcmp     strcmp
    #define dxvk_strcat     strcat
  #endif
  
#endif // DXVK_USE_ASMLIB