#pragma once

#include "./LengthHelper.h"

namespace mi {

template <typename Value, concepts::tensor_shape Shape, bool EnableU = true, bool EnableV = true> struct OrthoHelper {
public:
  static_assert(Shape::Rank == 2);

  using Float = to_float_t<Value>;

  using Field = to_field_t<Value>;

  static constexpr auto Rows = Shape::template Size<0>;

  static constexpr auto Cols = Shape::template Size<1>;

  static constexpr auto WorkSize = Shape::DynamicRank != 0 ? Dynamic : max(Rows, Cols);

  using TensorU = Tensor<Field, TensorShape<Rows, Rows>>;

  using TensorV = Tensor<Field, TensorShape<Cols, Cols>>;

  static constexpr Float DefaultThresh = constants::Eps<Float> * 32;

  template <typename Expr>
  OrthoHelper(Expr &&expr) : mCoeffsX(std::forward<Expr>(expr)), mRows{mCoeffsX.rows()}, mCols{mCoeffsX.cols()} {
    if constexpr (WorkSize == Dynamic) mCoeffsW.resize(max(mRows, mCols));
    if constexpr (EnableU && Rows == Dynamic) mCoeffsU.resize(mRows, mRows);
    if constexpr (EnableV && Cols == Dynamic) mCoeffsV.resize(mCols, mCols);
    if constexpr (EnableU) diag(mCoeffsU) = Field(1);
    if constexpr (EnableV) diag(mCoeffsV) = Field(1);
  }

public:
  [[nodiscard]] auto rows() const noexcept { return mRows; }

  [[nodiscard]] auto cols() const noexcept { return mCols; }

  [[nodiscard]] auto matrixX() const { return capture_in_tensor_lambda(mCoeffsX); }

  [[nodiscard]] auto matrixU() const requires(EnableU) { return adjoint(mCoeffsU); }

  [[nodiscard]] auto matrixV() const requires(EnableV) { return adjoint(mCoeffsV); }

public:
  /// Upper triangularize.
  ///
  /// Upper triangularize with Householder reflections applied on the left,
  /// thus only affecting the left matrix. This is the essentially the entire
  /// implementation of QR decomposition.
  ///
  [[gnu::flatten]] void upperTriangularize() {
    for (size_t k = 0; k < min(mRows, mCols); k++) reflectHouseholder<'U'>(k, k);
  }

  /// Lower triangularize.
  ///
  /// Lower triangularize with Householder reflections applied on the right,
  /// thus only affecting the right matrix. This is the essentially the entire
  /// implementation of LQ decomposition.
  ///
  [[gnu::flatten]] void lowerTriangularize() {
    for (size_t k = 0; k < min(mRows, mCols); k++) reflectHouseholder<'V'>(k, k);
  }

  /// Upper bidiagonalize.
  ///
  /// Upper bidiagonalize with Householder reflections applied in alternating
  /// steps on both sides. This is the initial step in the singular value
  /// decomposition of a matrix with more rows than columns.
  ///
  [[gnu::flatten]] void upperBidiagonalize() {
    // For lack of a better way to do this right now, specialize manually for
    // small matrices using the compile-time variant of the Householder reflection
    // routine. This leads to nearly 2x performance improvement.
    if constexpr (Rows != Dynamic && Cols != Dynamic && min(Rows, Cols) <= 3) {
      reflectHouseholderCompileTime<'U', 0, 0>(), reflectHouseholderCompileTime<'V', 0, 1>();
      reflectHouseholderCompileTime<'U', 1, 1>(), reflectHouseholderCompileTime<'V', 1, 2>();
      reflectHouseholderCompileTime<'U', 2, 2>(), reflectHouseholderCompileTime<'V', 2, 3>();
    } else {
      for (size_t k = 0; k < min(mRows, mCols); k++) {
        reflectHouseholder<'U'>(k, k);
        reflectHouseholder<'V'>(k, k + 1);
      }
    }
  }

  /// Lower bidiagonalize.
  ///
  /// Lower bidiagonalize with Householder reflections applied in alternating
  /// steps on both sides. This is the initial step in the singular value
  /// decomposition of a matrix with more columns than rows.
  ///
  [[gnu::flatten]] void lowerBidiagonalize() {
    for (size_t k = 0; k < min(mRows, mCols); k++) {
      reflectHouseholder<'V'>(k, k);
      reflectHouseholder<'U'>(k + 1, k);
    }
  }

  /// Tridiagonalize.
  ///
  /// Tridiagonalize with Householder reflections applied in alternating
  /// steps on both sides.
  ///
  [[gnu::flatten]] void tridiagonalize() {
    for (size_t k = 0; k < min(mRows, mCols); k++) {
      reflectHouseholder<'V'>(k, k + 1);
      reflectHouseholder<'U'>(k + 1, k);
    }
  }

  /// Diagonalize.
  ///
  /// Diagonalize by first upper or lower bidiagonalizing with Householder
  /// reflections, then iteratively diagonalize with Givens rotations until
  /// convergence.
  ///
  /// \throw std::runtime_error
  /// If the algorithm fails to converge.
  ///
  template <Float Thresh = DefaultThresh> [[gnu::flatten]] void diagonalize() {
    if (mRows >= mCols) {
      // Gate with a compile-time if to prevent unnecessary code-generation.
      if constexpr (Rows == Dynamic || Cols == Dynamic || Rows >= Cols) {
        upperBidiagonalize();
        diagonalizeWithGivensRotations<Thresh, 'U', 'V'>(mCoeffsX);
      }
    } else {
      // Gate with a compile-time if to prevent unnecessary code-generation.
      if constexpr (Rows == Dynamic || Cols == Dynamic || Rows < Cols) {
        lowerBidiagonalize();
        diagonalizeWithGivensRotations<Thresh, 'V', 'U'>(transpose(mCoeffsX));
      }
    }
    // Force diagonal and positive.
    for (size_t i = 0; i < mRows; i++) {
      for (size_t j = 0; j < mCols; j++) {
        if (i != j)
          mCoeffsX(i, j) = Field(0);
        else {
          if constexpr (EnableU)
            if (mCoeffsX(i, i) != Float(0)) mCoeffsU[i] *= sign(mCoeffsX(i, i));
          mCoeffsX(i, i) = abs(mCoeffsX(i, i));
        }
      }
    }
  }

public:
  /// The cofficients of the input matrix, modified in place.
  Tensor<Field, Shape> mCoeffsX;

  /// The number of rows, also the square dimension of the left matrix.
  const size_t mRows{0};

  /// The number of columns, also the square dimension of the right matrix.
  const size_t mCols{0};

  /// If enabled, the left matrix coefficients.
  ///
  /// \note
  /// While these coefficients determine the left matrix, they are not exactly
  /// equal to the left matrix due to the way transforms are accumulated. In
  /// particular, this is the adjoint (conjugate transpose) of the left
  /// matrix.
  ///
  conditional_member_t<EnableU, TensorU> mCoeffsU;

  /// If enabled, the right matrix coefficients.
  ///
  /// \note
  /// While these coefficients determine the right matrix, they are not exactly
  /// equal to the right matrix due to the way transforms are accumulated. In
  /// particular, this is the adjoint (conjugate transpose) of the right
  /// matrix.
  ///
  conditional_member_t<EnableV, TensorV> mCoeffsV;

  /// The coefficients used for Householder reflections.
  Tensor<Field, TensorShape<WorkSize>> mCoeffsW;

  /// The length helper.
  LengthHelper<Field, TensorShape<WorkSize>> mLengthHelper;

private:
  /// Construct and apply Householder reflection targeting the given indexes.
  template <char Side> [[strong_inline]] void reflectHouseholder(size_t targetI, size_t targetJ) {
    if (
      (Side == 'U' && targetI + 1 < mRows && targetJ < mCols) || //
      (Side == 'V' && targetI < mRows && targetJ + 1 < mCols)) {
      auto matrixY = transposeIf<Side == 'V'>(mCoeffsX(SliceToEnd(targetI), SliceToEnd(targetJ)));
      auto vectorW = mCoeffsW(Slice(0, matrixY.rows()));
      vectorW.assign(matrixY.col(0));
      matrixY.col(0).assign(Float(0));
      matrixY(0, 0) = -mLengthHelper.length(vectorW) * sign(vectorW[0]);
      vectorW(0) -= matrixY(0, 0);
      mLengthHelper.normalizeInPlace(vectorW);
      auto applyTransform = [&](auto &&matrixZ) {
        for (size_t j = 0; j < matrixZ.cols(); j++) matrixZ.col(j) -= Float(2) * dot(conj(vectorW), matrixZ.col(j)) * vectorW;
      };
      applyTransform(matrixY.col(Slice<1, ToEnd>()));
      if constexpr (EnableU && Side == 'U') applyTransform(mCoeffsU(SliceToEnd(targetI), Slice()));
      if constexpr (EnableV && Side == 'V') applyTransform(transpose(mCoeffsV(Slice(), SliceToEnd(targetJ))));
    }
  }

  /// Construct and apply Householder reflection targeting the given indexes at compile time.
  ///
  /// \note
  /// This is used for better code generation in small matrix cases.
  template <char Side, size_t TargetI, size_t TargetJ> [[strong_inline]] void reflectHouseholderCompileTime() {
    if constexpr (
      (Side == 'U' && TargetI + 1 < Rows && TargetJ < Cols) || //
      (Side == 'V' && TargetI < Rows && TargetJ + 1 < Cols)) {
      auto matrixY = transposeIf<Side == 'V'>(mCoeffsX(Slice<TargetI, ToEnd>(), Slice<TargetJ, ToEnd>()));
      auto vectorW = mCoeffsW(Slice<0, (Side == 'V' ? Cols - TargetJ : Rows - TargetI)>());
      vectorW.assign(matrixY.col(0));
      matrixY.col(0).assign(Float(0));
      matrixY(0, 0) = -sqrt(norm(vectorW).sum()) * sign(vectorW[0]);
      vectorW(0) -= matrixY(0, 0);
      mLengthHelper.normalizeInPlace(vectorW);
      auto applyTransform = [&](auto &&matrixZ) {
        for (size_t j = 0; j < matrixZ.cols(); j++) matrixZ.col(j) -= Float(2) * dot(conj(vectorW), matrixZ.col(j)) * vectorW;
      };
      applyTransform(matrixY.col(Slice<1, ToEnd>()));
      if constexpr (EnableU && Side == 'U') applyTransform(mCoeffsU(Slice<TargetI, ToEnd>(), Slice()));
      if constexpr (EnableV && Side == 'V') applyTransform(transpose(mCoeffsV(Slice(), Slice<TargetJ, ToEnd>())));
    }
  }

  /// Construct and apply Givens rotation targeting the given indexes.
  template <char Side> [[strong_inline]] void rotateGivens(size_t targetK0, size_t targetK1, Field coeffF, Field coeffG) {
    if (Side == 'U' && (targetK0 >= mRows || targetK1 >= mRows)) return;
    if (Side == 'V' && (targetK0 >= mCols || targetK1 >= mCols)) return;
    Float cosBeta = Float(1);
    Field sinBeta = Field(0);
    if (coeffG != Float(0)) [[likely]] {
      if (coeffF != Float(0)) [[likely]] {
        Float absF = abs(coeffF);
        Float absG = abs(coeffG);
        cosBeta = absF;
        sinBeta = (coeffF / absF) * conj(coeffG);
        Float denom = 1 / sqrt(absF * absF + absG * absG);
        cosBeta *= denom;
        sinBeta *= denom;
      } else {
        cosBeta = Float(0);
        sinBeta = sign(coeffG);
      }
    }
    auto applyTransform = [&](auto &&matrixZ) {
      for (size_t j = 0; j < matrixZ.cols(); j++) {
        Field coeff0 = matrixZ(targetK0, j);
        Field coeff1 = matrixZ(targetK1, j);
        matrixZ(targetK0, j) = coeff0 * cosBeta + coeff1 * sinBeta;
        matrixZ(targetK1, j) = coeff1 * cosBeta - coeff0 * conj(sinBeta);
      }
    };
    applyTransform(transposeIf<Side == 'V'>(mCoeffsX));
    if constexpr (Side == 'U' && EnableU) applyTransform(mCoeffsU);
    if constexpr (Side == 'V' && EnableV) applyTransform(transpose(mCoeffsV));
  }

  /// Diagonalize with alternating Givens rotations. This is the core of the SVD algorithm.
  template <Float Thresh, char SideU, char SideV> [[strong_inline]] void diagonalizeWithGivensRotations(auto &&matrixY) {
    static_assert((SideU == 'U' && SideV == 'V') || (SideU == 'V' && SideV == 'U'));
    size_t n = matrixY.cols();
    // Divide out largest value on the non-zero bidiagonal for basic preconditioning.
    Float factor = 0;
    for (size_t k = 0; k + 1 < n; k++) {
      factor = max(factor, abs(matrixY(k, k)));
      factor = max(factor, abs(matrixY(k, k + 1)));
    }
    factor = max(factor, abs(matrixY(n - 1, n - 1)));
    if (factor > 16 * constants::MinInv<Float>) [[likely]] {
      Float factorInv = 1 / factor;
      for (size_t k = 0; k + 1 < n; k++) matrixY(k, k) *= factorInv, matrixY(k, k + 1) *= factorInv;
      matrixY(n - 1, n - 1) *= factorInv;
    } else if (factor > 0) {
      for (size_t k = 0; k + 1 < n; k++) matrixY(k, k) /= factor, matrixY(k, k + 1) /= factor;
      matrixY(n - 1, n - 1) /= factor;
    }
    // Diagonalize.
    for (size_t numIters = 0; true; ++numIters) {
      size_t s = 0, t = 1;
      while (s + 1 < n && norm(matrixY(s, s + 1)) < sqr(Thresh)) ++s, ++t;
      while (t + 1 < n && norm(matrixY(t, t + 1)) > sqr(Thresh)) ++t;
      if (t == n) break;
      // Form Gram submatrix terms.
      Field coeffY0 = s + 1 < t ? matrixY(t - 2, t - 1) : Field(0);
      Field coeffY1 = matrixY(t - 1, t);
      Field coeffZ0 = matrixY(t - 1, t - 1);
      Field coeffZ1 = matrixY(t, t);
      Float coeffG00 = norm(coeffY0) + norm(coeffZ0);
      Float coeffG11 = norm(coeffY1) + norm(coeffZ1);
      Float coeffG01 = norm(coeffZ0) * norm(coeffY1);
      // Solve quadratic characteristic polynomial for eigenvalues.
      Float coeffB = (coeffG00 + coeffG11) * Float(0.5);
      Float coeffC = (coeffG00 * coeffG11) - coeffG01;
      Float coeffD = max(coeffB * coeffB - coeffC, Float(0));
      Float lambda0 = coeffB + copysign(sqrt(coeffD), coeffB);
      Float lambda1 = coeffC / lambda0;
      assert(isfinite(lambda0));
      assert(isfinite(lambda1));
      // Do Givens rotations.
      Field coeffF = norm(matrixY(s, s));
      Field coeffG = conj(matrixY(s, s)) * matrixY(s, s + 1);
      coeffF -= abs(lambda0 - coeffG11) < abs(lambda1 - coeffG11) ? lambda0 : lambda1;
      for (size_t k = s; k < t; k++) {
        rotateGivens<SideV>(k, k + 1, coeffF, coeffG);
        if (k != s && k + 1 != t) [[likely]]
          matrixY(k, k + 2) = Field(0);
        coeffF = matrixY(k, k);
        coeffG = matrixY(k + 1, k);
        rotateGivens<SideU>(k, k + 1, coeffF, coeffG);
        matrixY(k + 1, k) = Field(0);
        if (k + 1 != t) [[likely]] {
          coeffF = matrixY(k, k + 1);
          coeffG = matrixY(k, k + 2);
        }
      }
      if (numIters > 4096) [[unlikely]]
        throw Error(std::runtime_error("Diagonalization failed to converge!"));
    }
    // Re-apply preconditioning factor.
    for (size_t k = 0; k < n; k++) matrixY(k, k) *= factor;
  }
};

} // namespace mi
