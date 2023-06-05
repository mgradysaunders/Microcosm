/*-*- C++ -*-*/
#pragma once

#include "./GrowableStack.h"

namespace mi {

template <typename Value, size_t Size = 64, typename Pred = std::less<Value>> struct GrowableHeap : GrowableStack<Value, Size> {
public:
  using Super = GrowableStack<Value, Size>;

  constexpr GrowableHeap() noexcept = default;

  constexpr GrowableHeap(Pred &&pred) noexcept : pred(pred) {}

  GrowableHeap(const GrowableHeap &) = delete;

  GrowableHeap(GrowableHeap &&) = delete;

public:
  /// \name Heap API
  /// \{

  constexpr void push(const Value &value) {
    Super::push(value);
    std::push_heap(this->begin(), this->end(), pred);
  }

  constexpr Value pop() {
    std::pop_heap(this->begin(), this->end(), pred);
    return Super::pop();
  }

  /// \}

  Pred pred;
};

template <typename Value, size_t Size = 64> using GrowableMinHeap = GrowableHeap<Value, Size, std::greater<Value>>;

template <typename Value, size_t Size = 64> using GrowableMaxHeap = GrowableHeap<Value, Size, std::less<Value>>;

} // namespace mi
