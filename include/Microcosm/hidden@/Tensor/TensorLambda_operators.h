#pragma once

namespace mi {

template <typename Expr> requires(concepts::tensor<Expr>) //
[[nodiscard, strong_inline]] constexpr auto operator+(Expr &&expr) {
  return TensorLambda( //
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr { return +expr(i); });
}

template <typename Expr> requires(concepts::tensor<Expr>) //
[[nodiscard, strong_inline]] constexpr auto operator-(Expr &&expr) {
  return TensorLambda( //
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr { return -expr(i); });
}

template <typename Expr> requires(concepts::tensor<Expr>) //
[[nodiscard, strong_inline]] constexpr auto operator~(Expr &&expr) {
  return TensorLambda( //
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr { return ~expr(i); });
}

template <typename Expr> requires(concepts::tensor<Expr>) //
[[nodiscard, strong_inline]] constexpr auto operator!(Expr &&expr) {
  return TensorLambda( //
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr { return !expr(i); });
}

template <typename ExprA, typename ExprB> requires(concepts::tensor_op2<ExprA, ExprB>) //
[[nodiscard, strong_inline]] constexpr auto operator+(ExprA &&exprA, ExprB &&exprB) {
  auto shape = [&]() constexpr {
    if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>)
      return equalShapes(exprA.shape, exprB.shape);
    else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>)
      return exprA.shape;
    else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>)
      return exprB.shape;
  };
  return TensorLambda( //
    shape(), [exprA = capture_in_tensor_lambda(std::forward<ExprA>(exprA)),
              exprB = capture_in_tensor_lambda(std::forward<ExprB>(exprB))](auto i) constexpr {
      if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>)
        return exprA(i) + exprB(i);
      else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>)
        return exprA(i) + exprB;
      else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>)
        return exprA + exprB(i);
    });
}

template <typename ExprA, typename ExprB> requires(concepts::tensor_op2<ExprA, ExprB>) //
[[nodiscard, strong_inline]] constexpr auto operator-(ExprA &&exprA, ExprB &&exprB) {
  auto shape = [&]() constexpr {
    if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>)
      return equalShapes(exprA.shape, exprB.shape);
    else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>)
      return exprA.shape;
    else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>)
      return exprB.shape;
  };
  return TensorLambda( //
    shape(), [exprA = capture_in_tensor_lambda(std::forward<ExprA>(exprA)),
              exprB = capture_in_tensor_lambda(std::forward<ExprB>(exprB))](auto i) constexpr {
      if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>)
        return exprA(i) - exprB(i);
      else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>)
        return exprA(i) - exprB;
      else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>)
        return exprA - exprB(i);
    });
}

template <typename ExprA, typename ExprB> requires(concepts::tensor_op2<ExprA, ExprB>) //
[[nodiscard, strong_inline]] constexpr auto operator*(ExprA &&exprA, ExprB &&exprB) {
  auto shape = [&]() constexpr {
    if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>)
      return equalShapes(exprA.shape, exprB.shape);
    else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>)
      return exprA.shape;
    else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>)
      return exprB.shape;
  };
  return TensorLambda( //
    shape(), [exprA = capture_in_tensor_lambda(std::forward<ExprA>(exprA)),
              exprB = capture_in_tensor_lambda(std::forward<ExprB>(exprB))](auto i) constexpr {
      if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>)
        return exprA(i) * exprB(i);
      else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>)
        return exprA(i) * exprB;
      else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>)
        return exprA * exprB(i);
    });
}

template <typename ExprA, typename ExprB> requires(concepts::tensor_op2<ExprA, ExprB>) //
[[nodiscard, strong_inline]] constexpr auto operator/(ExprA &&exprA, ExprB &&exprB) {
  auto shape = [&]() constexpr {
    if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>)
      return equalShapes(exprA.shape, exprB.shape);
    else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>)
      return exprA.shape;
    else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>)
      return exprB.shape;
  };
  return TensorLambda( //
    shape(), [exprA = capture_in_tensor_lambda(std::forward<ExprA>(exprA)),
              exprB = capture_in_tensor_lambda(std::forward<ExprB>(exprB))](auto i) constexpr {
      if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>)
        return exprA(i) / exprB(i);
      else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>)
        return exprA(i) / exprB;
      else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>)
        return exprA / exprB(i);
    });
}

