/*-*- C++ -*-*/
#pragma once

#include "./common.h"
#include <bit>

namespace mi {

/// A half-precision float.
///
/// A half-precision float, an adaptation of [the ILM
/// implementation][1] by Florian Kains and Rod Bogart. Notably, the
/// implementation here requires neither pre-calculations nor lookup tables.
/// [1]: https://github.com/openexr/openexr/tree/develop/IlmBase/half
///
struct Half {
public:
  /// "From bits" tag.
  enum from_bits_tag { from_bits };

  constexpr Half() noexcept = default;

  Half(float value) noexcept;

  template <concepts::arithmetic Arith> Half(Arith value) noexcept : Half(float(value)) {}

  /// Construct from bits.
  ///
  /// \note
  /// Usage is `Half(0x12345, Half::from_bits)`.
  ///
  constexpr Half(uint16_t bits, from_bits_tag) noexcept : bits(bits) {}

public:
  [[nodiscard]] constexpr Half abs() const noexcept {
    Half half = *this;
    half.bits = half.bits & ~0x8000;
    return half;
  }

  [[nodiscard]] constexpr bool signbit() const noexcept { return (bits & 0x8000); }

  [[nodiscard]] constexpr bool isinf() const noexcept { return (bits & 0x7FFF) == 0x7C00; }

  [[nodiscard]] constexpr bool isnan() const noexcept { return (bits & 0x7FFF) != 0x7C00 && (bits & 0x7C00) == 0x7C00; }

  [[nodiscard]] constexpr bool isfinite() const noexcept { return (bits & 0x7C00) != 0x7C00; }

  [[nodiscard]] constexpr bool isnormal() const noexcept { return (bits & 0x7C00) != 0x7C00 && (bits & 0x7C00) != 0; }

  [[nodiscard]] constexpr Half increment() const noexcept {
    if (isinf()) return *this;
    uint16_t b = bits;
    if (b == 0x8000) b = 0;
    if (b & 0x8000)
      b--;
    else
      b++;
    return {b, from_bits};
  }

  [[nodiscard]] constexpr Half decrement() const noexcept {
    if (isinf()) return *this;
    uint16_t b = bits;
    if (b == 0x0000) b = 0x8000;
    if (b & 0x8000)
      b++;
    else
      b--;
    return {b, from_bits};
  }

public:
  [[nodiscard]] constexpr bool operator==(const Half &other) const noexcept { return bits == other.bits; }

  [[nodiscard]] constexpr bool operator!=(const Half &other) const noexcept { return bits != other.bits; }

  operator float() const noexcept;

  template <concepts::arithmetic Arith> requires(!std::same_as<Arith, float>) operator Arith() const noexcept {
    return Arith(float(*this));
  }

public:
  void onSerialize(auto &serializer) { serializer <=> bits; }

public:
  uint16_t bits = 0;

  // Sanity check.
  static_assert(std::numeric_limits<float>::is_iec559, "Half requires IEC-559/IEEE-754 single precision floats");
};

inline Half::Half(float f) noexcept {
  uint32_t u = std::bit_cast<uint32_t>(f);
  int32_t exponent = (u >> 23) & 0x01FF;
  int32_t negative = (u >> 31) & 0x0001;
  int32_t mantissa = u & 0x007FFFFF;
  if (f == 0)
    bits = negative << 15;
  else if (exponent > 0x0070 && exponent < 0x008E)
    bits = (exponent - 0x0070) * 0x0400 + ((mantissa + ((mantissa >> 13) & 1) + 0x0FFF) >> 13);
  else if (exponent > 0x0170 && exponent < 0x018E)
    bits = (exponent - 0x0170) * 0x0400 + ((mantissa + ((mantissa >> 13) & 1) + 0x0FFF) >> 13) + 0x8000;
  else {
    bits = negative << 15;
    exponent &= 255;
    exponent -= 112;
    if (exponent < 1) {
      if (exponent >= -10) {
        exponent = 13 - exponent;
        mantissa |= 0x800000;
        bits |= (mantissa + (1 << exponent) + ((mantissa >> (exponent + 1)) & 1) - 1) >> (exponent + 1);
      }
    } else if (exponent == 0x8F) {
      bits |= 0x7C00; // Inf
      if (mantissa != 0) {
        mantissa >>= 13;
        bits |= mantissa | (mantissa == 0); // NaN
      }
    } else {
      mantissa += (mantissa >> 13) & 1;
      mantissa += 0x0FFF;
      if (mantissa & 0x800000) mantissa = 0, exponent++;
      bits |= exponent > 30 ? 0x7C00 : (exponent << 10) | (mantissa >> 13);
    }
  }
}

inline Half::operator float() const noexcept {
  uint32_t u = 0;
  int32_t exponent = (bits >> 10) & 0x001F;
  int32_t negative = (bits >> 15) & 0x0001;
  int32_t mantissa = bits & 0x03FF;
  u = negative << 31;
  if (exponent == 0) {
    if (mantissa != 0) {
      exponent = 113;
      while (!(mantissa & 0x0400)) exponent--, mantissa <<= 1;
      u |= (exponent << 23) | ((mantissa & ~0x0400) << 13); // Subnormal
    }
  } else {
    u |= mantissa << 13;
    u |= exponent == 31 ? 0x7F800000 : ((exponent + 112) << 23);
  }
  return std::bit_cast<float>(u);
}

} // namespace mi
