#pragma once

#include <cctype>
#include <charconv>
#include <cwctype>
#include <fstream>
#include <streambuf>
#include <string>
#include <string_view>

#define FMT_HEADER_ONLY 1
#include <fmt/format.h>

#if (__GNUG__ || __clang__)
#include <cxxabi.h>
#endif // #if (__GNUG__ || __clang__)

#include "./ArrayLike.h"
#include "./common.h"

namespace mi {

namespace concepts {

template <typename> struct is_cstring : std::false_type {};

template <std::integral Char> struct is_cstring<const Char *> : std::true_type {};

template <std::integral Char, size_t N> struct is_cstring<Char[N]> : std::true_type {};

template <typename T>
concept cstring = is_cstring<T>::value;

template <typename T>
concept string = match<T, std::basic_string>;

template <typename T>
concept string_view = match<T, std::basic_string_view>;

template <typename T>
concept string_or_view = string<T> || string_view<T>;

template <typename T>
concept cstring_or_view = cstring<T> || string_view<T>;

template <typename T>
concept any_string = cstring<T> || string<T> || string_view<T>;

template <typename T>
concept any_char = std::integral<T> && sizeof(T) <= 8;

} // namespace concepts

template <typename> struct string_traits;

template <concepts::string_or_view String> struct string_traits<String> {
  using char_type = typename String::value_type;
  using traits_type = typename String::traits_type;
};

template <concepts::any_char Char> struct string_traits<const Char *> {
  using char_type = Char;
  using traits_type = std::char_traits<char_type>;
};

template <concepts::any_char Char, size_t N> struct string_traits<Char[N]> {
  using char_type = Char;
  using traits_type = std::char_traits<char_type>;
};

template <concepts::any_string String> using char_type_of_t = typename string_traits<String>::char_type;

template <concepts::any_string String> using traits_type_of_t = typename string_traits<String>::traits_type;

/// Convert any string type to `std::basic_string`.
template <concepts::any_string String> [[nodiscard]] constexpr auto toString(const String &str) {
  if constexpr (concepts::string<String>)
    return str;
  else if constexpr (concepts::string_view<String>)
    return std::basic_string<typename String::value_type, typename String::traits_type>{str.begin(), str.end()};
  else
    return std::basic_string(str);
}

/// Convert any string type to `std::basic_string_view`.
template <concepts::any_string String> [[nodiscard]] constexpr auto toStringView(const String &str) {
  if constexpr (concepts::string<String>)
    return std::basic_string_view<typename String::value_type, typename String::traits_type>(str.data());
  else if constexpr (concepts::string_view<String>)
    return str;
  else
    return std::basic_string_view(str);
}

using fmt::format;

using fmt::join;

using fmt::print;

template <typename Char, size_t N> struct ConstexprStringChars {
  using char_type = Char;
  constexpr ConstexprStringChars() noexcept = default;
  constexpr ConstexprStringChars(const Char (&ch)[N]) noexcept { std::ranges::copy(ch, chars); }
  Char chars[N]{};
};

template <ConstexprStringChars Chars> struct ConstexprString {
  [[nodiscard]] constexpr auto c_str() const noexcept { return &Chars.chars[0]; }
};

template <ConstexprStringChars Chars> struct FormatString {
  [[nodiscard]] constexpr auto operator()(auto &&...args) const {
    return fmt::format(Chars.chars, std::forward<decltype(args)>(args)...);
  }
};

inline namespace string_literals {

using namespace std::string_literals;

using namespace std::string_view_literals;

template <ConstexprStringChars Chars> constexpr auto operator""_constant() { return ConstexprString<Chars>(); }

template <ConstexprStringChars Chars> constexpr auto operator""_format() { return FormatString<Chars>(); }

} // namespace string_literals

namespace char_class {

struct Predicate {};

#define MI_STANDARD_CHAR_CLASS(Name)                                                                                    \
  constexpr struct Name##_Predicate final : Predicate {                                                                 \
    template <concepts::any_char Char> [[nodiscard, strong_inline]] constexpr bool operator()(Char ch) const noexcept { \
      if constexpr (sizeof(Char) == 1)                                                                                  \
        return std::is##Name(static_cast<unsigned char>(ch));                                                           \
      else                                                                                                              \
        return std::isw##Name(ch);                                                                                      \
    }                                                                                                                   \
  } Name = {}

MI_STANDARD_CHAR_CLASS(alnum);

MI_STANDARD_CHAR_CLASS(alpha);

MI_STANDARD_CHAR_CLASS(digit);

MI_STANDARD_CHAR_CLASS(xdigit);

MI_STANDARD_CHAR_CLASS(cntrl);

MI_STANDARD_CHAR_CLASS(graph);

MI_STANDARD_CHAR_CLASS(space);

MI_STANDARD_CHAR_CLASS(blank);

MI_STANDARD_CHAR_CLASS(punct);

MI_STANDARD_CHAR_CLASS(print);

MI_STANDARD_CHAR_CLASS(lower);

MI_STANDARD_CHAR_CLASS(upper);

#undef MI_STANDARD_CHAR_CLASS

struct these final : Predicate {
  constexpr these() noexcept = default;
  constexpr these(std::string_view s) noexcept : s(s) {}
  [[nodiscard, strong_inline]] constexpr bool operator()(auto c) const noexcept { return s.find(c) != s.npos; }
  std::string_view s;
};

template <std::derived_from<Predicate> PredA, std::derived_from<Predicate> PredB> struct PredicateOr final : Predicate {
  constexpr PredicateOr() noexcept = default;
  constexpr PredicateOr(PredA predA, PredB predB) noexcept : predA(predA), predB(predB) {}
  [[nodiscard, strong_inline]] constexpr bool operator()(auto c) const noexcept { return predA(c) || predB(c); }
  PredA predA;
  PredB predB;
};

template <std::derived_from<Predicate> PredA, std::derived_from<Predicate> PredB> struct PredicateAnd final : Predicate {
  constexpr PredicateAnd() noexcept = default;
  constexpr PredicateAnd(PredA predA, PredB predB) noexcept : predA(predA), predB(predB) {}
  [[nodiscard, strong_inline]] constexpr bool operator()(auto c) const noexcept { return predA(c) && predB(c); }
  PredA predA;
  PredB predB;
};

template <std::derived_from<Predicate> Pred> struct PredicateNot {
  constexpr PredicateNot() noexcept = default;
  constexpr PredicateNot(Pred pred) noexcept : pred(pred) {}
  [[nodiscard, strong_inline]] constexpr bool operator()(auto c) const noexcept { return !pred(c); }
  Pred pred;
};

template <std::derived_from<Predicate> PredA, std::derived_from<Predicate> PredB>
[[nodiscard, strong_inline]] constexpr auto operator||(PredA predA, PredB predB) noexcept {
  return PredicateOr<PredA, PredB>(predA, predB);
}

template <std::derived_from<Predicate> PredA, std::derived_from<Predicate> PredB>
[[nodiscard, strong_inline]] constexpr auto operator&&(PredA predA, PredB predB) noexcept {
  return PredicateAnd<PredA, PredB>(predA, predB);
}

template <std::derived_from<Predicate> Pred> [[nodiscard, strong_inline]] constexpr auto operator!(Pred pred) noexcept {
  return PredicateNot<Pred>(pred);
}

constexpr auto word = alnum || these("_");

} // namespace char_class

