#pragma once

#include "./string.h"

namespace mi {

struct UTF8Encoding : ArrayLike<UTF8Encoding> {
public:
  constexpr UTF8Encoding(char32_t codepoint) noexcept {
    if (codepoint <= 0x7F) {
      mBytes[0] = char8_t(codepoint);
      mSize = 1;
    } else if (codepoint <= 0x7FF) {
      mBytes[0] = char8_t(0xC0 | (0xFF & (codepoint >> 6))); // 110xxxxx
      mBytes[1] = char8_t(0x80 | (0x3F & (codepoint)));      // 10xxxxxx
      mSize = 2;
    } else if (codepoint <= 0xFFFF) {
      mBytes[0] = char8_t(0xE0 | (0xFF & (codepoint >> 12))); // 1110xxxx
      mBytes[1] = char8_t(0x80 | (0x3F & (codepoint >> 6)));  // 10xxxxxx
      mBytes[2] = char8_t(0x80 | (0x3F & (codepoint)));       // 10xxxxxx
      mSize = 3;
    } else if (codepoint <= 0x10FFFF) {
      mBytes[0] = char8_t(0xF0 | (0xFF & (codepoint >> 18))); // 11110xxx
      mBytes[1] = char8_t(0x80 | (0x3F & (codepoint >> 12))); // 10xxxxxx
      mBytes[2] = char8_t(0x80 | (0x3F & (codepoint >> 6)));  // 10xxxxxx
      mBytes[3] = char8_t(0x80 | (0x3F & (codepoint)));       // 10xxxxxx
      mSize = 4;
    }
  }

  MI_ARRAY_LIKE_CONSTEXPR_DATA(&mBytes[0])

  MI_ARRAY_LIKE_CONSTEXPR_SIZE(mSize)

private:
  char8_t mBytes[4]{};

  size_t mSize{};
};

template <concepts::ostream Stream> inline Stream &operator<<(Stream &stream, UTF8Encoding encoding) {
  for (auto each : encoding) stream << each;
  return stream;
}

template <typename Char, typename... Args> requires(sizeof(Char) == 1)
inline std::basic_string<Char, Args...> &operator+=(std::basic_string<Char, Args...> &string, UTF8Encoding encoding) {
  for (auto each : encoding) string += Char(each);
  return string;
}

template <typename Char = char> requires(sizeof(Char) == 1) struct UTF8DecodeRange {
  using Sentinel = nothing;

  struct Iterator : ArrayLike<Iterator> {
    using difference_type = ptrdiff_t;

    using iterator_category = std::input_iterator_tag;

    constexpr Iterator() noexcept = default;

    constexpr Iterator(const Char *pos, const Char *posEnd) noexcept : mPosB(pos), mPosEnd(posEnd) { operator++(); }

    MI_ARRAY_LIKE_CONSTEXPR_DATA(mPosA)

    MI_ARRAY_LIKE_CONSTEXPR_SIZE(mPosB - mPosA)

    constexpr Iterator &operator++() noexcept {
      constexpr uint8_t kSizes[32] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                      0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 3, 3, 4, 0};
      constexpr uint_fast32_t kZeroByteMasks[5] = {0x00, 0x7F, 0x1F, 0x0F, 0x07};
      constexpr uint_fast32_t kCodepointShifts[5] = {0, 18, 12, 6, 0};
      constexpr uint_fast32_t kCodepointMinimums[5] = {0x400000, 0, 0x80, 0x800, 0x10000};
      constexpr uint_fast32_t kErrorShifts[5] = {0, 6, 4, 2, 0};

      // Load all 4 bytes, shift away unused bits.
      uint_fast32_t size = kSizes[uint8_t(*mPosB) >> 3];
      uint_fast32_t skip = size + (size == 0);
      uint_fast32_t byte0 = mPosB + 0 < mPosEnd ? mPosB[0] : 0;
      uint_fast32_t byte1 = mPosB + 1 < mPosEnd ? mPosB[1] : 0;
      uint_fast32_t byte2 = mPosB + 2 < mPosEnd ? mPosB[2] : 0;
      uint_fast32_t byte3 = mPosB + 3 < mPosEnd ? mPosB[3] : 0;
      mCodepoint = (((byte0 & kZeroByteMasks[size]) << 18) | //
                    ((byte1 & uint_fast32_t(0x3F)) << 12) |  //
                    ((byte2 & uint_fast32_t(0x3F)) << 6) |   //
                    (byte3 & uint_fast32_t(0x3F))) >>
                   kCodepointShifts[size];

      // Validate.
      uint_fast32_t errors = 0;
      errors |= uint_fast32_t((mCodepoint < kCodepointMinimums[size])) << 6; // Non-canonical encoding?
      errors |= uint_fast32_t((mCodepoint >> 11) == 0x1B) << 7;              // Surrogate half?
      errors |= (byte1 & uint_fast32_t(0xC0)) >> 2;
      errors |= (byte2 & uint_fast32_t(0xC0)) >> 4;
      errors |= (byte3 >> 6);
      errors ^= uint_fast32_t(0x2A); // Top 2 bits of tail bytes correct?
      errors >>= kErrorShifts[size];
      if (errors) {
        skip = std::min(skip, uint_fast32_t((byte0 != 0) + (byte1 != 0) + (byte2 != 0) + (byte3 != 0)));
        mCodepoint = uint32_t(-1);
      }
      mPosA = mPosB;
      mPosB += skip;
      return *this;
    }

    constexpr Iterator operator++(int) noexcept {
      Iterator copy = *this;
      operator++();
      return copy;
    }

    [[nodiscard]] constexpr bool operator==(Sentinel) const noexcept { return *mPosA == 0 || mPosA >= mPosEnd; }

    [[nodiscard]] constexpr bool operator!=(Sentinel) const noexcept { return !operator==(Sentinel{}); }

    [[nodiscard]] constexpr char32_t operator*() const noexcept { return mCodepoint; }

  private:
    const Char *mPosA{nullptr};
    const Char *mPosB{nullptr};
    const Char *mPosEnd{nullptr};
    uint32_t mCodepoint{0};
  };

