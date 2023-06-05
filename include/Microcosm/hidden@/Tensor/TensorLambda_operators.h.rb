OP1 = ['+', '-', '~', '!']
OP2 = ['+', '-', '*', '/', '%', '&', '|', '^', '>>', '<<']
OPC = ['&&', '||', '==', '!=', '<', '>', '<=', '>=']

puts <<STR
\#pragma once

namespace mi {

STR

for op1 in OP1
    puts <<STR
template <typename Expr> requires(concepts::tensor<Expr>) //
[[nodiscard, strong_inline]] constexpr auto operator#{op1}(Expr&& expr) {
  return TensorLambda( //
    expr.shape, [expr = capture_in_tensor_lambda(std::forward<Expr>(expr))](auto i) constexpr {
    return #{op1}expr(i);
  });
}

STR
end

for op2 in (OP2 + OPC)
    puts <<STR
template <typename ExprA, typename ExprB> requires(concepts::tensor_op2<ExprA, ExprB>) //
[[nodiscard, strong_inline]] constexpr auto operator#{op2}(ExprA&& exprA, ExprB&& exprB) {
  auto shape = [&]() constexpr {
    if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>)
      return equalShapes(exprA.shape, exprB.shape);
    else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>)
      return exprA.shape;
    else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>)
      return exprB.shape;
  };
  return TensorLambda( //
    shape(),
    [exprA = capture_in_tensor_lambda(std::forward<ExprA>(exprA)),
     exprB = capture_in_tensor_lambda(std::forward<ExprB>(exprB))](auto i) constexpr {
      if constexpr (concepts::tensor_tensor_op2<ExprA, ExprB>)
        return exprA(i) #{op2} exprB(i);
      else if constexpr (concepts::tensor_number_op2<ExprA, ExprB>)
        return exprA(i) #{op2} exprB;
      else if constexpr (concepts::tensor_number_op2<ExprB, ExprA>)
        return exprA #{op2} exprB(i);
    });
}

STR
end

for op2 in OP2
  puts <<STR
template <typename ExprA, typename ExprB> requires(concepts::lvalue_tensor<ExprA> && concepts::tensor_op2<ExprA, ExprB>) //
[[strong_inline]] constexpr decltype(auto) operator#{op2}=(ExprA&& exprA, ExprB&& exprB) {
  return exprA.assign([&]() constexpr {
    if constexpr (concepts::number<ExprB> || concepts::lvalue_tensor<ExprB>)
      return exprA #{op2} exprB;
    else
      return (exprA #{op2} exprB).execute();
  }());
}

STR
end

puts <<STR
} // namespace mi
STR
