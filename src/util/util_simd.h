#pragma once

#include "util_bit.h"
#include "../util/util_asmlib.h"

#ifdef DXVK_USE_VCL
  #ifdef DXVK_ARCH_X86
    #include "../../include/vectorclass/vectorclass.h"
  #endif
#endif

namespace dxvk::util {

  /**
   * \brief SIMD-optimized image data packing
   * 
   * Uses AVX2/AVX-512 instructions when available to accelerate
   * format conversion and data packing operations.
   */
  #if defined(DXVK_USE_VCL) && defined(DXVK_ARCH_X86)
  
  /**
   * \brief Pack image data with SIMD optimization
   * 
   * Optimized version using VCL for vectorized memory operations.
   * Falls back to scalar implementation for small or unaligned data.
   * 
   * \param [in] dstBytes Destination buffer pointer
   * \param [in] srcBytes Source buffer pointer
   * \param [in] blockCount Number of blocks to copy
   * \param [in] blockSize Size of each block in bytes
   * \param [in] pitchPerRow Row pitch in bytes
   * \param [in] pitchPerLayer Layer pitch in bytes
   */
  inline void packImageDataSIMD(
          void*             dstBytes,
    const void*             srcBytes,
          VkExtent3D        blockCount,
          VkDeviceSize      blockSize,
          VkDeviceSize      pitchPerRow,
          VkDeviceSize      pitchPerLayer) {
    
    auto dstData = reinterpret_cast<      char*>(dstBytes);
    auto srcData = reinterpret_cast<const char*>(srcBytes);
    
    const VkDeviceSize bytesPerRow   = blockCount.width  * blockSize;
    const VkDeviceSize bytesPerLayer = blockCount.height * bytesPerRow;
    const VkDeviceSize bytesTotal    = blockCount.depth  * bytesPerLayer;
    
    // Check if we can do a direct copy
    const bool directCopy = ((bytesPerRow   == pitchPerRow  ) || (blockCount.height == 1))
                         && ((bytesPerLayer == pitchPerLayer) || (blockCount.depth  == 1));
    
    if (directCopy) {
      // For direct copies, use optimized memcpy
      dxvk_memcpy(dstData, srcData, bytesTotal);
      return;
    }
    
    // For non-direct copies, use SIMD where beneficial
    // Threshold for SIMD: at least 64 bytes per row (4 AVX2 vectors)
    const bool useSIMD = bytesPerRow >= 64 && instrset_detect() >= 8; // AVX2 or higher
    
    if (useSIMD) {
      // Process using AVX2/AVX-512 vectors
      const size_t vecSize = 32; // AVX2 vector size in bytes
      const size_t numVecsPerRow = bytesPerRow / vecSize;
      const size_t remainderBytes = bytesPerRow % vecSize;
      
      for (uint32_t i = 0; i < blockCount.depth; i++) {
        for (uint32_t j = 0; j < blockCount.height; j++) {
          const char* srcRow = srcData + j * pitchPerRow;
          char* dstRow = dstData + j * bytesPerRow;
          
          // Process full vectors
          if (numVecsPerRow > 0) {
            // Use Vec8f for 32-byte aligned operations
            for (size_t v = 0; v < numVecsPerRow; v++) {
              Vec8f vec;
              vec.load(reinterpret_cast<const float*>(srcRow + v * vecSize));
              vec.store(reinterpret_cast<float*>(dstRow + v * vecSize));
            }
          }
          
          // Handle remainder with optimized memcpy
          if (remainderBytes > 0) {
            dxvk_memcpy(dstRow + numVecsPerRow * vecSize,
                       srcRow + numVecsPerRow * vecSize,
                       remainderBytes);
          }
        }
        
        srcData += pitchPerLayer;
        dstData += bytesPerLayer;
      }
    } else {
      // Fall back to optimized scalar memcpy for smaller data
      for (uint32_t i = 0; i < blockCount.depth; i++) {
        for (uint32_t j = 0; j < blockCount.height; j++) {
          dxvk_memcpy(dstData + j * bytesPerRow,
                     srcData + j * pitchPerRow,
                     bytesPerRow);
        }
        
        srcData += pitchPerLayer;
        dstData += bytesPerLayer;
      }
    }
  }
  