  constexpr UTF8DecodeRange(const Char *from, const Char *to) noexcept : mPos(from), mPosEnd(to) {}

  template <typename... Args>
  constexpr UTF8DecodeRange(std::basic_string_view<Char, Args...> str) noexcept
    : UTF8DecodeRange(str.data(), str.data() + str.size()) {}

  template <typename... Args>
  constexpr UTF8DecodeRange(const std::basic_string<Char, Args...> &str) noexcept
    : UTF8DecodeRange(str.data(), str.data() + str.size()) {}

  constexpr UTF8DecodeRange(const Char *cstr) noexcept : UTF8DecodeRange(std::basic_string_view<Char>(cstr)) {}

  [[nodiscard]] constexpr Iterator begin() const noexcept { return {mPos, mPosEnd}; }

  [[nodiscard]] constexpr Sentinel end() const noexcept { return {}; }

  [[nodiscard]] constexpr size_t size() const noexcept { return std::ranges::distance(begin(), end()); }

  [[nodiscard]] uint32_t at(size_t pos) {
    auto itr = begin();
    for (; pos-- != 0; ++itr)
      if (itr == end()) throw Error(std::out_of_range("Index out of range!"));
    return *itr;
  }

private:
  const Char *mPos{nullptr};

  const Char *mPosEnd{nullptr};
};

template <typename Char> UTF8DecodeRange(const Char *, const Char *) -> UTF8DecodeRange<Char>;

template <typename Char> UTF8DecodeRange(const Char *) -> UTF8DecodeRange<Char>;

template <typename Char, typename... Args> UTF8DecodeRange(std::basic_string_view<Char, Args...>) -> UTF8DecodeRange<Char>;

template <typename Char, typename... Args> UTF8DecodeRange(const std::basic_string<Char, Args...> &) -> UTF8DecodeRange<Char>;

} // namespace mi
