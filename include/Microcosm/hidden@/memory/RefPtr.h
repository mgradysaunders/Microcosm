/*-*- C++ -*-*/
#pragma once

#include "../utility/common.h"
#include <atomic>
#include <memory>

namespace mi {

/// An atomic signed lock-free integer.
///
/// \note
/// This is provided by the C++20 standard as `std::atomic_signed_lock_free`,
/// however it is not quite available in GCC yet. This declaration should be
/// removed once it becomes available.
///
using atomic_signed_lock_free = std::conditional_t<
  std::atomic_ptrdiff_t::is_always_lock_free, std::atomic_ptrdiff_t,
  std::conditional_t<std::atomic_long::is_always_lock_free, std::atomic_long, std::atomic_int>>;

/// A reference-counted pointer.
///
/// This serves the same purpose as `std::shared_ptr`, but makes some
/// simplifying assumptions in order to eliminate the overhead of the
/// container, such that this structure should be the same size as a
/// single pointer.
///
/// The reference count is allocated alongside the reference-counted value
/// and stored at the front of the memory-block. This makes it possible to
/// perform reference counting without 1) having to allocate the reference
/// count separately or 2) having to virtually inherit from some base
/// class. The drawback is that client code _must_ allocate and construct
/// reference-counted values with `mi::make_ref<Value>(...)` or with the
/// in-place constructor. That is, you cannot `new` the pointer yourself
/// and then pass it to this container.
///
template <typename Value> struct RefPtr {
  using Int = atomic_signed_lock_free;

  static constexpr ptrdiff_t IntOffset = alignof(std::max_align_t);

  static_assert(sizeof(Int) <= IntOffset);

  constexpr RefPtr() noexcept = default;

  constexpr RefPtr(std::nullptr_t) noexcept {}

  template <typename... Args> explicit RefPtr(std::in_place_t, Args &&...args) {
    std::byte *ptr0 = new std::byte[IntOffset + sizeof(Value)];
    std::byte *ptr1 = ptr0 + IntOffset;
    new (ptr0) Int(1);
    new (ptr1) Value(std::forward<Args>(args)...);
    mPtr = ptr1;
  }

  explicit RefPtr(std::byte *ptr) noexcept : mPtr(ptr) { increment(); }

  RefPtr(const RefPtr &other) noexcept : mPtr(other.mPtr) { increment(); }

  RefPtr(RefPtr &&other) noexcept : mPtr(steal(other.mPtr)) {}

  ~RefPtr() { decrement(); }

  RefPtr &operator=(const RefPtr &other) noexcept {
    if (mPtr != other.mPtr) {
      decrement();
      mPtr = other.mPtr;
      increment();
    }
    return *this;
  }

  RefPtr &operator=(RefPtr &&other) noexcept {
    decrement();
    mPtr = steal(other.mPtr);
    return *this;
  }

public:
  [[nodiscard]] constexpr Value *get() noexcept { return reinterpret_cast<Value *>(mPtr); }

  [[nodiscard]] constexpr const Value *get() const noexcept { return reinterpret_cast<const Value *>(mPtr); }

  [[nodiscard]] constexpr auto &operator*() noexcept { return *get(); }

  [[nodiscard]] constexpr auto &operator*() const noexcept { return *get(); }

  [[nodiscard]] constexpr auto *operator->() noexcept { return get(); }

  [[nodiscard]] constexpr auto *operator->() const noexcept { return get(); }

  template <typename Other> [[nodiscard]] constexpr auto operator<=>(const RefPtr<Other> &other) const noexcept {
    return mPtr <=> other.mPtr;
  }

  template <typename Other> [[nodiscard]] constexpr bool operator==(const RefPtr<Other> &other) const noexcept {
    return mPtr == other.mPtr;
  }

  template <typename Other> [[nodiscard]] constexpr bool operator!=(const RefPtr<Other> &other) const noexcept {
    return mPtr != other.mPtr;
  }

  [[nodiscard]] constexpr bool operator==(std::nullptr_t) const noexcept { return mPtr == nullptr; }

  [[nodiscard]] constexpr bool operator!=(std::nullptr_t) const noexcept { return mPtr != nullptr; }

  [[nodiscard]] constexpr operator bool() const noexcept { return mPtr != nullptr; }

  template <typename Other> [[nodiscard]] constexpr RefPtr<Other> &cast() noexcept {
    return reinterpret_cast<RefPtr<Other> &>(*this);
  }

  template <typename Other> [[nodiscard]] constexpr const RefPtr<Other> &cast() const noexcept {
    return reinterpret_cast<const RefPtr<Other> &>(*this);
  }

  template <concepts::superclass<Value> Other> [[nodiscard]] constexpr operator RefPtr<Other> &() noexcept {
    return cast<Other>();
  }

  template <concepts::superclass<Value> Other> [[nodiscard]] constexpr operator const RefPtr<Other> &() const noexcept {
    return cast<Other>();
  }

  void reset() noexcept {
    decrement();
    mPtr = nullptr;
  }

  void swap(RefPtr &other) noexcept { std::swap(mPtr, other.mPtr); }

  [[nodiscard]] long use_count() const noexcept { return mPtr ? long(*reinterpret_cast<Int *>(mPtr - IntOffset)) : 0; }

private:
  std::byte *mPtr = nullptr;

  void increment() noexcept {
    if (mPtr) ++*reinterpret_cast<Int *>(mPtr - IntOffset);
  }

  void decrement() noexcept {
    if (mPtr && --*reinterpret_cast<Int *>(mPtr - IntOffset) == 0) {
      reinterpret_cast<Value *>(mPtr)->~Value();
      delete[] (mPtr - IntOffset);
      mPtr = nullptr;
    }
  }
};

template <typename Value, typename... Args> inline auto make_ref(Args &&...args) {
  return RefPtr<Value>(std::in_place, std::forward<Args>(args)...);
}

} // namespace mi