[[nodiscard]] constexpr int hexToInt(char c) noexcept {
  if ('0' <= c && c <= '9') return c - '0';
  if ('a' <= c && c <= 'f') return c - 'a' + 10;
  if ('A' <= c && c <= 'F') return c - 'A' + 10;
  return 0;
}

[[nodiscard]] inline std::string show(std::string_view s) {
  std::string r;
  r.reserve(s.size());
  r += '"';
  for (const char &c : s) {
    switch (c) {
    case '\t': r += "\\t"; break;
    case '\n': r += "\\n"; break;
    case '\r': r += "\\r"; break;
    case '\f': r += "\\f"; break;
    case '\v': r += "\\v"; break;
    case '\b': r += "\\b"; break;
    case '\"': r += "\\\""; break;
    default:
      if (char_class::print(c)) {
        r += c;
      } else {
        r += '\\';
        r += 'x';
        r += "0123456789ABCDEF"[(uint8_t(c) >> 4) & 0xF];
        r += "0123456789ABCDEF"[(uint8_t(c)) & 0xF];
      }
      break;
    }
  }
  r += '"';
  return r;
}

[[nodiscard]] inline std::string show(char c) { return show(std::string_view(&c, 1)); }

template <concepts::any_char Char> [[nodiscard, strong_inline]] inline auto toLower(Char ch) noexcept {
  if constexpr (sizeof(Char) > 1)
    return std::towlower(ch);
  else
    return std::tolower(static_cast<unsigned char>(ch));
}

