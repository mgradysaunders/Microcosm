#pragma once

#include "./LengthHelper.h"

namespace mi {

/// Absolute value of dot product.
template <typename ExprA, typename ExprB> requires(concepts::tensor_with_rank<ExprA, 1> && concepts::tensor_with_rank<ExprB, 1>)
[[nodiscard, strong_inline]] constexpr auto absDot(ExprA &&exprA, ExprB &&exprB) {
  return abs(dot(std::forward<ExprA>(exprA), std::forward<ExprB>(exprB)));
}

/// Euclidean length.
template <typename Expr> requires(concepts::tensor_with_rank<Expr, 1>)
[[nodiscard, strong_inline]] inline auto length(Expr &&expr) {
  return LengthHelperFor<Expr>().length(auto_forward(expr));
}

/// Euclidean length squared.
template <typename Expr> requires(concepts::tensor_with_rank<Expr, 1>)
[[nodiscard, strong_inline]] constexpr auto lengthSquare(Expr &&expr) {
  return norm(auto_forward(expr)).sum();
}

/// Euclidean distance.
template <typename ExprA, typename ExprB> requires(concepts::tensor_with_rank<ExprA, 1> && concepts::tensor_with_rank<ExprB, 1>)
[[nodiscard, strong_inline]] inline auto distance(ExprA &&exprA, ExprB &&exprB) {
  return length(auto_forward(exprB) - auto_forward(exprA));
}

/// Euclidean distance squared.
template <typename ExprA, typename ExprB> requires(concepts::tensor_with_rank<ExprA, 1> && concepts::tensor_with_rank<ExprB, 1>)
[[nodiscard, strong_inline]] constexpr auto distanceSquare(ExprA &&exprA, ExprB &&exprB) {
  return lengthSquare(auto_forward(exprB) - auto_forward(exprA));
}

/// Normalize by Euclidean length.
template <typename Expr> requires(concepts::tensor_with_rank<Expr, 1>)
[[nodiscard, strong_inline]] inline auto normalize(Expr &&expr) {
  return LengthHelperFor<Expr>().normalize(auto_forward(expr));
}

/// Normalize by Euclidean length in-place.
template <typename Expr> requires(concepts::tensor_with_rank<Expr, 1> && concepts::lvalue_tensor<Expr>)
[[nodiscard, strong_inline]] inline auto normalizeInPlace(Expr &&expr) {
  return LengthHelperFor<Expr>().normalizeInPlace(auto_forward(expr));
}

/// Clamp Euclidean length.
template <typename Expr> requires(concepts::tensor_with_rank<Expr, 1>) //
[[nodiscard, strong_inline]] inline auto
clampLength(Expr &&expr, concepts::arithmetic auto minLen, concepts::arithmetic auto maxLen) {
  return LengthHelperFor<Expr>().clampLength(auto_forward(expr), minLen, maxLen);
}

/// Clamp Euclidean length in-place.
template <typename Expr> requires(concepts::tensor_with_rank<Expr, 1> && concepts::lvalue_tensor<Expr>)
[[nodiscard, strong_inline]] inline auto
clampLengthInPlace(Expr &&expr, concepts::arithmetic auto minLen, concepts::arithmetic auto maxLen) {
  return LengthHelperFor<Expr>().clampLengthInPlace(auto_forward(expr), minLen, maxLen);
}

/// A convenience method to calculate length and direction simultaneously.
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
/// auto [vectorLen, vectorDir] = lengthAndDirection(vector);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// \note
/// This helps eliminate either redundant calculations or dumb temporary variables and
/// manual normalization whenever we want to normalize a vector, but also need its length
/// for something.
///
template <typename Expr> requires(concepts::tensor_with_rank<Expr, 1>)
[[nodiscard, strong_inline]] inline auto lengthAndDirection(Expr &&expr) {
  auto vec = expr.doIt();
  auto len = LengthHelperFor<Expr>().normalizeInPlace(vec);
  return std::make_pair(len, vec);
}

/// A convenience method to calculate distance and direction simultaneously.
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
/// auto [vectorLen, vectorDir] = distanceAndDirection(pointA, pointB);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
template <typename ExprA, typename ExprB> requires(concepts::tensor_with_rank<ExprA, 1> && concepts::tensor_with_rank<ExprB, 1>)
[[nodiscard, strong_inline]] inline auto distanceAndDirection(ExprA &&exprA, ExprB &&exprB) {
  return lengthAndDirection(auto_forward(exprB) - auto_forward(exprA));
}

/// Euclidean length. Fast version with no protection against overflow and underflow.
template <typename Expr> requires(concepts::tensor_with_rank<Expr, 1>)
[[nodiscard, strong_inline]] inline auto fastLength(Expr &&expr) {
  return sqrt(lengthSquare(auto_forward(expr)));
}

/// Normalize by Euclidean length. Fast version with no protection against overflow and underflow.
template <typename Expr> requires(concepts::tensor_with_rank<Expr, 1>)
[[nodiscard, strong_inline]] inline auto fastNormalize(Expr &&expr) {
  auto vector = expr.doIt();
  auto vectorLen = fastLength(vector);
  auto vectorLenInv = vectorLen > constants::MinInv<decltype(vectorLen)> ? 1 / vectorLen : 0;
  vector *= vectorLenInv;
  return vector;
}

/// Angle between vectors.
template <typename ExprA, typename ExprB> requires(concepts::tensor_with_rank<ExprA, 1> && concepts::tensor_with_rank<ExprB, 1>)
[[nodiscard, strong_inline]] inline auto angleBetween(ExprA &&exprA, ExprB &&exprB) {
  auto shape = equalShapes(exprA.shape, exprB.shape);
  using Shape = std::decay_t<decltype(shape)>;
  using ValueA = value_type_t<ExprA>;
  using ValueB = value_type_t<ExprB>;
  using Float = to_float_t<ValueA, ValueB>;
  auto vectorU = exprA.template cast<Float>().doIt();
  auto vectorV = exprB.template cast<Float>().doIt();
  LengthHelper<Float, Shape> lengthHelper;
  volatile Float minLen = lengthHelper.length(vectorU);
  volatile Float maxLen = lengthHelper.length(vectorV);
  volatile Float sepLen = lengthHelper.length(vectorV - vectorU);
  if (!(minLen < maxLen)) {
    std::swap(minLen, maxLen);
  }
  volatile Float coeff = minLen >= sepLen ? sepLen - (maxLen - minLen) : minLen - (maxLen - sepLen);
  volatile Float numer = (maxLen - minLen + sepLen) * coeff;
  volatile Float denom = (minLen + sepLen + maxLen) * (maxLen - sepLen + minLen);
  return 2 * atan(sqrt(max(numer / denom, Float(0))));
}

/// Angle between unit-length vectors.
template <typename ExprA, typename ExprB> requires(concepts::tensor_with_rank<ExprA, 1> && concepts::tensor_with_rank<ExprB, 1>)
[[nodiscard, strong_inline]] inline auto angleBetweenUnitLength(ExprA &&exprA, ExprB &&exprB) {
  auto numer{fastLength(auto_forward(exprB) - auto_forward(exprA))};
  auto denom{sqrt(max((2 + numer) * (2 - numer), decltype(numer)(0)))};
  return 2 * atan(numer / denom);
}

/// 2-dimensional Hodge-star operator (counter-clockwise perpendicular).
template <typename Expr, typename Value = value_type_t<Expr>> requires(concepts::tensor_with_shape<Expr, 2>) //
[[nodiscard, strong_inline]] constexpr Vector2<Value> hodge(Expr &&expr) {
  return {-expr[1], expr[0]};
}

/// 3-dimensional Hodge-star operator.
template <typename Expr, typename Value = value_type_t<Expr>> requires(concepts::tensor_with_shape<Expr, 3>) //
[[nodiscard, strong_inline]] constexpr Matrix3<Value> hodge(Expr &&expr) {
  Value exprX = expr[0];
  Value exprY = expr[1];
  Value exprZ = expr[2];
  return {
    {Value(0), -exprZ, +exprY}, //
    {+exprZ, Value(0), -exprX}, //
    {-exprY, +exprX, Value(0)}};
}

/// 2-dimensional cross product.
template <typename ExprA, typename ExprB>
requires(concepts::tensor_with_shape<ExprA, 2> && concepts::tensor_with_shape<ExprB, 2>)
[[nodiscard, strong_inline]] constexpr auto cross(ExprA &&exprA, ExprB &&exprB) {
  return dot(hodge(std::forward<ExprA>(exprA)), std::forward<ExprB>(exprB));
}

/// 3-dimensional cross product.
template <
  typename ExprA, typename ExprB, typename ValueA = value_type_t<ExprA>, typename ValueB = value_type_t<ExprB>,
  typename Result = decltype(ValueA() * ValueB())>
requires(concepts::tensor_with_shape<ExprA, 3> && concepts::tensor_with_shape<ExprB, 3>)
[[nodiscard, strong_inline]] constexpr Vector3<Result> cross(ExprA &&exprA, ExprB &&exprB) {
  auto vectorA = exprA.doIt();
  auto vectorB = exprB.doIt();
  return {
    vectorA[1] * vectorB[2] - vectorA[2] * vectorB[1], //
    vectorA[2] * vectorB[0] - vectorA[0] * vectorB[2], //
    vectorA[0] * vectorB[1] - vectorA[1] * vectorB[0]};
}

[[nodiscard, strong_inline]] inline auto unitCircle(auto theta) noexcept -> Vector2<to_float_t<decltype(theta)>> {
  return {cos(theta), sin(theta)};
}

[[nodiscard, strong_inline]] inline auto unitSphere(auto theta, auto phi) noexcept
  -> Vector3<to_float_t<decltype(theta), decltype(phi)>> {
  auto sinTheta = sin(theta);
  auto cosTheta = cos(theta);
  auto sinPhi = sin(phi);
  auto cosPhi = cos(phi);
  return {sinTheta * cosPhi, sinTheta * sinPhi, cosTheta};
}

template <std::floating_point Float> struct UnitCircleIterator {
public:
  using value_type = Vector2<Float>;
  using difference_type = ptrdiff_t;
  using reference = Vector2<Float>;
  using pointer = void;
  using iterator_category = std::forward_iterator_tag;

  constexpr UnitCircleIterator() noexcept = default;
  constexpr UnitCircleIterator(int count, Vector2<Float> delta, Vector2<Float> theta) noexcept
    : count(count), delta(delta), theta(theta) {}
  constexpr UnitCircleIterator &operator++() noexcept {
    theta = {
      +delta[0] * theta[0] - delta[1] * theta[1], //
      +delta[1] * theta[0] + delta[0] * theta[1]};
    count--;
    return *this;
  }
  constexpr UnitCircleIterator operator++(int) noexcept {
    const UnitCircleIterator copy = *this;
    operator++();
    return copy;
  }
  [[nodiscard]] constexpr Vector2<Float> operator*() const noexcept { return theta; }
  [[nodiscard]] constexpr bool operator==(nothing) const noexcept { return count <= 0; }
  [[nodiscard]] constexpr bool operator!=(nothing) const noexcept { return count > 0; }

  int count = 0;                 ///< The step count.
  Vector2<Float> delta = {1, 0}; ///< The cosine and sine of delta.
  Vector2<Float> theta = {1, 0}; ///< The cosine and sine of theta.
};

template <typename ValueA, typename ValueB, typename Expr> requires(
  (concepts::arithmetic<ValueA> || concepts::match<ValueA, Exclusive>) &&
  (concepts::arithmetic<ValueB> || concepts::match<ValueB, Exclusive>) && (concepts::tensor_with_shape<Expr, 2>))
[[nodiscard]] inline auto unitCircleLinspace(int count, ValueA thetaA, ValueB thetaB, Expr &&initTheta) noexcept {
  using FloatA = to_float_t<ValueA>;
  using FloatB = to_float_t<ValueB>;
  using Float = std::common_type_t<FloatA, FloatB>;
  constexpr int includeA = concepts::match<ValueA, Exclusive> ? 0 : 1;
  constexpr int excludeA = concepts::match<ValueA, Exclusive> ? 1 : 0;
  constexpr int excludeB = concepts::match<ValueB, Exclusive> ? 1 : 0;
  Float delta = (Float(thetaB) - Float(thetaA)) / (count - includeA + excludeB);
  UnitCircleIterator<Float> itr = {count, unitCircle(delta), std::forward<Expr>(initTheta)};
  if constexpr (excludeA) ++itr, ++itr.count;
  return IteratorRange<UnitCircleIterator<Float>, nothing>(itr, nothing());
}

template <typename ValueA, typename ValueB> requires(
  (concepts::arithmetic<ValueA> || concepts::match<ValueA, Exclusive>) &&
  (concepts::arithmetic<ValueB> || concepts::match<ValueB, Exclusive>))
[[nodiscard]] inline auto unitCircleLinspace(int count, ValueA thetaA, ValueB thetaB) noexcept {
  using FloatA = to_float_t<ValueA>;
  using FloatB = to_float_t<ValueB>;
  using Float = std::common_type_t<FloatA, FloatB>;
  return unitCircleLinspace(count, thetaA, thetaB, unitCircle(Float(thetaA)));
}

template <typename Expr> requires(concepts::tensor_with_rank<Expr, 1> || concepts::tensor_with_rank<Expr, 2>)
[[nodiscard, strong_inline]] constexpr auto adjoint(Expr &&expr) {
  if constexpr (concepts::arithmetic<value_type_t<Expr>>)
    return transpose(auto_forward(expr));
  else
    return transpose(conj(auto_forward(expr)));
}

template <typename Expr>
requires((concepts::tensor_with_rank<Expr, 1> || concepts::tensor_with_rank<Expr, 2>) && concepts::lvalue_tensor<Expr>)
constexpr void adjointInPlace(Expr &&expr) {
  if constexpr (std::decay_t<Expr>::Rank == 1) {
    for (size_t i = 0; i < expr.size(); i++) expr(i) = conj(expr(i));
  } else {
    equalShapes(expr.shape.template extract<0>(), expr.shape.template extract<1>());
    for (size_t i = 0; i < expr.rows(); i++) {
      expr(i, i) = conj(expr(i, i));
      for (size_t j = i + 1; j < expr.cols(); j++) {
        auto term0{expr(i, j)};
        auto term1{expr(j, i)};
        expr(i, j) = conj(term1);
        expr(j, i) = conj(term0);
      }
    }
  }
}

template <typename Value, concepts::tensor_shape Shape>
[[nodiscard, strong_inline]] constexpr auto identity(const Shape &shape) {
  return TensorLambda(shape, [](auto i) constexpr {
    for (size_t j = 1; j < i.size(); j++)
      if (i[j] != i[0]) return Value(0);
    return Value(1);
  });
}

/// Check that a matrix expression is nearly the identity matrix.
template <auto Thresh, typename Expr> requires(concepts::tensor_with_rank<Expr, 2>)
[[nodiscard, strong_inline]] constexpr bool isNearIdentity(Expr &&expr) {
  using Float = std::decay_t<decltype(Thresh)>;
  static_assert(std::floating_point<Float>);
  for (size_t i = 0; i < expr.rows(); i++)
    for (size_t j = 0; j < expr.cols(); j++)
      if (Float v = abs(expr(i, j)); !(abs(v - (i == j ? 1 : 0)) <= Float(Thresh))) return false;
  return true;
}

/// Check that a matrix expression is nearly unitary.
template <auto Thresh, typename Expr> requires(concepts::tensor_with_rank<Expr, 2>)
[[nodiscard, strong_inline]] constexpr bool isNearUnitary(Expr &&expr) {
  return isNearIdentity<Thresh>(dot(expr, adjoint(expr)));
}

/// Check that a tensor expression is nearly zero.
template <auto Thresh, typename Expr> requires(concepts::tensor<Expr>)
[[nodiscard, strong_inline]] constexpr bool isNearZero(Expr &&expr) {
  using Float = std::decay_t<decltype(Thresh)>;
  static_assert(std::floating_point<Float>);
  bool result = true;
  expr.shape.forEach([&](auto i) constexpr { return result = abs(expr(i)) <= Float(Thresh); });
  return result;
}

/// Check that a pair of tensor expressions are nearly equivalent.
template <auto Thresh, typename ExprA, typename ExprB> requires(concepts::tensor<ExprA> && concepts::tensor<ExprB>)
[[nodiscard, strong_inline]] constexpr bool isNear(ExprA &&exprA, ExprB &&exprB) {
  return isNearZero<Thresh>(auto_forward(exprA) - auto_forward(exprB));
}

} // namespace mi
