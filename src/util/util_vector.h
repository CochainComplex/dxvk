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
  // KISS Implementation: Vector4 IS Vec4f with named access
  // ALL operators come from Vec4f - we don't redefine them (DRY principle)
  
  struct alignas(16) Vector4 {
    // Single storage as Vec4f with union overlay for named access
    union {
      Vec4f v;                           // The actual VCL vector - has ALL operators
      float data[4];                     // Array access overlay
      struct { float x, y, z, w; };     // Named access overlay
      struct { float r, g, b, a; };     // Color access overlay
    };
    
    // Constructors - minimal set, delegate to Vec4f
    Vector4() : v(0.0f) {}
    Vector4(float splat) : v(splat) {}
    Vector4(float x, float y, float z, float w) : v(x, y, z, w) {}
    Vector4(const float xyzw[4]) : v() { v.load(xyzw); }
    Vector4(__m128 xmm) : v(xmm) {}
    Vector4(const Vec4f& vec) : v(vec) {}
    
    // Implicit conversion to Vec4f - this enables ALL of Agner's operators!
    // When you write: Vector4 a, b; auto c = a + b;
    // It becomes: auto c = Vec4f(a) + Vec4f(b); using Agner's operator+
    operator Vec4f() const { return v; }
    operator Vec4f&() { return v; }
    operator __m128() const { return v; }
    
    // Element access
    float& operator[](size_t index) { return data[index]; }
    const float& operator[](size_t index) const { return data[index]; }
    
    // Comparison operators - these need special handling for boolean result
    bool operator==(const Vector4& other) const {
      Vec4fb cmp = v == other.v;  // Uses Vec4f's operator==
      return horizontal_and(cmp);
    }
    bool operator!=(const Vector4& other) const { 
      return !operator==(other); 
    }
  };
  
  // NO OPERATOR DEFINITIONS NEEDED!
  // Vec4f already has ALL arithmetic operators defined by Agner Fog:
  // - operator+, -, *, / (both vector-vector and vector-scalar)
  // - operator+=, -=, *=, /=
  // - operator- (unary negation)
  // Our implicit conversion to Vec4f makes them all work automatically!
  
  // KISS Implementation: Vector4i IS Vec4i with named access
  // ALL operators come from Vec4i - we don't redefine them (DRY principle)
  struct alignas(16) Vector4i {
    // Single storage as Vec4i with union overlay for named access
    union {
      Vec4i v;                           // The actual VCL vector - has ALL operators
      int32_t data[4];                   // Array access overlay
      struct { int32_t x, y, z, w; };   // Named access overlay
      struct { int32_t r, g, b, a; };   // Color access overlay
    };
    
    // Constructors - minimal set, delegate to Vec4i
    Vector4i() : v(0) {}
    Vector4i(int32_t splat) : v(splat) {}
    Vector4i(int32_t x, int32_t y, int32_t z, int32_t w) : v(x, y, z, w) {}
    Vector4i(const int32_t xyzw[4]) : v() { v.load(xyzw); }
    Vector4i(__m128i xmm) : v(xmm) {}
    Vector4i(const Vec4i& vec) : v(vec) {}
    
    // Implicit conversion to Vec4i - this enables ALL of Agner's operators!
    operator Vec4i() const { return v; }
    operator Vec4i&() { return v; }
    operator __m128i() const { return v; }
    
    // Element access
    int32_t& operator[](size_t index) { return data[index]; }
    const int32_t& operator[](size_t index) const { return data[index]; }
    
    // Comparison operators - these need special handling for boolean result
    bool operator==(const Vector4i& other) const {
      Vec4ib cmp = v == other.v;  // Uses Vec4i's operator==
      return horizontal_and(cmp);
    }
    bool operator!=(const Vector4i& other) const {
      return !operator==(other);
    }
  };
  
  // NO OPERATOR DEFINITIONS NEEDED!
  // Vec4i already has ALL arithmetic operators defined by Agner Fog:
  // - operator+, -, * (both vector-vector and vector-scalar)
  // - operator+=, -=, *=
  // - operator- (unary negation)
  // - bitwise operators: &, |, ^, ~, <<, >>
  // Our implicit conversion to Vec4i makes them all work automatically!
  
  // DXVK-specific vector operations using VCL functions
  // These use Vec4f functions through implicit conversion
  inline float dot(const Vector4& a, const Vector4& b) {
    return horizontal_add(Vec4f(a) * Vec4f(b));  // Uses Vec4f's operator*
  }
  
  inline float lengthSqr(const Vector4& a) { 
    return dot(a, a); 
  }
  
  inline float length(const Vector4& a) { 
    return std::sqrt(lengthSqr(a)); 
  }
  
  inline Vector4 normalize(const Vector4& a) {
    float len = length(a);
    return len != 0.0f ? Vector4(Vec4f(a) / len) : Vector4(0.0f);  // Uses Vec4f's operator/
  }
  
  // Hot path: NaN replacement for D3D9 shader constants
  inline Vector4 replaceNaN(const Vector4& a) {
    // Uses Vec4f's is_nan and select functions
    Vec4fb nan_mask = is_nan(Vec4f(a));
    return select(nan_mask, Vec4f(0.0f), Vec4f(a));
  }

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

  // Output stream operator for Vector4Base (non-VCL mode)
  template <typename T>
  std::ostream& operator<<(std::ostream& os, const Vector4Base<T>& v) {
    return os << "Vector4(" << v[0] << ", " << v[1] << ", " << v[2] << ", " << v[3] << ")";
  }

#endif // DXVK_USE_VCL && DXVK_ARCH_X86

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