#pragma once

#include "./TensorLike.h"

namespace mi {

template <typename Lambda, concepts::tensor_shape Shape>
struct TensorLambda : TensorLike<TensorLambda<Lambda, Shape>, std::invoke_result_t<Lambda, IndexVector<Shape::Rank>>, Shape> {
public:
  using Super = TensorLike<TensorLambda<Lambda, Shape>, std::invoke_result_t<Lambda, IndexVector<Shape::Rank>>, Shape>;

  using Super::DynamicRank;

  using Super::IsLValue;

  using Super::Rank;

  using Super::shape;

  using access_type = typename Super::access_type;

  using value_type = typename Super::value_type;

  using shape_type = typename Super::shape_type;

  using index_type = typename Super::index_type;

  using initializer_list = typename Super::initializer_list;

  using tensor_lambda_tag = std::true_type;

public:
  [[strong_inline]] constexpr TensorLambda(Shape shape, Lambda &&lambda) noexcept
    : Super(shape), lambda{std::forward<Lambda>(lambda)} {}

  template <typename OtherLambda, concepts::tensor_shape OtherShape>
  [[nodiscard, strong_inline]] static constexpr auto deduce(OtherShape shape, OtherLambda &&lambda) {
    return TensorLambda<std::decay_t<OtherLambda>, std::decay_t<OtherShape>>(shape, std::forward<OtherLambda>(lambda));
  }

  template <size_t K = 0, typename First, typename... Other> requires(K < Rank && !std::floating_point<First>)
  [[nodiscard, strong_inline]] constexpr decltype(auto) access(First first, Other... other) const {
    if constexpr (sizeof...(Other) != 0) {
      if constexpr (std::integral<First> && (std::integral<Other> && ...)) {
        // We want to make things as easy as possible for the compiler,
        // so explicitly optimize when it is possible to directly invoke
        // the lambda.
        return lambda(index_type{first, other...});
      } else {
        return access<K>(first).template access<K + (std::integral<First> ? 0 : 1)>(other...);
      }
    } else if constexpr (std::same_as<First, index_type>) {
      return lambda(first);
    } else if constexpr (std::integral<First>) {
      if constexpr (Rank == 1) {
        return lambda(index_type{first});
      } else {
        return deduce(shape.template drop<K>(), [expr = *this, first = first](auto i) constexpr -> access_type {
          index_type j;
          std::copy(&i[0], &i[K], &j[0]);
          std::copy(&i[K], &i[Rank - 1], &j[K + 1]);
          j[K] = first;
          return expr(j);
        });
      }
    } else {
      if constexpr (First::IsNoOp) {
        return TensorLambda(*this);
      } else {
        return deduce(shape.template bind<K>(first), [expr = *this, offset = first.offset()](auto i) constexpr -> access_type {
          i[K] += offset;
          return expr(i);
        });
      }
    }
  }

  /// Execute the expression.
  ///
  /// This method materializes the lazy expression into a concrete tensor
  /// object. Note that tensor construction and assignment will invoke this
  /// method as necessary, so client code should rarely if ever need to
  /// invoke this explicitly.
  ///
  template <typename Result = Tensor<value_type, Shape, 0>> //
  [[nodiscard, strong_inline]] constexpr auto execute() const {
    Result result{shape};
    result.shape.forEach([&](auto i) constexpr { result(i) = lambda(i); });
    return result;
  }

  /// Cast to another scalar type.
  template <typename Other> [[nodiscard, strong_inline]] constexpr decltype(auto) cast() const {
    if constexpr (std::same_as<Other, access_type>)
      return *this;
    else
      return deduce(shape, [expr = *this](auto i) constexpr -> Other { return static_cast<Other>(expr(i)); });
  }

  template <typename Other> requires(IsLValue && std::convertible_to<Other, value_type>)
  [[strong_inline]] constexpr void operator=(const Other &other) {
    this->assign(other);
  }