template <concepts::any_char Char> [[nodiscard, strong_inline]] inline auto toUpper(Char ch) noexcept {
  if constexpr (sizeof(Char) > 1)
    return std::towupper(ch);
  else
    return std::toupper(static_cast<unsigned char>(ch));
}

/// Convert any string type to lowercase `std::basic_string`.
template <concepts::any_string String> [[nodiscard]] inline auto toLower(String str) {
  if constexpr (!concepts::string<String>) {
    return toLower(toString(str));
  } else {
    std::transform(str.begin(), str.end(), str.begin(), toLower<char_type_of_t<String>>);
    return str;
  }
}

/// Convert any string type to uppercase `std::basic_string`.
template <concepts::any_string String> [[nodiscard]] inline auto toUpper(String str) {
  if constexpr (!concepts::string<String>) {
    return toUpper(toString(str));
  } else {
    std::transform(str.begin(), str.end(), str.begin(), toUpper<char_type_of_t<String>>);
    return str;
  }
}

/// Case-insensitive comparison.
///
/// \returns
/// A member constant of `std::strong_ordering`.
///
template <concepts::any_string StringA, concepts::any_string StringB>
[[nodiscard]] inline auto icaseCompare(const StringA &strA, const StringB &strB) {
  if constexpr (
    !concepts::string_view<StringA> || //
    !concepts::string_view<StringB>) {
    return icaseCompare(toStringView(strA), toStringView(strB));
  } else {
    auto itrA = strA.begin();
    auto itrB = strB.begin();
    for (; itrA < strA.end() && itrB < strB.end(); ++itrA, ++itrB) {
      auto ch0 = toLower(*itrA);
      auto ch1 = toLower(*itrB);
      if (ch0 < ch1) return std::strong_ordering::less;
      if (ch1 < ch0) return std::strong_ordering::greater;
    }
    if (itrA == strA.end() && itrB == strB.end())
      return std::strong_ordering::equal;
    else
      return itrA == strA.end() ? std::strong_ordering::less : std::strong_ordering::greater;
  }
}

/// Case-insensitive equal.
template <concepts::any_string StringA, concepts::any_string StringB>
[[nodiscard]] inline bool icaseEqual(const StringA &strA, const StringB &strB) {
  return icaseCompare(strA, strB) == std::strong_ordering::equal;
}

/// Case-insensitive less.
template <concepts::any_string StringA, concepts::any_string StringB>
[[nodiscard]] inline bool icaseLess(const StringA &strA, const StringB &strB) {
  return icaseCompare(strA, strB) == std::strong_ordering::less;
}

/// Case-insensitive greater.
template <concepts::any_string StringA, concepts::any_string StringB>
[[nodiscard]] inline bool icaseGreater(const StringA &strA, const StringB &strB) {
  return icaseCompare(strA, strB) == std::strong_ordering::greater;
}

/// Trim characters off of left.
template <concepts::any_string String, typename Pred> [[nodiscard]] inline auto trimLeft(const String &str, Pred &&pred) {
  if constexpr (concepts::cstring<String>) {
    return trimLeft(toStringView(str), std::forward<Pred>(pred));
  } else {
    auto itr = str.begin();
    for (; itr < str.end(); ++itr)
      if (!std::invoke(std::forward<Pred>(pred), *itr)) break;
    return String(itr, str.end());
  }
}

/// Trim characters off of right.
template <concepts::any_string String, typename Pred> [[nodiscard]] inline auto trimRight(const String &str, Pred &&pred) {
  if constexpr (concepts::cstring<String>) {
    return trimRight(toStringView(str), std::forward<Pred>(pred));
  } else {
    if (str.empty()) return String(); // Be careful with nulls!
    auto itr = str.rbegin();
    for (; itr < str.rend(); ++itr)
      if (!std::invoke(std::forward<Pred>(pred), *itr)) break;
    return String(&*str.begin(), &*itr + 1);
  }
}

/// Trim characters off of left and right.
template <concepts::any_string String, typename Pred> [[nodiscard]] inline auto trim(const String &str, Pred &&pred) {
  return trimLeft(trimRight(str, pred), pred);
}

/// Trim whitespace off of left.
template <concepts::any_string String> [[nodiscard]] inline auto trimLeft(const String &str) {
  return trimLeft(str, char_class::space);
}

/// Trim whitespace off of right.
template <concepts::any_string String> [[nodiscard]] inline auto trimRight(const String &str) {
  return trimRight(str, char_class::space);
}

