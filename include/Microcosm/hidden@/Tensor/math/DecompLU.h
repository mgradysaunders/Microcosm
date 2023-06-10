#pragma once

namespace mi {

template <typename Value, concepts::tensor_shape Shape> requires(Shape::Rank == 2) struct DecompLU {
public:
  using Float = to_float_t<Value>;
  using Field = to_field_t<Value>;
  static constexpr auto Size = Shape::SizeIfSame;

  template <typename Expr> [[gnu::flatten]] DecompLU(Expr &&expr) : mCoeffs(std::forward<Expr>(expr)) {
    equalShapes(
      mCoeffs.shape.template take<0>(), //
      mCoeffs.shape.template take<1>());
    size_t size{mCoeffs.shape.rows()};
    if constexpr (Size == Dynamic) mPivots.resize(size);
    for (size_t j = 0; j < size; j++) mPivots[j] = j;
    for (size_t j = 0; j < size; j++) {
      // Pivoting.
      if (size_t k = argmax(norm(mCoeffs(Slice(j), j))) + j; k != j) {
        mCoeffs.swapRowsInPlace(j, k);
        mPivots.swapInPlace(j, k);
        mSign = -mSign;
      }
      // Make sure the matrix isn't singular.
      if (!(norm(mCoeffs(j, j)) > norm(constants::MinInv<Float>))) [[unlikely]] {
        throw Error(std::runtime_error("LU decomposition given singular matrix!"));
      }
      // Incrementally assemble the decomposition.
      Field denom = Float(1) / mCoeffs(j, j);
      for (size_t i = j + 1; i < size; i++) {
        mCoeffs(i, j) *= denom;
        for (size_t k = j + 1; k < size; k++) {
          mCoeffs(i, k) -= mCoeffs(i, j) * mCoeffs(j, k);
        }
      }
    }
  }

public:
  /// Construct an expression for the permutation matrix.
  [[nodiscard, strong_inline]] auto matrixP() const noexcept {
    return TensorLambda(mCoeffs.shape, [&](auto ij) noexcept -> Field { return Field(ij[0] == mPivots[ij[1]] ? 1 : 0); });
  }

  /// Construct an expression for the lower triangular matrix.
  [[nodiscard, strong_inline]] auto matrixL() const noexcept {
    return TensorLambda(mCoeffs.shape, [&](auto ij) noexcept -> Field {
      return ij[0] == ij[1] ? Field(1) : ij[0] > ij[1] ? mCoeffs(ij) : Field(0);
    });
  }

  /// Construct an expression for the upper triangular matrix.
  [[nodiscard, strong_inline]] auto matrixU() const noexcept {
    return TensorLambda(mCoeffs.shape, [&](auto ij) noexcept -> Field { return ij[0] <= ij[1] ? mCoeffs(ij) : Field(); });
  }

public:
  /// Solve a linear system.
  template <typename Expr> requires(concepts::tensor_vector<Expr> || concepts::tensor_matrix<Expr>)
  [[nodiscard, gnu::flatten]] auto solve(Expr &&matrixB) const {
    auto &matrixA{mCoeffs};
    equalShapes(
      matrixA.shape.template take<0>(), //
      matrixB.shape.template take<0>());
    auto shape = [&] {
      if constexpr (concepts::tensor_vector<Expr>)
        return matrixA.shape.template take<1>();
      else
        return matrixA.shape.template take<1>().append(matrixB.shape.template take<1>());
    }();
    using ValueB = value_type_t<Expr>;
    using ValueX = std::decay_t<decltype(Field() * ValueB())>;
    Tensor<ValueX, std::decay_t<decltype(shape)>> matrixX{shape};
    auto solveColumn = [&](auto &&vectorB, auto &&vectorX) {
      // Solve Ly = b.
      ssize_t m = vectorB.size();
      for (ssize_t i = 0; i < m; i++) {
        // Ideally we want to use the dot function here, but we can't rely on the compiler to know
        // that it can safely ignore the size-checks associated with the dynamic slices. So for
        // maximum performance, write out the loop manually.
        ValueX value{};
        for (ssize_t k = 0; k < i; k++) value += matrixA(i, k) * vectorX[k];
        vectorX[i] = vectorB[mPivots[i]] - value /* dot(matrixA(i, Slice(0, i)), vectorX[Slice(0, i)]) */;
      }
      // Solve Ux = y.
      for (ssize_t i = m - 1; i >= 0; i--) {
        // Again, ideally we want to use the dot function, but for the same reasons as
        // listed above, write out the loop manually.
        ValueX value{};
        for (ssize_t k = i + 1; k < m; k++) value += matrixA(i, k) * vectorX[k];
        vectorX[i] -= value /* dot(matrixA(i, Slice(i + 1)), vectorX[Slice(i + 1)]) */;
        vectorX[i] /= matrixA(i, i);
      }
    };
    if constexpr (concepts::tensor_vector<Expr>) {
      solveColumn(matrixB, matrixX);
    } else {
      for (size_t j = 0; j < matrixB.cols(); j++) {
        solveColumn(matrixB.col(j), matrixX.col(j));
      }
    }
    return matrixX;
  }

public:
  /// Calculate the inverse matrix.
  [[nodiscard, strong_inline]] auto inverse() const { return solve(identity<Field>(mCoeffs.shape)); }

  /// Calculate the determinant.
  [[nodiscard, strong_inline]] auto determinant() const { return mSign * diag(mCoeffs).product(); }

private:
  Tensor<Field, TensorShape<Size, Size>> mCoeffs;

  Tensor<size_t, TensorShape<Size>> mPivots;

  Field mSign = Field(1);
};

template <typename Expr>
DecompLU(Expr &&) -> DecompLU<typename std::decay_t<Expr>::value_type, typename std::decay_t<Expr>::shape_type>;

template <typename Expr> requires(concepts::tensor_with_rank<Expr, 2>) [[nodiscard]] inline auto inverse(Expr &&expr) {
  return DecompLU(std::forward<Expr>(expr)).inverse();
}

template <typename Expr> requires(concepts::tensor_with_rank<Expr, 2>) [[nodiscard]] inline auto determinant(Expr &&expr) {
  using Shape = typename std::decay_t<Expr>::shape_type;
  if constexpr (std::same_as<Shape, TensorShape<1, 1>>) {
    return expr(0, 0);
  } else if constexpr (std::same_as<Shape, TensorShape<2, 2>>) {
    return expr(0, 0) * expr(1, 1) - expr(0, 1) * expr(1, 0);
  } else if constexpr (std::same_as<Shape, TensorShape<3, 3>>) {
    auto matrix = expr.doIt();
    auto vectorX = matrix[0];
    auto vectorY = matrix[1];
    auto vectorZ = matrix[2];
    return dot(vectorX, cross(vectorY, vectorZ));
  } else {
    return DecompLU(std::forward<Expr>(expr)).determinant();
  }
}

} // namespace mi
