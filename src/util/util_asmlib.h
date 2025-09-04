#pragma once

/**
 * \file util_asmlib.h
 * \brief Low-overhead dispatcher for asmlib optimized functions
 * 
 * Provides conditional compilation support for Agner Fog's asmlib.
 * When DXVK_USE_ASMLIB is defined, memory functions are replaced
 * with highly optimized assembly implementations that automatically
 * detect and use the best instruction set for the CPU.
 */

#ifdef DXVK_USE_ASMLIB
  // Forward declare asmlib functions we need
  extern "C" {
    void * A_memcpy (void * dest, const void * src, size_t count);
    void * A_memmove(void * dest, const void * src, size_t count);
    void * A_memset (void * dest, int c, size_t count);
    int    A_memcmp (const void * buf1, const void * buf2, size_t count);
    size_t A_strlen (const char * str);
    char * A_strcpy (char * dest, const char * src);
    int    A_strcmp (const char * str1, const char * str2);
    char * A_strcat (char * dest, const char * src);
  }
  
  namespace dxvk::asm_dispatch {
    /**
     * \brief Initialize asmlib (if needed)
     * 
     * asmlib automatically detects CPU capabilities on first use.
     * This function ensures proper initialization.
     */
    inline void init() {
      static bool initialized = false;
      if (!initialized) {
        // asmlib auto-detects CPU and sets up optimal paths internally
        // Just ensure it's linked and ready
        initialized = true;
      }
    }
  }
  
  // Direct replacements with zero overhead
  // These use the A_ prefixed versions from asmlib
  #define dxvk_memcpy     A_memcpy
  #define dxvk_memmove    A_memmove
  #define dxvk_memset     A_memset
  #define dxvk_memcmp     A_memcmp
  #define dxvk_strlen     A_strlen
  #define dxvk_strcpy     A_strcpy
  #define dxvk_strcmp     A_strcmp
  #define dxvk_strcat     A_strcat
  
#else
  // Fallback to standard library when asmlib is not enabled
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
  
#endif // DXVK_USE_ASMLIB