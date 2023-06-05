/*-*- C++ -*-*/
#pragma once

#include <limits>

namespace mi::constants {

/// Minimum representable value.
template <concepts::arithmetic Arith> constexpr Arith Min = std::numeric_limits<Arith>::min();

/// Maximum representable value.
template <concepts::arithmetic Arith> constexpr Arith Max = std::numeric_limits<Arith>::max();

/// Infinity.
template <typename Float> constexpr auto Inf = std::numeric_limits<to_float_t<Float>>::infinity();

constexpr auto Inff = Inf<float>;

constexpr auto Infd = Inf<double>;

/// Not-a-number.
template <typename Float> constexpr auto NaN = std::numeric_limits<to_float_t<Float>>::quiet_NaN();

constexpr auto NaNf = NaN<float>;

constexpr auto NaNd = NaN<double>;

/// Epsilon, or the difference between one and largest representable value less than one.
template <typename Float> constexpr auto Eps = std::numeric_limits<to_float_t<Float>>::epsilon();

constexpr auto Epsf = Eps<float>;

constexpr auto Epsd = Eps<double>;

/// Machine epsilon, or the largest positive value that is rounded to zero when added to one.
template <typename Float> constexpr auto MachineEps = std::numeric_limits<to_float_t<Float>>::epsilon() / 2;

/// Machine echelon.
template <typename Float, unsigned int N> constexpr auto MachineEch = (MachineEps<Float> * N) / (1 - MachineEps<Float> * N);

/// The minimum invertible value, or the smallest positive value that can be divided into one without overflow.
template <typename Float>
constexpr auto MinInv = std::numeric_limits<to_float_t<Float>>::min() / 4 + //
                        std::numeric_limits<to_float_t<Float>>::denorm_min();

/// The minimum squarable value, or the smallest positive value that can be squared without underflow.
template <typename Float>
constexpr auto MinSqr = []() consteval {
  if constexpr (std::same_as<to_float_t<Float>, float>) {
    return 0x1.000002p-75f; // 2^(-75)
  } else if constexpr (std::same_as<to_float_t<Float>, double>) {
    return 0x1.6a09e667f3bcdp-538; // 2^(-537.5)
  } else if constexpr (std::same_as<to_float_t<Float>, long double>) {
    return 0x8.000000000000001p-8226L; // 2^(-8223)
  } else {
    return 0;
  }
}();

template <typename Float = double> constexpr auto ExpOne = to_float_t<Float>(2.7182818284590452353602874713526625L);

template <typename Float = double> constexpr auto LogBaseTwoOfE = to_float_t<Float>(1.4426950408889634073599246810018921L);

template <typename Float = double> constexpr auto LogBaseTenOfE = to_float_t<Float>(0.4342944819032518276511289189166051L);

template <typename Float = double> constexpr auto LnTwo = to_float_t<Float>(0.6931471805599453094172321214581766L);

template <typename Float = double> constexpr auto LnTen = to_float_t<Float>(2.3025850929940456840179914546843642L);

template <typename Float = double> constexpr auto Pi = to_float_t<Float>(3.1415926535897932384626433832795029L);

template <typename Float = double> constexpr auto TwoPi = to_float_t<Float>(2) * Pi<Float>;

template <typename Float = double> constexpr auto FourPi = to_float_t<Float>(4) * Pi<Float>;

template <typename Float = double> constexpr auto PiOverTwo = to_float_t<Float>(0.5) * Pi<Float>;

template <typename Float = double> constexpr auto PiOverFour = to_float_t<Float>(0.25) * Pi<Float>;

template <typename Float = double> constexpr auto OneOverPi = to_float_t<Float>(1) / Pi<Float>;

template <typename Float = double> constexpr auto OneOverTwoPi = to_float_t<Float>(1) / TwoPi<Float>;

template <typename Float = double> constexpr auto OneOverFourPi = to_float_t<Float>(1) / FourPi<Float>;

template <typename Float = double> constexpr auto TwoOverPi = to_float_t<Float>(2) / Pi<Float>;

template <typename Float = double> constexpr auto TwoOverSqrtPi = to_float_t<Float>(1.1283791670955125738961589031215452L);

template <typename Float = double> constexpr auto OneOverSqrtPi = to_float_t<Float>(0.5) * TwoOverSqrtPi<Float>;

template <typename Float = double> constexpr auto SqrtTwo = to_float_t<Float>(1.4142135623730950488016887242096981L);

template <typename Float = double> constexpr auto OneOverSqrtTwo = to_float_t<Float>(0.7071067811865475244008443621048490L);

template <typename Float = double> constexpr auto OneOverSqrtTwoPi = OneOverSqrtTwo<Float> * OneOverSqrtPi<Float>;

template <typename Float = double> constexpr auto EulerGamma = to_float_t<Float>(0.5772156649015328606065120900824024L);

template <typename Float = double> constexpr auto PlanckH = to_float_t<Float>(6.62607015e-34L); // Joules * seconds

template <typename Float = double> constexpr auto LightSpeed = to_float_t<Float>(299792458); // meters / seconds

} // namespace mi::constants

consteval auto operator""_degreesf(long double x) noexcept { return float(x * mi::constants::Pi<long double> / 180.0L); }

consteval auto operator""_degrees(long double x) noexcept { return double(x * mi::constants::Pi<long double> / 180.0L); }
