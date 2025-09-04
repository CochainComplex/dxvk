#include "util_matrix.h"
#include <cmath>

namespace dxvk {

  // Arithmetic operators implementation
  Matrix4& Matrix4::operator+=(const Matrix4& m) {
    data[0] += m.data[0];
    data[1] += m.data[1];
    data[2] += m.data[2];
    data[3] += m.data[3];
    return *this;
  }
  
  Matrix4& Matrix4::operator-=(const Matrix4& m) {
    data[0] -= m.data[0];
    data[1] -= m.data[1];
    data[2] -= m.data[2];
    data[3] -= m.data[3];
    return *this;
  }
  
  Matrix4& Matrix4::operator*=(float s) {
    data[0] *= s;
    data[1] *= s;
    data[2] *= s;
    data[3] *= s;
    return *this;
  }
  
  bool Matrix4::operator==(const Matrix4& m) const {
    return data[0] == m.data[0] && 
           data[1] == m.data[1] && 
           data[2] == m.data[2] && 
           data[3] == m.data[3];
  }
  
  bool Matrix4::operator!=(const Matrix4& m) const {
    return !operator==(m);
  }
  
  Matrix4 Matrix4::operator+(const Matrix4& m) const {
    Matrix4 result = *this;
    result += m;
    return result;
  }
  
  Matrix4 Matrix4::operator-(const Matrix4& m) const {
    Matrix4 result = *this;
    result -= m;
    return result;
  }
  
  Matrix4 Matrix4::operator*(float s) const {
    Matrix4 result = *this;
    result *= s;
    return result;
  }
  
  // Matrix multiplication - essential for transforms
  Matrix4 Matrix4::operator*(const Matrix4& m) const {
#if defined(DXVK_USE_VCL) && defined(DXVK_ARCH_X86)
    // VCL-optimized matrix multiplication
    Matrix4 result;
    for (int i = 0; i < 4; i++) {
      Vec4f row = Vec4f(data[i].xmm);
      Vec4f c0 = Vec4f(m.data[0].xmm) * permute4<0,0,0,0>(row);
      Vec4f c1 = Vec4f(m.data[1].xmm) * permute4<1,1,1,1>(row);
      Vec4f c2 = Vec4f(m.data[2].xmm) * permute4<2,2,2,2>(row);
      Vec4f c3 = Vec4f(m.data[3].xmm) * permute4<3,3,3,3>(row);
      result.data[i] = Vector4(c0 + c1 + c2 + c3);
    }
    return result;
#else
    // Scalar matrix multiplication
    Matrix4 result;
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
        result.data[i][j] = 
          data[i][0] * m.data[0][j] +
          data[i][1] * m.data[1][j] +
          data[i][2] * m.data[2][j] +
          data[i][3] * m.data[3][j];
      }
    }
    return result;
#endif
  }
  
  // Matrix-vector multiplication - essential for position transforms
  Vector4 Matrix4::operator*(const Vector4& v) const {
#if defined(DXVK_USE_VCL) && defined(DXVK_ARCH_X86)
    // VCL-optimized matrix-vector multiplication
    Vec4f vec(v.xmm);
    Vec4f c0 = Vec4f(data[0].xmm) * permute4<0,0,0,0>(vec);
    Vec4f c1 = Vec4f(data[1].xmm) * permute4<1,1,1,1>(vec);
    Vec4f c2 = Vec4f(data[2].xmm) * permute4<2,2,2,2>(vec);
    Vec4f c3 = Vec4f(data[3].xmm) * permute4<3,3,3,3>(vec);
    
    Vec4f t01 = c0 + c1;
    Vec4f t23 = c2 + c3;
    return Vector4(t01 + t23);
#else
    // Scalar matrix-vector multiplication
    return Vector4(
      dot(data[0], v),
      dot(data[1], v),
      dot(data[2], v),
      dot(data[3], v)
    );
#endif
  }
  
  // Transpose - essential for normal matrix calculation
  Matrix4 transpose(const Matrix4& m) {
#if defined(DXVK_USE_VCL) && defined(DXVK_ARCH_X86)
    // VCL-optimized transpose using blend operations
    Vec4f r0 = Vec4f(m.data[0].xmm);
    Vec4f r1 = Vec4f(m.data[1].xmm);
    Vec4f r2 = Vec4f(m.data[2].xmm);
    Vec4f r3 = Vec4f(m.data[3].xmm);
    
    Vec4f t0 = blend4<0,4,2,6>(r0, r1);
    Vec4f t1 = blend4<1,5,3,7>(r0, r1);
    Vec4f t2 = blend4<0,4,2,6>(r2, r3);
    Vec4f t3 = blend4<1,5,3,7>(r2, r3);
    
    Matrix4 result;
    result.data[0] = Vector4(blend4<0,1,4,5>(t0, t2));
    result.data[1] = Vector4(blend4<0,1,4,5>(t1, t3));
    result.data[2] = Vector4(blend4<2,3,6,7>(t0, t2));
    result.data[3] = Vector4(blend4<2,3,6,7>(t1, t3));
    return result;
#else
    // Scalar transpose
    Matrix4 result;
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
        result.data[i][j] = m.data[j][i];
      }
    }
    return result;
