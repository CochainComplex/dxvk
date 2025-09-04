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
    Vector4() : xmm(Vec4f(0.0f)) {}
    Vector4(float splat) : xmm(Vec4f(splat)) {}
    Vector4(float x, float y, float z, float w) : xmm(Vec4f(x, y, z, w)) {}
    Vector4(const float xyzw[4]) : xmm(Vec4f().load(xyzw)) {}
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
    // Optimized dot product avoiding slow horizontal_add
    Vec4f prod = Vec4f(a.xmm) * Vec4f(b.xmm);
    
    // Use VCL's optimized horizontal reduction
    // This typically compiles to fewer instructions than horizontal_add
    Vec4f high64 = permute4<2,3,0,1>(prod);  // Swap high/low 64-bit halves
    Vec4f sum64 = prod + high64;              // Add corresponding elements
    Vec4f high32 = permute4<1,0,3,2>(sum64); // Swap 32-bit elements
    Vec4f finalSum = sum64 + high32;          // Final sum
    
    return finalSum[0];  // Extract scalar result
  }
  
  // 3D dot product (ignoring w component) - common in graphics
  inline float dot3(const Vector4& a, const Vector4& b) {
    // Mask out w component for 3D dot product
    static const Vec4f mask3d(1.0f, 1.0f, 1.0f, 0.0f);
    Vec4f prod = Vec4f(a.xmm) * Vec4f(b.xmm) * mask3d;
    
    // Optimized horizontal reduction
    Vec4f high64 = permute4<2,3,0,1>(prod);
    Vec4f sum64 = prod + high64;
    Vec4f high32 = permute4<1,0,3,2>(sum64);
    Vec4f finalSum = sum64 + high32;
    
    return finalSum[0];
  }
  
  // Batch dot product - compute multiple dot products simultaneously
  inline Vector4 dotBatch(const Vector4& a, const Vector4& b0, const Vector4& b1, 
                         const Vector4& b2, const Vector4& b3) {
    // Compute 4 dot products in parallel using matrix operations
    Vec4f av = Vec4f(a.xmm);
    Vec4f x = permute4<0,0,0,0>(av);
    Vec4f y = permute4<1,1,1,1>(av);
    Vec4f z = permute4<2,2,2,2>(av);
    Vec4f w = permute4<3,3,3,3>(av);
    
    Vec4f dots = x * Vec4f(b0[0], b1[0], b2[0], b3[0]) +
                 y * Vec4f(b0[1], b1[1], b2[1], b3[1]) +
                 z * Vec4f(b0[2], b1[2], b2[2], b3[2]) +
                 w * Vec4f(b0[3], b1[3], b2[3], b3[3]);
    
    return Vector4(dots);
  }
  
  inline float lengthSqr(const Vector4& a) {
    return dot(a, a);
  }
  
  inline float length(const Vector4& a) {
    // Use VCL's optimized sqrt for better performance than std::sqrt
    Vec4f lengthSq = Vec4f(lengthSqr(a));
    return sqrt(lengthSq)[0];
  }
  
  inline Vector4 normalize(const Vector4& a) {
    // Optimized normalize using single square root operation
    float lenSq = lengthSqr(a);
    if (unlikely(lenSq <= std::numeric_limits<float>::min())) {
      return Vector4(0.0f);  // Handle zero vector
    }
    
    // Use VCL's optimized reciprocal square root for better performance
    Vec4f invLength = Vec4f(1.0f / std::sqrt(lenSq));
    return Vec4f(a.xmm) * invLength;
  }
  
  // Fast normalize for graphics where approximate reciprocal sqrt is acceptable
  inline Vector4 normalizeFast(const Vector4& a) {
    Vec4f lengthSq = Vec4f(lengthSqr(a));
    
    // Guard against zero/near-zero vectors
    Vec4fb mask = lengthSq > Vec4f(std::numeric_limits<float>::min());
    
    // Use approximate reciprocal square root (Newton-Raphson iteration)
    // This is faster but slightly less accurate than full sqrt + divide
    Vec4f approxInvSqrt = approx_rsqrt(lengthSq);
    
    // One Newton-Raphson iteration for better accuracy: x' = x * (1.5 - 0.5 * a * x^2)
    Vec4f refined = approxInvSqrt * (Vec4f(1.5f) - Vec4f(0.5f) * lengthSq * approxInvSqrt * approxInvSqrt);
    
    return Vector4(select(mask, Vec4f(a.xmm) * refined, Vec4f(0.0f)));
  }
  
  // Optimized NaN replacement using VCL's select function
  inline Vector4 replaceNaN(Vector4 a) {
    Vec4f v(a.xmm);
    return select(is_nan(v), Vec4f(0.0f), v);
  }
  
  // Additional optimized vector operations commonly used in graphics
  
  // Cross product for 3D vectors (w component ignored)
  inline Vector4 cross3(const Vector4& a, const Vector4& b) {
    Vec4f av = Vec4f(a.xmm);
    Vec4f bv = Vec4f(b.xmm);
    
    // Cross product: (a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x, 0)
    Vec4f a_yzx = permute4<1,2,0,3>(av);  // a.y, a.z, a.x, a.w
    Vec4f b_zxy = permute4<2,0,1,3>(bv);  // b.z, b.x, b.y, b.w
    Vec4f a_zxy = permute4<2,0,1,3>(av);  // a.z, a.x, a.y, a.w
    Vec4f b_yzx = permute4<1,2,0,3>(bv);  // b.y, b.z, b.x, b.w
    
    Vec4f result = a_yzx * b_zxy - a_zxy * b_yzx;
    
    // Zero out w component
    return Vector4(blend4<0,1,2,7>(result, Vec4f(0.0f)));
  }
  
  // Reflect vector across normal (common in lighting calculations)
  inline Vector4 reflect(const Vector4& incident, const Vector4& normal) {
    // r = i - 2 * dot(n, i) * n
    float dotProduct = dot(normal, incident);
    return incident - normal * (2.0f * dotProduct);
  }
  
  // Linear interpolation
  inline Vector4 lerp(const Vector4& a, const Vector4& b, float t) {
    Vec4f tv = Vec4f(t);
    return Vec4f(a.xmm) + (Vec4f(b.xmm) - Vec4f(a.xmm)) * tv;
  }
  
  // Component-wise minimum/maximum
  inline Vector4 min(const Vector4& a, const Vector4& b) {
    return Vector4(min(Vec4f(a.xmm), Vec4f(b.xmm)));
  }
  
  inline Vector4 max(const Vector4& a, const Vector4& b) {
    return Vector4(max(Vec4f(a.xmm), Vec4f(b.xmm)));
  }
  
  // Clamp vector components
  inline Vector4 clamp(const Vector4& v, const Vector4& minVec, const Vector4& maxVec) {
    return min(max(v, minVec), maxVec);
  }
  
  // Distance between two points
  inline float distance(const Vector4& a, const Vector4& b) {
    return length(a - b);
  }
  
  // Squared distance (avoids sqrt for performance when only comparison is needed)
  inline float distanceSqr(const Vector4& a, const Vector4& b) {
    return lengthSqr(a - b);
  }
  
  // VCL-optimized Vector4i implementation
  // Uses direct SIMD register storage with union overlay for DXVK compatibility
  // Optimized for data storage and copying operations (primary use case in DXVK)
  
  struct alignas(16) Vector4i {
    // Core storage - direct SIMD register with union overlay
    union {
      __m128i xmm;                      // Direct SIMD register storage
      int data[4];                      // Array access for compatibility
      struct { int x, y, z, w; };       // Named access
      struct { int r, g, b, a; };       // Color access
    };
    
    // Constructors - inline for zero overhead
    Vector4i() : xmm(Vec4i(0)) {}
    Vector4i(int splat) : xmm(Vec4i(splat)) {}
    Vector4i(int x, int y, int z, int w) : xmm(Vec4i(x, y, z, w)) {}
    Vector4i(const int xyzw[4]) : xmm(Vec4i().load(xyzw)) {}
    Vector4i(__m128i v) : xmm(v) {}
    Vector4i(const Vector4i& other) = default;
    Vector4i& operator=(const Vector4i& other) = default;
    
    // Implicit conversions for VCL interop (zero-cost abstractions)
    operator Vec4i() const { return Vec4i(xmm); }
    Vector4i(const Vec4i& v) : xmm(v) {}
    
    // Element access
    inline int& operator[](size_t index) { return data[index]; }
    inline const int& operator[](size_t index) const { return data[index]; }
    
    // Comparison operators - optimized for DXVK's usage patterns
    bool operator==(const Vector4i& other) const {
      // Use VCL's SIMD comparison, then check if all elements are equal
      Vec4ib cmp = Vec4i(xmm) == Vec4i(other.xmm);
      return horizontal_and(cmp);
    }
    
    bool operator!=(const Vector4i& other) const {
      return !operator==(other);
    }
    
    // Arithmetic operators - delegate to VCL for SIMD optimization
    // Note: These are rarely used in DXVK but provided for completeness
    Vector4i operator-() const { 
      return -Vec4i(xmm);
    }
    
    Vector4i operator+(const Vector4i& other) const {
      return Vec4i(xmm) + Vec4i(other.xmm);
    }
    
    Vector4i operator-(const Vector4i& other) const {
      return Vec4i(xmm) - Vec4i(other.xmm);
    }
    
    Vector4i operator*(int scalar) const {
      return Vec4i(xmm) * scalar;
    }
    
    Vector4i operator*(const Vector4i& other) const {
      return Vec4i(xmm) * Vec4i(other.xmm);
    }
    
    Vector4i operator/(const Vector4i& other) const {
      // Integer division is not directly supported by VCL for efficiency reasons
      // Fall back to scalar implementation for this rare operation
      return Vector4i(data[0] / other.data[0], data[1] / other.data[1], 
                     data[2] / other.data[2], data[3] / other.data[3]);
    }
    
    Vector4i operator/(int scalar) const {
      // Integer division by scalar - fall back to component-wise operation
      return Vector4i(data[0] / scalar, data[1] / scalar, 
                     data[2] / scalar, data[3] / scalar);
    }
    
    // In-place operators
    Vector4i& operator+=(const Vector4i& other) {
      xmm = (Vec4i(xmm) + Vec4i(other.xmm));
      return *this;
    }
    
    Vector4i& operator-=(const Vector4i& other) {
      xmm = (Vec4i(xmm) - Vec4i(other.xmm));
      return *this;
    }
    
    Vector4i& operator*=(int scalar) {
      xmm = (Vec4i(xmm) * scalar);
      return *this;
    }
    
    Vector4i& operator/=(int scalar) {
      // Component-wise division for efficiency
      x /= scalar;
      y /= scalar;
      z /= scalar;
      w /= scalar;
      return *this;
    }
    
    // Integer-specific bitwise operations (VCL optimized)
    Vector4i operator&(const Vector4i& other) const {
      return Vec4i(xmm) & Vec4i(other.xmm);
    }
    
    Vector4i operator|(const Vector4i& other) const {
      return Vec4i(xmm) | Vec4i(other.xmm);
    }
    
    Vector4i operator^(const Vector4i& other) const {
      return Vec4i(xmm) ^ Vec4i(other.xmm);
    }
    
    Vector4i operator~() const {
      return ~Vec4i(xmm);
    }
    
    // Shift operations
    Vector4i operator<<(int shift) const {
      return Vec4i(xmm) << shift;
    }
    
    Vector4i operator>>(int shift) const {
      return Vec4i(xmm) >> shift;
    }
  };
  
  // Scalar * Vector
  inline Vector4i operator*(int scalar, const Vector4i& vector) {
    return vector * scalar;
  }
  
  // Optimized integer vector operations using VCL
  inline Vector4i min(const Vector4i& a, const Vector4i& b) {
    return min(Vec4i(a.xmm), Vec4i(b.xmm));
  }
  
  inline Vector4i max(const Vector4i& a, const Vector4i& b) {
    return max(Vec4i(a.xmm), Vec4i(b.xmm));
  }
  
  // Absolute value
  inline Vector4i abs(const Vector4i& a) {
    return abs(Vec4i(a.xmm));
  }
  
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
    for (int i = 0; i < 4; i++)
      a[i] = std::isnan(a[i]) ? 0.0f : a[i];
    return a;
  }

  // Output operator for scalar implementation
  template <typename T>
  std::ostream& operator<<(std::ostream& os, const Vector4Base<T>& v) {
    return os << "Vector4(" << v[0] << ", " << v[1] << ", " << v[2] << ", " << v[3] << ")";
  }
#endif

  // Output operators for VCL implementation
  inline std::ostream& operator<<(std::ostream& os, const Vector4& v) {
    return os << "Vector4(" << v[0] << ", " << v[1] << ", " << v[2] << ", " << v[3] << ")";
  }

  inline std::ostream& operator<<(std::ostream& os, const Vector4i& v) {
    return os << "Vector4i(" << v[0] << ", " << v[1] << ", " << v[2] << ", " << v[3] << ")";
  }

  // Static assertions for size guarantees
  static_assert(sizeof(Vector4)  == sizeof(float) * 4);
  static_assert(sizeof(Vector4i) == sizeof(int)   * 4);
}