  /**
   * \brief SIMD-optimized format conversion
   * 
   * Converts between different pixel formats using SIMD instructions.
   * Particularly optimized for common conversions like RGBA8 to BGRA8.
   */
  template<typename SrcFormat, typename DstFormat>
  inline void convertFormatSIMD(
          void*       dstBytes,
    const void*       srcBytes,
          uint32_t    pixelCount) {
    // This is a template for future format-specific optimizations
    // For now, fall back to scalar conversion
    auto src = reinterpret_cast<const SrcFormat*>(srcBytes);
    auto dst = reinterpret_cast<DstFormat*>(dstBytes);
    
    for (uint32_t i = 0; i < pixelCount; i++) {
      dst[i] = DstFormat(src[i]);
    }
  }
  
  /**
   * \brief RGBA8 to BGRA8 conversion with SIMD
   * 
   * Specialized fast path for the common RGBA<->BGRA conversion.
   */
  inline void convertRGBA8toBGRA8_SIMD(
          uint32_t*   dst,
    const uint32_t*   src,
          uint32_t    pixelCount) {
    
    if (instrset_detect() >= 8) { // AVX2 available
      const size_t vecSize = 8; // Process 8 pixels at once
      const size_t numVecs = pixelCount / vecSize;
      const size_t remainder = pixelCount % vecSize;
      
      // Shuffle mask for RGBA -> BGRA: swap R and B channels
      const Vec32uc shuffleMask(2,1,0,3, 6,5,4,7, 10,9,8,11, 14,13,12,15,
                                18,17,16,19, 22,21,20,23, 26,25,24,27, 30,29,28,31);
      
      for (size_t i = 0; i < numVecs; i++) {
        Vec32uc pixels;
        pixels.load(reinterpret_cast<const uint8_t*>(src + i * vecSize));
        Vec32uc swapped = permute32(pixels, shuffleMask);
        swapped.store(reinterpret_cast<uint8_t*>(dst + i * vecSize));
      }
      
      // Handle remainder pixels
      for (size_t i = numVecs * vecSize; i < pixelCount; i++) {
        uint32_t pixel = src[i];
        dst[i] = ((pixel & 0x00FF0000) >> 16) |  // Move R to B position
                 ((pixel & 0x0000FF00))       |  // Keep G in place
                 ((pixel & 0x000000FF) << 16) |  // Move B to R position
                 ((pixel & 0xFF000000));         // Keep A in place
      }
    } else {
      // Fallback for non-AVX2 systems
      for (uint32_t i = 0; i < pixelCount; i++) {
        uint32_t pixel = src[i];
        dst[i] = ((pixel & 0x00FF0000) >> 16) |
                 ((pixel & 0x0000FF00))       |
                 ((pixel & 0x000000FF) << 16) |
                 ((pixel & 0xFF000000));
      }
    }
  }
  
  #else
  
  // Fallback implementations when VCL is not available
  inline void packImageDataSIMD(
          void*             dstBytes,
    const void*             srcBytes,
          VkExtent3D        blockCount,
          VkDeviceSize      blockSize,
          VkDeviceSize      pitchPerRow,
          VkDeviceSize      pitchPerLayer) {
    // Fall back to standard packImageData
    packImageData(dstBytes, srcBytes, blockCount, blockSize, pitchPerRow, pitchPerLayer);
  }
  
  inline void convertRGBA8toBGRA8_SIMD(
          uint32_t*   dst,
    const uint32_t*   src,
          uint32_t    pixelCount) {
    for (uint32_t i = 0; i < pixelCount; i++) {
      uint32_t pixel = src[i];
      dst[i] = ((pixel & 0x00FF0000) >> 16) |
               ((pixel & 0x0000FF00))       |
               ((pixel & 0x000000FF) << 16) |
               ((pixel & 0xFF000000));
    }
  }
  
  #endif // DXVK_USE_VCL && DXVK_ARCH_X86

}