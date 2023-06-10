#pragma once

#include "./OrthoHelper.h"

namespace mi {

template <typename Value, concepts::tensor_shape Shape, bool EnableU = true, bool EnableV = true> requires(Shape::Rank == 2)
struct DecompSVD {
public:
  using Float = to_float_t<Value>;

  using Field = to_field_t<Value>;

  static constexpr size_t Rows = Shape::template Size<0>;

  static constexpr size_t Cols = Shape::template Size<1>;

  static constexpr size_t DiagSize = Rows == Dynamic || Cols == Dynamic ? Dynamic : min(Rows, Cols);

  static constexpr Float DefaultThresh = 16 * constants::MinInv<Float>;

  DecompSVD(auto &&expr) : mHelper{auto_forward(expr)} {
    // Diagonalize.
    mHelper.diagonalize();
    // Sort diagonal values into decreasing order.
    if constexpr (DiagSize == Dynamic) mSort.resize(min(mHelper.mRows, mHelper.mCols));
    for (size_t i = 0; i < mSort.size(); i++) mSort[i] = i;
    std::sort(mSort.begin(), mSort.end(), [&](size_t iA, size_t iB) {
      return real(mHelper.mCoeffsX(iA, iA)) > real(mHelper.mCoeffsX(iB, iB));
    });
  }

public:
  /// The number of singular values.
  [[nodiscard, strong_inline]] size_t size() const noexcept { return mSort.size(); }

  /// The number of rows.
  [[nodiscard, strong_inline]] size_t rows() const noexcept { return mHelper.rows(); }

  /// The number of columns.
  [[nodiscard, strong_inline]] size_t cols() const noexcept { return mHelper.cols(); }

  /// The singular value for the given index.
  [[nodiscard, strong_inline]] Float singularValue(size_t i) const noexcept { return real(diag(mHelper.mCoeffsX)[mSort[i]]); }

  /// The left singular vector for the given index.
  ///
  /// The left singular vector for the given index, must be less than rows().
  /// If the given index is less than size(), corresponds to a singular value.
  /// If the given index is greater than or equal to size(), characterizes the
  /// implicit null space due to there being more rows than columns.
  ///
  [[nodiscard, strong_inline]] auto singularVectorU(size_t i) const requires(EnableU) {
    return conj(mHelper.mCoeffsU.row(i < mSort.size() ? mSort[i] : i));
  }

  /// The right singular vector for the given index.
  ///
  /// The right singular vector for the given index, must be less than cols().
  /// If the given index is less than size(), corresponds to a singular value.
  /// If the given index is greater than or equal to size(), characterizes the
  /// implicit null space due to there being more columns than rows.
  ///
  [[nodiscard, strong_inline]] auto singularVectorV(size_t i) const requires(EnableV) {
    return conj(mHelper.mCoeffsV.col(i < mSort.size() ? mSort[i] : i));
  }

  /// The vector of singular values.
  [[nodiscard, strong_inline]] auto vectorS() const {
    return TensorLambda(mSort.shape, [self = this](auto i) -> Float { return self->singularValue(i[0]); });
  }

  /// The matrix of singular values.
  ///
  /// The matrix of singular values is the generally rectangular rows() by
  /// cols() matrix which is entirely full of zeros, except for the singular
  /// values appearing in decreasing order on the diagonal.
  ///
  [[nodiscard, strong_inline]] auto matrixS() const {
    return TensorLambda(mHelper.mCoeffsX.shape, [self = this](auto ij) -> Float {
      return ij[0] == ij[1] ? self->singularValue(ij[0]) : Float(0);
    });
  }

  /// The square matrix of left singular vectors.
  [[nodiscard, strong_inline]] auto matrixU() const requires(EnableU) {
    return TensorLambda(
      mHelper.mCoeffsU.shape, [self = this](auto ij) -> Field { return self->singularVectorU(ij[1])[ij[0]]; });
  }

