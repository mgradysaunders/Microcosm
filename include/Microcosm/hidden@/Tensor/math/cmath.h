#pragma once

namespace mi {

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto abs(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return abs(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto arg(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return arg(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto real(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return real(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto imag(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return imag(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto conj(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return conj(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto norm(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return norm(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto dual(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return dual(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto sign(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return sign(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto softSign(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return softSign(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto softPlus(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return softPlus(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto saturate(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return saturate(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto fabs(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return fabs(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto nearbyint(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return nearbyint(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto floor(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return floor(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto ceil(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return ceil(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto trunc(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return trunc(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto round(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return round(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto rint(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return rint(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto lrint(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return lrint(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto llrint(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return llrint(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto lround(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return lround(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto llround(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return llround(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto signbit(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return signbit(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto isnan(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return isnan(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto isinf(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return isinf(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto isfinite(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return isfinite(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto isnormal(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return isnormal(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto exp(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return exp(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto log(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return log(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto exp2(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return exp2(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto log2(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return log2(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto log10(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return log10(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto expm1(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return expm1(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto log1p(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return log1p(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto sqrt(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return sqrt(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto cbrt(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return cbrt(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto erf(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return erf(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto erfc(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return erfc(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto lgamma(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return lgamma(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto tgamma(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return tgamma(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto sin(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return sin(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto cos(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return cos(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto tan(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return tan(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto asin(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return asin(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto acos(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return acos(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto atan(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return atan(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto sinh(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return sinh(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto cosh(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return cosh(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto tanh(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return tanh(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto asinh(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return asinh(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto acosh(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return acosh(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto atanh(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return atanh(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto nextFloat(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return nextFloat(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto prevFloat(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return prevFloat(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto fastFloor(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return fastFloor(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto fastCeil(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return fastCeil(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto fastTrunc(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return fastTrunc(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto fastRound(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return fastRound(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto fastFract(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return fastFract(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto sinPi(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return sinPi(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto cosPi(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return cosPi(expr(i));
    });
}

template <typename Expr> requires(concepts::tensor<Expr>) [[nodiscard, strong_inline]] constexpr auto erfInverse(Expr &&expr) {
  return TensorLambda(
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr -> decltype(auto) { //
      return erfInverse(expr(i));
    });
}

template <typename ExprA, typename ExprB> requires(concepts::tensor_op2<ExprA, ExprB>)
[[nodiscard, strong_inline]] constexpr auto min(ExprA &&exprA, ExprB &&exprB) {
  auto shape = [&]() constexpr {
    if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>) {
      return equalShapes(exprA.shape, exprB.shape);
    } else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>) {
      return exprA.shape;
    } else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>) {
      return exprB.shape;
    }
  };
  return TensorLambda( //
    shape(), [exprA = capture_in_tensor_lambda(std::forward<ExprA>(exprA)),
              exprB = capture_in_tensor_lambda(std::forward<ExprB>(exprB))](auto i) constexpr {
      if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>) {
        return min(exprA(i), exprB(i));
      } else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>) {
        return min(exprA(i), exprB);
      } else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>) {
        return min(exprA, exprB(i));
      }
    });
}

template <typename ExprA, typename ExprB> requires(concepts::tensor_op2<ExprA, ExprB>)
[[nodiscard, strong_inline]] constexpr auto max(ExprA &&exprA, ExprB &&exprB) {
  auto shape = [&]() constexpr {
    if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>) {
      return equalShapes(exprA.shape, exprB.shape);
    } else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>) {
      return exprA.shape;
    } else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>) {
      return exprB.shape;
    }
  };
  return TensorLambda( //
    shape(), [exprA = capture_in_tensor_lambda(std::forward<ExprA>(exprA)),
              exprB = capture_in_tensor_lambda(std::forward<ExprB>(exprB))](auto i) constexpr {
      if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>) {
        return max(exprA(i), exprB(i));
      } else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>) {
        return max(exprA(i), exprB);
      } else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>) {
        return max(exprA, exprB(i));
      }
    });
}

template <typename ExprA, typename ExprB> requires(concepts::tensor_op2<ExprA, ExprB>)
[[nodiscard, strong_inline]] constexpr auto copysign(ExprA &&exprA, ExprB &&exprB) {
  auto shape = [&]() constexpr {
    if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>) {
      return equalShapes(exprA.shape, exprB.shape);
    } else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>) {
      return exprA.shape;
    } else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>) {
      return exprB.shape;
    }
  };
  return TensorLambda( //
    shape(), [exprA = capture_in_tensor_lambda(std::forward<ExprA>(exprA)),
              exprB = capture_in_tensor_lambda(std::forward<ExprB>(exprB))](auto i) constexpr {
      if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>) {
        return copysign(exprA(i), exprB(i));
      } else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>) {
        return copysign(exprA(i), exprB);
      } else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>) {
        return copysign(exprA, exprB(i));
      }
    });
}

} // namespace mi
