#pragma once

#include <iostream>
#include "util_bit.h"
#include "util_math.h"

// Conditional VCL integration
#ifdef DXVK_USE_VCL
  #ifdef DXVK_ARCH_X86
    #include "../../include/vectorclass/vectorclass.h"
  #endif
#endif

namespace dxvk {

#if defined(DXVK_USE_VCL) && defined(DXVK_ARCH_X86)
  // VCL-optimized Vector4 implementation
  // Uses direct SIMD register storage with union overlay for DXVK compatibility
  
  struct alignas(16) Vector4 {
    // Core storage - direct SIMD register with union overlay
    union {
      __m128 xmm;                      // Direct SIMD register storage
      float data[4];                    // Array access for compatibility
      struct { float x, y, z, w; };     // Named access
      struct { float r, g, b, a; };     // Color access
    };
    
    // Constructors - inline for zero overhead
    Vector4() : xmm(_mm_setzero_ps()) {}
    Vector4(float splat) : xmm(_mm_set1_ps(splat)) {}
    Vector4(float x, float y, float z, float w) : xmm(_mm_set_ps(w, z, y, x)) {}
    Vector4(const float xyzw[4]) : xmm(_mm_loadu_ps(xyzw)) {}
    Vector4(__m128 v) : xmm(v) {}
    Vector4(const Vector4& other) = default;
    Vector4& operator=(const Vector4& other) = default;
    
    // Implicit conversions for VCL interop (zero-cost abstractions)
    operator Vec4f() const { return Vec4f(xmm); }
    Vector4(const Vec4f& v) : xmm(v) {}
    
    // Element access
    inline float& operator[](size_t index) { return data[index]; }
    inline const float& operator[](size_t index) const { return data[index]; }
    
    // Comparison operators
    bool operator==(const Vector4& other) const {
      return horizontal_and(Vec4f(xmm) == Vec4f(other.xmm));
    }
    
    bool operator!=(const Vector4& other) const {
      return !operator==(other);
    }
    
    // Arithmetic operators - all delegate to VCL for SIMD optimization
    Vector4 operator-() const { 
      return Vec4f(xmm) * Vec4f(-1.0f);
    }
    
    Vector4 operator+(const Vector4& other) const {
      return Vec4f(xmm) + Vec4f(other.xmm);
    }
    
    Vector4 operator-(const Vector4& other) const {
      return Vec4f(xmm) - Vec4f(other.xmm);
    }
    
    Vector4 operator*(float scalar) const {
      return Vec4f(xmm) * scalar;
    }
    
    Vector4 operator*(const Vector4& other) const {
      return Vec4f(xmm) * Vec4f(other.xmm);
    }
    
    Vector4 operator/(const Vector4& other) const {
      return Vec4f(xmm) / Vec4f(other.xmm);
    }
    
    Vector4 operator/(float scalar) const {
      return Vec4f(xmm) / scalar;
    }
    
    // In-place operators
    Vector4& operator+=(const Vector4& other) {
      xmm = (Vec4f(xmm) + Vec4f(other.xmm));
      return *this;
    }
    
    Vector4& operator-=(const Vector4& other) {
      xmm = (Vec4f(xmm) - Vec4f(other.xmm));
      return *this;
    }
    
    Vector4& operator*=(float scalar) {
      xmm = (Vec4f(xmm) * scalar);
      return *this;
    }
    
    Vector4& operator/=(float scalar) {
      xmm = (Vec4f(xmm) / scalar);
      return *this;
    }
  };
  
  // Scalar * Vector
  inline Vector4 operator*(float scalar, const Vector4& vector) {
    return vector * scalar;
  }
  
  // Free functions using VCL operations directly for maximum performance
  inline float dot(const Vector4& a, const Vector4& b) {
    return horizontal_add(Vec4f(a.xmm) * Vec4f(b.xmm));
  }
  
  inline float lengthSqr(const Vector4& a) {
    return dot(a, a);
  }
  
  inline float length(const Vector4& a) {
    return std::sqrt(lengthSqr(a));
  }
  
  inline Vector4 normalize(const Vector4& a) {
    return a * (1.0f / length(a));
  }
  
  // Optimized NaN replacement using VCL's select function
  inline Vector4 replaceNaN(Vector4 a) {
    Vec4f v(a.xmm);
    return select(is_nan(v), Vec4f(0.0f), v);
  }
  
  // Vector4i - keep scalar implementation for now
  // TODO: Optimize with Vec4i when needed
  template <typename T>
  struct Vector4Base {
    Vector4Base()
      : x{ }, y{ }, z{ }, w{ } { }

    Vector4Base(T splat)
      : x(splat), y(splat), z(splat), w(splat) { }

    Vector4Base(T x, T y, T z, T w)
      : x(x), y(y), z(z), w(w) { }

    Vector4Base(const T xyzw[4])
      : x(xyzw[0]), y(xyzw[1]), z(xyzw[2]), w(xyzw[3]) { }

    Vector4Base(const Vector4Base<T>& other) = default;
    Vector4Base& operator=(const Vector4Base<T>& other) = default;