template <typename ExprA, typename ExprB> requires(concepts::tensor_op2<ExprA, ExprB>) //
[[nodiscard, strong_inline]] constexpr auto operator%(ExprA &&exprA, ExprB &&exprB) {
  auto shape = [&]() constexpr {
    if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>)
      return equalShapes(exprA.shape, exprB.shape);
    else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>)
      return exprA.shape;
    else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>)
      return exprB.shape;
  };
  return TensorLambda( //
    shape(), [exprA = capture_in_tensor_lambda(std::forward<ExprA>(exprA)),
              exprB = capture_in_tensor_lambda(std::forward<ExprB>(exprB))](auto i) constexpr {
      if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>)
        return exprA(i) % exprB(i);
      else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>)
        return exprA(i) % exprB;
      else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>)
        return exprA % exprB(i);
    });
}

template <typename ExprA, typename ExprB> requires(concepts::tensor_op2<ExprA, ExprB>) //
[[nodiscard, strong_inline]] constexpr auto operator&(ExprA &&exprA, ExprB &&exprB) {
  auto shape = [&]() constexpr {
    if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>)
      return equalShapes(exprA.shape, exprB.shape);
    else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>)
      return exprA.shape;
    else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>)
      return exprB.shape;
  };
  return TensorLambda( //
    shape(), [exprA = capture_in_tensor_lambda(std::forward<ExprA>(exprA)),
              exprB = capture_in_tensor_lambda(std::forward<ExprB>(exprB))](auto i) constexpr {
      if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>)
        return exprA(i) & exprB(i);
      else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>)
        return exprA(i) & exprB;
      else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>)
        return exprA & exprB(i);
    });
}

template <typename ExprA, typename ExprB> requires(concepts::tensor_op2<ExprA, ExprB>) //
[[nodiscard, strong_inline]] constexpr auto operator|(ExprA &&exprA, ExprB &&exprB) {
  auto shape = [&]() constexpr {
    if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>)
      return equalShapes(exprA.shape, exprB.shape);
    else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>)
      return exprA.shape;
    else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>)
      return exprB.shape;
  };
  return TensorLambda( //
    shape(), [exprA = capture_in_tensor_lambda(std::forward<ExprA>(exprA)),
              exprB = capture_in_tensor_lambda(std::forward<ExprB>(exprB))](auto i) constexpr {
      if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>)
        return exprA(i) | exprB(i);
      else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>)
        return exprA(i) | exprB;
      else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>)
        return exprA | exprB(i);
    });
}

template <typename ExprA, typename ExprB> requires(concepts::tensor_op2<ExprA, ExprB>) //
[[nodiscard, strong_inline]] constexpr auto operator^(ExprA &&exprA, ExprB &&exprB) {
  auto shape = [&]() constexpr {
    if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>)
      return equalShapes(exprA.shape, exprB.shape);
    else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>)
      return exprA.shape;
    else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>)
      return exprB.shape;
  };
  return TensorLambda( //
    shape(), [exprA = capture_in_tensor_lambda(std::forward<ExprA>(exprA)),
              exprB = capture_in_tensor_lambda(std::forward<ExprB>(exprB))](auto i) constexpr {
      if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>)
        return exprA(i) ^ exprB(i);
      else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>)
        return exprA(i) ^ exprB;
      else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>)
        return exprA ^ exprB(i);
    });
}

template <typename ExprA, typename ExprB> requires(concepts::tensor_op2<ExprA, ExprB>) //
[[nodiscard, strong_inline]] constexpr auto operator>>(ExprA &&exprA, ExprB &&exprB) {
  auto shape = [&]() constexpr {
    if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>)
      return equalShapes(exprA.shape, exprB.shape);
    else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>)
      return exprA.shape;
    else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>)
      return exprB.shape;
  };
  return TensorLambda( //
    shape(), [exprA = capture_in_tensor_lambda(std::forward<ExprA>(exprA)),
              exprB = capture_in_tensor_lambda(std::forward<ExprB>(exprB))](auto i) constexpr {
      if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>)
        return exprA(i) >> exprB(i);
      else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>)
        return exprA(i) >> exprB;
      else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>)
        return exprA >> exprB(i);
    });
}

