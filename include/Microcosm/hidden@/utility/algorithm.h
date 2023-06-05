#pragma once

#include "./IteratorRange.h"
#include "./common.h"
#include <algorithm>
#include <random>
#include <variant>
#include <vector>

namespace mi {

[[nodiscard, strong_inline]] constexpr auto sqr(auto value) noexcept { return value * value; }

[[nodiscard, strong_inline]] constexpr auto
min(concepts::arithmetic_or_enum auto valueX, concepts::arithmetic_or_enum auto valueY) noexcept {
  return valueX < valueY ? valueX : valueY;
}

[[nodiscard, strong_inline]] constexpr auto
max(concepts::arithmetic_or_enum auto valueX, concepts::arithmetic_or_enum auto valueY) noexcept {
  return valueX > valueY ? valueX : valueY;
}

template <typename Value> [[nodiscard, strong_inline]] constexpr Value &minReference(Value &valueX, Value &valueY) noexcept {
  return *(valueX < valueY ? &valueX : &valueY);
}

template <typename Value> [[nodiscard, strong_inline]] constexpr Value &maxReference(Value &valueX, Value &valueY) noexcept {
  return *(valueX < valueY ? &valueY : &valueX);
}

[[strong_inline, strong_inline]] constexpr bool
minimize(concepts::arithmetic_or_enum auto &valueX, concepts::arithmetic_or_enum auto valueY) noexcept {
  auto backup = valueX;
  return (valueX = min(valueX, valueY)) != backup;
}

[[strong_inline, strong_inline]] constexpr bool
maximize(concepts::arithmetic_or_enum auto &valueX, concepts::arithmetic_or_enum auto valueY) noexcept {
  auto backup = valueX;
  return (valueX = max(valueX, valueY)) != backup;
}

/// Clamp in range.
[[nodiscard, strong_inline]] constexpr auto clamp(
  concepts::arithmetic_or_enum auto value, concepts::arithmetic_or_enum auto minValue,
  concepts::arithmetic_or_enum auto maxValue) noexcept {
  return min(max(value, minValue), maxValue);
}

template <std::ranges::range Range, typename... Args>
[[nodiscard]] constexpr auto surroundingPair(Range &&range, Args &&...args) {
  auto itr0 = std::ranges::begin(range);
  auto itr1 = std::ranges::end(range);
  auto itrA = std::lower_bound(itr0, itr1, std::forward<Args>(args)...);
  auto itrB = itrA;
  if (itrA != itr0) --itrA;
  return std::make_pair(itrA, itrB);
}

template <std::ranges::range Range, typename... Args>
[[nodiscard]] constexpr auto lowerBoundIndex(Range &&range, Args &&...args) {
  return std::distance(
    std::ranges::begin(range),
    std::lower_bound(std::ranges::begin(range), std::ranges::end(range), std::forward<Args>(args)...));
}

template <std::ranges::range Range, typename... Args>
[[nodiscard]] constexpr auto upperBoundIndex(Range &&range, Args &&...args) {
  return std::distance(
    std::ranges::begin(range),
    std::upper_bound(std::ranges::begin(range), std::ranges::end(range), std::forward<Args>(args)...));
}

/// Lower bound index, optimized for sequential calls.
template <std::signed_integral Index, std::ranges::range Range>
constexpr bool sequentialLowerBoundIndex(Index &index, Range &&range, const auto &value, auto &&predicate) {
  auto count{std::ranges::ssize(range)};
  auto first{std::ranges::begin(range)};
  if constexpr (std::ranges::random_access_range<Range>) {
    if (index <= 0 || index >= Index(count)) {
      index = 0; // Reset.
    } else {
      bool greaterThanPrev{predicate(first[index - 1], value)};
      bool greaterThanNext{predicate(first[index], value)};
      if (!greaterThanNext && greaterThanPrev) return false;
      if (!greaterThanPrev) {
        // Check neighbor.
        if (index == 1) return false; // Can't decrement!
        if (predicate(first[index - 2], value)) {
          --index;
          return true;
        }
        // At least reduce search range.
        count = index;
        index = 0;
      } else {
        // Check neighbor.
        if (index + 1 == Index(count)) return false; // Can't increment!
        if (!predicate(first[index + 1], value)) {
          ++index;
          return true;
        }
        // At least reduce search range.
        first += index;
        count -= index;
      }
    }
  }

  while (count > 0) {
    auto middleIndex{count / 2};
    auto middleValue{std::next(first, middleIndex)};
    if (predicate(*middleValue, value)) {
      first = ++middleValue;
      count -= middleIndex + 1;
      index += middleIndex + 1;
    } else {
      count = middleIndex;
    }
  }
  if (index == 0) index = 1;
  return true;
}

template <typename... Values> constexpr void variantSetIndex(std::variant<Values...> &values, size_t index) {
  const std::variant<Values...> defaults[] = {Values()...};
  values = defaults[index];
}

/// Round up to multiple.
template <size_t M> [[nodiscard]] constexpr size_t roundUpTo(size_t value) noexcept {
  static_assert(M != 0);
  if (value == 0) return M;
  size_t remainder = value % M;
  return remainder == 0 ? value : value + M - remainder;
}

/// Integer-only factorial implementation.
[[nodiscard]] constexpr size_t factorial(size_t value) noexcept { //
  return value < 2 ? 1 : value * factorial(value - 1);
}

/// Integer-only binomial coefficient (N-choose-K).
[[nodiscard]] constexpr size_t choose(size_t valueN, size_t valueK) noexcept {
  size_t valueC = 1;
  for (size_t j = 0; j < valueK; ++j) valueC *= valueN - j, valueC /= j + 1;
  return valueC;
}

/// XOR-Shift Left.
template <std::integral Int> [[nodiscard]] constexpr Int xorshl(Int value, int shift) noexcept {
  return (value << shift) ^ value;
}

/// XOR-Shift Right.
template <std::integral Int> [[nodiscard]] constexpr Int xorshr(Int value, int shift) noexcept {
  return (value >> shift) ^ value;
}

template <typename Value> [[nodiscard]] constexpr Value nthPow(Value value, int power) noexcept {
  if (power < 0) {
    return Value(1) / nthPow(value, -power);
  } else {
    Value total = 1;
    while (true) {
      if ((power & 1)) total *= value;
      if ((power >>= 1) == 0) break;
      value *= value;
    }
    return total;
  }
}

/// Encode string in base64.
[[nodiscard]] inline std::vector<uint8_t> encodeBase64(IteratorRange<const uint8_t *> decoded) {
  static const char *remap = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  if (decoded.empty()) return {};
  std::vector<uint8_t> encoded((decoded.size() + 2) / 3 * 4, '=');
  size_t pad = decoded.size() % 3;
  auto decItr = &decoded[0];
  auto encItr = &encoded[0];
  while (decItr < &decoded[0] + decoded.size() - pad) {
    uint32_t value = (uint32_t(decItr[0]) << 16) | (uint32_t(decItr[1]) << 8) | (uint32_t(decItr[2]));
    encItr[0] = remap[(value >> 18)];
    encItr[1] = remap[(value >> 12) & 0x3F];
    encItr[2] = remap[(value >> 6) & 0x3F];
    encItr[3] = remap[value & 0x3F];
    decItr += 3;
    encItr += 4;
  }
  if (pad) {
    --pad;
    uint32_t b0 = *decItr++;
    uint32_t b1 = pad ? *decItr++ : 0;
    uint32_t value = pad ? (b0 << 8) | b1 : b0;
    *encItr++ = remap[pad ? (value >> 10) & 0x3F : (value >> 2)];
    *encItr++ = remap[pad ? (value >> 4) & 0x3F : (value << 4) & 0x3F];
    *encItr++ = pad ? remap[(value << 2) & 0x3F] : '=';
  }
  return encoded;
}

/// Decode string from base64.
[[nodiscard]] inline std::vector<uint8_t> decodeBase64(IteratorRange<const uint8_t *> encoded) {
  static constexpr uint8_t remap[256] = {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  //
                                         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  //
                                         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  62, 63, 62, 62, 63, //
                                         52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 0,  0,  0,  0,  0,  0,  //
                                         0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, //
                                         15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 0,  0,  0,  0,  63, //
                                         0,  26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, //
                                         41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0,  0,  0,  0,  0};
  if (encoded.empty()) return {};
  size_t size = encoded.size();
  size_t pad1 = size % 4 || encoded[size - 1] == '=';
  size_t pad2 = pad1 && (size % 4 == 3 || encoded[size - 2] != '=');
  size_t last = (size - pad1) & ~size_t(3); // / 4) << 2;
  std::vector<uint8_t> decoded(last / 4 * 3 + pad1 + pad2, '\0');
  auto encItr = &encoded[0];
  auto decItr = &decoded[0];
  while (encItr < &encoded[0] + last) {
    uint32_t value = (uint32_t(remap[encItr[0]]) << 18) | //
                     (uint32_t(remap[encItr[1]]) << 12) | //
                     (uint32_t(remap[encItr[2]]) << 6) |  //
                     (uint32_t(remap[encItr[3]]));
    decItr[0] = (value >> 16);
    decItr[1] = (value >> 8) & 0xFF;
    decItr[2] = value & 0xFF;
    decItr += 3;
    encItr += 4;
  }
  if (pad1) {
    uint32_t value = (uint32_t(remap[encItr[0]]) << 18) | //
                     (uint32_t(remap[encItr[1]]) << 12);
    decItr[0] = value >> 16;
    if (pad2) decItr[1] = ((value | (uint32_t(remap[encItr[2]]) << 6)) >> 8) & 0xFF;
  }
  return decoded;
}

/// Port of the MurmurHash3 implementation put in the public domain by Austin Appleby.
[[nodiscard]] inline std::pair<uint64_t, uint64_t> murmurHash3(uint64_t seed, size_t keyLength, const void *key) noexcept {
  constexpr uint64_t c1 = 0x87c37b91114253d5ULL;
  constexpr uint64_t c2 = 0x4cf5ad432745937fULL;
  uint64_t h1 = seed;
  uint64_t h2 = seed;
  const uint64_t *blocks = std::bit_cast<const uint64_t *>(key);
  const size_t numBlocks = keyLength / 16;
  for (size_t i = 0; i < numBlocks; i++) {
    uint64_t k1 = blocks[i * 2 + 0];
    uint64_t k2 = blocks[i * 2 + 1];
    k1 *= c1, k1 = std::rotl(k1, 31), k1 *= c2, h1 ^= k1, h1 = std::rotl(h1, 27), h1 += h2, h1 = h1 * 5 + 0x52dce729;
    k2 *= c2, k2 = std::rotl(k2, 33), k2 *= c1, h2 ^= k2, h2 = std::rotl(h2, 31), h2 += h1, h2 = h2 * 5 + 0x38495ab5;
  }
  const uint8_t *tail = std::bit_cast<const uint8_t *>(key) + numBlocks * 16;
  uint64_t k1 = 0;
  uint64_t k2 = 0;
  switch (keyLength & 15) {
  case 15: k2 ^= uint64_t(tail[14]) << 48; [[fallthrough]];
  case 14: k2 ^= uint64_t(tail[13]) << 40; [[fallthrough]];
  case 13: k2 ^= uint64_t(tail[12]) << 32; [[fallthrough]];
  case 12: k2 ^= uint64_t(tail[11]) << 24; [[fallthrough]];
  case 11: k2 ^= uint64_t(tail[10]) << 16; [[fallthrough]];
  case 10: k2 ^= uint64_t(tail[9]) << 8; [[fallthrough]];
  case 9: k2 ^= uint64_t(tail[8]), k2 *= c2, k2 = std::rotl(k2, 33), k2 *= c1, h2 ^= k2; [[fallthrough]];
  case 8: k1 ^= uint64_t(tail[7]) << 56; [[fallthrough]];
  case 7: k1 ^= uint64_t(tail[6]) << 48; [[fallthrough]];
  case 6: k1 ^= uint64_t(tail[5]) << 40; [[fallthrough]];
  case 5: k1 ^= uint64_t(tail[4]) << 32; [[fallthrough]];
  case 4: k1 ^= uint64_t(tail[3]) << 24; [[fallthrough]];
  case 3: k1 ^= uint64_t(tail[2]) << 16; [[fallthrough]];
  case 2: k1 ^= uint64_t(tail[1]) << 8; [[fallthrough]];
  case 1: k1 ^= uint64_t(tail[0]), k1 *= c1, k1 = std::rotl(k1, 31), k1 *= c2, h1 ^= k1; [[fallthrough]];
  default: break;
  };
  h1 ^= uint64_t(keyLength);
  h2 ^= uint64_t(keyLength);
  h1 += h2;
  h2 += h1;
  h1 ^= h1 >> 33, h1 *= 0xff51afd7ed558ccdULL, h1 ^= h1 >> 33, h1 *= 0xc4ceb9fe1a85ec53ULL, h1 ^= h1 >> 33;
  h2 ^= h2 >> 33, h2 *= 0xff51afd7ed558ccdULL, h2 ^= h2 >> 33, h2 *= 0xc4ceb9fe1a85ec53ULL, h2 ^= h2 >> 33;
  h1 += h2;
  h2 += h1;
  return {h1, h2};
}

template <concepts::istream Stream> [[nodiscard]] inline bool consume(Stream &stream, char what) {
  using Char = typename Stream::char_type;
  using CharTraits = typename Stream::traits_type;
  Char ch;
  if (!(stream >> ch) || !CharTraits::eq(ch, CharTraits::to_char_type(what))) {
    stream.setstate(std::ios::failbit);
    return false;
  }
  return true;
}

template <concepts::istream Stream> [[nodiscard]] inline bool consume(Stream &stream, const char *what) {
  while (what && *what != '\0')
    if (!consume(stream, *what++)) return false;
  return true;
}

template <concepts::istream Stream> [[nodiscard]] inline std::streamsize remainingStreamsize(Stream &stream) {
  std::streamsize pos0 = stream.tellg();
  stream.seekg(0, std::ios::end);
  std::streamsize pos1 = stream.tellg();
  stream.seekg(pos0, std::ios::beg);
  return stream.fail() ? -1 : pos1 - pos0;
}

template <typename Value>
[[nodiscard, strong_inline]] inline auto randomize(std::uniform_random_bit_generator auto &gen) -> decltype(Value(gen)) {
  return Value(gen);
}

template <std::integral Int> //
[[nodiscard, strong_inline]] inline auto randomize(std::uniform_random_bit_generator auto &gen) {
  return Int(gen()); // TODO If int is bigger than result?
}

template <std::floating_point Float, size_t Bits = 32> //
[[nodiscard, strong_inline]] inline auto randomize(std::uniform_random_bit_generator auto &gen) {
  return std::generate_canonical<Float, Bits>(gen);
}

template <std::floating_point Float, size_t Bits = 32> //
[[nodiscard, strong_inline]] inline auto randomize(std::uniform_random_bit_generator auto &gen, int howMany) {
  return std::views::iota(0, howMany) | std::views::transform([&](int) { return randomize<Float, Bits>(gen); });
}

/// Declare and reserve space in an ordinary STL vector.
template <typename... Args> [[nodiscard, strong_inline]] inline auto reservedVectorSTL(size_t size) {
  std::vector<Args...> vector;
  vector.reserve(size);
  return vector;
}

} // namespace mi
