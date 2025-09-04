#pragma once

/*
 * SHA Hardware Acceleration Dispatcher for DXVK-Fogged
 * 
 * This header provides zero-overhead macros to dispatch SHA operations
 * to hardware-accelerated implementations when available.
 * 
 * Build flag: -Denable_sha_hw=true
 * Compiler flag: -msha (automatically added when enabled)
 * 
 * Supported CPUs:
 * - Intel: Goldmont/Goldmont+ (2016+), Cannon Lake (2018+)
 * - AMD: All Ryzen processors (Zen architecture, 2017+)
 * 
 * Performance: 3-4x improvement over software implementation
 */

#include <stdint.h>
#include <stddef.h>

// Enable SHA hardware acceleration based on build flag
#ifdef DXVK_USE_SHA_HW

  // Only available on x86/x86-64 platforms
  #if defined(DXVK_ARCH_X86) || defined(DXVK_ARCH_X86_64)
    
    // Check for compiler support
    #ifdef __SHA__
      #define DXVK_SHA_HW_AVAILABLE 1
      
      namespace dxvk::sha1 {
        // Function declarations from sha1_hw.cpp
        bool has_sha_extensions();
        void SHA1Transform_HW(uint32_t state[5], const uint8_t block[64]);
      }
      
      // Runtime dispatch macro - checks CPU support once and caches result
      #define dxvk_sha1_transform(state, block) \
        (dxvk::sha1::has_sha_extensions() ? \
          dxvk::sha1::SHA1Transform_HW(state, block) : \
          SHA1Transform(state, block))
      
      // Batch processing macro for multiple blocks
      #define dxvk_sha1_transform_batch(state, blocks, count) \
        do { \
          if (dxvk::sha1::has_sha_extensions()) { \
            for (size_t i = 0; i < count; i++) { \
              dxvk::sha1::SHA1Transform_HW(state, blocks + (i * 64)); \
            } \
          } else { \
            for (size_t i = 0; i < count; i++) { \
              SHA1Transform(state, blocks + (i * 64)); \
            } \
          } \
        } while(0)
      
    #else
      // Compiler doesn't support SHA instructions
      #define DXVK_SHA_HW_AVAILABLE 0
      #define dxvk_sha1_transform(state, block) SHA1Transform(state, block)
      #define dxvk_sha1_transform_batch(state, blocks, count) \
        do { \
          for (size_t i = 0; i < count; i++) { \
            SHA1Transform(state, blocks + (i * 64)); \
          } \
        } while(0)
    #endif // __SHA__
    
  #else
    // Non-x86 platform
    #define DXVK_SHA_HW_AVAILABLE 0
    #define dxvk_sha1_transform(state, block) SHA1Transform(state, block)
    #define dxvk_sha1_transform_batch(state, blocks, count) \
      do { \
        for (size_t i = 0; i < count; i++) { \
          SHA1Transform(state, blocks + (i * 64)); \
        } \
      } while(0)
  #endif // DXVK_ARCH_X86
  
#else
  // SHA hardware acceleration disabled
  #define DXVK_SHA_HW_AVAILABLE 0
  #define dxvk_sha1_transform(state, block) SHA1Transform(state, block)
  #define dxvk_sha1_transform_batch(state, blocks, count) \
    do { \
      for (size_t i = 0; i < count; i++) { \
        SHA1Transform(state, blocks + (i * 64)); \
      } \
    } while(0)
#endif // DXVK_USE_SHA_HW

// Static assertion to verify SHA hardware detection
#if DXVK_SHA_HW_AVAILABLE
  static_assert(sizeof(void*) >= 4, "SHA hardware acceleration requires 32-bit or 64-bit platform");
#endif