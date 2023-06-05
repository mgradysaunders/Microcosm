#pragma once

#include "./common.h"

namespace mi {

template <size_t Rank> struct IndexVector : ArrayLike<IndexVector<Rank>> {
  constexpr IndexVector() noexcept = default;

  template <std::integral... Ints> requires(sizeof...(Ints) == Rank)
  [[strong_inline]] constexpr IndexVector(Ints... ints) noexcept : mValues{size_t(ints)...} {}

  [[nodiscard, strong_inline]] constexpr operator size_t() const noexcept requires(Rank == 1) { return mValues[0]; }

  MI_ARRAY_LIKE_CONSTEXPR_DATA(&mValues[0])

  MI_ARRAY_LIKE_STATIC_CONSTEXPR_SIZE(Rank)

  [[nodiscard, strong_inline]] constexpr bool operator==(const IndexVector &other) const noexcept {
    return mValues == other.mValues;
  }

  [[nodiscard, strong_inline]] constexpr bool operator!=(const IndexVector &other) const noexcept {
    return mValues != other.mValues;
  }

  [[nodiscard, strong_inline]] constexpr auto operator<=>(const IndexVector &other) const noexcept {
    return mValues <=> other.mValues;
  }

  [[strong_inline]] constexpr void incrementInPlace(const IndexVector &limit) noexcept {
    for (size_t k = 0; k < Rank; k++) {
      auto &currLimit = limit.mValues[Rank - 1 - k];
      auto &currIndex = this->mValues[Rank - 1 - k];
      if (++currIndex >= currLimit)
        currIndex = 0;
      else
        break;
    }
  }

  std::array<size_t, Rank> mValues{};
};

template <typename... Ints> requires((std::integral<Ints> && ...) && sizeof...(Ints) >= 1)
IndexVector(Ints...) -> IndexVector<sizeof...(Ints)>;

template <size_t N, size_t K> [[nodiscard]] constexpr auto combination(size_t i) noexcept -> IndexVector<K> {
  static_assert(N >= 1 && N >= K);
  IndexVector<K> index{};
  size_t n = 0;
  size_t s = 0;
  for (size_t k = 0; k < K - 1; k++) {
    while (true) {
      if (size_t t = choose((N - 1) - n, (K - 1) - k); s + t <= i) {
        s += t;
        n += 1;
      } else {
        break;
      }
    }
    index[k] = n++;
  }
  if constexpr (K > 1) index[K - 1] = index[K - 2] + 1 + i - s;
  return index;
}

template <size_t N, size_t K> [[nodiscard]] constexpr auto combinations() noexcept {
  return std::views::iota(size_t(0), choose(N, K)) |
         std::views::transform([](auto i) constexpr noexcept { return combination<N, K>(i); });
}

} // namespace mi
