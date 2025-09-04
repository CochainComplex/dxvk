#pragma once

#include <iostream>
#include <cmath>
#include "util_bit.h"
#include "util_math.h"

#ifdef DXVK_USE_VCL
  #ifdef DXVK_ARCH_X86
    #include "../../include/vectorclass/vectorclass.h"
  #endif
#endif

namespace dxvk {

#if defined(DXVK_USE_VCL) && defined(DXVK_ARCH_X86)
  // VCL-optimized Vector4 implementation (lean version)
  // Direct SIMD register storage with zero-overhead VCL operations
  
  struct alignas(16) Vector4 {
    // Core storage - direct SIMD register with union overlay for compatibility
    union {
      __m128 xmm;
      float data[4];
      struct { float x, y, z, w; };
      struct { float r, g, b, a; };
    };
    
    // Constructors
    Vector4() : xmm(Vec4f(0.0f)) {}
    Vector4(float splat) : xmm(Vec4f(splat)) {}
    Vector4(float x, float y, float z, float w) : xmm(Vec4f(x, y, z, w)) {}
    Vector4(const float xyzw[4]) : xmm(Vec4f().load(xyzw)) {}
    Vector4(__m128 v) : xmm(v) {}
    Vector4(const Vec4f& v) : xmm(v) {}
    Vector4(const Vector4& other) = default;
    Vector4& operator=(const Vector4& other) = default;
    
    // Element access
    float& operator[](size_t index) { return data[index]; }
    const float& operator[](size_t index) const { return data[index]; }
    
    // Comparison
    bool operator==(const Vector4& other) const {
      Vec4fb cmp = Vec4f(xmm) == Vec4f(other.xmm);
      return horizontal_and(cmp);
    }
    bool operator!=(const Vector4& other) const { return !operator==(other); }
    
    // Arithmetic operators - lean VCL delegation
    Vector4 operator-() const { return -Vec4f(xmm); }
    Vector4 operator+(const Vector4& other) const { return Vec4f(xmm) + Vec4f(other.xmm); }
    Vector4 operator-(const Vector4& other) const { return Vec4f(xmm) - Vec4f(other.xmm); }
    Vector4 operator*(float scalar) const { return Vec4f(xmm) * scalar; }
    Vector4 operator*(const Vector4& other) const { return Vec4f(xmm) * Vec4f(other.xmm); }
    Vector4 operator/(const Vector4& other) const { return Vec4f(xmm) / Vec4f(other.xmm); }
    Vector4 operator/(float scalar) const { return Vec4f(xmm) / scalar; }
    
    // In-place operators
    Vector4& operator+=(const Vector4& other) { xmm = Vec4f(xmm) + Vec4f(other.xmm); return *this; }
    Vector4& operator-=(const Vector4& other) { xmm = Vec4f(xmm) - Vec4f(other.xmm); return *this; }
    Vector4& operator*=(float scalar) { xmm = Vec4f(xmm) * scalar; return *this; }
    Vector4& operator/=(float scalar) { xmm = Vec4f(xmm) / scalar; return *this; }
  };
  
  // Vector4i implementation with VCL
  struct alignas(16) Vector4i {
    union {
      __m128i xmm;
      int32_t data[4];
      struct { int32_t x, y, z, w; };
      struct { int32_t r, g, b, a; };
    };
    
    Vector4i() : xmm(Vec4i(0)) {}
    Vector4i(int32_t splat) : xmm(Vec4i(splat)) {}
    Vector4i(int32_t x, int32_t y, int32_t z, int32_t w) : xmm(Vec4i(x, y, z, w)) {}
    Vector4i(const int32_t xyzw[4]) : xmm(Vec4i().load(xyzw)) {}
    Vector4i(__m128i v) : xmm(v) {}
    Vector4i(const Vec4i& v) : xmm(v) {}
    Vector4i(const Vector4i& other) = default;
    Vector4i& operator=(const Vector4i& other) = default;
    
