/*
 * Hardware-Accelerated SHA1 Implementation for DXVK
 * Uses Intel SHA-NI and AMD SHA extensions when available
 * Falls back to standard implementation on unsupported hardware
 * 
 * Based on:
 * - Intel SHA Extensions Programming Guide (2013-2018)
 * - Intel Intrinsics Guide: https://www.intel.com/content/www/us/en/docs/intrinsics-guide/
 * - OpenSSL SHA-NI implementation (Apache 2.0 License)
 * - Linux Kernel SHA-NI implementation (GPL v2)
 * 
 * SHA-NI intrinsics reference implementation derived from Intel's
 * public documentation and programming examples.
 */

#include "sha1.h"
#include "../util_asmlib.h"

// Only compile hardware version on x86/x86-64 platforms
#if defined(DXVK_ARCH_X86) || defined(DXVK_ARCH_X86_64)

// Check for compiler support
#ifdef __SHA__

#include <immintrin.h>
#include <cpuid.h>

namespace dxvk::sha1 {

// CPU feature detection - cached for performance
static bool g_sha_hw_available = false;
static bool g_sha_hw_checked = false;

bool has_sha_extensions() {
  if (!g_sha_hw_checked) {
    unsigned int eax, ebx, ecx, edx;
    // Check for SHA extensions in CPUID leaf 7, subleaf 0
    if (__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx)) {
      g_sha_hw_available = (ebx & (1 << 29)) != 0; // SHA bit is bit 29 of EBX
    }
    g_sha_hw_checked = true;
  }
  return g_sha_hw_available;
}

