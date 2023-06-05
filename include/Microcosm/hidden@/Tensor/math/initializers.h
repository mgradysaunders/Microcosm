#pragma once

namespace mi {

template <typename Value, size_t N> requires(N != Dynamic) //
struct Tensor_initializers<Value, TensorShape<N>> {
  [[nodiscard]] static constexpr auto unitVector(size_t i) noexcept {
    Tensor<Value, TensorShape<N>> unit;
    unit[i] = Value(1);
    return unit;
  }

  [[nodiscard]] static constexpr auto unitX() noexcept requires(N >= 1) { return unitVector(0); }

  [[nodiscard]] static constexpr auto unitY() noexcept requires(N >= 2) { return unitVector(1); }

  [[nodiscard]] static constexpr auto unitZ() noexcept requires(N >= 3) { return unitVector(2); }

  [[nodiscard]] static constexpr auto unitW() noexcept requires(N >= 4) { return unitVector(3); }
};

template <std::floating_point Float> //
struct Tensor_initializers<Float, TensorShape<2>> {
  [[nodiscard]] static Vector2<Float> polar(Float r, Float phi) noexcept { return {r * cos(phi), r * sin(phi)}; }
};

template <std::floating_point Float> //
struct Tensor_initializers<Float, TensorShape<2, 2>> {
  [[nodiscard]] static constexpr Matrix2<Float> identity() noexcept {
    return {
      {Float(1), Float(0)}, //
      {Float(0), Float(1)}};
  }
  [[nodiscard]] static Matrix2<Float> rotate(Float theta) noexcept {
    auto cosTheta = cos(theta);
    auto sinTheta = sin(theta);
    return {{+cosTheta, -sinTheta}, {+sinTheta, +cosTheta}};
  }
};

template <std::floating_point Float> struct Tensor_initializers<Float, TensorShape<3, 3>> {
  [[nodiscard]] static constexpr Matrix3<Float> identity() noexcept {
    return {
      {Float(1), Float(0), Float(0)}, //
      {Float(0), Float(1), Float(0)},
      {Float(0), Float(0), Float(1)}};
  }

  /// Build an orthonormal basis with the input vector as the Z-axis.
  [[nodiscard]] static Matrix3<Float> orthonormalBasis(Vector3<Float> hatZ) noexcept {
    Vector3<Float> hatX = {};
    Vector3<Float> hatY = {};
    if (hatZ[2] < Float(-0.9999999)) {
      hatZ[2] = -1;
      hatX[1] = -1;
      hatY[0] = -1;
    } else {
      Float alpha0 = -1 / (hatZ[2] + 1);
      Float alpha1 = alpha0 * hatZ[0] * hatZ[1];
      Float alpha2 = alpha0 * hatZ[0] * hatZ[0] + 1;
      Float alpha3 = alpha0 * hatZ[1] * hatZ[1] + 1;
      hatX = {alpha2, alpha1, -hatZ[0]};
      hatY = {alpha1, alpha3, -hatZ[1]};
      if (hatZ[2] < Float(-0.999)) {
        // Accuracy begins to suffer, so manually orthonormalize.
        hatX = normalize(hatX - dot(hatX, hatZ) * hatZ);
        hatY = normalize(hatY - dot(hatY, hatX) * hatX - dot(hatY, hatZ) * hatZ);
      } else {
        hatX /= sqrt(lengthSquare(hatX));
        hatY /= sqrt(lengthSquare(hatY));
      }
    }
    return {
      {hatX[0], hatY[0], hatZ[0]}, //
      {hatX[1], hatY[1], hatZ[1]},
      {hatX[2], hatY[2], hatZ[2]}};
  }

  /// Build an orthonormal basis with the input vector as the Z-axis.
  ///
  /// \note
  /// If it is no big deal for the basis to change discontinously, then this
  /// is a more robust alternative to `orthonormalBasis()`.
  ///
  [[nodiscard]] static Matrix3<Float> orthonormalBasisDiscontinuous(Vector3<Float> hatZ) noexcept {
    size_t kZ = argmin(abs(hatZ));
    size_t kX = (kZ + 1) % 3;
    size_t kY = (kZ + 2) % 3;
    Matrix3<Float> matrixK = orthonormalBasis({hatZ[kX], hatZ[kY], hatZ[kZ]});
    Matrix3<Float> matrixZ;
    matrixZ.row(kX).assign(matrixK.row(0));
    matrixZ.row(kY).assign(matrixK.row(1));
    matrixZ.row(kZ).assign(matrixK.row(2));
    return matrixZ;
  }

  /// Rotate counter-clockwise around arbitrary axis.
  [[nodiscard]] static Matrix3<Float> rotate(Float theta, Vector3<Float> hatV) noexcept {
    auto cosTheta = cos(theta);
    auto sinTheta = sin(theta);
    Float vX = hatV[0];
    Float vY = hatV[1];
    Float vZ = hatV[2];
    Float vXvX = vX * vX, vXvY = vX * vY, vXvZ = vX * vZ;
    Float vYvY = vY * vY, vYvZ = vY * vZ;
    Float vZvZ = vZ * vZ;
    return {
      {vXvX * (1 - cosTheta) + cosTheta, vXvY * (1 - cosTheta) - vZ * sinTheta, vXvZ * (1 - cosTheta) + vY * sinTheta},
      {vXvY * (1 - cosTheta) + vZ * sinTheta, vYvY * (1 - cosTheta) + cosTheta, vYvZ * (1 - cosTheta) - vX * sinTheta},
      {vXvZ * (1 - cosTheta) - vY * sinTheta, vYvZ * (1 - cosTheta) + vX * sinTheta, vZvZ * (1 - cosTheta) + cosTheta}};
  }

  /// Rotate counter-clockwise around X-axis.
  [[nodiscard]] static Matrix3<Float> rotateX(Float theta) noexcept { return rotate(theta, Vector3<Float>{1, 0, 0}); }

  /// Rotate counter-clockwise around Y-axis.
  [[nodiscard]] static Matrix3<Float> rotateY(Float theta) noexcept { return rotate(theta, Vector3<Float>{0, 1, 0}); }

  /// Rotate counter-clockwise around Z-axis.
  [[nodiscard]] static Matrix3<Float> rotateZ(Float theta) noexcept { return rotate(theta, Vector3<Float>{0, 0, 1}); }
};

template <std::floating_point Float> //
struct Tensor_initializers<Float, TensorShape<4, 4>> {
  [[nodiscard]] static constexpr Matrix4<Float> identity() noexcept {
    return {
      {Float(1), Float(0), Float(0), Float(0)}, //
      {Float(0), Float(1), Float(0), Float(0)},
      {Float(0), Float(0), Float(1), Float(0)},
      {Float(0), Float(0), Float(0), Float(1)}};
  }