    int32_t& operator[](size_t index) { return data[index]; }
    const int32_t& operator[](size_t index) const { return data[index]; }
    
    bool operator==(const Vector4i& other) const {
      Vec4ib cmp = Vec4i(xmm) == Vec4i(other.xmm);
      return horizontal_and(cmp);
    }
    bool operator!=(const Vector4i& other) const { return !operator==(other); }
    
    Vector4i operator-() const { return -Vec4i(xmm); }
    Vector4i operator+(const Vector4i& other) const { return Vec4i(xmm) + Vec4i(other.xmm); }
    Vector4i operator-(const Vector4i& other) const { return Vec4i(xmm) - Vec4i(other.xmm); }
    Vector4i operator*(int32_t scalar) const { return Vec4i(xmm) * scalar; }
    Vector4i operator*(const Vector4i& other) const { return Vec4i(xmm) * Vec4i(other.xmm); }
    
    Vector4i& operator+=(const Vector4i& other) { xmm = Vec4i(xmm) + Vec4i(other.xmm); return *this; }
    Vector4i& operator-=(const Vector4i& other) { xmm = Vec4i(xmm) - Vec4i(other.xmm); return *this; }
    Vector4i& operator*=(int32_t scalar) { xmm = Vec4i(xmm) * scalar; return *this; }
  };
  
  // Essential vector operations used by DXVK
  inline float dot(const Vector4& a, const Vector4& b) {
    // Custom optimized dot product for x86
    #if defined(__SSE4_1__)
      return _mm_cvtss_f32(_mm_dp_ps(a.xmm, b.xmm, 0xF1));
    #else
      Vec4f product = Vec4f(a.xmm) * Vec4f(b.xmm);
      return horizontal_add(product);
    #endif
  }
  
  inline float lengthSqr(const Vector4& a) { return dot(a, a); }
  inline float length(const Vector4& a) { return std::sqrt(lengthSqr(a)); }
  
  inline Vector4 normalize(const Vector4& a) {
    float len = length(a);
    return len != 0.0f ? a / len : Vector4(0.0f);
  }
  
  // Hot path: NaN replacement for D3D9 shader constants
  inline Vector4 replaceNaN(const Vector4& a) {
    Vec4f v(a.xmm);
    Vec4fb nan_mask = is_nan(v);
    return select(nan_mask, Vec4f(0.0f), v);
  }
  
  // Scalar multiplication (both orders)
  inline Vector4 operator*(float scalar, const Vector4& v) { return v * scalar; }
  inline Vector4i operator*(int32_t scalar, const Vector4i& v) { return v * scalar; }

#else
  // Original DXVK template implementation (when VCL is disabled)
  template <typename T>
  struct Vector4Base {
    Vector4Base() : x{}, y{}, z{}, w{} {}
    Vector4Base(T splat) : x(splat), y(splat), z(splat), w(splat) {}
    Vector4Base(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}
    Vector4Base(const T xyzw[4]) : x(xyzw[0]), y(xyzw[1]), z(xyzw[2]), w(xyzw[3]) {}
    Vector4Base(const Vector4Base<T>& other) = default;
    Vector4Base& operator=(const Vector4Base<T>& other) = default;

    T& operator[](size_t index) { return data[index]; }
    const T& operator[](size_t index) const { return data[index]; }

    bool operator==(const Vector4Base<T>& other) const {
      for (uint32_t i = 0; i < 4; i++) {
        if (data[i] != other.data[i])
          return false;
      }
      return true;
    }

    bool operator!=(const Vector4Base<T>& other) const {
      return !operator==(other);
    }