/// Trim whitespace off of left and right.
template <concepts::any_string String> [[nodiscard]] inline auto trim(const String &str) {
  return trim(str, char_class::space);
}

/// Convert arithmetic value to string.
template <concepts::arithmetic Value> inline std::string toString(Value value) {
  if constexpr (std::same_as<Value, bool>) {
    return value ? "true" : "false";
  } else {
    if constexpr (std::floating_point<Value>) {
      if (std::isfinite(value) && value == Value(int64_t(value))) {
        return toString(int64_t(value));
      }
    }
    char chars[32] = {};
    std::to_chars_result result = std::to_chars(&chars[0], &chars[0] + 31, value);
    if (result.ec != std::errc()) {
      const char *reason = "Unknown error";
      if (result.ec == std::errc::value_too_large) reason = "Value too large";
      throw Error(std::invalid_argument(reason));
    }
    return &chars[0];
  }
}

/// Convert string to arithmetic value.
template <concepts::arithmetic Value, concepts::any_string String> inline Value stringTo(const String &str) {
  if constexpr (!concepts::string_view<String>) {
    return stringTo<Value>(toStringView(str));
  } else {
    auto throwError = [&] { throw Error(std::invalid_argument("Can't convert: " + show(str))); };
    auto strv = trim(str);
    if (strv.starts_with("+")) strv = strv.substr(1);
    if (strv.empty()) throwError();
    if constexpr (std::same_as<Value, bool>) {
      if (icaseEqual(strv, "true")) return true;
      if (icaseEqual(strv, "false")) return false;
      return stringTo<unsigned long>(str) != 0UL;
    } else {
      Value value = 0;
      std::from_chars_result result;
      if constexpr (std::integral<Value>) {
        int base = 10;
        if (strv.size() > 2 && (strv.starts_with("0x") || strv.starts_with("0X"))) {
          strv = strv.substr(2), base = 16;
        } else if (strv.size() > 2 && (strv.starts_with("0b") || strv.starts_with("0B"))) {
          strv = strv.substr(2), base = 2;
        } else if (strv.size() > 1 && strv.starts_with("0")) {
          strv = strv.substr(1), base = 8;
        }
        result = std::from_chars(strv.data(), strv.data() + strv.size(), value, base);
      } else {
        result = std::from_chars(strv.data(), strv.data() + strv.size(), value);
      }
      if (result.ec != std::errc()) throwError();
      return value;
    }
  }
  static_assert(std::same_as<char_type_of_t<String>, char>);
}

template <concepts::string_view StringView, typename Pred> struct SplitString {
public:
  using Sentinel = nothing;

  struct Iterator {
  public:
    using difference_type = ptrdiff_t;
    using iterator_category = std::input_iterator_tag;

    constexpr Iterator() noexcept = default;

    constexpr Iterator(StringView src, Pred delim, bool skipEmpty = true) noexcept
      : mSrc(src), mPos0(src.begin()), mPos1(src.begin()), mDelim(std::move(delim)), mSkipEmpty(skipEmpty) {
      operator++();
    }

    constexpr Iterator &operator++() noexcept {
      while (mPos1 < mSrc.end() && mDelim(*mPos1)) {
        ++mPos1;
        if (!mSkipEmpty) break;
      }
      mPos0 = mPos1;
      while (mPos1 < mSrc.end() && !mDelim(*mPos1)) ++mPos1;
      return *this;
    }

    constexpr Iterator operator++(int) noexcept {
      Iterator copy = *this;
      operator++();
      return copy;
    }

    [[nodiscard]] constexpr StringView operator*() const noexcept { return StringView(&*mPos0, mPos1 - mPos0); }

    [[nodiscard]] constexpr bool operator==(Sentinel) const noexcept { return mPos0 == mSrc.end(); }

    [[nodiscard]] constexpr bool operator!=(Sentinel) const noexcept { return mPos0 != mSrc.end(); }

  private:
    StringView mSrc{};

    typename StringView::iterator mPos0{};

    typename StringView::iterator mPos1{};

    Pred mDelim{};

    bool mSkipEmpty{true};

    friend struct SplitString;
  };

  constexpr SplitString() noexcept = default;

  constexpr SplitString(StringView src, Pred delim, bool skipEmpty = true) noexcept : mBegin(src, delim, skipEmpty) {}

  [[nodiscard]] constexpr Iterator begin() const noexcept { return mBegin; }

  [[nodiscard]] constexpr Sentinel end() const noexcept { return {}; }

  [[nodiscard]] constexpr size_t size() const noexcept { return std::ranges::distance(begin(), end()); }

  [[nodiscard]] constexpr StringView at(size_t i) const {
    Iterator itr = begin();
    size_t j = 0;
    while (j < i) {
      if (itr != end())
        ++itr;
      else
        throw Error(std::out_of_range("Index {} out of range: {}"_format(i, show(mBegin.mSrc))));
      ++j;
    }
    return *itr;
  }

  template <size_t N> [[nodiscard]] constexpr std::array<StringView, N> destructure() const {
    std::array<StringView, N> tokens{};
    Iterator itr = begin();
    size_t i = 0;
    while (i < N && itr != end()) tokens[i++] = *itr++;
    if (i != N || itr != end())
      throw Error(std::out_of_range("Destructure expects {} tokens: {}"_format(N, show(mBegin.mSrc))));
    return tokens;
  }

private:
  Iterator mBegin{};
};

