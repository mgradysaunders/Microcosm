#pragma once

#include "./OrthoHelper.h"

namespace mi {

template <typename Value, concepts::tensor_shape Shape> requires(Shape::Rank == 2) struct DecompQR {
public:
  using Float = to_float_t<Value>;
  using Field = to_field_t<Value>;

  template <typename Expr> DecompQR(Expr &&expr) : mHelper{std::forward<Expr>(expr)} { mHelper.upperTriangularize(); }

public:
  /// Construct an expression for the orthogonal matrix.
  [[nodiscard, strong_inline]] auto matrixQ() const { return mHelper.matrixU(); }

  /// Construct an expression for the upper triangular matrix.
  [[nodiscard, strong_inline]] auto matrixR() const { return mHelper.matrixX(); }

private:
  OrthoHelper<Value, Shape, /*EnableU=*/true, /*EnableV=*/false> mHelper;
};

template <typename Expr>
DecompQR(Expr &&) -> DecompQR<typename std::decay_t<Expr>::value_type, typename std::decay_t<Expr>::shape_type>;

} // namespace mi