    Vector4Base operator-() const { return {-x, -y, -z, -w}; }
    Vector4Base operator+(const Vector4Base<T>& other) const {
      return {x + other.x, y + other.y, z + other.z, w + other.w};
    }
    Vector4Base operator-(const Vector4Base<T>& other) const {
      return {x - other.x, y - other.y, z - other.z, w - other.w};
    }
    Vector4Base operator*(T scalar) const {
      return {scalar * x, scalar * y, scalar * z, scalar * w};
    }
    Vector4Base operator*(const Vector4Base<T>& other) const {
      return {x * other.x, y * other.y, z * other.z, w * other.w};
    }
    Vector4Base operator/(const Vector4Base<T>& other) const {
      return {x / other.x, y / other.y, z / other.z, w / other.w};
    }
    Vector4Base operator/(T scalar) const {
      return {x / scalar, y / scalar, z / scalar, w / scalar};
    }

    Vector4Base& operator+=(const Vector4Base<T>& other) {
      x += other.x; y += other.y; z += other.z; w += other.w;
      return *this;
    }
    Vector4Base& operator-=(const Vector4Base<T>& other) {
      x -= other.x; y -= other.y; z -= other.z; w -= other.w;
      return *this;
    }
    Vector4Base& operator*=(T scalar) {
      x *= scalar; y *= scalar; z *= scalar; w *= scalar;
      return *this;
    }
    Vector4Base& operator/=(T scalar) {
      x /= scalar; y /= scalar; z /= scalar; w /= scalar;
      return *this;
    }

    union {
      T data[4];
      struct { T x, y, z, w; };
      struct { T r, g, b, a; };
    };
  };

  // Scalar multiplication
  template <typename T>
  inline Vector4Base<T> operator*(T scalar, const Vector4Base<T>& vector) {
    return vector * scalar;
  }

  // Essential vector operations
  template <typename T>
  float dot(const Vector4Base<T>& a, const Vector4Base<T>& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
  }

  template <typename T>
  T lengthSqr(const Vector4Base<T>& a) { return dot(a, a); }

  template <typename T>
  float length(const Vector4Base<T>& a) { return std::sqrt(float(lengthSqr(a))); }

  template <typename T>
  Vector4Base<T> normalize(const Vector4Base<T>& a) { 
    float len = length(a);
    return len != 0.0f ? a * T(1.0f / len) : Vector4Base<T>(0);
  }

  using Vector4  = Vector4Base<float>;
  using Vector4i = Vector4Base<int>;

  // Hot path: SSE-optimized NaN replacement for shader constants
  inline Vector4 replaceNaN(Vector4 a) {
    #ifdef DXVK_ARCH_X86
    Vector4 result;
    __m128 value = _mm_loadu_ps(a.data);
    __m128 mask  = _mm_cmpeq_ps(value, value);
           value = _mm_and_ps(value, mask);
    _mm_storeu_ps(result.data, value);
    return result;
    #else
    for (int i = 0; i < 4; i++)
      a[i] = std::isnan(a[i]) ? 0.0f : a[i];
    return a;
    #endif
  }

#endif // DXVK_USE_VCL && DXVK_ARCH_X86

  // Output stream operator
  template <typename T>
  std::ostream& operator<<(std::ostream& os, const Vector4Base<T>& v) {
    return os << "Vector4(" << v[0] << ", " << v[1] << ", " << v[2] << ", " << v[3] << ")";
  }
  
#if defined(DXVK_USE_VCL) && defined(DXVK_ARCH_X86)
  inline std::ostream& operator<<(std::ostream& os, const Vector4& v) {
    return os << "Vector4(" << v[0] << ", " << v[1] << ", " << v[2] << ", " << v[3] << ")";
  }
  inline std::ostream& operator<<(std::ostream& os, const Vector4i& v) {
    return os << "Vector4i(" << v[0] << ", " << v[1] << ", " << v[2] << ", " << v[3] << ")";
  }
#endif

  // Static assertions for size guarantees
  static_assert(sizeof(Vector4)  == sizeof(float) * 4);
  static_assert(sizeof(Vector4i) == sizeof(int) * 4);

}