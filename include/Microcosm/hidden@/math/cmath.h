#pragma once

namespace mi {

[[gnu::always_inline]] inline auto abs(auto x) -> decltype(std::abs(x)) { return std::abs(x); }

[[gnu::always_inline]] inline auto arg(auto x) -> decltype(std::arg(x)) { return std::arg(x); }

[[gnu::always_inline]] inline auto fabs(auto x) -> decltype(std::fabs(x)) { return std::fabs(x); }

[[gnu::always_inline]] inline auto fma(auto x, auto y, auto z) -> decltype(std::fma(x, y, z)) { return std::fma(x, y, z); }

[[gnu::always_inline]] inline auto fmin(auto x, auto y) -> decltype(std::fmin(x, y)) { return std::fmin(x, y); }

[[gnu::always_inline]] inline auto fmax(auto x, auto y) -> decltype(std::fmax(x, y)) { return std::fmax(x, y); }

[[gnu::always_inline]] inline auto fdim(auto x, auto y) -> decltype(std::fdim(x, y)) { return std::fdim(x, y); }

[[gnu::always_inline]] inline auto fmod(auto x, auto y) -> decltype(std::fmod(x, y)) { return std::fmod(x, y); }

[[gnu::always_inline]] inline auto remquo(auto x, auto y, int *q) -> decltype(std::remquo(x, y, q)) {
  return std::remquo(x, y, q);
}

[[gnu::always_inline]] inline auto remainder(auto x, auto y) -> decltype(std::remainder(x, y)) { return std::remainder(x, y); }

[[gnu::always_inline]] inline auto nearbyint(auto x) -> decltype(std::nearbyint(x)) { return std::nearbyint(x); }

[[gnu::always_inline]] inline auto floor(auto x) -> decltype(std::floor(x)) { return std::floor(x); }

[[gnu::always_inline]] inline auto ceil(auto x) -> decltype(std::ceil(x)) { return std::ceil(x); }

[[gnu::always_inline]] inline auto trunc(auto x) -> decltype(std::trunc(x)) { return std::trunc(x); }

[[gnu::always_inline]] inline auto round(auto x) -> decltype(std::round(x)) { return std::round(x); }

[[gnu::always_inline]] inline auto rint(auto x) -> decltype(std::rint(x)) { return std::rint(x); }

[[gnu::always_inline]] inline auto lrint(auto x) -> decltype(std::lrint(x)) { return std::lrint(x); }

[[gnu::always_inline]] inline auto llrint(auto x) -> decltype(std::llrint(x)) { return std::llrint(x); }

[[gnu::always_inline]] inline auto lround(auto x) -> decltype(std::lround(x)) { return std::lround(x); }

[[gnu::always_inline]] inline auto llround(auto x) -> decltype(std::llround(x)) { return std::llround(x); }

[[gnu::always_inline]] inline auto frexp(auto x, int *p) -> decltype(std::frexp(x, p)) { return std::frexp(x, p); }

[[gnu::always_inline]] inline auto ldexp(auto x, int p) -> decltype(std::ldexp(x, p)) { return std::ldexp(x, p); }

[[gnu::always_inline]] inline auto logb(auto x) -> decltype(std::logb(x)) { return std::logb(x); }

[[gnu::always_inline]] inline auto ilogb(auto x) -> decltype(std::ilogb(x)) { return std::ilogb(x); }

[[gnu::always_inline]] inline auto scalbn(auto x, int p) -> decltype(std::scalbn(x, p)) { return std::scalbn(x, p); }

[[gnu::always_inline]] inline auto scalbln(auto x, long p) -> decltype(std::scalbln(x, p)) { return std::scalbln(x, p); }

[[gnu::always_inline]] inline auto modf(auto x, auto *p) -> decltype(std::modf(x, p)) { return std::modf(x, p); }

[[gnu::always_inline]] inline auto nextafter(auto x, auto y) -> decltype(std::nextafter(x, y)) { return std::nextafter(x, y); }

[[gnu::always_inline]] inline auto nexttoward(auto x, long double y) -> decltype(std::nexttoward(x, y)) {
  return std::nexttoward(x, y);
}

[[gnu::always_inline]] inline auto copysign(auto x, auto y) -> decltype(std::copysign(x, y)) { return std::copysign(x, y); }

[[gnu::always_inline]] inline auto signbit(auto x) -> decltype(std::signbit(x)) { return std::signbit(x); }

[[gnu::always_inline]] inline auto isnan(auto x) -> decltype(std::isnan(x)) { return std::isnan(x); }

[[gnu::always_inline]] inline auto isinf(auto x) -> decltype(std::isinf(x)) { return std::isinf(x); }

[[gnu::always_inline]] inline auto isfinite(auto x) -> decltype(std::isfinite(x)) { return std::isfinite(x); }

[[gnu::always_inline]] inline auto isnormal(auto x) -> decltype(std::isnormal(x)) { return std::isnormal(x); }

[[gnu::always_inline]] inline auto exp(auto x) -> decltype(std::exp(x)) { return std::exp(x); }

[[gnu::always_inline]] inline auto log(auto x) -> decltype(std::log(x)) { return std::log(x); }

[[gnu::always_inline]] inline auto exp2(auto x) -> decltype(std::exp2(x)) { return std::exp2(x); }

[[gnu::always_inline]] inline auto log2(auto x) -> decltype(std::log2(x)) { return std::log2(x); }

[[gnu::always_inline]] inline auto log10(auto x) -> decltype(std::log10(x)) { return std::log10(x); }

[[gnu::always_inline]] inline auto expm1(auto x) -> decltype(std::expm1(x)) { return std::expm1(x); }

[[gnu::always_inline]] inline auto log1p(auto x) -> decltype(std::log1p(x)) { return std::log1p(x); }

[[gnu::always_inline]] inline auto pow(auto x, auto y) -> decltype(std::pow(x, y)) { return std::pow(x, y); }

[[gnu::always_inline]] inline auto sqrt(auto x) -> decltype(std::sqrt(x)) { return std::sqrt(x); }

[[gnu::always_inline]] inline auto cbrt(auto x) -> decltype(std::cbrt(x)) { return std::cbrt(x); }

[[gnu::always_inline]] inline auto hypot(auto x, auto y) -> decltype(std::hypot(x, y)) { return std::hypot(x, y); }

[[gnu::always_inline]] inline auto erf(auto x) -> decltype(std::erf(x)) { return std::erf(x); }

[[gnu::always_inline]] inline auto erfc(auto x) -> decltype(std::erfc(x)) { return std::erfc(x); }

[[gnu::always_inline]] inline auto lgamma(auto x) -> decltype(std::lgamma(x)) { return std::lgamma(x); }

[[gnu::always_inline]] inline auto tgamma(auto x) -> decltype(std::tgamma(x)) { return std::tgamma(x); }

[[gnu::always_inline]] inline auto sin(auto x) -> decltype(std::sin(x)) { return std::sin(x); }

[[gnu::always_inline]] inline auto cos(auto x) -> decltype(std::cos(x)) { return std::cos(x); }

[[gnu::always_inline]] inline auto tan(auto x) -> decltype(std::tan(x)) { return std::tan(x); }

[[gnu::always_inline]] inline auto asin(auto x) -> decltype(std::asin(x)) { return std::asin(x); }

[[gnu::always_inline]] inline auto acos(auto x) -> decltype(std::acos(x)) { return std::acos(x); }

[[gnu::always_inline]] inline auto atan(auto x) -> decltype(std::atan(x)) { return std::atan(x); }

[[gnu::always_inline]] inline auto atan2(auto y, auto x) -> decltype(std::atan2(y, x)) { return std::atan2(y, x); }

[[gnu::always_inline]] inline auto sinh(auto x) -> decltype(std::sinh(x)) { return std::sinh(x); }

[[gnu::always_inline]] inline auto cosh(auto x) -> decltype(std::cosh(x)) { return std::cosh(x); }

[[gnu::always_inline]] inline auto tanh(auto x) -> decltype(std::tanh(x)) { return std::tanh(x); }

[[gnu::always_inline]] inline auto asinh(auto x) -> decltype(std::asinh(x)) { return std::asinh(x); }

[[gnu::always_inline]] inline auto acosh(auto x) -> decltype(std::acosh(x)) { return std::acosh(x); }

[[gnu::always_inline]] inline auto atanh(auto x) -> decltype(std::atanh(x)) { return std::atanh(x); }

} // namespace mi