template <typename ExprA, typename ExprB> requires(concepts::tensor_op2<ExprA, ExprB>) //
[[nodiscard, strong_inline]] constexpr auto operator<<(ExprA &&exprA, ExprB &&exprB) {
  auto shape = [&]() constexpr {
    if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>)
      return equalShapes(exprA.shape, exprB.shape);
    else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>)
      return exprA.shape;
    else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>)
      return exprB.shape;
  };
  return TensorLambda( //
    shape(), [exprA = capture_in_tensor_lambda(std::forward<ExprA>(exprA)),
              exprB = capture_in_tensor_lambda(std::forward<ExprB>(exprB))](auto i) constexpr {
      if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>)
        return exprA(i) << exprB(i);
      else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>)
        return exprA(i) << exprB;
      else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>)
        return exprA << exprB(i);
    });
}

template <typename ExprA, typename ExprB> requires(concepts::tensor_op2<ExprA, ExprB>) //
[[nodiscard, strong_inline]] constexpr auto operator&&(ExprA &&exprA, ExprB &&exprB) {
  auto shape = [&]() constexpr {
    if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>)
      return equalShapes(exprA.shape, exprB.shape);
    else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>)
      return exprA.shape;
    else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>)
      return exprB.shape;
  };
  return TensorLambda( //
    shape(), [exprA = capture_in_tensor_lambda(std::forward<ExprA>(exprA)),
              exprB = capture_in_tensor_lambda(std::forward<ExprB>(exprB))](auto i) constexpr {
      if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>)
        return exprA(i) && exprB(i);
      else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>)
        return exprA(i) && exprB;
      else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>)
        return exprA && exprB(i);
    });
}

template <typename ExprA, typename ExprB> requires(concepts::tensor_op2<ExprA, ExprB>) //
[[nodiscard, strong_inline]] constexpr auto operator||(ExprA &&exprA, ExprB &&exprB) {
  auto shape = [&]() constexpr {
    if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>)
      return equalShapes(exprA.shape, exprB.shape);
    else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>)
      return exprA.shape;
    else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>)
      return exprB.shape;
  };
  return TensorLambda( //
    shape(), [exprA = capture_in_tensor_lambda(std::forward<ExprA>(exprA)),
              exprB = capture_in_tensor_lambda(std::forward<ExprB>(exprB))](auto i) constexpr {
      if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>)
        return exprA(i) || exprB(i);
      else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>)
        return exprA(i) || exprB;
      else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>)
        return exprA || exprB(i);
    });
}

template <typename ExprA, typename ExprB> requires(concepts::tensor_op2<ExprA, ExprB>) //
[[nodiscard, strong_inline]] constexpr auto operator==(ExprA &&exprA, ExprB &&exprB) {
  auto shape = [&]() constexpr {
    if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>)
      return equalShapes(exprA.shape, exprB.shape);
    else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>)
      return exprA.shape;
    else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>)
      return exprB.shape;
  };
  return TensorLambda( //
    shape(), [exprA = capture_in_tensor_lambda(std::forward<ExprA>(exprA)),
              exprB = capture_in_tensor_lambda(std::forward<ExprB>(exprB))](auto i) constexpr {
      if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>)
        return exprA(i) == exprB(i);
      else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>)
        return exprA(i) == exprB;
      else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>)
        return exprA == exprB(i);
    });
}

template <typename ExprA, typename ExprB> requires(concepts::tensor_op2<ExprA, ExprB>) //
[[nodiscard, strong_inline]] constexpr auto operator!=(ExprA &&exprA, ExprB &&exprB) {
  auto shape = [&]() constexpr {
    if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>)
      return equalShapes(exprA.shape, exprB.shape);
    else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>)
      return exprA.shape;
    else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>)
      return exprB.shape;
  };
  return TensorLambda( //
    shape(), [exprA = capture_in_tensor_lambda(std::forward<ExprA>(exprA)),
              exprB = capture_in_tensor_lambda(std::forward<ExprB>(exprB))](auto i) constexpr {
      if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>)
        return exprA(i) != exprB(i);
      else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>)
        return exprA(i) != exprB;
      else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>)
        return exprA != exprB(i);
    });
}

