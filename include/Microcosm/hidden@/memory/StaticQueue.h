/*-*- C++ -*-*/
#pragma once

#include "../utility/ArrayLike.h"

namespace mi {

/// A static queue.
///
/// A static queue (first-in first-out), where elements are
/// pushed onto the top/back and popped from the bottom/front.
///
/// \note
/// The implementation minimizes move operations by tracking
/// the effective bottom and top of the queue. That is, popping
/// shifts the bottom up instead of moving the elements
/// down. When pushing shifts the top past the queue capacity, the
/// implementation moves all elements down and resets the bottom
/// to zero.
///
template <typename Value, size_t MaxSize> struct StaticQueue : ArrayLike<StaticQueue<Value, MaxSize>, true> {
public:
  // Sanity check.
  static_assert(MaxSize > 0);

  using Super = ArrayLike<StaticQueue<Value, MaxSize>, true>;

  using Super::Super;

public:
  /// \name Container API
  /// \{

  MI_ARRAY_LIKE_CONSTEXPR_DATA(&mValues[0] + mBottom)

  MI_ARRAY_LIKE_CONSTEXPR_SIZE(mTop - mBottom)

  [[nodiscard]] static constexpr size_t max_size() noexcept { return MaxSize; }

  [[nodiscard]] static constexpr size_t capacity() noexcept { return MaxSize; }

  [[nodiscard]] constexpr bool full() const noexcept { return size() == max_size(); }

  constexpr void clear() noexcept {
    for (Value &value : *this) value.~Value();
    mBottom = mTop = 0;
  }

  /// \}

public:
  /// \name Queue API
  /// \{

  /// Push top/back value.
  ///
  /// \throw std::length_error  If full.
  ///
  constexpr void push(const Value &value) {
    if (mTop >= MaxSize) {
      if (mBottom == 0) throw Error(std::length_error("Overflow!"));
      Value *itr0 = &mValues[0];
      Value *itr1 = &mValues[0] + mBottom;
      for (; itr1 < &mValues[0] + mTop; ++itr0, ++itr1) {
        *itr0 = std::move(*itr1);
        itr1->~Value();
      }
      mTop -= mBottom;
      mBottom = 0;
    }
    mValues[mTop++] = value;
  }

  /// Pop and return bottom/front value.
  ///
  /// \throw std::runtime_error  If empty.
  ///
  constexpr Value pop() {
    if (mBottom == mTop) throw Error(std::runtime_error("Underflow!"));
    Value &value = mValues[mBottom++];
    Value res = std::move(value);
    value.~Value();
    if (mBottom == mTop) // Empty now?
      mBottom = mTop = 0;
    return res;
  }

  /// \}

  void onSerialize(auto &serializer) {
    serializer <=> mValues;
    serializer <=> mBottom;
    serializer <=> mTop;
  }

private:
  Value mValues[MaxSize] = {};

  size_t mBottom = 0;

  size_t mTop = 0;
};

} // namespace mi
