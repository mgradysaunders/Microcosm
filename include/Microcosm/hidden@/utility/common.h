/*-*- C++ -*-*/
#pragma once

#include <cassert>
#include <complex>
#include <concepts>
#include <functional>
#include <iosfwd>
#include <optional>
#include <ranges>
#include <source_location>
#include <stdexcept>
#include <type_traits>
#include <utility>

// Forward-decltype is very useful, but annoying to type every time.
#define auto_forward(X) std::forward<decltype(X)>(X)

// Invoke forward-decltype is very useful, by annoying to type every time.
#define auto_invoke(X, ...) std::invoke(std::forward<decltype(X)>(X), __VA_ARGS__)

// It is very unlikely that Microcosm compiles at all on MSVC right now, but
// in order to be somewhat future-proof we are hiding the always-inline attribute
// behind a macro, since we do not care about GNU-specific behavior. It is just a
// strong suggestion that a function is template meta-garbage that should expand
// like a macro.
#ifndef strong_inline
#if (_MSC_VER && !(__GNUG__ || __clang__))
#define strong_inline msvc::forceinline
#else
#define strong_inline gnu::always_inline
#endif
#endif // #ifndef strong_inline

namespace mi {

template <typename Base> struct Error : public Base {
public:
  Error(Base &&base, const std::source_location &location = std::source_location::current())
    : Base(std::move(base)), location(location) {}
  std::source_location location;
};

template <typename Base, typename... Args> Error(Base &&, Args &&...) -> Error<std::decay_t<Base>>;

template <typename T> struct using_type {
  using type = T;
};

template <typename> struct to_float;

template <typename... T> using to_float_t = typename to_float<std::common_type_t<std::decay_t<T>...>>::type;

template <std::integral T> struct to_float<T> : std::conditional<sizeof(T) < sizeof(float), float, double> {};

template <std::floating_point T> struct to_float<T> : using_type<T> {};

template <std::floating_point T> struct to_float<std::complex<T>> : using_type<T> {};

template <typename T> struct to_float<T &> : using_type<to_float_t<T>> {};

struct Half;

template <> struct to_float<Half> : using_type<float> {};

template <typename T> struct to_field : using_type<std::decay_t<decltype(T() * to_float_t<T>())>> {};

template <typename T> using to_field_t = typename to_field<T>::type;

struct nothing {};

template <bool B, typename T> using conditional_member_t = std::conditional_t<B, T, nothing>;

template <std::default_initializable Object> constexpr decltype(auto) steal(Object &x) noexcept {
  return std::exchange(x, Object());
}

/// An RAII scoped management helper.
template <std::invocable<> Ctor, std::invocable<> Dtor> struct Scope {
  constexpr Scope(Ctor &&ctor, Dtor &&dtor) : mCtor(std::forward<Ctor>(ctor)), mDtor(std::forward<Dtor>(dtor)) {
    std::invoke(this->mCtor);
  }

  constexpr ~Scope() { std::invoke(mDtor); }

  // Non-copyable.
  Scope(const Scope &) = delete;

private:
  Ctor mCtor;
  Dtor mDtor;
};

template <std::invocable<> Ctor, std::invocable<> Dtor> Scope(Ctor &&, Dtor &&) -> Scope<Ctor, Dtor>;

/// An RAII scoped assignment helper.
template <typename Lhs> struct ScopeAssign {
  template <typename Rhs> constexpr ScopeAssign(Lhs &lhs, Rhs &&rhs, bool assign = true) : mValue(lhs) {
    if (assign) {
      mIsAssigned = true;
      mSaved = std::move(mValue);
      mValue = std::forward<Rhs>(rhs);
    }
  }

  constexpr ~ScopeAssign() {
    if (mIsAssigned) mValue = std::move(mSaved);
  }

  ScopeAssign(const ScopeAssign &) = delete;

private:
  bool mIsAssigned = false;
  Lhs mSaved;
  Lhs &mValue;
};

template <typename Lhs, typename Rhs> ScopeAssign(Lhs &, Rhs &&) -> ScopeAssign<Lhs>;

template <typename Lhs, typename Rhs> ScopeAssign(Lhs &, Rhs &&, bool) -> ScopeAssign<Lhs>;

/// An RAII scoped assignment helper.
template <typename Value> struct Preserve {
  constexpr Preserve(Value &value) : mValue(value) { mSaved = mValue; }

  constexpr ~Preserve() { mValue = mSaved; }

  Preserve(const Preserve &) = delete;

private:
  Value mSaved;
  Value &mValue;
};

template <typename Value> Preserve(Value &) -> Preserve<Value>;

namespace ranges {

template <typename Value, size_t N, typename... Values> struct make_array_tuple {
  using type = typename make_array_tuple<Value, N - 1, Value, Values...>::type;
};

template <typename Value, typename... Values> struct make_array_tuple<Value, 0, Values...> {
  using type = std::tuple<Values...>;
};

template <typename Value, size_t N> using make_array_tuple_t = typename make_array_tuple<Value, N>::type;

template <size_t N, std::ranges::range Range> struct Adjacent : std::ranges::view_base {
public:
  constexpr Adjacent() noexcept = default;

  constexpr Adjacent(Range &&range, bool wrap = false) noexcept
    : from(std::ranges::begin(range)), to(std::ranges::end(range)), wrap(wrap) {}

  using range_value = std::ranges::range_value_t<Range>;

  using range_reference = std::ranges::range_reference_t<Range>;

  using range_iterator = std::ranges::iterator_t<Range>;

  using range_sentinel = std::ranges::sentinel_t<Range>;

  struct Sentinel {};

  struct Iterator {
    using difference_type = std::ptrdiff_t;

    using value_type = make_array_tuple_t<range_value, N>;

    using reference = make_array_tuple_t<range_reference, N>;

    using pointer = void;

    using iterator_category = std::forward_iterator_tag;

    [[strong_inline]] constexpr Iterator() noexcept = default;

    [[strong_inline]] constexpr Iterator(range_iterator from, range_sentinel to, bool wrap) noexcept
      : from(from), to(to), wrap(wrap) {
      for (size_t i = 0; i < N; i++) {
        iterators[i] = from;
        for (size_t j = 0; j < i; j++)
          if (++iterators[i] == to) iterators[i] = from;
      }
    }

    [[strong_inline]] constexpr decltype(auto) operator*() noexcept {
      return std::apply([](auto &...iterators) { return reference((*iterators)...); }, iterators);
    }

    [[strong_inline]] constexpr Iterator &operator++() noexcept {
      for (range_iterator &each : iterators) ++each;
      if (wrap)
        for (range_iterator &each : iterators | std::views::drop(1))
          if (each == to) each = from;
      return *this;
    }

    [[strong_inline]] constexpr Iterator operator++(int) noexcept {
      Iterator copy = *this;
      operator++();
      return copy;
    }

    [[nodiscard, strong_inline]] constexpr bool operator==(Sentinel) const noexcept {
      return iterators[wrap ? 0 : N - 1] == to;
    }

    range_iterator from;

    range_sentinel to;

    bool wrap = false;

    std::array<range_iterator, N> iterators;
  };

  [[nodiscard, strong_inline]] constexpr Iterator begin() noexcept { return {from, to, wrap}; }

  [[nodiscard, strong_inline]] constexpr Sentinel end() noexcept { return {}; }

public:
  range_iterator from = {};

  range_sentinel to = {};

  bool wrap = false;
};

template <size_t N, std::ranges::range Range>
[[nodiscard]] constexpr Adjacent<N, Range> adjacent(Range &&range, bool wrap = false) noexcept {
  return {std::forward<Range>(range), wrap};
}

template <std::ranges::range... Ranges> struct zip : std::ranges::view_base {
  [[strong_inline]] constexpr zip() noexcept = default;

  [[strong_inline]] constexpr zip(Ranges &&...ranges) noexcept
    : from(std::forward_as_tuple(std::ranges::begin(ranges)...)), to(std::forward_as_tuple(std::ranges::end(ranges)...)) {}

  using range_values = std::tuple<std::ranges::range_value_t<Ranges>...>;

  using range_references = std::tuple<std::ranges::range_reference_t<Ranges>...>;

  using range_iterators = std::tuple<std::ranges::iterator_t<Ranges>...>;

  using range_sentinels = std::tuple<std::ranges::sentinel_t<Ranges>...>;

  struct Sentinel {
    [[strong_inline]] constexpr Sentinel() noexcept = default;

    [[strong_inline]] constexpr Sentinel(const range_sentinels &sentinels) noexcept : sentinels(sentinels) {}

    range_sentinels sentinels;
  };

  struct Iterator {
    using difference_type = std::ptrdiff_t;

    using value_type = range_values;

    using reference = range_references;

    using pointer = void;

    using iterator_category = std::forward_iterator_tag;

    [[strong_inline]] constexpr Iterator() noexcept = default;

    [[strong_inline]] constexpr Iterator(const range_iterators &iterators) noexcept : iterators(iterators) {}

    [[strong_inline]] constexpr decltype(auto) operator*() noexcept {
      return std::apply([](auto &...iterators) { return reference((*iterators)...); }, iterators);
    }

    [[strong_inline]] constexpr Iterator &operator++() noexcept {
      std::apply([](auto &...iterators) constexpr { ((++iterators), ...); }, iterators);
      return *this;
    }

    [[strong_inline]] constexpr Iterator operator++(int) noexcept {
      Iterator copy = *this;
      operator++();
      return copy;
    }

    [[nodiscard, strong_inline]] constexpr bool operator==(const Sentinel &sentinel) const noexcept {
      return std::get<0>(iterators) == std::get<0>(sentinel.sentinels);
    }

    range_iterators iterators;
  };

  [[nodiscard, strong_inline]] constexpr Iterator begin() noexcept { return from; }

  [[nodiscard, strong_inline]] constexpr Sentinel end() noexcept { return to; }

  Iterator from;

  Sentinel to;
};

template <std::ranges::range... Ranges> zip(Ranges &&...) -> zip<Ranges...>;

template <std::ranges::range Range> [[nodiscard]] constexpr auto enumerate(Range &&range) {
  return zip(std::forward<Range>(range), std::views::iota(0));
}

} // namespace ranges

namespace concepts {

/// Is type a pointer?
template <typename T>
concept pointer = std::is_pointer_v<T>;

/// Is type a reference?
template <typename T>
concept reference = std::is_reference_v<T>;

/// Is type an lvalue reference?
template <typename T>
concept lvalue_reference = std::is_lvalue_reference_v<T>;

/// Is type an rvalue reference?
template <typename T>
concept rvalue_reference = std::is_rvalue_reference_v<T>;

/// Is type idempotent under the decay operation?
template <typename T>
concept decayed = std::same_as<T, std::decay_t<T>>;

/// Is type a subclass of X?
template <typename T, typename X>
concept subclass = std::is_base_of_v<X, T>;

/// Is type a superclass of X?
template <typename T, typename X>
concept superclass = std::is_base_of_v<T, X>;

// Is instantiation?
template <typename, template <typename...> typename> struct is_instantiation : std::false_type {};

// Is instantiation? true specialization.
template <typename... Ts, template <typename...> typename X> struct is_instantiation<X<Ts...>, X> : std::true_type {};

/// Matches template?
template <typename T, template <typename...> typename X>
concept match = is_instantiation<T, X>::value;

/// Does not match template?
template <typename T, template <typename...> typename X>
concept not_match = !is_instantiation<T, X>::value;

/// Matches `std::basic_istream`?
template <typename T>
concept istream = requires { requires subclass<T, std::basic_istream<typename T::char_type, typename T::traits_type>>; };

/// Matches `std::basic_ostream`?
template <typename T>
concept ostream = requires { requires subclass<T, std::basic_ostream<typename T::char_type, typename T::traits_type>>; };

// Is arithmetic?
template <typename T>
concept arithmetic = std::is_arithmetic_v<T>;

// Is arithmetic or enum?
template <typename T>
concept arithmetic_or_enum = std::is_arithmetic_v<T> || std::is_enum_v<T>;

/// Is arithmetic or standard complex?
template <typename T>
concept arithmetic_or_complex = arithmetic<T> || match<T, std::complex>;

/// Is floating point or standard complex?
template <typename T>
concept floating_point_or_complex = std::floating_point<T> || match<T, std::complex>;

/// Is neither arithmetic nor standard complex?
template <typename T>
concept not_arithmetic_or_complex = !arithmetic_or_complex<T>;

template <typename T>
concept number = arithmetic_or_complex<std::decay_t<T>> || std::same_as<typename std::decay_t<T>::number_tag, std::true_type>;

template <typename T>
concept complex = match<T, std::complex>;

/// Is scoped enum?
template <typename T>
concept scoped_enum = std::is_enum_v<T> && !std::is_convertible_v<T, std::underlying_type_t<T>>;

/// Is trivial?
template <typename T>
concept trivial = std::is_trivial_v<T>;

/// Is compound?
template <typename T>
concept compound = std::is_compound_v<T>;

/// Is primitive? (arithmetic or `std::byte`)
template <typename T>
concept primitive = std::same_as<T, std::byte> || (arithmetic<T> && sizeof(T) <= 8);

/// Same as any of the given types?
template <typename T, typename A, typename... B>
concept same_as_any = (std::same_as<T, A> || ... || std::same_as<T, B>);

/// Size is equal to the given number of bytes?
template <typename T, size_t N>
concept size_is = sizeof(T) == N;

// Is functor?
template <typename T>
concept functor = requires { requires std::is_member_function_pointer_v<decltype(&T::operator())>; };

template <typename T>
concept member_function_pointer = std::is_member_function_pointer_v<T>;

/// Is basic container?
template <typename T>
concept basic_container = requires(T &c) {
  requires std::default_initializable<T>;
  requires std::destructible<T>;
  requires std::signed_integral<typename T::difference_type>;
  requires std::unsigned_integral<typename T::size_type>;
  { c.begin() } -> std::convertible_to<typename T::iterator>;
  { c.end() } -> std::convertible_to<typename T::iterator>;
  { c.cbegin() } -> std::convertible_to<typename T::const_iterator>;
  { c.cend() } -> std::convertible_to<typename T::const_iterator>;
  { c.size() } -> std::convertible_to<typename T::size_type>;
};

/// Is allocator aware?
template <typename T>
concept allocator_aware = requires(T &c) {
  requires std::constructible_from<T, const T &, typename T::allocator_type>;
  requires std::constructible_from<T, T &&, typename T::allocator_type>;
  requires std::assignable_from<T, const T &>;
  requires std::assignable_from<T, T &&>;
  { c.get_allocator() } -> std::convertible_to<typename T::allocator_type>;
};

/// Is sequence constructible?
template <typename T>
concept sequence_constructible =
  requires { requires std::constructible_from<T, const typename T::value_type *, const typename T::value_type *>; };

/// Is trivially_destructible?
template <typename T>
concept trivially_destructible = std::is_trivially_destructible_v<T>;

/// Is range of given value type?
template <typename T, typename Value>
concept range_of = (std::ranges::range<T> && std::convertible_to<std::ranges::range_value_t<T> *, Value *>);

/// Is random access range of given value type?
template <typename T, typename Value>
concept random_access_range_of =
  (std::ranges::random_access_range<T> && std::convertible_to<std::ranges::range_value_t<T> *, Value *>);

template <typename T>
concept has_value_type = requires { typename T::value_type; };

} // namespace concepts

template <typename T> struct value_type_of : using_type<T> {};

template <typename T> requires concepts::has_value_type<T> struct value_type_of<T> : using_type<typename T::value_type> {};

template <typename T> using value_type_t = typename value_type_of<std::decay_t<T>>::type;

/// A helper struct to represent user data.
///
/// This is meant to be unsafe/dumb data of trivial type, less
/// than or equal to the size of a pointer.
///
struct UserData {
  constexpr UserData() noexcept = default;

  template <concepts::trivial Data> constexpr UserData(const Data &data) noexcept { as<Data>() = data; }

  template <concepts::trivial Data> constexpr UserData &operator=(const Data &data) noexcept {
    as<Data>() = data;
    return *this;
  }

  template <concepts::trivial Data> [[nodiscard]] constexpr Data &as() noexcept {
    static_assert(sizeof(Data) <= sizeof(void *));
    return *reinterpret_cast<Data *>(&mData[0]);
  }

  template <concepts::trivial Data> [[nodiscard]] constexpr const Data &as() const noexcept {
    static_assert(sizeof(Data) <= sizeof(void *));
    return *reinterpret_cast<const Data *>(&mData[0]);
  }

  constexpr operator bool() const noexcept { return as<void *>() != nullptr; }

  alignas(alignof(void *)) std::byte mData[sizeof(void *)];
};

using std::complex;

using namespace std::complex_literals;

} // namespace mi
