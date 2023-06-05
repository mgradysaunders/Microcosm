/*-*- C++ -*-*/
#pragma once

#include "../utility/ArrayLike.h"

namespace mi {

/// A static stack.
///
/// A static stack (first-in last-out), where elements are
/// pushed onto and popped from the top/back.
///
template <typename Value, size_t MaxSize> struct StaticStack : ArrayLike<StaticStack<Value, MaxSize>, true> {
public:
  using Super = ArrayLike<StaticStack<Value, MaxSize>, true>;

  constexpr StaticStack() = default;

  constexpr StaticStack(std::initializer_list<Value> values) : StaticStack(values.begin(), values.end()) {}

  template <std::input_or_output_iterator Iterator> explicit constexpr StaticStack(Iterator from, Iterator to) {
    while (from != to) push(*from++);
  }

public:
  /// \name Container API
  /// \{

  MI_ARRAY_LIKE_CONSTEXPR_DATA(&mValues[0])

  MI_ARRAY_LIKE_CONSTEXPR_SIZE(mTop)

  [[nodiscard]] static constexpr size_t max_size() noexcept { return MaxSize; }

  [[nodiscard]] static constexpr size_t capacity() noexcept { return MaxSize; }

  [[nodiscard]] constexpr bool full() const noexcept { return size() == max_size(); }

  constexpr void clear() noexcept {
    for (Value &value : *this) value.~Value();
    mTop = 0;
  }

  constexpr void resize(size_t sz) {
    if (sz > MaxSize) throw Error(std::runtime_error("Invalid size!"));
    while (mTop < sz) mValues[mTop++] = Value();
    while (mTop > sz) mValues[--mTop].~Value();
  }

  /// \}

public:
  /// \name Stack API
  /// \{

  constexpr Value &emplace() {
    if (mTop >= MaxSize) throw Error(std::length_error("Overflow!"));
    return mValues[mTop++];
  }

  /// Push top/back value.
  ///
  /// \throw std::length_error  If full.
  ///
  constexpr void push(const Value &value) {
    if (mTop >= MaxSize) throw Error(std::length_error("Overflow!"));
    mValues[mTop++] = value;
  }

  /// Pop and return top/back value.
  ///
  /// \throw std::runtime_error  If empty.
  ///
  constexpr Value pop() {
    if (mTop == 0) throw Error(std::runtime_error("Underflow!"));
    Value &value = mValues[--mTop];
    Value result = std::move(value);
    value.~Value();
    return result;
  }

  /// \}

  void onSerialize(auto &serializer) {
    serializer <=> mValues;
    serializer <=> mTop;
  }

private:
  Value mValues[MaxSize] = {};

  size_t mTop = 0;
};

} // namespace mi
