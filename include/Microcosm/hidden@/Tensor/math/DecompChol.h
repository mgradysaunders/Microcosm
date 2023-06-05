#pragma once

namespace mi {

template <typename Value, concepts::tensor_shape Shape> requires(Shape::Rank == 2) struct DecompChol {
public:
  using Float = to_float_t<Value>;
  using Field = to_field_t<Value>;
  static constexpr auto Size = Shape::SizeIfSame;

  template <typename Expr> [[gnu::flatten]] DecompChol(Expr &&expr) : mCoeffs(std::forward<Expr>(expr)) {
    equalShapes(
      mCoeffs.shape.template take<0>(), //
      mCoeffs.shape.template take<1>());
    Float eps{constants::MinInv<Float>};
    size_t size{mCoeffs.shape.rows()};
    if constexpr (Size == Dynamic) mPivots.resize(size);
    for (size_t k = 0; k < size; k++) mPivots[k] = k;
    for (size_t k = 0; k < size; k++) {
      if (size_t l = argmax(abs(diag(mCoeffs)[Slice(k)])) + k; l != k) {
        mCoeffs.swapRowsInPlace(k, l);
        mCoeffs.swapColsInPlace(k, l);
        mPivots.swapInPlace(k, l);
      }
      if (k == 0) eps = abs(mCoeffs(0, 0)) * constants::Eps<Float>;
      if (!(abs(mCoeffs(k, k)) > eps)) { // Positive semi-definite?
        for (size_t i = k; i < size; i++) {
          mCoeffs(i, Slice(i)) = Field(0);
        }
        break;
      }
      Field coeff = mCoeffs(k, k) = sqrt(mCoeffs(k, k));
      if (!(isfinite(coeff) && abs(coeff) > eps)) {
        throw std::runtime_error("Cholesky decomposition given non-positive-definite matrix!");
      }
      mCoeffs(k, Slice(k + 1)) /= coeff;
      for (size_t j = k + 1; j < size; j++) {
        for (size_t i = k + 1; i < j + 1; i++) {
          mCoeffs(i, j) = mCoeffs(i, j) - mCoeffs(k, j) * conj(mCoeffs(k, i));
          mCoeffs(j, i) = conj(mCoeffs(i, j));
        }
      }
    }
    for (size_t j = 0; j < size; j++) {
      for (size_t i = j + 1; i < size; i++) {
        mCoeffs(i, j) = Field(0);
      }
    }
  }

public:
  /// Construct an expression for the permutation matrix.
  [[nodiscard, strong_inline]] auto matrixP() const noexcept {
    return TensorLambda(mCoeffs.shape, [&](auto ij) noexcept -> Field { return Field(mPivots[ij[1]] == ij[0] ? 1 : 0); });
  }

  /// Construct an expression for the lower triangular matrix.
  [[nodiscard, strong_inline]] auto matrixL() const noexcept { return adjoint(mCoeffs); }

public:
  /// Solve a linear system with a matrix on the right-hand side.
  template <typename Expr> requires(concepts::tensor_with_rank<Expr, 2>)
  [[nodiscard, gnu::flatten]] auto solve(Expr &&matrixB) const {
    auto &matrixA{mCoeffs};
    equalShapes(
      matrixA.shape.template take<0>(), //
      matrixB.shape.template take<0>());
    auto shape = matrixA.shape.template take<1>().append(matrixB.shape.template take<1>());
    using ValueB = typename std::decay_t<Expr>::value_type;
    using ValueX = std::decay_t<decltype(Field() * ValueB())>;
    Tensor<ValueX, std::decay_t<decltype(shape)>> matrixX{shape};
    Tensor<ValueX, TensorShape<Size>> vectorY{shape.template take<0>()};
    ssize_t rows = matrixB.rows();
    ssize_t cols = matrixB.cols();
    for (ssize_t j = 0; j < cols; j++) {
      // Solve (R*)y = b.
      for (ssize_t i = 0; i < rows; i++) {
        vectorY[i] = matrixB(mPivots[i], j) - dot(conj(matrixA(Slice(0, i), i)), vectorY[Slice(0, i)]);
        if (Field denom = matrixA(i, i); denom != Float(0)) vectorY[i] /= conj(denom);
      }
      // Solve Rx = y.
      for (ssize_t i = rows - 1; i >= 0; i--) {
        vectorY[i] -= dot(matrixA(i, Slice(i + 1)), vectorY[Slice(i + 1)]);
        if (Field denom = matrixA(i, i); denom != Float(0)) vectorY[i] /= denom;
      }
      // Unpivot.
      for (ssize_t i = 0; i < rows; i++) matrixX(mPivots[i], j) = vectorY[i];
    }
    return matrixX;
  }

public:
  /// Calculate the inverse matrix.
  [[nodiscard, strong_inline]] auto inverse() const { return solve(identity<Field>(mCoeffs.shape)); }

  /// Calculate the determinant.
  [[nodiscard, strong_inline]] auto determinant() const { return sqr(diag(mCoeffs).product()); }

private:
  Tensor<Field, TensorShape<Size, Size>> mCoeffs;
  Tensor<size_t, TensorShape<Size>> mPivots;
};

template <typename Expr>
DecompChol(Expr &&) -> DecompChol<typename std::decay_t<Expr>::value_type, typename std::decay_t<Expr>::shape_type>;

} // namespace mi