#endif
  }
  
  // Determinant - used by inverse
  float determinant(const Matrix4& m) {
    // Calculate 2x2 determinants
    float s0 = m.data[0][0] * m.data[1][1] - m.data[1][0] * m.data[0][1];
    float s1 = m.data[0][0] * m.data[1][2] - m.data[1][0] * m.data[0][2];
    float s2 = m.data[0][0] * m.data[1][3] - m.data[1][0] * m.data[0][3];
    float s3 = m.data[0][1] * m.data[1][2] - m.data[1][1] * m.data[0][2];
    float s4 = m.data[0][1] * m.data[1][3] - m.data[1][1] * m.data[0][3];
    float s5 = m.data[0][2] * m.data[1][3] - m.data[1][2] * m.data[0][3];

    // Calculate 3x3 cofactors
    float c5 = m.data[2][2] * m.data[3][3] - m.data[3][2] * m.data[2][3];
    float c4 = m.data[2][1] * m.data[3][3] - m.data[3][1] * m.data[2][3];
    float c3 = m.data[2][1] * m.data[3][2] - m.data[3][1] * m.data[2][2];
    float c2 = m.data[2][0] * m.data[3][3] - m.data[3][0] * m.data[2][3];
    float c1 = m.data[2][0] * m.data[3][2] - m.data[3][0] * m.data[2][2];
    float c0 = m.data[2][0] * m.data[3][1] - m.data[3][0] * m.data[2][1];

    // Calculate determinant
    return s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0;
  }
  
  // Inverse - essential for normal matrix calculation
  Matrix4 inverse(const Matrix4& m) {
    // Calculate 2x2 determinants for cofactor matrix
    float s0 = m.data[0][0] * m.data[1][1] - m.data[1][0] * m.data[0][1];
    float s1 = m.data[0][0] * m.data[1][2] - m.data[1][0] * m.data[0][2];
    float s2 = m.data[0][0] * m.data[1][3] - m.data[1][0] * m.data[0][3];
    float s3 = m.data[0][1] * m.data[1][2] - m.data[1][1] * m.data[0][2];
    float s4 = m.data[0][1] * m.data[1][3] - m.data[1][1] * m.data[0][3];
    float s5 = m.data[0][2] * m.data[1][3] - m.data[1][2] * m.data[0][3];

    float c5 = m.data[2][2] * m.data[3][3] - m.data[3][2] * m.data[2][3];
    float c4 = m.data[2][1] * m.data[3][3] - m.data[3][1] * m.data[2][3];
    float c3 = m.data[2][1] * m.data[3][2] - m.data[3][1] * m.data[2][2];
    float c2 = m.data[2][0] * m.data[3][3] - m.data[3][0] * m.data[2][3];
    float c1 = m.data[2][0] * m.data[3][2] - m.data[3][0] * m.data[2][2];
    float c0 = m.data[2][0] * m.data[3][1] - m.data[3][0] * m.data[2][1];

    // Calculate determinant
    float det = s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0;
    
    if (std::abs(det) < 1e-10f) {
      // Matrix is singular, return identity
      return Matrix4();
    }

    float invdet = 1.0f / det;

    // Calculate adjugate matrix and multiply by 1/det
    Matrix4 result;
    
    result.data[0][0] = ( m.data[1][1] * c5 - m.data[1][2] * c4 + m.data[1][3] * c3) * invdet;
    result.data[0][1] = (-m.data[0][1] * c5 + m.data[0][2] * c4 - m.data[0][3] * c3) * invdet;
    result.data[0][2] = ( m.data[3][1] * s5 - m.data[3][2] * s4 + m.data[3][3] * s3) * invdet;
    result.data[0][3] = (-m.data[2][1] * s5 + m.data[2][2] * s4 - m.data[2][3] * s3) * invdet;

    result.data[1][0] = (-m.data[1][0] * c5 + m.data[1][2] * c2 - m.data[1][3] * c1) * invdet;
    result.data[1][1] = ( m.data[0][0] * c5 - m.data[0][2] * c2 + m.data[0][3] * c1) * invdet;
    result.data[1][2] = (-m.data[3][0] * s5 + m.data[3][2] * s2 - m.data[3][3] * s1) * invdet;
    result.data[1][3] = ( m.data[2][0] * s5 - m.data[2][2] * s2 + m.data[2][3] * s1) * invdet;

    result.data[2][0] = ( m.data[1][0] * c4 - m.data[1][1] * c2 + m.data[1][3] * c0) * invdet;
    result.data[2][1] = (-m.data[0][0] * c4 + m.data[0][1] * c2 - m.data[0][3] * c0) * invdet;
    result.data[2][2] = ( m.data[3][0] * s4 - m.data[3][1] * s2 + m.data[3][3] * s0) * invdet;
    result.data[2][3] = (-m.data[2][0] * s4 + m.data[2][1] * s2 - m.data[2][3] * s0) * invdet;

    result.data[3][0] = (-m.data[1][0] * c3 + m.data[1][1] * c1 - m.data[1][2] * c0) * invdet;
    result.data[3][1] = ( m.data[0][0] * c3 - m.data[0][1] * c1 + m.data[0][2] * c0) * invdet;
    result.data[3][2] = (-m.data[3][0] * s3 + m.data[3][1] * s1 - m.data[3][2] * s0) * invdet;
    result.data[3][3] = ( m.data[2][0] * s3 - m.data[2][1] * s1 + m.data[2][2] * s0) * invdet;

    return result;
  }
  
  // Output stream
  std::ostream& operator<<(std::ostream& os, const Matrix4& m) {
    os << "Matrix4(\n";
    for (int i = 0; i < 4; i++) {
      os << "  " << m.data[i] << "\n";
    }
    os << ")";
    return os;
  }

}