  /// The square matrix of right singular vectors.
  [[nodiscard, strong_inline]] auto matrixV() const requires(EnableV) {
    return TensorLambda(
      mHelper.mCoeffsV.shape, [self = this](auto ij) -> Field { return self->singularVectorV(ij[0])[ij[1]]; });
  }

  /// The matrix rank.
  template <auto Thresh = DefaultThresh> [[nodiscard]] size_t rank() const noexcept {
    static_assert(Thresh >= Float(0));
    if constexpr (Thresh > Float(0))
      for (size_t i = 0; i < size(); i++)
        if (!(singularValue(i) >= Thresh)) return i;
    return size();
  }

  /// The matrix condition number.
  ///
  /// The matrix condition number, computed as the ratio of
  /// the largest to smallest singular values. Small condition numbers,
  /// i.e., near the minimum possible value of one, signify well-behaved
  /// linear systems. Larger condition numbers signify greater numerical
  /// instability.
  ///
  /// A condition number of infinity indicates that the matrix does not
  /// have full rank. In other words, the smallest singular value is either
  /// identically zero, or is so much smaller than the largest singular
  /// value that the ratio overflows. To compute the condition number
  /// constrained to the effective rank of the system, use a non-zero
  /// threshold.
  ///
  template <auto Thresh = Float(0)> [[nodiscard]] Float conditionNumber() const noexcept {
    return singularValue(0) / singularValue(rank<Thresh>() - 1);
  }

  template <auto Thresh = DefaultThresh> [[nodiscard, strong_inline]] auto nullMatrixU() const requires(EnableU) {
    return matrixU().col(SliceToEnd(rank<Thresh>()));
  }

  template <auto Thresh = DefaultThresh> [[nodiscard, strong_inline]] auto nullMatrixV() const requires(EnableV) {
    return matrixV().row(SliceToEnd(rank<Thresh>()));
  }

  /// Orthogonalize.
  ///
  /// Orthogonalize the original matrix by multiplying the left and right
  /// orthogonal spaces as if all singular values were equal to one.
  ///
  [[nodiscard, strong_inline]] auto orthogonalize() const requires(EnableU && EnableV) {
    return TensorLambda( //
      mHelper.mCoeffsX.shape, [self = this, rank = size()](auto ij) -> Field {
        Field result{};
        size_t i = ij[0];
        size_t j = ij[1];
        for (size_t k = 0; k < rank; k++) result += self->singularVectorU(k)[i] * self->singularVectorV(k)[j];
        return result;
      });
  }

  template <auto Thresh = DefaultThresh> [[nodiscard, strong_inline]] auto pseudoDeterminant() const {
    return vectorS()[Slice(0, rank<Thresh>())].product();
  }

  /// Calculate the pseudo-inverse, also known as the Moore-Penrose inverse.
  ///
  /// There are many reasons SVD is a powerful Swiss-army knife for cutting through
  /// linear algebra. Being able to robustly calculate the pseudo-inverse is one of
  /// them. The pseudo-inverse is the ordinary inverse when the matrix is square and has
  /// full rank. But it always exists, even if the matrix is not square or does not have
  /// full rank, in which case it represents the best fit or best approximation of the
  /// inverse in the sense of least-squares solutions to linear systems.
  ///
  template <auto Thresh = DefaultThresh> [[nodiscard, strong_inline]] auto pseudoInverse() const requires(EnableU && EnableV) {
    return TensorLambda(
      mHelper.mCoeffsX.shape.template take<1, 0>(), [self = this, rank = this->rank<Thresh>()](auto ij) -> Field {
        Field result{};
        size_t i = ij[0];
        size_t j = ij[1];
        for (size_t k = 0; k < rank; k++)
          result += conj(self->singularVectorV(k)[i] * self->singularVectorU(k)[j] / self->singularValue(k));
        return result;
      });
  }