// Hardware-accelerated SHA1 block processing using Intel SHA-NI
void SHA1Transform_HW(uint32_t state[5], const uint8_t block[SHA1_BLOCK_LENGTH]) {
  __m128i ABCD, ABCD_SAVE, E0, E0_SAVE, E1;
  __m128i MSG0, MSG1, MSG2, MSG3;
  
  // Byte swap mask for big-endian conversion
  const __m128i MASK = _mm_set_epi64x(0x0001020304050607ULL, 0x08090a0b0c0d0e0fULL);
  
  // Load initial state
  ABCD = _mm_loadu_si128((const __m128i*)state);
  E0 = _mm_set_epi32(state[4], 0, 0, 0);
  ABCD = _mm_shuffle_epi32(ABCD, 0x1B); // 0x1B = 00 01 10 11 = reverse order
  
  // Save initial state for final addition
  ABCD_SAVE = ABCD;
  E0_SAVE = E0;
  
  // Rounds 0-3
  MSG0 = _mm_loadu_si128((const __m128i*)(block + 0));
  MSG0 = _mm_shuffle_epi8(MSG0, MASK);
  E0 = _mm_add_epi32(E0, MSG0);
  E1 = ABCD;
  ABCD = _mm_sha1rnds4_epu32(ABCD, E0, 0);
  
  // Rounds 4-7
  MSG1 = _mm_loadu_si128((const __m128i*)(block + 16));
  MSG1 = _mm_shuffle_epi8(MSG1, MASK);
  E1 = _mm_sha1nexte_epu32(E1, MSG1);
  E0 = ABCD;
  ABCD = _mm_sha1rnds4_epu32(ABCD, E1, 0);
  MSG0 = _mm_sha1msg1_epu32(MSG0, MSG1);
  
  // Rounds 8-11
  MSG2 = _mm_loadu_si128((const __m128i*)(block + 32));
  MSG2 = _mm_shuffle_epi8(MSG2, MASK);
  E0 = _mm_sha1nexte_epu32(E0, MSG2);
  E1 = ABCD;
  ABCD = _mm_sha1rnds4_epu32(ABCD, E0, 0);
  MSG1 = _mm_sha1msg1_epu32(MSG1, MSG2);
  MSG0 = _mm_xor_si128(MSG0, MSG2);
  
  // Rounds 12-15
  MSG3 = _mm_loadu_si128((const __m128i*)(block + 48));
  MSG3 = _mm_shuffle_epi8(MSG3, MASK);
  E1 = _mm_sha1nexte_epu32(E1, MSG3);
  E0 = ABCD;
  MSG0 = _mm_sha1msg2_epu32(MSG0, MSG3);
  ABCD = _mm_sha1rnds4_epu32(ABCD, E1, 0);
  MSG2 = _mm_sha1msg1_epu32(MSG2, MSG3);
  MSG1 = _mm_xor_si128(MSG1, MSG3);
  
  // Rounds 16-19
  E0 = _mm_sha1nexte_epu32(E0, MSG0);
  E1 = ABCD;
  MSG1 = _mm_sha1msg2_epu32(MSG1, MSG0);
  ABCD = _mm_sha1rnds4_epu32(ABCD, E0, 0);
  MSG3 = _mm_sha1msg1_epu32(MSG3, MSG0);
  MSG2 = _mm_xor_si128(MSG2, MSG0);
  
  // Rounds 20-23 (function 1)
  E1 = _mm_sha1nexte_epu32(E1, MSG1);
  E0 = ABCD;
  MSG2 = _mm_sha1msg2_epu32(MSG2, MSG1);
  ABCD = _mm_sha1rnds4_epu32(ABCD, E1, 1);
  MSG0 = _mm_sha1msg1_epu32(MSG0, MSG1);
  MSG3 = _mm_xor_si128(MSG3, MSG1);
  
  // Rounds 24-27 (function 1)
  E0 = _mm_sha1nexte_epu32(E0, MSG2);
  E1 = ABCD;
  MSG3 = _mm_sha1msg2_epu32(MSG3, MSG2);
  ABCD = _mm_sha1rnds4_epu32(ABCD, E0, 1);
  MSG1 = _mm_sha1msg1_epu32(MSG1, MSG2);
  MSG0 = _mm_xor_si128(MSG0, MSG2);
  
  // Rounds 28-31 (function 1)
  E1 = _mm_sha1nexte_epu32(E1, MSG3);
  E0 = ABCD;
  MSG0 = _mm_sha1msg2_epu32(MSG0, MSG3);
  ABCD = _mm_sha1rnds4_epu32(ABCD, E1, 1);
  MSG2 = _mm_sha1msg1_epu32(MSG2, MSG3);
  MSG1 = _mm_xor_si128(MSG1, MSG3);
  
  // Rounds 32-35 (function 1)
  E0 = _mm_sha1nexte_epu32(E0, MSG0);
  E1 = ABCD;
  MSG1 = _mm_sha1msg2_epu32(MSG1, MSG0);
  ABCD = _mm_sha1rnds4_epu32(ABCD, E0, 1);
  MSG3 = _mm_sha1msg1_epu32(MSG3, MSG0);
  MSG2 = _mm_xor_si128(MSG2, MSG0);
  
  // Rounds 36-39 (function 1)
  E1 = _mm_sha1nexte_epu32(E1, MSG1);
  E0 = ABCD;
  MSG2 = _mm_sha1msg2_epu32(MSG2, MSG1);
  ABCD = _mm_sha1rnds4_epu32(ABCD, E1, 1);
  MSG0 = _mm_sha1msg1_epu32(MSG0, MSG1);
  MSG3 = _mm_xor_si128(MSG3, MSG1);
  
  // Rounds 40-43 (function 2)
  E0 = _mm_sha1nexte_epu32(E0, MSG2);
  E1 = ABCD;
  MSG3 = _mm_sha1msg2_epu32(MSG3, MSG2);
  ABCD = _mm_sha1rnds4_epu32(ABCD, E0, 2);
  MSG1 = _mm_sha1msg1_epu32(MSG1, MSG2);
  MSG0 = _mm_xor_si128(MSG0, MSG2);
  
  // Rounds 44-47 (function 2)
  E1 = _mm_sha1nexte_epu32(E1, MSG3);
  E0 = ABCD;
  MSG0 = _mm_sha1msg2_epu32(MSG0, MSG3);
  ABCD = _mm_sha1rnds4_epu32(ABCD, E1, 2);
  MSG2 = _mm_sha1msg1_epu32(MSG2, MSG3);
  MSG1 = _mm_xor_si128(MSG1, MSG3);
  
  // Rounds 48-51 (function 2)
  E0 = _mm_sha1nexte_epu32(E0, MSG0);
  E1 = ABCD;
  MSG1 = _mm_sha1msg2_epu32(MSG1, MSG0);
  ABCD = _mm_sha1rnds4_epu32(ABCD, E0, 2);
  MSG3 = _mm_sha1msg1_epu32(MSG3, MSG0);
  MSG2 = _mm_xor_si128(MSG2, MSG0);
  
  // Rounds 52-55 (function 2)
  E1 = _mm_sha1nexte_epu32(E1, MSG1);
  E0 = ABCD;
  MSG2 = _mm_sha1msg2_epu32(MSG2, MSG1);
  ABCD = _mm_sha1rnds4_epu32(ABCD, E1, 2);
  MSG0 = _mm_sha1msg1_epu32(MSG0, MSG1);
  MSG3 = _mm_xor_si128(MSG3, MSG1);
  
  // Rounds 56-59 (function 2)
  E0 = _mm_sha1nexte_epu32(E0, MSG2);
  E1 = ABCD;
  MSG3 = _mm_sha1msg2_epu32(MSG3, MSG2);
  ABCD = _mm_sha1rnds4_epu32(ABCD, E0, 2);
  MSG1 = _mm_sha1msg1_epu32(MSG1, MSG2);
  MSG0 = _mm_xor_si128(MSG0, MSG2);
  
  // Rounds 60-63 (function 3)
  E1 = _mm_sha1nexte_epu32(E1, MSG3);
  E0 = ABCD;
  MSG0 = _mm_sha1msg2_epu32(MSG0, MSG3);
  ABCD = _mm_sha1rnds4_epu32(ABCD, E1, 3);
  MSG2 = _mm_sha1msg1_epu32(MSG2, MSG3);
  MSG1 = _mm_xor_si128(MSG1, MSG3);
  
  // Rounds 64-67 (function 3)
  E0 = _mm_sha1nexte_epu32(E0, MSG0);
  E1 = ABCD;
  MSG1 = _mm_sha1msg2_epu32(MSG1, MSG0);
  ABCD = _mm_sha1rnds4_epu32(ABCD, E0, 3);
  MSG3 = _mm_sha1msg1_epu32(MSG3, MSG0);
  MSG2 = _mm_xor_si128(MSG2, MSG0);
  
  // Rounds 68-71 (function 3)
  E1 = _mm_sha1nexte_epu32(E1, MSG1);
  E0 = ABCD;
  MSG2 = _mm_sha1msg2_epu32(MSG2, MSG1);
  ABCD = _mm_sha1rnds4_epu32(ABCD, E1, 3);
  MSG3 = _mm_xor_si128(MSG3, MSG1);
  
  // Rounds 72-75 (function 3)
  E0 = _mm_sha1nexte_epu32(E0, MSG2);
  E1 = ABCD;
  MSG3 = _mm_sha1msg2_epu32(MSG3, MSG2);
  ABCD = _mm_sha1rnds4_epu32(ABCD, E0, 3);
  
  // Rounds 76-79 (function 3)
  E1 = _mm_sha1nexte_epu32(E1, MSG3);
  E0 = ABCD;
  ABCD = _mm_sha1rnds4_epu32(ABCD, E1, 3);
  
  // Add initial state to final result
  E0 = _mm_sha1nexte_epu32(E0, E0_SAVE);
  ABCD = _mm_add_epi32(ABCD, ABCD_SAVE);
  
  // Shuffle ABCD back to correct order and store
  ABCD = _mm_shuffle_epi32(ABCD, 0x1B); // Reverse order back
  _mm_storeu_si128((__m128i*)state, ABCD);
  state[4] = _mm_extract_epi32(E0, 3);
}

} // namespace dxvk::sha1

#endif // __SHA__
#endif // DXVK_ARCH_X86 || DXVK_ARCH_X86_64