  [[strong_inline]] constexpr void operator=(initializer_list ilist) requires(IsLValue && Rank <= 2) {
    if constexpr (Rank == 1) {
      auto itr = ilist.begin();
      for (size_t i = 0; i < std::min(this->size(), ilist.size()); i++) access(i) = *itr++;
    } else {
      auto itr0 = ilist.begin();
      for (size_t i = 0; i < std::min(this->rows(), ilist.size()); i++) {
        const auto &ilist1 = *itr0++;
        auto itr1 = ilist1.begin();
        for (size_t j = 0; j < std::min(this->cols(), ilist1.size()); j++) access(i, j) = *itr1++;
      }
    }
  }

public:
  /// The lambda function object.
  Lambda lambda;
};

[[nodiscard, strong_inline]] constexpr auto capture_in_tensor_lambda(auto &&expr) noexcept { return std::move(expr); }

[[nodiscard, strong_inline]] constexpr auto capture_in_tensor_lambda(const auto &expr) { return expr; }

template <typename Expr> requires(concepts::tensor_object<Expr>)
[[nodiscard, strong_inline]] constexpr auto capture_in_tensor_lambda(Expr &&expr) noexcept {
  return TensorLambda(expr.shape, [expr = std::move(expr)](auto i) constexpr { return expr(i); });
}

template <typename Expr> requires(concepts::tensor_object<Expr>)
[[nodiscard, strong_inline]] constexpr auto capture_in_tensor_lambda(Expr &expr) noexcept {
  return TensorLambda(expr.shape, [&](auto i) constexpr -> decltype(auto) { return (expr(i)); });
}

template <typename Expr> requires(concepts::tensor_object<Expr>)
[[nodiscard, strong_inline]] constexpr auto capture_in_tensor_lambda(const Expr &expr) noexcept {
  return TensorLambda(expr.shape, [&](auto i) constexpr -> decltype(auto) { return (expr(i)); });
}

/// Vector/matrix dot product.
///
/// This function is the main implementation for dot products
/// involving vector and matrix expressions. If both expressions are
/// vectors, the dot product is evaluated immediately and the scalar
/// result is returned. Otherwise, an expression is constructed to
/// evaluate the dot product.
///
/// Dimension inference and error checking is performed at compile-time
/// if possible. Otherwise, error checking is performed at runtime. For
/// example:
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
/// Matrix<float, 3, 5> matrixA;
/// Vector<float, 3> vectorU;
/// Vector<float, 5> vectorV = dot(vectorU, matrixA);
/// Matrix<float, 3, 3> matrixB = dot(matrixA, transpose(matrixA));
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
template <typename ExprA, typename ExprB, size_t RankA = std::decay_t<ExprA>::Rank, size_t RankB = std::decay_t<ExprB>::Rank>
requires(concepts::tensor_tensor_op2<ExprA, ExprB> && RankA <= 2 && RankB <= 2)
[[nodiscard, strong_inline]] constexpr auto dot(ExprA &&exprA, ExprB &&exprB) {
  using ValueA = typename std::decay_t<ExprA>::value_type;
  using ValueB = typename std::decay_t<ExprB>::value_type;
  using Result = decltype(ValueA() * ValueB());
  if constexpr (RankA == 1 && RankB == 1) {
    equalShapes(exprA.shape, exprB.shape);
    Result result{};
    for (size_t k = 0; k < exprA.size(); k++) result += exprA(k) * exprB(k);
    return result;
  } else {
    auto shape = [&]() constexpr {
      if constexpr (RankA == 2 && RankB == 1) {
        equalShapes(exprA.shape.template take<1>(), exprB.shape);
        return exprA.shape.template take<0>();
      } else if constexpr (RankA == 1 && RankB == 2) {
        equalShapes(exprA.shape, exprB.shape.template take<0>());
        return exprB.shape.template take<1>();
      } else if constexpr (RankA == 2 && RankB == 2) {
        equalShapes(
          exprA.shape.template take<1>(), //
          exprB.shape.template take<0>());
        return exprA.shape.template take<0>().append(exprB.shape.template take<1>());
      }
    };
    return TensorLambda( //
             shape(),
             [exprA = capture_in_tensor_lambda(std::forward<ExprA>(exprA)),
              exprB = capture_in_tensor_lambda(std::forward<ExprB>(exprB))](auto i) constexpr {
               Result result{};
               if constexpr (RankA == 2 && RankB == 1)
                 for (size_t k = 0; k < exprB.shape.size(); k++) result += exprA(IndexVector{i[0], k}) * exprB(k);
               else if constexpr (RankA == 1 && RankB == 2)
                 for (size_t k = 0; k < exprA.shape.size(); k++) result += exprA(k) * exprB(IndexVector{k, i[0]});
               else if constexpr (RankA == 2 && RankB == 2)
                 for (size_t k = 0; k < exprA.shape.cols(); k++)
                   result += exprA(IndexVector{i[0], k}) * exprB(IndexVector{k, i[1]});
               return result;
             })
      .execute(); // It is not generally safe/intuitive to leave this unrealized.
  }
}

template <typename ExprA, typename ExprB, typename... Exprs> requires(sizeof...(Exprs) > 0)
[[nodiscard, strong_inline]] constexpr auto dot(ExprA &&exprA, ExprB &&exprB, Exprs &&...exprs) {
  return dot(std::forward<ExprA>(exprA), dot(std::forward<ExprB>(exprB), std::forward<Exprs>(exprs)...));
}

/// Outer product.
///
/// Constructs an expression for the outer product, also known as the
/// tensor product, of the given tensor expressions. The resulting tensor
/// expression joins the dimensions of the arguments together, simply
/// multiplying every possible pairing of the values from each tensor.
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
/// // Here matrixA(i, j) = vectorU(i) * vectorV(i)
/// Vector<float, 3> vectorU;
/// Vector<float, 4> vectorV;
/// Matrix<float, 3, 4> matrixA = outer(vectorU, vectorV);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
template <typename ExprA, typename ExprB, size_t RankA = std::decay_t<ExprA>::Rank, size_t RankB = std::decay_t<ExprB>::Rank>
requires(concepts::tensor_tensor_op2<ExprA, ExprB>)
[[nodiscard, strong_inline]] constexpr auto outer(ExprA &&exprA, ExprB &&exprB) {
  return TensorLambda( //
    exprA.shape.append(exprB.shape), [exprA = capture_in_tensor_lambda(std::forward<ExprA>(exprA)),
                                      exprB = capture_in_tensor_lambda(std::forward<ExprB>(exprB))](auto i) constexpr {
      IndexVector<RankA> iA{};
      IndexVector<RankB> iB{};
      auto itrA = i.begin();
      auto itrB = i.begin() + RankA;
      auto itrC = i.end();
      std::copy(itrA, itrB, iA.begin());
      std::copy(itrB, itrC, iB.begin());
      return exprA(iA) * exprB(iB);
    });
}

template <typename ExprA, typename ExprB, typename... Exprs> requires(sizeof...(Exprs) > 0)
[[nodiscard, strong_inline]] constexpr auto outer(ExprA &&exprA, ExprB &&exprB, Exprs &&...exprs) {
  return outer(std::forward<ExprA>(exprA), outer(std::forward<ExprB>(exprB), std::forward<Exprs>(exprs)...));
}

/// Transpose.
///
/// Constructs an expression for the transpose of the given
/// vector or matrix expression. For vectors, this is simply a no-op
/// since, by convention, a vector may be interpreted as a row or a
/// column from context. For matrix expressions, the indexes and
/// dimensions are swapped.
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
/// Vector<float, 5> vectorU;
/// Vector<float, 5> vectorV = transpose(vectorU); // No-op
/// Matrix<float, 4, 5> matrixA;
/// Matrix<float, 5, 4> matrixB = transpose(matrixA);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
template <typename Expr> requires((concepts::tensor_with_rank<Expr, 1> || concepts::tensor_with_rank<Expr, 2>))
[[nodiscard, strong_inline]] constexpr auto transpose(Expr &&expr) {
  if constexpr (std::decay_t<Expr>::Rank == 1) {
    return capture_in_tensor_lambda(std::forward<Expr>(expr));
  } else {
    return TensorLambda( //
      expr.shape.template take<1, 0>(),
      [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto ij) constexpr -> decltype(auto) {
        return expr(IndexVector{ij[1], ij[0]});
      });
  }
}

template <bool Flag> [[nodiscard, strong_inline]] constexpr auto transposeIf(auto &&expr) {
  if constexpr (!Flag)
    return capture_in_tensor_lambda(std::forward<decltype(expr)>(expr));
  else
    return transpose(capture_in_tensor_lambda(std::forward<decltype(expr)>(expr)));
}

/// Elementwise minimum of tensor expressions.
template <typename ExprA, typename ExprB> requires(concepts::tensor<ExprA> && concepts::tensor<ExprB>)
[[nodiscard, strong_inline]] constexpr auto min(ExprA &&exprA, ExprB &&exprB) {
  return TensorLambda( //
    equalShapes(exprA.shape, exprB.shape),
    [exprA = capture_in_tensor_lambda(std::forward<ExprA>(exprA)),
     exprB = capture_in_tensor_lambda(std::forward<ExprB>(exprB))](auto i) constexpr { return min(exprA(i), exprB(i)); });
}

/// Elementwise maximum of tensor expressions.
template <typename ExprA, typename ExprB> requires(concepts::tensor<ExprA> && concepts::tensor<ExprB>)
[[nodiscard, strong_inline]] constexpr auto max(ExprA &&exprA, ExprB &&exprB) {
  return TensorLambda( //
    equalShapes(exprA.shape, exprB.shape),
    [exprA = capture_in_tensor_lambda(std::forward<ExprA>(exprA)),
     exprB = capture_in_tensor_lambda(std::forward<ExprB>(exprB))](auto i) constexpr { return max(exprA(i), exprB(i)); });
}

/// Access the diagonal of a matrix.
///
/// Constructs a vector expression for the diagonal of the
/// given matrix expression. The resulting expression preserves
/// compile-time dimensions if possible, and also preserves whether
/// or not the expression is an lvalue.
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
/// Matrix<float, 5, 6> matrixA;
/// diag(matrixA)[0] = 1; // Use as lvalue
/// diag(matrixA)[1] = 2;
/// Vector<float, 5> vectorU = diag(matrixA); // Extract
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
template <typename Expr> requires(concepts::tensor_with_rank<Expr, 2>)
[[nodiscard, strong_inline]] constexpr auto diag(Expr &&expr) {
  using Shape = typename std::decay_t<Expr>::shape_type;
  auto shape = [&]() constexpr {
    if constexpr (Shape::DynamicRank == 0)
      return TensorShape<std::min(Shape::template Size<0>, Shape::template Size<1>)>();
    else
      return TensorShape<Dynamic>(std::min(expr.size(0), expr.size(1)));
  };
  return TensorLambda( //
    shape(), [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto ij) constexpr -> decltype(auto) {
      return expr(IndexVector{ij[0], ij[0]});
    });
}

/// Sum of diagonal of matrix.
template <typename Expr> requires(concepts::tensor_with_rank<Expr, 2>)
[[nodiscard, strong_inline]] constexpr auto trace(Expr &&expr) {
  return diag(std::forward<Expr>(expr)).sum();
}

} // namespace mi

#include "TensorLambda_operators.h"