template <concepts::any_string String, typename Pred, typename... Args>
SplitString(const String &, Pred &&, Args &&...)
  -> SplitString<std::basic_string_view<char_type_of_t<String>, traits_type_of_t<String>>, std::decay_t<Pred>>;

template <concepts::string_view StringView> struct Scanner {
public:
  using Char = typename StringView::value_type;

  constexpr Scanner() noexcept = default;

  constexpr Scanner(StringView src) noexcept : mState{src, 1} {}

  /// Is end-of-file?
  [[nodiscard]] constexpr bool isEOF() const noexcept { return mState.mSrc.empty(); }

  /// Peek at the next character.
  [[nodiscard]] constexpr Char peek() const noexcept { return isEOF() ? Char() : mState.mSrc[0]; }

  /// Extract the remainder of the source string.
  [[nodiscard]] constexpr StringView remainder() const noexcept { return mState.mSrc; }

  /// Ignore.
  constexpr Scanner &ignore(size_t howMany = 1) noexcept {
    howMany = std::min(howMany, mState.mSrc.size());
    mState.mLineNo += std::count(mState.mSrc.begin(), mState.mSrc.begin() + howMany, '\n');
    mState.mSrc.remove_prefix(howMany);
    return *this;
  }

  /// Ignore while predicate returns true.
  constexpr Scanner &ignore(std::invocable<Char> auto &&pred) noexcept {
    while (!isEOF() && std::invoke(auto_forward(pred), peek())) ignore(1);
    return *this;
  }

  /// Accept character.
  [[nodiscard]] constexpr bool accept(Char token) noexcept { return mState.mSrc.starts_with(token) ? (ignore(), true) : false; }

  /// Accept character string. (Note: This either accepts the entire token and returns true, or rejects the
  /// entire token and returns false. There is no partial consumption.)
  [[nodiscard]] constexpr bool accept(StringView token) noexcept {
    return mState.mSrc.starts_with(token) ? (ignore(token.size()), true) : false;
  }

  /// Accept while predicate returns true. When either the predicate returns false or we reach the end of
  /// string, whichever happens first, stop and return the substring for which the predicate returned true.
  [[nodiscard]] constexpr StringView accept(std::invocable<Char> auto &&pred) noexcept {
    size_t pos = 0;
    while (pos < mState.mSrc.size() && std::invoke(auto_forward(pred), mState.mSrc[pos])) pos++;
    StringView token = mState.mSrc.substr(0, pos);
    ignore(pos);
    return token;
  }

  /// Accept quote between given delimiters, with an escape character to escape the delimiter and
  /// extend the accepted substring. (Note: The implementation succeeds if and only if both delimiters
  /// appear, and the returned string includes both of the delimiters. Importantly, if the next character
  /// is not the opening delimiter or if the end of string is reached before finding an unescaped closing
  /// delimiter, the implementation returns an empty string.)
  [[nodiscard]] constexpr StringView quote(Char delimL = '"', Char delimR = '"', Char escape = '\\') noexcept {
    if (peek() != delimL) return {};
    size_t pos = 1;
    while (pos < mState.mSrc.size() && mState.mSrc[pos] != delimR) pos += (mState.mSrc[pos] == escape) + 1;
    if (++pos > mState.mSrc.size()) return {}; // Fail!
    StringView token = mState.mSrc.substr(0, pos);
    ignore(pos);
    return token;
  }

  /// Demand character. So either accept it, or fail and throw an exception.
  void demand(Char token) {
    if (!accept(token)) fail("Expected {}!"_format(show(token)));
  }

  /// Demand character string. So either accept it, or fail and throw an exception.
  void demand(StringView token) {
    if (!accept(token)) fail("Expected {}!"_format(show(token)));
  }

  /// Push the current state on the stack of save states. (Note: Each call to save()
  /// must be matched by exactly one call to either keep() or rewind(). This functionality is
  /// to permit recursive descent parsing, where there may be multiple candidate ways to parse
  /// something with some understood priority. If the current parse fails, meaning that it
  /// subverted our expected syntax somehow, we can rewind() back to the beginning and try
  /// to parse it a different way, and so on until we either find an acceptable way to parse
  /// the string, or we exhaust all possibilities and fail.)
  void save() { mSaveStates.push_back(mState); }

  /// Pop the most recent save state, keeping all changes made to the current state.
  void keep() {
    if (!mSaveStates.empty()) [[likely]]
      mSaveStates.pop_back();
    else
      throw Error(std::logic_error("No candidate state to keep!"));
  }

  /// Pop the most recent save state and rewind the current state back to it, discarding
  /// any changes made in the meantime.
  void rewind() {
    if (!mSaveStates.empty()) [[likely]]
      mState = mSaveStates.back(), mSaveStates.pop_back();
    else
      throw Error(std::logic_error("No candidate state to rewind to!"));
  }

  void fail(std::string_view message) const { throw Error(std::runtime_error("Line {}: {}"_format(mState.mLineNo, message))); }

private:
  struct State {
    StringView mSrc{};

    size_t mLineNo{1};
  };

  State mState{};

  std::vector<State> mSaveStates;
};

