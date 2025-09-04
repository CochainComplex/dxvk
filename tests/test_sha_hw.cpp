/*
 * Test program for SHA1 hardware acceleration in DXVK
 * Validates correctness and measures performance improvement
 */

#include <iostream>
#include <iomanip>
#include <chrono>
#include <cstring>
#include <vector>
#include "../src/util/sha1/sha1.h"
#include "../src/util/sha1/sha1_util.h"

// Check if SHA hardware is available
#ifdef DXVK_USE_SHA_HW
  #include "../src/util/util_sha_hw.h"
  #if defined(DXVK_ARCH_X86) || defined(DXVK_ARCH_X86_64)
    #ifdef __SHA__
      extern "C" {
        namespace dxvk::sha1 {
          extern bool has_sha_extensions();
        }
      }
      #define SHA_HW_AVAILABLE 1
    #endif
  #endif
#endif

// Test vectors from RFC 3174
struct TestVector {
  const char* input;
  const char* expected_sha1;
  int repeat_count;
};

const TestVector test_vectors[] = {
  // Test 1: "abc"
  { "abc", "a9993e364706816aba3e25717850c26c9cd0d89d", 1 },
  
  // Test 2: "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
  { "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", 
    "84983e441c3bd26ebaae4aa1f95129e5e54670f1", 1 },
  
  // Test 3: "a" repeated 1,000,000 times
  { "a", "34aa973cd4c4daa4f61eeb2bdbad27316534016f", 1000000 },
  
  // Test 4: "0123456701234567..." repeated 10 times (640 chars)
  { "01234567012345670123456701234567012345670123456701234567012345670123456701234567",
    "dea356a2cddd90c7a7ecedc5ebb563934f460452", 10 }
};

void print_hex(const uint8_t* data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    std::cout << std::hex << std::setfill('0') << std::setw(2) 
              << static_cast<int>(data[i]);
  }
}

bool run_sha1_test(const TestVector& test) {
  std::cout << "Testing: ";
  if (test.repeat_count > 1) {
    std::cout << "\"" << test.input << "\" repeated " 
              << test.repeat_count << " times";
  } else {
    std::cout << "\"" << test.input << "\"";
  }
  std::cout << std::endl;
  
  // Prepare input data
  std::vector<uint8_t> input_data;
  size_t input_len = strlen(test.input);
  input_data.reserve(input_len * test.repeat_count);
  
  for (int i = 0; i < test.repeat_count; i++) {
    input_data.insert(input_data.end(), 
                      reinterpret_cast<const uint8_t*>(test.input),
                      reinterpret_cast<const uint8_t*>(test.input) + input_len);
  }
  
  // Calculate SHA1
  SHA1_CTX ctx;
  SHA1Init(&ctx);
  SHA1Update(&ctx, input_data.data(), input_data.size());
  
  uint8_t digest[SHA1_DIGEST_LENGTH];
  SHA1Final(digest, &ctx);
  
  // Convert to hex string
  std::stringstream result_hex;
  for (int i = 0; i < SHA1_DIGEST_LENGTH; i++) {
    result_hex << std::hex << std::setfill('0') << std::setw(2) 
               << static_cast<int>(digest[i]);
  }
  
  // Compare with expected
  bool passed = (result_hex.str() == test.expected_sha1);
  
  std::cout << "  Expected: " << test.expected_sha1 << std::endl;
  std::cout << "  Got:      " << result_hex.str() << std::endl;
  std::cout << "  Result:   " << (passed ? "PASSED" : "FAILED") << std::endl;
  
  return passed;
}

void benchmark_sha1(size_t data_size, int iterations) {
  std::cout << "\nBenchmarking SHA1 with " << data_size 
            << " bytes, " << iterations << " iterations..." << std::endl;
  
  // Generate random data
  std::vector<uint8_t> data(data_size);
  for (size_t i = 0; i < data_size; i++) {
    data[i] = static_cast<uint8_t>(i & 0xFF);
  }
  
  // Warm-up
  for (int i = 0; i < 10; i++) {
    SHA1_CTX ctx;
    SHA1Init(&ctx);
    SHA1Update(&ctx, data.data(), data.size());
    uint8_t digest[SHA1_DIGEST_LENGTH];
    SHA1Final(digest, &ctx);
  }
  
  // Benchmark
  auto start = std::chrono::high_resolution_clock::now();
  
  for (int i = 0; i < iterations; i++) {
    SHA1_CTX ctx;
    SHA1Init(&ctx);
    SHA1Update(&ctx, data.data(), data.size());
    uint8_t digest[SHA1_DIGEST_LENGTH];
    SHA1Final(digest, &ctx);
  }
  
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  
  double total_mb = (data_size * iterations) / (1024.0 * 1024.0);
  double seconds = duration.count() / 1000000.0;
  double throughput = total_mb / seconds;
  
  std::cout << "  Time:       " << duration.count() << " microseconds" << std::endl;
  std::cout << "  Throughput: " << std::fixed << std::setprecision(2) 
            << throughput << " MB/s" << std::endl;
}

int main() {
  std::cout << "=== DXVK SHA1 Hardware Acceleration Test ===" << std::endl;
  
  // Check if hardware acceleration is available
#ifdef SHA_HW_AVAILABLE
  bool sha_hw_enabled = dxvk::sha1::has_sha_extensions();
  std::cout << "SHA hardware acceleration: " 
            << (sha_hw_enabled ? "AVAILABLE (using SHA-NI)" : "NOT AVAILABLE (using software)")
            << std::endl;
#else
  std::cout << "SHA hardware acceleration: NOT COMPILED (build with -Denable_sha_hw=true)" 
            << std::endl;
#endif
  
  std::cout << std::endl;
  
  // Run correctness tests
  std::cout << "=== Correctness Tests ===" << std::endl;
  int passed = 0;
  int total = sizeof(test_vectors) / sizeof(test_vectors[0]);
  
  for (int i = 0; i < total; i++) {
    if (run_sha1_test(test_vectors[i])) {
      passed++;
    }
    std::cout << std::endl;
  }
  
  std::cout << "Results: " << passed << "/" << total << " tests passed" << std::endl;
  
  // Run performance benchmarks
  std::cout << "\n=== Performance Benchmarks ===" << std::endl;
  
  // Small data (typical shader key size)
  benchmark_sha1(64, 100000);    // 64 bytes (1 block)
  benchmark_sha1(256, 50000);    // 256 bytes (4 blocks)
  
  // Medium data (typical shader size)
  benchmark_sha1(4096, 5000);    // 4 KB
  benchmark_sha1(16384, 1000);   // 16 KB
  
  // Large data (stress test)
  benchmark_sha1(1048576, 100);  // 1 MB
  
  return (passed == total) ? 0 : 1;
}