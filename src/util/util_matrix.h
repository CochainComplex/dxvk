#pragma once

#include "util_vector.h"

namespace dxvk {

  class alignas(16) Matrix4 {
  public:
    // Storage - 4 vectors representing rows
    Vector4 data[4];
    
    // Constructors
    inline Matrix4() {
      // Identity matrix
      data[0] = Vector4(1.0f, 0.0f, 0.0f, 0.0f);
      data[1] = Vector4(0.0f, 1.0f, 0.0f, 0.0f);
      data[2] = Vector4(0.0f, 0.0f, 1.0f, 0.0f);
      data[3] = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
    }
    
    inline explicit Matrix4(float x) {
      // Scalar matrix
      data[0] = Vector4(x, 0.0f, 0.0f, 0.0f);
      data[1] = Vector4(0.0f, x, 0.0f, 0.0f);
      data[2] = Vector4(0.0f, 0.0f, x, 0.0f);
      data[3] = Vector4(0.0f, 0.0f, 0.0f, x);
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
      data[0] = Vector4(matrix[0][0], matrix[0][1], matrix[0][2], matrix[0][3]);
      data[1] = Vector4(matrix[1][0], matrix[1][1], matrix[1][2], matrix[1][3]);
      data[2] = Vector4(matrix[2][0], matrix[2][1], matrix[2][2], matrix[2][3]);
      data[3] = Vector4(matrix[3][0], matrix[3][1], matrix[3][2], matrix[3][3]);
    }
    
    // Element access
    inline       Vector4& operator[](uint32_t i)       { return data[i]; }
    inline const Vector4& operator[](uint32_t i) const { return data[i]; }
    
    // Arithmetic operators
    Matrix4& operator+=(const Matrix4& m);
    Matrix4& operator-=(const Matrix4& m);
    Matrix4& operator*=(float s);
    
    bool operator==(const Matrix4& m) const;
    bool operator!=(const Matrix4& m) const;
    
    Matrix4 operator+(const Matrix4& m) const;
    Matrix4 operator-(const Matrix4& m) const;
    Matrix4 operator*(const Matrix4& m) const;
    Matrix4 operator*(float s) const;
    
    Vector4 operator*(const Vector4& v) const;
  };
  
  // Essential matrix operations used by DXVK
  Matrix4 transpose(const Matrix4& m);
  Matrix4 inverse(const Matrix4& m);
  float determinant(const Matrix4& m);
  
  // Output stream
  std::ostream& operator<<(std::ostream& os, const Matrix4& m);

}