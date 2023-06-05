#pragma once

#include "./common.h"

namespace mi {

static constexpr size_t Dynamic = size_t(-1);

static constexpr size_t ToEnd = size_t(-2);

template <size_t...> struct Slice;

namespace concepts {

template <typename> struct is_slice : std::false_type {};

template <size_t... Args> struct is_slice<Slice<Args...>> : std::true_type {};

template <typename T>
concept slice = is_slice<std::decay_t<T>>::value;

} // namespace concepts

template <> struct Slice<> {
  template <size_t> static constexpr size_t Size = Dynamic;

  static constexpr bool IsNoOp = false;

  constexpr Slice() noexcept = default;

  constexpr Slice(size_t from, size_t to = ToEnd) noexcept : from{from}, to{to} {}

  [[nodiscard]] constexpr size_t offset() const noexcept { return from; }

  [[nodiscard]] constexpr size_t extent(size_t size) const noexcept {
    size = std::min(size, to);
    return from > size ? 0 : size - from;
  }

  const size_t from{0}, to{ToEnd};
};

template <> struct Slice<ToEnd> {
  template <size_t> static constexpr size_t Size = Dynamic;

  static constexpr bool IsNoOp = false;

  constexpr Slice() noexcept = default;

  constexpr Slice(size_t from) noexcept : from(from) {}

  [[nodiscard]] constexpr size_t offset() const noexcept { return from; }

  [[nodiscard]] constexpr size_t extent(size_t size) const noexcept { return from > size ? 0 : size - from; }

  const size_t from{0};
};

template <size_t From, size_t To> struct Slice<From, To> {
  static_assert(To >= From);

  static_assert(To != Dynamic && From != Dynamic);

  template <size_t CurrSize>
  static constexpr size_t Size = To == ToEnd ? (CurrSize == Dynamic ? Dynamic : CurrSize - From) : To - From;

  static constexpr bool IsNoOp = From == 0 && To == ToEnd;

  [[nodiscard]] static constexpr size_t offset() noexcept { return From; }

  [[nodiscard]] static constexpr size_t extent(size_t size) noexcept {
    if constexpr (To == ToEnd)
      return From > size ? 0 : size - From;
    else
      return To - From;
  }
};

Slice(auto, auto) -> Slice<>;

Slice(auto) -> Slice<ToEnd>;

// clang-format off
Slice() -> Slice<0, ToEnd>;
// clang-format on

using SliceToEnd = Slice<ToEnd>;

} // namespace mi