template <typename ExprA, typename ExprB> requires(concepts::tensor_op2<ExprA, ExprB>) //
[[nodiscard, strong_inline]] constexpr auto operator<(ExprA &&exprA, ExprB &&exprB) {
  auto shape = [&]() constexpr {
    if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>)
      return equalShapes(exprA.shape, exprB.shape);
    else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>)
      return exprA.shape;
    else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>)
      return exprB.shape;
  };
  return TensorLambda( //
    shape(), [exprA = capture_in_tensor_lambda(std::forward<ExprA>(exprA)),
              exprB = capture_in_tensor_lambda(std::forward<ExprB>(exprB))](auto i) constexpr {
      if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>)
        return exprA(i) < exprB(i);
      else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>)
        return exprA(i) < exprB;
      else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>)
        return exprA < exprB(i);
    });
}

template <typename ExprA, typename ExprB> requires(concepts::tensor_op2<ExprA, ExprB>) //
[[nodiscard, strong_inline]] constexpr auto operator>(ExprA &&exprA, ExprB &&exprB) {
  auto shape = [&]() constexpr {
    if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>)
      return equalShapes(exprA.shape, exprB.shape);
    else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>)
      return exprA.shape;
    else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>)
      return exprB.shape;
  };
  return TensorLambda( //
    shape(), [exprA = capture_in_tensor_lambda(std::forward<ExprA>(exprA)),
              exprB = capture_in_tensor_lambda(std::forward<ExprB>(exprB))](auto i) constexpr {
      if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>)
        return exprA(i) > exprB(i);
      else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>)
        return exprA(i) > exprB;
      else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>)
        return exprA > exprB(i);
    });
}

template <typename ExprA, typename ExprB> requires(concepts::tensor_op2<ExprA, ExprB>) //
[[nodiscard, strong_inline]] constexpr auto operator<=(ExprA &&exprA, ExprB &&exprB) {
  auto shape = [&]() constexpr {
    if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>)
      return equalShapes(exprA.shape, exprB.shape);
    else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>)
      return exprA.shape;
    else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>)
      return exprB.shape;
  };
  return TensorLambda( //
    shape(), [exprA = capture_in_tensor_lambda(std::forward<ExprA>(exprA)),
              exprB = capture_in_tensor_lambda(std::forward<ExprB>(exprB))](auto i) constexpr {
      if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>)
        return exprA(i) <= exprB(i);
      else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>)
        return exprA(i) <= exprB;
      else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>)
        return exprA <= exprB(i);
    });
}

template <typename ExprA, typename ExprB> requires(concepts::tensor_op2<ExprA, ExprB>) //
[[nodiscard, strong_inline]] constexpr auto operator>=(ExprA &&exprA, ExprB &&exprB) {
  auto shape = [&]() constexpr {
    if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>)
      return equalShapes(exprA.shape, exprB.shape);
    else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>)
      return exprA.shape;
    else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>)
      return exprB.shape;
  };
  return TensorLambda( //
    shape(), [exprA = capture_in_tensor_lambda(std::forward<ExprA>(exprA)),
              exprB = capture_in_tensor_lambda(std::forward<ExprB>(exprB))](auto i) constexpr {
      if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>)
        return exprA(i) >= exprB(i);
      else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>)
        return exprA(i) >= exprB;
      else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>)
        return exprA >= exprB(i);
    });
}

template <typename ExprA, typename ExprB> requires(concepts::lvalue_tensor<ExprA> && concepts::tensor_op2<ExprA, ExprB>) //
[[strong_inline]] constexpr decltype(auto) operator+=(ExprA &&exprA, ExprB &&exprB) {
  return exprA.assign([&]() constexpr {
    if constexpr (concepts::number<ExprB> || concepts::lvalue_tensor<ExprB>)
      return exprA + exprB;
    else
      return (exprA + exprB).doIt();
  }());
}

template <typename ExprA, typename ExprB> requires(concepts::lvalue_tensor<ExprA> && concepts::tensor_op2<ExprA, ExprB>) //
[[strong_inline]] constexpr decltype(auto) operator-=(ExprA &&exprA, ExprB &&exprB) {
  return exprA.assign([&]() constexpr {
    if constexpr (concepts::number<ExprB> || concepts::lvalue_tensor<ExprB>)
      return exprA - exprB;
    else
      return (exprA - exprB).doIt();
  }());
}

