#pragma once

namespace mi {

namespace stats {

template <typename Expr> requires(concepts::tensor_vector<Expr>)
[[nodiscard, strong_inline]] constexpr auto mean(Expr &&expr) noexcept {
  using Float = to_float_t<value_type_t<Expr>>;
  using Field = to_field_t<value_type_t<Expr>>;
  return expr.empty() ? Field() : Field((Float(1) / expr.size()) * expr.sum());
}

template <typename Expr> requires(concepts::tensor_vector<Expr>)
[[nodiscard, strong_inline]] constexpr auto variance(Expr &&expr) noexcept {
  auto values = expr.doIt();
  using Float = to_float_t<value_type_t<Expr>>;
  using Field = to_field_t<value_type_t<Expr>>;
  return values.size() < 2 ? Field() : (Float(1) / (values.size() - 1)) * sqr(values - mean(values)).sum();
}

template <typename Expr> requires(concepts::tensor_vector<Expr>)
[[nodiscard, strong_inline]] inline auto stddev(Expr &&expr) noexcept {
  return sqrt(variance(auto_forward(expr)));
}

} // namespace stats

} // namespace mi