    inline       T& operator[](size_t index)       { return data[index]; }
    inline const T& operator[](size_t index) const { return data[index]; }

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
      Vector4Base result;
      for (uint32_t i = 0; i < 4; i++)
        result[i] = data[i] * other.data[i];
      return result;
    }

    Vector4Base operator/(const Vector4Base<T>& other) const {
      Vector4Base result;
      for (uint32_t i = 0; i < 4; i++)
        result[i] = data[i] / other.data[i];
      return result;
    }

    Vector4Base operator/(T scalar) const {
      return {x / scalar, y / scalar, z / scalar, w / scalar};
    }

    Vector4Base& operator+=(const Vector4Base<T>& other) {
      x += other.x;
      y += other.y;
      z += other.z;
      w += other.w;
      return *this;
    }

    Vector4Base& operator-=(const Vector4Base<T>& other) {
      x -= other.x;
      y -= other.y;
      z -= other.z;
      w -= other.w;
      return *this;
    }

    Vector4Base& operator*=(T scalar) {
      x *= scalar;
      y *= scalar;
      z *= scalar;
      w *= scalar;
      return *this;
    }

    Vector4Base& operator/=(T scalar) {
      x /= scalar;
      y /= scalar;
      z /= scalar;
      w /= scalar;
      return *this;
    }

    union {
      T data[4];
      struct {
        T x, y, z, w;
      };
      struct {
        T r, g, b, a;
      };
    };
  };
  
  using Vector4i = Vector4Base<int>;
  
#else
  // Original scalar implementation (fallback when VCL not available)
  
  template <typename T>
  struct Vector4Base {
    Vector4Base()
      : x{ }, y{ }, z{ }, w{ } { }

    Vector4Base(T splat)
      : x(splat), y(splat), z(splat), w(splat) { }

    Vector4Base(T x, T y, T z, T w)
      : x(x), y(y), z(z), w(w) { }

    Vector4Base(const T xyzw[4])
      : x(xyzw[0]), y(xyzw[1]), z(xyzw[2]), w(xyzw[3]) { }

    Vector4Base(const Vector4Base<T>& other) = default;
    Vector4Base& operator=(const Vector4Base<T>& other) = default;

    inline       T& operator[](size_t index)       { return data[index]; }
    inline const T& operator[](size_t index) const { return data[index]; }

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
      Vector4Base result;
      for (uint32_t i = 0; i < 4; i++)
        result[i] = data[i] * other.data[i];
      return result;
    }

    Vector4Base operator/(const Vector4Base<T>& other) const {
      Vector4Base result;
      for (uint32_t i = 0; i < 4; i++)
        result[i] = data[i] / other.data[i];
      return result;
    }

    Vector4Base operator/(T scalar) const {
      return {x / scalar, y / scalar, z / scalar, w / scalar};
    }

    Vector4Base& operator+=(const Vector4Base<T>& other) {
      x += other.x;
      y += other.y;
      z += other.z;
      w += other.w;
      return *this;
    }

    Vector4Base& operator-=(const Vector4Base<T>& other) {
      x -= other.x;
      y -= other.y;
      z -= other.z;
      w -= other.w;
      return *this;
    }

    Vector4Base& operator*=(T scalar) {
      x *= scalar;
      y *= scalar;
      z *= scalar;
      w *= scalar;
      return *this;
    }

    Vector4Base& operator/=(T scalar) {
      x /= scalar;
      y /= scalar;
      z /= scalar;
      w /= scalar;
      return *this;
    }

    union {
      T data[4];
      struct {
        T x, y, z, w;
      };
      struct {
        T r, g, b, a;
      };
    };
  };

  template <typename T>
  inline Vector4Base<T> operator*(T scalar, const Vector4Base<T>& vector) {
    return vector * scalar;
  }

  template <typename T>
  float dot(const Vector4Base<T>& a, const Vector4Base<T>& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
  }

  template <typename T>
  T lengthSqr(const Vector4Base<T>& a) { return dot(a, a); }

  template <typename T>
  float length(const Vector4Base<T>& a) { return std::sqrt(float(lengthSqr(a))); }

  template <typename T>
  Vector4Base<T> normalize(const Vector4Base<T>& a) { return a * T(1.0f / length(a)); }

  using Vector4  = Vector4Base<float>;
  using Vector4i = Vector4Base<int>;

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
#endif

  // Output operator for both implementations
  template <typename T>
  std::ostream& operator<<(std::ostream& os, const Vector4Base<T>& v) {
    return os << "Vector4(" << v[0] << ", " << v[1] << ", " << v[2] << ", " << v[3] << ")";
  }

  inline std::ostream& operator<<(std::ostream& os, const Vector4& v) {
    return os << "Vector4(" << v[0] << ", " << v[1] << ", " << v[2] << ", " << v[3] << ")";
  }

  // Static assertions for size guarantees
  static_assert(sizeof(Vector4)  == sizeof(float) * 4);
  static_assert(sizeof(Vector4i) == sizeof(int)   * 4);
}