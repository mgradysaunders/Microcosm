#pragma once

namespace mi {

template <typename Value, concepts::tensor_shape Shape> requires(Shape::Rank == 1) struct LengthHelper {
public:
  using Float = to_float_t<Value>;
  using Field = to_field_t<Value>;
  static constexpr size_t Size = Shape::template Size<0>;

  /// Euclidean length.
  template <typename Expr> requires(concepts::tensor_with_rank<Expr, 1>) [[nodiscard]] Float length(Expr &&expr) {
    // Just to be safe, make sure we always handle the empty
    // case correctly.
    if (expr.size() == 0) [[unlikely]]
      return Float(0);

    // If the size is known at compile-time to be either 1 or 2
    // dimensional, immediately delegate to the absolute value and
    // hypotenuse functions.
    if constexpr (Size == 1) {
      return abs(expr[0]);
    } else if constexpr (Size == 2 && concepts::arithmetic<Value>) {
      return std::hypot(expr[0], expr[1]);
    } else if constexpr (Size == 2) {
      return std::hypot(abs(expr[0]), abs(expr[1]));
    } else {
      if constexpr (Size == Dynamic) {
        mTerms.resizeLike(expr);
        // Mimic compile-time logic in the runtime case.
        if (expr.size() == 1) return abs(expr[0]);
        if (expr.size() == 2) return std::hypot(abs(expr[0]), abs(expr[1]));
      }
      assert(expr.size() <= mTerms.size());
      auto terms = mTerms[Slice(0, expr.size())];
      auto maxTerm = Float(0);
      for (size_t i = 0; i < expr.size(); i++) {
        terms[i] = abs(expr[i]);
        if (maxTerm < terms[i]) maxTerm = terms[i];
      }
      // Try to stablize behavior in dubious circumstances:
      // 1. When the maximum term is less than the minimum squarable float.
      // 2. When the maximum term squared exceeds the maximum float, divided
      //    by the number of terms we are going to sum.
      if (maxTerm <= constants::MinSqr<Float> || maxTerm * maxTerm >= constants::Max<Float> / expr.size()) [[unlikely]] {
        // If the maximum term is exactly zero, all terms are exactly zero.
        if (maxTerm == Float(0)) [[unlikely]] {
          return Float(0);
        } else {
          // The terms are concerningly small or concerningly large, in
          // the sense that underflow or overflow may occur. To mitigate
          // this, we divide the maximum term out before squaring. Then
          // multiply it back after square rooting.
          terms /= maxTerm;
          return std::sqrt(dot(terms, terms)) * maxTerm;
        }
      }
      // The terms appear to be reasonable, so compute length as usual.
      return std::sqrt(dot(terms, terms));
    }
  }

  /// Normalize by Euclidean length in-place.
  template <typename Expr> requires(concepts::tensor_with_rank<Expr, 1> && concepts::lvalue_tensor<Expr>)
  [[strong_inline]] Float normalizeInPlace(Expr &&expr) {
    const Float len = this->length(expr);
    if (len == 0) [[unlikely]] {
      expr = Float(0);
      return Float(0);
    }
    // If the denominator is very small, explicitly divide.
    // Otherwise, it is safe to multiply by the reciprocal, which is
    // generally going to be more performant.
    if (len <= 8 * constants::MinInv<Float>) [[unlikely]]
      expr /= len;
    else
      expr *= Float(1) / len;
    return len;
  }

  /// Normalize by Euclidean length.
  template <typename Expr> requires(concepts::tensor_with_rank<Expr, 1>)
  [[nodiscard, strong_inline]] auto normalize(Expr &&expr) {
    auto vec = expr.template cast<Field>().doIt();
    void(normalizeInPlace(vec));
    return vec;
  }

  /// Clamp Euclidean length in-place.
  template <typename Expr> requires(concepts::tensor_with_rank<Expr, 1> && concepts::lvalue_tensor<Expr>)
  [[strong_inline]] Float clampLengthInPlace(Expr &&expr, Float minLen, Float maxLen) {
    const Float len = this->length(expr);
    if (len < minLen && len == 0) [[unlikely]] {
      expr = Float(0), expr[0] = minLen;
    } else if (len < minLen) {
      expr *= minLen / len;
    } else if (len > maxLen) {
      expr *= maxLen / len;
    }
    return len;
  }

  /// Clamp Euclidean length.
  template <typename Expr> requires(concepts::tensor_with_rank<Expr, 1>)
  [[nodiscard, strong_inline]] auto clampLength(Expr &&expr, Float minLen, Float maxLen) {
    auto vec = expr.template cast<Field>().doIt();
    void(clampLengthInPlace(vec, minLen, maxLen));
    return vec;
  }

private:
  conditional_member_t<(Size >= 3), Tensor<Float, Shape>> mTerms = {};
};

template <typename Expr>
using LengthHelperFor = LengthHelper<typename std::decay_t<Expr>::value_type, typename std::decay_t<Expr>::shape_type>;

} // namespace mi
