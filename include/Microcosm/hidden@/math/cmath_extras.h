/*-*- C++ -*-*/
#pragma once

#include "../utility/ArrayLike.h"
#include "../utility/algorithm.h"
#include <numeric>

namespace mi {

[[nodiscard]] constexpr auto real(concepts::arithmetic auto value) noexcept { return value; }

[[nodiscard]] constexpr auto imag(concepts::arithmetic auto value) noexcept { return decltype(value)(0); }

[[nodiscard]] constexpr auto conj(concepts::arithmetic auto value) noexcept { return value; }

[[nodiscard]] constexpr auto norm(concepts::arithmetic auto value) noexcept { return value * value; }

[[nodiscard]] constexpr auto real(concepts::complex auto value) noexcept { return value.real(); }

[[nodiscard]] constexpr auto imag(concepts::complex auto value) noexcept { return value.imag(); }

[[nodiscard]] constexpr auto conj(concepts::complex auto value) noexcept { return decltype(value)(real(value), -imag(value)); }

[[nodiscard]] constexpr auto norm(concepts::complex auto value) noexcept { return real(value) * real(value) + imag(value) * imag(value); }

[[nodiscard]] inline bool isinf(concepts::complex auto value) noexcept { return std::isinf(real(value)) || std::isinf(imag(value)); }

[[nodiscard]] inline bool isnan(concepts::complex auto value) noexcept { return std::isnan(real(value)) || std::isnan(imag(value)); }

[[nodiscard]] inline bool isfinite(concepts::complex auto value) noexcept { return std::isfinite(real(value)) && std::isfinite(imag(value)); }

[[nodiscard]] inline bool isnormal(concepts::complex auto value) noexcept { return std::isnormal(real(value)) && std::isnormal(imag(value)); }

template <typename... Args> requires(sizeof...(Args) > 1) [[nodiscard]] inline bool isinf(Args &&...args) noexcept { return (isinf(std::forward<Args>(args)) || ...); }

template <typename... Args> requires(sizeof...(Args) > 1) [[nodiscard]] inline bool isnan(Args &&...args) noexcept { return (isnan(std::forward<Args>(args)) || ...); }

template <typename... Args> requires(sizeof...(Args) > 1) [[nodiscard]] inline bool isfinite(Args &&...args) noexcept { return (isfinite(std::forward<Args>(args)) && ...); }

template <typename... Args> requires(sizeof...(Args) > 1) [[nodiscard]] inline bool isnormal(Args &&...args) noexcept { return (isnormal(std::forward<Args>(args)) && ...); }

[[nodiscard]] inline auto sign(concepts::arithmetic auto value) noexcept { return std::copysign(decltype(value)(1), value); }

[[nodiscard]] inline auto sign(concepts::complex auto value) noexcept {
  if (imag(value) == 0) [[unlikely]]
    return decltype(value)(sign(real(value)), imag(value));
  else
    return decltype(value)(value / abs(value));
}

enum class FuzzySign : int { Negative = -1, Zero = 0, Positive = +1 };

[[nodiscard]] constexpr auto operator<=>(FuzzySign signA, FuzzySign signB) noexcept { return int(signA) <=> int(signB); }

[[nodiscard]] constexpr bool operator==(FuzzySign signA, FuzzySign signB) noexcept { return int(signA) == int(signB); }

[[nodiscard]] constexpr bool operator!=(FuzzySign signA, FuzzySign signB) noexcept { return int(signA) != int(signB); }

template <std::floating_point Float, Float Thresh = constants::Eps<Float>> [[nodiscard]] constexpr auto fuzzySign(Float value) noexcept { return abs(value) < Thresh ? FuzzySign::Zero : std::signbit(value) ? FuzzySign::Negative : FuzzySign::Positive; }

/// Soft-sign activation.
[[nodiscard]] inline auto softSign(std::floating_point auto value) noexcept { return value / (1 + abs(value)); }

/// Soft-plus activation.
[[nodiscard]] inline auto softPlus(std::floating_point auto value) noexcept { return log1p(exp(value)); }

/// Saturate, meaning clamp onto [0, 1].
[[nodiscard]] constexpr auto saturate(std::floating_point auto value) noexcept { return clamp(value, 0, 1); }

/// Fast floor by int casting.
template <std::integral Int = int, std::floating_point Float> [[nodiscard]] constexpr Int fastFloor(Float value) noexcept {
  Int intValue = static_cast<Int>(value);
  return intValue - (static_cast<Float>(intValue) > value ? 1 : 0);
}

/// Fast ceil by int casting.
template <std::integral Int = int, std::floating_point Float> [[nodiscard]] constexpr Int fastCeil(Float value) noexcept {
  Int intValue = static_cast<Int>(value);
  return intValue + (static_cast<Float>(intValue) < value ? 1 : 0);
}

/// Fast round by int casting.
template <std::integral Int = int, std::floating_point Float> [[nodiscard]] constexpr Int fastRound(Float value) noexcept { return fastFloor<Int>(value + Float(0.5)); }

/// Fast trunc by int casting.
template <std::integral Int = int, std::floating_point Float> [[nodiscard]] constexpr Int fastTrunc(Float value) noexcept { return static_cast<Int>(value); }

/// Fraction with respect to floor.
template <std::floating_point Float> [[nodiscard]] constexpr Float fastFract(Float value, int *intPart) noexcept {
  int intPart0{};
  if (intPart == nullptr) intPart = &intPart0;
  return value - (*intPart = fastFloor(value));
}

/// Fraction with respect to floor.
template <std::floating_point Float> [[nodiscard]] constexpr Float fastFract(Float value) noexcept { return value - fastFloor(value); }

/// Is relatively tiny? (a much less than b)
[[nodiscard]] inline bool isTiny(std::floating_point auto valueA, std::floating_point auto valueB) noexcept {
  volatile auto valueX = valueA;
  volatile auto valueY = valueB;
  volatile auto valueZ = valueX + valueY;
  return valueZ == valueY;
}

/// Is relatively huge? (a much greater than b)
[[nodiscard]] inline bool isHuge(std::floating_point auto valueA, std::floating_point auto valueB) noexcept { return isTiny(valueB, valueA); }

/// Finite or alternative.
[[nodiscard]] inline auto finiteOr(concepts::arithmetic auto value, concepts::arithmetic auto value0) noexcept { return isfinite(value) ? value : value0; }

/// Finite or zero.
[[nodiscard]] inline auto finiteOrZero(concepts::arithmetic auto value) noexcept { return finiteOr(value, std::decay_t<decltype(value)>(0)); }

/// Safe square root.
[[nodiscard]] inline auto safeSqrt(concepts::arithmetic auto value) noexcept { return sqrt(max(value, 0)); }

/// Safe ratio of numbers, protects against NaNs due to 0/0.
[[nodiscard]] inline auto safeRatio(const auto &numer, const auto &denom) noexcept { return numer == std::decay_t<decltype(numer)>(0) ? numer : numer / denom; }

/// Increment float to next representable value.
template <std::floating_point Float> [[nodiscard]] inline Float nextFloat(Float value) noexcept {
  if constexpr (std::same_as<Float, float> && std::numeric_limits<float>::is_iec559) {
    uint32_t bits = std::bit_cast<uint32_t>(value);
    if (bits != (0x7f8UL << 20)) {       // Not +Inf?
      if (bits == (1UL << 31)) bits = 0; // Ignore -0.0.
      if (bits & (1UL << 31))
        --bits;
      else
        ++bits;
    }
    return std::bit_cast<float>(bits);
  } else if constexpr (std::same_as<Float, double> && std::numeric_limits<double>::is_iec559) {
    uint64_t bits = std::bit_cast<uint64_t>(value);
    if (bits != (0x7ffULL << 52)) {       // Not +Inf?
      if (bits == (1ULL << 63)) bits = 0; // Ignore -0.0.
      if (bits & (1ULL << 63))
        --bits;
      else
        ++bits;
    }
    return std::bit_cast<double>(bits);
  } else {
    return std::nextafter(value, +constants::Inf<Float>);
  }
}

/// Decrement float to next representable value.
template <std::floating_point Float> [[nodiscard]] inline Float prevFloat(Float value) noexcept {
  if constexpr (std::same_as<Float, float> && std::numeric_limits<float>::is_iec559) {
    uint32_t bits = std::bit_cast<uint32_t>(value);
    if (bits != (0xff8UL << 20)) {       // Not -Inf?
      if (bits == 0) bits = (1UL << 31); // Ignore +0.0.
      if (bits & (1UL << 31))
        ++bits;
      else
        --bits;
    }
    return std::bit_cast<float>(bits);
  } else if constexpr (std::same_as<Float, double> && std::numeric_limits<double>::is_iec559) {
    uint64_t bits = std::bit_cast<uint64_t>(value);
    if (bits != (0xfffULL << 52)) {       // Not -Inf?
      if (bits == 0) bits = (1ULL << 63); // Ignore +0.0.
      if (bits & (1ULL << 63))
        ++bits;
      else
        --bits;
    }
    return std::bit_cast<double>(bits);
  } else {
    return std::nextafter(value, -constants::Inf<Float>);
  }
}

/// Calculate sine of the product of pi with the given value.
[[nodiscard]] inline auto sinPi(concepts::arithmetic auto value) noexcept {
  int quo = 0;
  using Float = to_float_t<decltype(value)>;
  Float rem = std::remquo(Float(value), Float(1), &quo);
  Float res = std::sin(constants::Pi<Float> * rem);
  if (unsigned(quo) & 1) res = -res;
  return res;
}

/// Calculate cosine of the product of pi with the given value.
[[nodiscard]] inline auto cosPi(concepts::arithmetic auto value) noexcept {
  int quo = 0;
  using Float = to_float_t<decltype(value)>;
  Float rem = std::remquo(Float(value), Float(1), &quo);
  Float res = std::cos(constants::Pi<Float> * rem);
  if (unsigned(quo) & 1) res = -res;
  return res;
}

/// Calculate sine and cosine of the product of pi with the given value, return as pair.
[[nodiscard]] inline auto sinCosPi(concepts::arithmetic auto value) noexcept {
  int quo = 0;
  using Float = to_float_t<decltype(value)>;
  Float rem = std::remquo(Float(value), Float(1), &quo);
  Float sinValue = std::sin(constants::Pi<Float> * rem);
  Float cosValue = std::cos(constants::Pi<Float> * rem);
  if (unsigned(quo) & 1) {
    sinValue = -sinValue;
    cosValue = -cosValue;
  }
  return std::make_pair(sinValue, cosValue);
}

/// An overload of `exp2()` for `std::complex`, not provided by STL.
template <std::floating_point Float> [[nodiscard, strong_inline]] inline auto exp2(const std::complex<Float> &value) noexcept { return std::exp(constants::LnTwo<Float> * value); }

/// An overload of `log2()` for `std::complex`, not provided by STL.
template <std::floating_point Float> [[nodiscard, strong_inline]] inline auto log2(const std::complex<Float> &value) noexcept { return std::log(value) / constants::LnTwo<Float>; }

/// An overload of `cbrt()` for `std::complex`, not provided by STL.
template <std::floating_point Float> [[nodiscard, strong_inline]] inline auto cbrt(const std::complex<Float> &value) noexcept { return std::pow(value, std::complex<Float>(1.0 / 3.0)); }

// TODO
template <std::floating_point Float> [[nodiscard, strong_inline]] inline auto atan2(const std::complex<Float> &valueY, const std::complex<Float> &valueX) noexcept {
  return atan(valueY / valueX);
}

/// Error function inverse.
template <std::floating_point Float> [[nodiscard]] inline Float erfInverse(Float valueY) noexcept {
  Float valueW = -std::log((1 - valueY) * (1 + valueY));
  Float valueX = 0;
  if (valueW < Float(5)) {
    valueW = valueW - Float(2.5);
    valueX = std::fma(valueW, Float(+2.81022636e-08), Float(+3.43273939e-7));
    valueX = std::fma(valueW, valueX, Float(-3.52338770e-6));
    valueX = std::fma(valueW, valueX, Float(-4.39150654e-6));
    valueX = std::fma(valueW, valueX, Float(+2.18580870e-4));
    valueX = std::fma(valueW, valueX, Float(-1.25372503e-3));
    valueX = std::fma(valueW, valueX, Float(-4.17768164e-3));
    valueX = std::fma(valueW, valueX, Float(+2.46640727e-1));
    valueX = std::fma(valueW, valueX, Float(+1.50140941));
  } else {
    valueW = std::sqrt(valueW) - 3;
    valueX = std::fma(valueX, Float(-2.00214257e-4), Float(+1.00950558e-4));
    valueX = std::fma(valueW, valueX, Float(+1.34934322e-3));
    valueX = std::fma(valueW, valueX, Float(-3.67342844e-3));
    valueX = std::fma(valueW, valueX, Float(+5.73950773e-3));
    valueX = std::fma(valueW, valueX, Float(-7.62246130e-3));
    valueX = std::fma(valueW, valueX, Float(+9.43887047e-3));
    valueX = std::fma(valueW, valueX, Float(+1.00167406));
    valueX = std::fma(valueW, valueX, Float(+2.83297682));
  }
  valueX *= valueY;
  if constexpr (std::same_as<Float, double>) {
    // Two rounds of Newton iteration.
    valueX -= finiteOrZero((std::erf(valueX) - valueY) / (constants::TwoOverSqrtPi<double> * std::exp(-valueX * valueX)));
    valueX -= finiteOrZero((std::erf(valueX) - valueY) / (constants::TwoOverSqrtPi<double> * std::exp(-valueX * valueX)));
  }
  return valueX;
}

template <std::floating_point Float, size_t N> struct RealRoots final : ArrayLike<RealRoots<Float, N>> {
public:
  constexpr RealRoots() noexcept = default;

  constexpr RealRoots(std::floating_point auto... roots) noexcept requires(sizeof...(roots) <= N) : mRoots{Float(roots)...}, mRootCount(sizeof...(roots)) { std::sort(this->begin(), this->end()); }

  template <size_t M> requires(M < N) constexpr RealRoots(const RealRoots<Float, M> &other) noexcept {
    std::copy(other.begin(), other.end(), this->begin());
    mRootCount = other.size();
  }

public:
  MI_ARRAY_LIKE_CONSTEXPR_DATA(&mRoots[0])

  MI_ARRAY_LIKE_CONSTEXPR_SIZE(mRootCount)

  [[nodiscard]] constexpr operator bool() const noexcept { return mRootCount > 0; }

private:
  Float mRoots[N]{};

  size_t mRootCount{0};
};

template <std::floating_point Float> [[nodiscard]] inline RealRoots<Float, 2> solveQuadratic(Float coeffA, Float coeffB, Float coeffC) noexcept {
  if (isTiny(coeffA, abs(coeffB) + abs(coeffC))) [[unlikely]] {
    if (Float root = -coeffC / coeffB; isfinite(root)) {
      return {root};
    } else {
      return {};
    }
  } else {
    coeffB /= coeffA;
    coeffC /= coeffA;
    if (!isfinite(coeffB) || !isfinite(coeffC)) return {};
    Float discrim = coeffB * coeffB - 4 * coeffC;
    if (!isfinite(discrim)) discrim = coeffB * (coeffB - 4 * (coeffC / coeffB)); // Try again
    if (!isfinite(discrim) || discrim < 0) return {};
    Float root0 = -Float(0.5) * (coeffB + copysign(sqrt(discrim), coeffB));
    Float root1 = coeffC / root0;
    if (abs(root0 * root0 - coeffC) < Float(1e-5) * abs(coeffC)) {
      return {root0};
    } else {
      return {min(root0, root1), max(root0, root1)};
    }
  }
}

template <std::floating_point Float> [[nodiscard]] inline RealRoots<Float, 3> solveCubic(Float coeffA, Float coeffB, Float coeffC, Float coeffD) noexcept {
  if (isTiny(coeffA, abs(coeffB) + abs(coeffB) + abs(coeffC))) [[unlikely]]
    return solveQuadratic(coeffB, coeffC, coeffD);
  coeffB /= coeffA;
  coeffC /= coeffA;
  coeffD /= coeffA;
  if (!isfinite(coeffB) || !isfinite(coeffC) || !isfinite(coeffD)) return {};
  Float coeffBOverThree = coeffB / 3;
  Float coeffQ = (3 * coeffC - sqr(coeffB)) / 9;
  Float coeffR = (9 * coeffB * coeffC - 27 * coeffD - 2 * nthPow(coeffB, 3)) / 54;
  Float discrim = nthPow(coeffQ, 3) + sqr(coeffR);
  if (discrim >= 0) {
    Float sqrtDiscrim = sqrt(discrim);
    Float coeffS = cbrt(coeffR + sqrtDiscrim);
    Float coeffT = cbrt(coeffR - sqrtDiscrim);
    Float root = -coeffBOverThree + (coeffS + coeffT);
    if (abs(coeffS - coeffT) < abs(root) * 1e-6) {
      return {root, -coeffBOverThree};
    } else {
      return {root};
    }
  } else {
    Float theta = acos(coeffR / sqrt(nthPow(coeffQ, 3))) / 3;
    Float twoSqrtCoeffQ = 2 * sqrt(-coeffQ);
    return {
      twoSqrtCoeffQ * cos(theta) - coeffBOverThree, //
      twoSqrtCoeffQ * cos(theta + 2 * constants::Pi<Float> / 3) - coeffBOverThree, twoSqrtCoeffQ * cos(theta + 4 * constants::Pi<Float> / 3) - coeffBOverThree};
  }
}

#if 0
namespace concepts {

template <typename T>
concept arithmetic_or_complex_range =
  (std::ranges::range<T> && arithmetic_or_complex<std::decay_t<std::ranges::range_value_t<T>>>);

} // namespace concepts

/// Polynomial value.
template <concepts::arithmetic_or_complex_range Coeffs, concepts::arithmetic_or_complex Coord>
constexpr auto polynomial(const Coeffs &coeffs, Coord coord) noexcept {
  using Coeff = std::decay_t<std::ranges::range_value_t<Coeffs>>;
  using Value = std::decay_t<decltype(Coeff() * Coord())>;
  Value value = {};
  for (Coeff coeff : coeffs) value = value * coord + coeff;
  return value;
}

/// Rational polynomial value.
template <
  concepts::arithmetic_or_complex_range Coeffs0,
  concepts::arithmetic_or_complex_range Coeffs1,
  concepts::arithmetic_or_complex Coord>
constexpr auto polynomial(const Coeffs0 &coeffs0, const Coeffs1 &coeffs1, Coord coord) noexcept {
  if constexpr (std::integral<Coord>)
    return polynomial(coeffs0, coeffs1, double(coord));
  else {
    if (abs(coord) <= 1) return polynomial(coeffs0, coord) / polynomial(coeffs1, coord);
    coord = Coord(1) / coord;
    auto numer = polynomial(std::ranges::reverse_view(coeffs0), coord);
    auto denom = polynomial(std::ranges::reverse_view(coeffs1), coord);
    int size0 = std::ranges::ssize(coeffs0);
    int size1 = std::ranges::ssize(coeffs1);
    if (size0 < size1) numer *= nthPow(coord, size1 - size0);
    if (size1 < size0) denom *= nthPow(coord, size0 - size1);
    return numer / denom;
  }
}
#endif

// TODO Rethink wrap API

enum class Wrap { Clamp = 0, Repeat, Mirror };

/// Wrap integer in range.
template <std::integral Int> [[nodiscard]] constexpr Int repeat(Int valueK, Int valueN) noexcept {
  if constexpr (std::unsigned_integral<Int>) {
    return valueK % valueN;
  } else {
    if (valueK < Int(0)) return -repeat(-valueK, -valueN);
    if (valueN > Int(0)) return valueK % valueN;
    valueK %= valueN;
    valueK += valueN;
    return valueK == valueN ? 0 : valueK;
  }
}

/// Wrap integer in range and mirror with each repeat.
template <std::integral Int> [[nodiscard]] constexpr Int mirror(Int valueK, Int valueN) noexcept {
  if (valueN < Int(0)) {
    return -mirror(-valueK, -valueN);
  } else {
    Int rem = valueK % valueN;
    Int quo = valueK / valueN;
    if (rem < Int(0)) {
      rem += valueN;
      quo++;
    }
    if (quo & Int(1)) rem = valueN - rem - Int(1);
    return rem;
  }
}

/// Wrap floating point number in range.
template <std::floating_point Float> [[nodiscard]] inline Float repeat(Float valueX, std::type_identity_t<Float> valueA = 0, std::type_identity_t<Float> valueB = 1) noexcept {
  valueX -= valueA;
  valueB -= valueA;
  Float rem = std::remainder(valueX, valueB);
  rem += rem < 0 ? valueB : 0;
  rem += valueA;
  return rem;
}

/// Wrap floating point number in range and mirror with each repeat.
template <std::floating_point Float> [[nodiscard]] inline Float mirror(Float valueX, std::type_identity_t<Float> valueA = 0, std::type_identity_t<Float> valueB = 1) noexcept {
  valueX -= valueA;
  valueB -= valueA;
  int quo = 0;
  Float rem = std::remquo(valueX, valueB, &quo);
  if (rem < 0) rem += valueB, quo++;
  if (quo & 1) rem = valueB - rem;
  return rem + valueA;
}

} // namespace mi
