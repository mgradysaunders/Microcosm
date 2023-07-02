/*-*- C++ -*-*/
#pragma once

#include "../utility/ArrayLike.h"
#include "../utility/common.h"
#include <string>
#include <string_view>

namespace mi {

/// A static string.
///
/// \tparam Char
/// String character type.
///
/// \tparam BufSize
/// String character capacity, must be greater than 1 (because
/// 1 character is always used as null terminator).
///
template <typename Char, size_t BufSize> struct StaticString : ArrayLike<StaticString<Char, BufSize>> {
public:
  // Sanity check.
  static_assert(std::integral<Char> && BufSize > 1);

  constexpr StaticString() noexcept = default;

  constexpr StaticString(const StaticString &) noexcept = default;

  constexpr StaticString(StaticString &&) noexcept = default;

  template <std::convertible_to<Char>... Chars> constexpr StaticString(Char c0, Chars... cs) noexcept {
    static_assert(1 + sizeof...(Chars) <= BufSize);
    Char chars[] = {c0, Char(cs)...};
    std::copy(std::ranges::begin(chars), std::ranges::end(chars), data());
  }

  template <size_t N> constexpr StaticString(const Char (&str)[N]) noexcept {
    static_assert(N <= BufSize);
    std::copy(std::ranges::begin(str), std::ranges::end(str), data());
  }

  template <typename... Args> StaticString(const std::basic_string<Char, Args...> &str) {
    if (str.size() > max_size()) throw Error(std::invalid_argument("Max size exceeded!"));
    std::copy(str.begin(), str.end(), data());
    mLen = str.size();
  }

  template <typename... Args> constexpr StaticString(const std::basic_string_view<Char, Args...> &str) {
    if (str.size() > max_size()) throw Error(std::invalid_argument("Max size exceeded!"));
    std::copy(str.begin(), str.end(), data());
    mLen = str.size();
  }

  constexpr StaticString(const Char *cstr) : StaticString(std::basic_string_view<Char>(cstr)) {}

  template <size_t OtherBufSize> constexpr StaticString(const StaticString<Char, OtherBufSize> &str) : StaticString(str.view()) {}

  constexpr StaticString &operator=(const StaticString &) = default;

  constexpr StaticString &operator=(StaticString &&) = default;

public:
  /// \name Container API
  /// \{

  MI_ARRAY_LIKE_CONSTEXPR_DATA(&mBuf[0])

  MI_ARRAY_LIKE_CONSTEXPR_SIZE(mLen)

  [[nodiscard]] static constexpr size_t max_size() noexcept { return BufSize - 1; }

  [[nodiscard]] static constexpr size_t capacity() noexcept { return BufSize - 1; }

  constexpr void clear() noexcept {
    std::fill(this->begin(), this->end(), Char(0));
    mLen = 0;
  }

  constexpr void resize(size_t len) {
    if (len > max_size()) throw Error(std::invalid_argument("Max size exceeded!"));
    mLen = len;
    mBuf[len] = Char(0);
  }

  /// \}

  /// \name String
  /// \{

  [[nodiscard]] constexpr size_t length() const noexcept { return mLen; }

  [[nodiscard]] constexpr const Char *c_str() const noexcept { return &mBuf[0]; }

  template <typename... Args> operator std::basic_string<Char, Args...>() const { return &mBuf[0]; }

  template <typename... Args> constexpr operator std::basic_string_view<Char, Args...>() const noexcept { return {&mBuf[0], mLen}; }

  constexpr std::basic_string_view<Char> view() const noexcept { return *this; }

  constexpr auto operator<=>(const Char *other) const noexcept { return view() <=> std::basic_string_view<Char>(other); }

  constexpr bool operator==(const Char *other) const noexcept { return view() == std::basic_string_view<Char>(other); }

  template <size_t OtherBufSize> constexpr auto operator<=>(const StaticString<Char, OtherBufSize> &other) const noexcept { return view() <=> other.view(); }

  template <size_t OtherBufSize> constexpr bool operator==(const StaticString<Char, OtherBufSize> &other) const noexcept { return view() == other.view(); }

  /// \}

public:
  void onSerialize(auto &serializer) {
    serializer <=> mBuf;
    serializer <=> mLen;
  }

  template <concepts::ostream Stream> friend Stream &operator<<(Stream &stream, const StaticString &str) {
    stream << str.c_str();
    return stream;
  }

  template <concepts::istream Stream> friend Stream &operator>>(Stream &stream, StaticString &str) {
    std::basic_string<Char> tmp;
    stream >> tmp;
    str = tmp;
    return stream;
  }

private:
  Char mBuf[BufSize] = {};

  size_t mLen = 0;
};

} // namespace mi