  [[nodiscard]] static Matrix4<Float> translate(const Vector3<Float> &vectorV) noexcept {
    return {
      {Float(1), Float(0), Float(0), vectorV[0]},
      {Float(0), Float(1), Float(0), vectorV[1]},
      {Float(0), Float(0), Float(1), vectorV[2]},
      {Float(0), Float(0), Float(0), Float(1)}};
  }

  /// An OpenGL-flavor look-at matrix.
  ///
  /// \param[in] source  The source location.
  /// \param[in] target  The target location.
  /// \param[in] up      The up vector.
  ///
  /// The returned matrix is a local-to-world coordinate system looking down
  /// the negative Z-axis, such that the Z-axis column of the matrix is aligned to
  /// the vector from the target location to the source location. The Y-axis is in
  /// the plane spanned by the Z-axis and the given up vector.
  ///
  [[nodiscard]] static Matrix4<Float>
  lookAt(const Vector3<Float> &source, const Vector3<Float> &target, const Vector3<Float> &up) noexcept {
    Vector3<Float> vectorZ = source - target;
    Vector3<Float> vectorX = cross(up, vectorZ);
    Vector3<Float> hatZ = normalize(vectorZ);
    Vector3<Float> hatX = normalize(vectorX);
    Vector3<Float> hatY = cross(hatZ, hatX);
    return {
      {hatX[0], hatY[0], hatZ[0], source[0]},
      {hatX[1], hatY[1], hatZ[1], source[1]},
      {hatX[2], hatY[2], hatZ[2], source[2]},
      {0, 0, 0, 1}};
  }
};

} // namespace mi
