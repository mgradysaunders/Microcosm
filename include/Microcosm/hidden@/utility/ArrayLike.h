/*-*- C++ -*-*/
#pragma once

#include "./common.h"
#include <cstddef>
#include <iterator>
#include <tuple>
#include <type_traits>

namespace mi {

/// Array-like CRTP.
template <typename Subclass, bool NegativeFromBack = false> struct ArrayLike {
  using this_is_array_like = std::true_type;

  using size_type = size_t;

  using difference_type = std::ptrdiff_t;

  [[nodiscard]] constexpr auto *begin() noexcept { return static_cast<Subclass &>(*this).data(); }

  [[nodiscard]] constexpr auto *begin() const noexcept { return static_cast<const Subclass &>(*this).data(); }

  [[nodiscard]] constexpr auto *end() noexcept { return begin() + static_cast<const Subclass &>(*this).size(); }

  [[nodiscard]] constexpr auto *end() const noexcept { return begin() + static_cast<const Subclass &>(*this).size(); }

  [[nodiscard]] constexpr bool empty() const noexcept { return static_cast<const Subclass &>(*this).size() == 0; }

  [[nodiscard]] constexpr auto cbegin() const noexcept { return static_cast<const Subclass &>(*this).begin(); }

  [[nodiscard]] constexpr auto cend() const noexcept { return static_cast<const Subclass &>(*this).end(); }

  [[nodiscard]] constexpr auto rbegin() noexcept { return std::reverse_iterator(static_cast<Subclass &>(*this).end()); }

  [[nodiscard]] constexpr auto rbegin() const noexcept {
    return std::reverse_iterator(static_cast<const Subclass &>(*this).end());
  }

  [[nodiscard]] constexpr auto crbegin() const noexcept { return rbegin(); }

  [[nodiscard]] constexpr auto rend() noexcept { return std::reverse_iterator(static_cast<Subclass &>(*this).begin()); }

  [[nodiscard]] constexpr auto rend() const noexcept {
    return std::reverse_iterator(static_cast<const Subclass &>(*this).begin());
  }

  [[nodiscard]] constexpr auto crend() const noexcept { return rend(); }

  [[nodiscard]] constexpr auto &front() noexcept { return *static_cast<Subclass &>(*this).begin(); }

  [[nodiscard]] constexpr auto &front() const noexcept { return *static_cast<const Subclass &>(*this).begin(); }

  [[nodiscard]] constexpr auto &back() noexcept { return *rbegin(); }

  [[nodiscard]] constexpr auto &back() const noexcept { return *rbegin(); }

  template <std::integral Int> [[nodiscard]] constexpr auto &operator[](Int i) noexcept {
    if constexpr (NegativeFromBack && std::signed_integral<Int>) i += i < 0 ? static_cast<Subclass &>(*this).size() : 0;
    return static_cast<Subclass &>(*this).begin()[i];
  }

  template <std::integral Int> [[nodiscard]] constexpr auto &operator[](Int i) const noexcept {
    if constexpr (NegativeFromBack && std::signed_integral<Int>) i += i < 0 ? static_cast<const Subclass &>(*this).size() : 0;
    return static_cast<const Subclass &>(*this).begin()[i];
  }

  template <std::integral Int> [[nodiscard]] constexpr auto &at(Int i) {
    Int sz = static_cast<Subclass &>(*this).size();
    if constexpr (NegativeFromBack && std::signed_integral<Int>) i += i < 0 ? sz : 0;
    if (i < 0 || i >= sz) throw Error(std::out_of_range("Index out of range!"));
    return static_cast<Subclass &>(*this).begin()[i];
  }

  template <std::integral Int> [[nodiscard]] constexpr auto &at(Int i) const {
    Int sz = static_cast<const Subclass &>(*this).size();
    if constexpr (NegativeFromBack && std::signed_integral<Int>) i += i < 0 ? sz : 0;
    if (i < 0 || i >= sz) throw Error(std::out_of_range("Index out of range!"));
    return static_cast<const Subclass &>(*this).begin()[i];
  }

  template <size_t Index> [[nodiscard]] constexpr auto &get() noexcept { return begin()[Index]; }

  template <size_t Index> [[nodiscard]] constexpr auto &get() const noexcept { return begin()[Index]; }

  template <std::integral Int> [[nodiscard]] constexpr bool is_out_of_range(Int i) const noexcept {
    Int sz = static_cast<const Subclass &>(*this).size();
    if constexpr (NegativeFromBack && std::signed_integral<Int>) i += i < 0 ? sz : 0;
    return i < 0 || i >= sz;
  }

  template <typename What> [[nodiscard]] constexpr bool contains(const What &what) const {
    for (const auto &value : *this)
      if (value == what) return true;
    return false;
  }
};

#define MI_ARRAY_LIKE_DATA(Data)                                      \
  [[nodiscard, strong_inline]] auto *data() noexcept { return Data; } \
  [[nodiscard, strong_inline]] auto *data() const noexcept { return Data; }

#define MI_ARRAY_LIKE_CONSTEXPR_DATA(Data)                                      \
  [[nodiscard, strong_inline]] constexpr auto *data() noexcept { return Data; } \
  [[nodiscard, strong_inline]] constexpr auto *data() const noexcept { return Data; }

#define MI_ARRAY_LIKE_SIZE(Size) \
  [[nodiscard, strong_inline]] size_t size() const noexcept { return Size; }

#define MI_ARRAY_LIKE_CONSTEXPR_SIZE(Size) \
  [[nodiscard, strong_inline]] constexpr size_t size() const noexcept { return Size; }

#define MI_ARRAY_LIKE_STATIC_CONSTEXPR_SIZE(Size) \
  [[nodiscard, strong_inline]] static constexpr size_t size() noexcept { return Size; }

namespace concepts {

template <typename T>
concept array_like = std::same_as<std::true_type, typename T::this_is_array_like>;

template <typename T>
concept array_like_constant_size = array_like<T> && requires {
  { T::size() } -> std::convertible_to<size_t>;
};

} // namespace concepts

} // namespace mi

namespace std {

template <mi::concepts::array_like_constant_size Subclass>
struct tuple_size<Subclass> : public integral_constant<size_t, Subclass::size()> {};

template <size_t Index, mi::concepts::array_like_constant_size Subclass> struct tuple_element<Index, Subclass> {
  using type = std::ranges::range_value_t<Subclass>;
};

} // namespace std
