/*-*- C++ -*-*/
#pragma once

#include "./common.h"

namespace mi {

namespace concepts {

template <typename Range, typename Iterator, typename Sentinel>
concept compatible_range = std::convertible_to<std::ranges::iterator_t<Range>, Iterator> &&
                           std::convertible_to<std::ranges::sentinel_t<Range>, Sentinel>;

template <typename Range, typename Iterator, typename Sentinel>
concept compatible_range_from_data =
  !compatible_range<Range, Iterator, Sentinel> && std::ranges::contiguous_range<Range> && std::same_as<Iterator, Sentinel> &&
  std::convertible_to<std::remove_reference_t<std::ranges::range_reference_t<Range>> *, Iterator>;

} // namespace concepts

/// An iterator range.
template <std::input_or_output_iterator Iterator, std::sentinel_for<Iterator> Sentinel = Iterator>
struct IteratorRange : std::ranges::view_base {
public:
  using iterator = Iterator;

  using sentinel = Sentinel;

  using value_type = std::iter_value_t<iterator>;

  using difference_type = std::iter_difference_t<iterator>;

  using reference = std::iter_reference_t<iterator>;

  static constexpr bool contiguous = concepts::pointer<iterator> && std::same_as<iterator, sentinel>;

public:
  /// Default constructor.
  constexpr IteratorRange() noexcept = default;

  /// Construct directly from iterator and sentinel.
  constexpr IteratorRange(Iterator from, Sentinel to) noexcept : mBegin(from), mEnd(to) {}

  constexpr IteratorRange(Iterator from) noexcept requires(std::same_as<Sentinel, nothing>) : mBegin(from) {}

  /// Construct from iterator/sentinel pair.
  constexpr IteratorRange(const std::pair<Iterator, Sentinel> &range) noexcept : IteratorRange(range.first, range.second) {}

  /// Construct from a compatible range.
  template <concepts::compatible_range<Iterator, Sentinel> Range>
  constexpr IteratorRange(Range &&range) noexcept : IteratorRange(std::ranges::begin(range), std::ranges::end(range)) {}

  /// Construct from a compatible range from data and size.
  ///
  /// \note
  /// This only applies to contiguous ranges where both the iterator and
  /// sentinel are the same pointer type. The idea is to allow implicit
  /// construction from containers like `std::vector` to keep code readable.
  ///
  template <concepts::compatible_range_from_data<Iterator, Sentinel> Range>
  constexpr IteratorRange(Range &&range) noexcept
    : IteratorRange(std::ranges::data(range), std::ranges::data(range) + std::ranges::size(range)) {}

  /// Construct from an initializer list of the value type.
  ///
  /// \note
  /// This is useful to allow users to pass an initializer directly to
  /// a function accepting an iterator range, without having to declare
  /// a temporary array.
  ///
  constexpr IteratorRange(std::initializer_list<value_type> values) noexcept requires(contiguous)
    : IteratorRange(values.begin(), values.end()) {}

  /// Construct from a pointer and a count.
  constexpr IteratorRange(Iterator first, size_t count) noexcept requires(contiguous) : IteratorRange(first, first + count) {}

public:
  [[nodiscard, strong_inline]] constexpr auto size() const noexcept { return std::ranges::distance(mBegin, mEnd); }

  [[nodiscard, strong_inline]] constexpr auto data() const noexcept requires(contiguous) { return mBegin; }

  [[nodiscard, strong_inline]] constexpr bool empty() const noexcept { return mBegin == mEnd; }

  [[nodiscard, strong_inline]] constexpr auto begin() const noexcept { return mBegin; }

  [[nodiscard, strong_inline]] constexpr auto end() const noexcept { return mEnd; }

  [[nodiscard, strong_inline]] constexpr reference front() const { return *mBegin; }

  [[nodiscard, strong_inline]] constexpr reference back() const { return operator[](size() - 1); }

  [[nodiscard, strong_inline]] constexpr reference operator[](difference_type pos) const noexcept {
    return *std::ranges::next(mBegin, pos);
  }

private:
  Iterator mBegin{};

  Sentinel mEnd{};

public:
  [[strong_inline]] constexpr void fill(auto &&what) {
    for (auto &&each : *this) each = what;
  }

  [[nodiscard, strong_inline]] constexpr bool contains(auto &&what) {
    for (auto &&each : *this)
      if (each == what) return true;
    return false;
  }

  [[nodiscard, strong_inline]] constexpr auto count(auto &&what) {
    size_t result = 0;
    for (auto &&each : *this) result += each == what ? 1 : 0;
    return result;
  }

  [[nodiscard, strong_inline]] constexpr auto countIf(auto &&pred) {
    size_t result = 0;
    for (auto &&each : *this) result += std::invoke(auto_forward(pred), each) ? 1 : 0;
    return result;
  }

  [[nodiscard, strong_inline]] constexpr auto find(auto &&what) {
    auto each = mBegin;
    while (each != mEnd && !(*each == what)) ++each;
    return each;
  }

  [[nodiscard, strong_inline]] constexpr auto findIf(auto &&pred) {
    auto each = mBegin;
    while (each != mEnd && !std::invoke(auto_forward(pred), *each)) ++each;
    return each;
  }

  [[nodiscard, strong_inline]] constexpr decltype(auto) findIfOrElse(auto &&elseValue, auto &&pred) {
    auto each = mBegin;
    while (each != mEnd && !std::invoke(auto_forward(pred), *each)) ++each;
    return each != mEnd ? (*each) : auto_forward(elseValue);
  }
};

template <std::input_or_output_iterator Iterator, std::sentinel_for<Iterator> Sentinel>
IteratorRange(Iterator &&, Sentinel &&) -> IteratorRange<Iterator, Sentinel>;

template <std::input_or_output_iterator Iterator, std::sentinel_for<Iterator> Sentinel>
IteratorRange(std::pair<Iterator, Sentinel> &&) -> IteratorRange<Iterator, Sentinel>;

template <std::ranges::range Range>
IteratorRange(Range &&) -> IteratorRange<std::ranges::iterator_t<Range>, std::ranges::sentinel_t<Range>>;

template <concepts::pointer Iterator> IteratorRange(Iterator, size_t) -> IteratorRange<Iterator>;

} // namespace mi
