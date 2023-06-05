/*-*- C++ -*-*/
#pragma once

#include "../utility/ArrayLike.h"

namespace mi {

template <typename Value, size_t Size = 64> struct GrowableStack : ArrayLike<GrowableStack<Value, Size>, true> {
public:
  constexpr GrowableStack() noexcept { mStack = &mArray[0]; }

  GrowableStack(const GrowableStack &) = delete;

  GrowableStack(GrowableStack &&) = delete;

  ~GrowableStack() {
    if (mStack != &mArray[0]) mAlloc.deallocate(mStack, mStackCapacity);
    mStack = nullptr;
    mStackTop = 0;
    mStackCapacity = 0;
  }

public:
  /// \name Container API
  /// \{

  MI_ARRAY_LIKE_CONSTEXPR_DATA(mStack)

  MI_ARRAY_LIKE_CONSTEXPR_SIZE(mStackTop)

  [[nodiscard]] constexpr size_t max_size() const noexcept { return std::numeric_limits<size_t>::max(); }

  [[nodiscard]] constexpr size_t capacity() const noexcept { return mStackCapacity; }

  constexpr void clear() noexcept {
    for (auto &value : *this) value.~Value();
    mStackTop = 0;
  }

  /// \}

public:
  /// \name Stack API
  /// \{

  constexpr void push(const Value &value) {
    if (mStackCapacity == mStackTop) {
      mStackCapacity *= 2;
      Value *stack = mAlloc.allocate(mStackCapacity);
      std::move(mStack, mStack + mStackTop, stack);
      std::swap(mStack, stack);
      if (stack != &mArray[0]) mAlloc.deallocate(stack, mStackTop);
    }
    mStack[mStackTop++] = value;
  }

  constexpr Value pop() { return mStack[--mStackTop]; }

  /// \}

private:
  Value mArray[Size];

  Value *mStack = nullptr;

  size_t mStackTop = 0;

  size_t mStackCapacity = Size;

  std::allocator<Value> mAlloc;
};

} // namespace mi