[[nodiscard]] inline std::string typenameString(const std::type_info &info) {
  std::string str = info.name();
#if (__GNUG__ || __clang__)
  int status = 0;
  char *name = abi::__cxa_demangle(str.c_str(), nullptr, nullptr, &status);
  str = name;
  free(name);
#endif // #if (__GNUG__ || __clang__)
  return str;
}

template <typename Type> [[nodiscard]] inline std::string typenameString() {
  if constexpr (std::same_as<Type, bool>)
    return "bool";
  else if constexpr (std::same_as<Type, signed char>)
    return "char";
  else if constexpr (std::same_as<Type, signed short>)
    return "short";
  else if constexpr (std::same_as<Type, signed int>)
    return "int";
  else if constexpr (std::same_as<Type, signed long>)
    return "long";
  else if constexpr (std::same_as<Type, signed long long>)
    return "long long";
  else if constexpr (std::same_as<Type, unsigned char>)
    return "unsigned char";
  else if constexpr (std::same_as<Type, unsigned short>)
    return "unsigned short";
  else if constexpr (std::same_as<Type, unsigned int>)
    return "unsigned int";
  else if constexpr (std::same_as<Type, unsigned long>)
    return "unsigned long";
  else if constexpr (std::same_as<Type, unsigned long long>)
    return "unsigned long long";
  else if constexpr (std::same_as<Type, float>)
    return "float";
  else if constexpr (std::same_as<Type, double>)
    return "double";
  else if constexpr (std::same_as<Type, long double>)
    return "long double";
  else if constexpr (std::same_as<Type, std::complex<float>>)
    return "complex<float>";
  else if constexpr (std::same_as<Type, std::complex<double>>)
    return "complex<double>";
  else if constexpr (std::same_as<Type, std::complex<long double>>)
    return "complex<long double>";
  else
    return typenameString(typeid(Type));
}

/// Open an input filestream or throw an error.
[[nodiscard]] inline std::ifstream openIFStreamOrThrow(const std::string &filename) {
  std::ifstream stream(filename, std::ios::in | std::ios::binary);
  if (!stream.is_open()) throw Error(std::runtime_error("Can't open {}: {}"_format(show(filename), std::strerror(errno))));
  return stream;
}

/// Open an output filestream or throw an error.
[[nodiscard]] inline std::ofstream openOFStreamOrThrow(const std::string &filename) {
  std::ofstream stream(filename, std::ios::out | std::ios::binary);
  if (!stream.is_open()) throw Error(std::runtime_error("Can't open {}: {}"_format(show(filename), std::strerror(errno))));
  return stream;
}

/// Load file to string.
[[nodiscard]] inline std::string loadFileToString(const std::string &filename) {
  auto stream = openIFStreamOrThrow(filename);
  return std::string((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
}

/// Save string to file.
inline void saveStringToFile(const std::string &filename, const std::string &str) {
  auto stream = openOFStreamOrThrow(filename);
  stream.write(str.c_str(), str.size());
}

} // namespace mi