template <typename ExprA, typename ExprB> requires(concepts::lvalue_tensor<ExprA> && concepts::tensor_op2<ExprA, ExprB>) //
[[strong_inline]] constexpr decltype(auto) operator*=(ExprA &&exprA, ExprB &&exprB) {
  return exprA.assign([&]() constexpr {
    if constexpr (concepts::number<ExprB> || concepts::lvalue_tensor<ExprB>)
      return exprA * exprB;
    else
      return (exprA * exprB).doIt();
  }());
}

template <typename ExprA, typename ExprB> requires(concepts::lvalue_tensor<ExprA> && concepts::tensor_op2<ExprA, ExprB>) //
[[strong_inline]] constexpr decltype(auto) operator/=(ExprA &&exprA, ExprB &&exprB) {
  return exprA.assign([&]() constexpr {
    if constexpr (concepts::number<ExprB> || concepts::lvalue_tensor<ExprB>)
      return exprA / exprB;
    else
      return (exprA / exprB).doIt();
  }());
}

template <typename ExprA, typename ExprB> requires(concepts::lvalue_tensor<ExprA> && concepts::tensor_op2<ExprA, ExprB>) //
[[strong_inline]] constexpr decltype(auto) operator%=(ExprA &&exprA, ExprB &&exprB) {
  return exprA.assign([&]() constexpr {
    if constexpr (concepts::number<ExprB> || concepts::lvalue_tensor<ExprB>)
      return exprA % exprB;
    else
      return (exprA % exprB).doIt();
  }());
}

template <typename ExprA, typename ExprB> requires(concepts::lvalue_tensor<ExprA> && concepts::tensor_op2<ExprA, ExprB>) //
[[strong_inline]] constexpr decltype(auto) operator&=(ExprA &&exprA, ExprB &&exprB) {
  return exprA.assign([&]() constexpr {
    if constexpr (concepts::number<ExprB> || concepts::lvalue_tensor<ExprB>)
      return exprA & exprB;
    else
      return (exprA & exprB).doIt();
  }());
}

template <typename ExprA, typename ExprB> requires(concepts::lvalue_tensor<ExprA> && concepts::tensor_op2<ExprA, ExprB>) //
[[strong_inline]] constexpr decltype(auto) operator|=(ExprA &&exprA, ExprB &&exprB) {
  return exprA.assign([&]() constexpr {
    if constexpr (concepts::number<ExprB> || concepts::lvalue_tensor<ExprB>)
      return exprA | exprB;
    else
      return (exprA | exprB).doIt();
  }());
}

template <typename ExprA, typename ExprB> requires(concepts::lvalue_tensor<ExprA> && concepts::tensor_op2<ExprA, ExprB>) //
[[strong_inline]] constexpr decltype(auto) operator^=(ExprA &&exprA, ExprB &&exprB) {
  return exprA.assign([&]() constexpr {
    if constexpr (concepts::number<ExprB> || concepts::lvalue_tensor<ExprB>)
      return exprA ^ exprB;
    else
      return (exprA ^ exprB).doIt();
  }());
}

template <typename ExprA, typename ExprB> requires(concepts::lvalue_tensor<ExprA> && concepts::tensor_op2<ExprA, ExprB>) //
[[strong_inline]] constexpr decltype(auto) operator>>=(ExprA &&exprA, ExprB &&exprB) {
  return exprA.assign([&]() constexpr {
    if constexpr (concepts::number<ExprB> || concepts::lvalue_tensor<ExprB>)
      return exprA >> exprB;
    else
      return (exprA >> exprB).doIt();
  }());
}

template <typename ExprA, typename ExprB> requires(concepts::lvalue_tensor<ExprA> && concepts::tensor_op2<ExprA, ExprB>) //
[[strong_inline]] constexpr decltype(auto) operator<<=(ExprA &&exprA, ExprB &&exprB) {
  return exprA.assign([&]() constexpr {
    if constexpr (concepts::number<ExprB> || concepts::lvalue_tensor<ExprB>)
      return exprA << exprB;
    else
      return (exprA << exprB).doIt();
  }());
}

} // namespace mi
