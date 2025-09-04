#pragma once

#include "util_vector.h"

namespace dxvk {

#if defined(DXVK_USE_VCL) && defined(DXVK_ARCH_X86)
  // VCL-optimized Matrix4 implementation
  // Uses 4 SIMD vectors for maximum performance
  
  class alignas(16) Matrix4 {
  public:
    // Store as 4 SIMD vectors for optimal performance
    union {
      __m128 rows[4];     // Direct SIMD storage
      Vector4 data[4];    // Vector4 access
      float m[4][4];      // Element access
    };
    
    // Constructors
    inline Matrix4() {
      // Identity matrix
      rows[0] = Vec4f(1.0f, 0.0f, 0.0f, 0.0f);
      rows[1] = Vec4f(0.0f, 1.0f, 0.0f, 0.0f);
      rows[2] = Vec4f(0.0f, 0.0f, 1.0f, 0.0f);
      rows[3] = Vec4f(0.0f, 0.0f, 0.0f, 1.0f);
    }
    
    inline explicit Matrix4(float x) {
      // Scalar matrix
      rows[0] = Vec4f(x, 0.0f, 0.0f, 0.0f);
      rows[1] = Vec4f(0.0f, x, 0.0f, 0.0f);
      rows[2] = Vec4f(0.0f, 0.0f, x, 0.0f);
      rows[3] = Vec4f(0.0f, 0.0f, 0.0f, x);
    }
    
    inline Matrix4(
      const Vector4& v0,
      const Vector4& v1,
      const Vector4& v2,
      const Vector4& v3) {
      rows[0] = v0.xmm;
      rows[1] = v1.xmm;
      rows[2] = v2.xmm;
      rows[3] = v3.xmm;
    }
    
    inline Matrix4(const float matrix[4][4]) {
      rows[0] = Vec4f().load(matrix[0]);
      rows[1] = Vec4f().load(matrix[1]);
      rows[2] = Vec4f().load(matrix[2]);
      rows[3] = Vec4f().load(matrix[3]);
    }
    
    Matrix4(const Matrix4& other) = default;
    Matrix4& operator=(const Matrix4& other) = default;
    
    // Element/row access
    Vector4& operator[](size_t index);
    const Vector4& operator[](size_t index) const;
    
    // Comparison
    bool operator==(const Matrix4& m2) const;
    bool operator!=(const Matrix4& m2) const;
    
    // Arithmetic operators
    Matrix4 operator+(const Matrix4& other) const;
    Matrix4 operator-(const Matrix4& other) const;
    Matrix4 operator*(const Matrix4& m2) const;
    Vector4 operator*(const Vector4& v) const;
    Matrix4 operator*(float scalar) const;
    Matrix4 operator/(float scalar) const;
    
    // In-place operators
    Matrix4& operator+=(const Matrix4& other);
    Matrix4& operator-=(const Matrix4& other);
    Matrix4& operator*=(const Matrix4& other);
  };
  
#else
  // Original scalar Matrix4 implementation
  
  class Matrix4 {
  public:
    // Identity
    inline Matrix4() {
      data[0] = { 1, 0, 0, 0 };
      data[1] = { 0, 1, 0, 0 };
      data[2] = { 0, 0, 1, 0 };
      data[3] = { 0, 0, 0, 1 };
    }

    // Produces a scalar matrix, x * Identity
    inline explicit Matrix4(float x) {
      data[0] = { x, 0, 0, 0 };
      data[1] = { 0, x, 0, 0 };
      data[2] = { 0, 0, x, 0 };
      data[3] = { 0, 0, 0, x };
    }

    inline Matrix4(
      const Vector4& v0,
      const Vector4& v1,
      const Vector4& v2,
      const Vector4& v3) {
      data[0] = v0;
      data[1] = v1;
      data[2] = v2;
      data[3] = v3;
    }

    inline Matrix4(const float matrix[4][4]) {
      data[0] = Vector4(matrix[0]);
      data[1] = Vector4(matrix[1]);
      data[2] = Vector4(matrix[2]);
      data[3] = Vector4(matrix[3]);
    }

    Matrix4(const Matrix4& other) = default;
    Matrix4& operator=(const Matrix4& other) = default;

    Vector4& operator[](size_t index);
    const Vector4& operator[](size_t index) const;

    bool operator==(const Matrix4& m2) const;
    bool operator!=(const Matrix4& m2) const;

    Matrix4 operator+(const Matrix4& other) const;
    Matrix4 operator-(const Matrix4& other) const;

    Matrix4 operator*(const Matrix4& m2) const;
    Vector4 operator*(const Vector4& v) const;
    Matrix4 operator*(float scalar) const;

    Matrix4 operator/(float scalar) const;

    Matrix4& operator+=(const Matrix4& other);
    Matrix4& operator-=(const Matrix4& other);

    Matrix4& operator*=(const Matrix4& other);

    Vector4 data[4];
  };
#endif

  static_assert(sizeof(Matrix4) == sizeof(Vector4) * 4);

  inline Matrix4 operator*(float scalar, const Matrix4& m) { return m * scalar; }

  Matrix4 transpose(const Matrix4& m);
  float determinant(const Matrix4& m);
  Matrix4 inverse(const Matrix4& m);
  Matrix4 hadamardProduct(const Matrix4& a, const Matrix4& b);
  std::ostream& operator<<(std::ostream& os, const Matrix4& m);

}