  /// Solve a linear system, or find the best solution in the sense of linear least squares.
  template <auto Thresh = DefaultThresh, typename Expr> requires(concepts::tensor_vector<Expr> || concepts::tensor_matrix<Expr>)
  [[nodiscard, gnu::flatten]] auto solve(Expr &&matrixB) requires(EnableU && EnableV) {
    equalShapes(matrixU().shape.template take<0>(), matrixB.shape.template take<0>());
    auto shape = [&] {
      if constexpr (concepts::tensor_vector<Expr>)
        return matrixV().shape.template take<1>();
      else
        return matrixV().shape.template take<1>().append(matrixB.shape.template take<1>());
    }();
    using ValueB = value_type_t<Expr>;
    using ValueX = std::decay_t<decltype(Field() * ValueB())>;
    Tensor<ValueX, std::decay_t<decltype(shape)>> matrixX{shape};
    // Local variables to track the relevant dimensions:
    //   m = The number of rows.
    //   n = The number of columns.
    //   r = The effective rank. Importantly, this is always less than or equal to
    //       the number of singular values, which is the minimum of the number of rows
    //       and the number of columns, and which is also the size of the mSort vector.
    //       So, we don't need to do bounds checking on mSort lookups.
    size_t m = mHelper.mRows;
    size_t n = mHelper.mCols;
    size_t r = this->rank<Thresh>();
    // The work vector, to store the intermediate matrix-vector product when solving
    // for each column. Note that this is predominantly used by OrthoHelper for Householder
    // reflections when doing the initial bidiagonalization. But once the decomposition is
    // constructed, it is no longer actively used for anything, so we are free to borrow it
    // for this task instead of (potentially dynamically) allocating another vector.
    auto &vectorW = mHelper.mCoeffsW;
    auto solveColumn = [&](auto &&vectorB, auto &&vectorX) {
      for (size_t i = 0; i < r; i++) {
        size_t p = mSort[i];
        // Initialize vector W as the product of the adjoint of matrix U with the
        // solution vector B. Notice that, in performing the diagonalization, we accumulate
        // forward transforms in the U and V matrices. In the end, it is the inverse (adjoint, or
        // conjugate transpose) transforms that form the decomposition, but we never actually
        // do this because it is unnecessary. Instead, the matrix view observers encode the
        // adjoint by swapping the indexes and conjugating the result. All of this is to
        // clarify that mCoeffsU is already the adjoint of U, which is why we're not
        // swapping the indexes nor conjugating the value.
        ValueX value{};
        for (size_t k = 0; k < m; k++) value += mHelper.mCoeffsU(p, k) * vectorB[k];
        // Now divide out the singular value. Notice that we're only looping up to
        // the effective rank, so this should always be safe.
        vectorW[i] = value / real(mHelper.mCoeffsX(p, p));
      }
      // Finally compute vector X as the product of the adjoint of matrix V with the work vector. For
      // same reasons mentioned in multiplication with matrix U, we intentionally do not swap the indexes
      // or conjugate because mCoeffsV is already the adjoint of the understood matrix V.
      for (size_t j = 0; j < n; j++) {
        ValueX value{};
        for (size_t k = 0; k < r; k++) value += mHelper.mCoeffsV(j, mSort[k]) * vectorW[k];
        vectorX[j] = value;
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

private:
  OrthoHelper<Value, Shape, EnableU, EnableV> mHelper;

  Tensor<size_t, TensorShape<DiagSize>> mSort;
};

template <typename Expr>
DecompSVD(Expr &&) -> DecompSVD<typename std::decay_t<Expr>::value_type, typename std::decay_t<Expr>::shape_type>;

template <typename Expr> requires(concepts::tensor_with_rank<Expr, 2>) [[nodiscard]] inline auto orthogonalize(Expr &&expr) {
  return DecompSVD(std::forward<Expr>(expr)).orthogonalize().doIt();
}

template <typename Expr> requires(concepts::tensor_with_rank<Expr, 2>) [[nodiscard]] inline auto pseudoInverse(Expr &&expr) {
  return DecompSVD(std::forward<Expr>(expr)).pseudoInverse().doIt();
}

} // namespace mi
