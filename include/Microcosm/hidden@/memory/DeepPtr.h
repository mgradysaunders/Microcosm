/*-*- C++ -*-*/
#pragma once

#include <memory>

namespace mi {

template <typename Value, typename Deleter = std::default_delete<Value>> struct DeepPtr {
public:
  using pointer = Value *;

  using element_type = Value;

  using deleter_type = Deleter;

  constexpr DeepPtr() noexcept = default;

  constexpr DeepPtr(std::nullptr_t) noexcept {}

  explicit DeepPtr(pointer p) noexcept : mPtr(p) {}

  template <typename... Args> DeepPtr(pointer p, Args &&...args) noexcept : mPtr(p, std::forward<Args>(args)...) {}

  DeepPtr(const DeepPtr &other) {
    if (other) mPtr.reset(new Value(*other.mPtr));
  }

  DeepPtr(DeepPtr &&other) noexcept : mPtr(std::move(other.mPtr)) {}

  template <typename OtherValue, typename OtherDeleter> DeepPtr(DeepPtr<OtherValue, OtherDeleter> &&other) noexcept : mPtr(std::move(other.mPtr)) {}

  DeepPtr &operator=(const DeepPtr &other) {
    if (this != &other) {
      if (other)
        reset(new Value(*other));
      else
        reset();
    }
    return *this;
  }

  DeepPtr &operator=(DeepPtr &&other) {
    mPtr = std::move(other.mPtr);
    return *this;
  }

  template <typename OtherValue, typename OtherDeleter> DeepPtr &operator=(DeepPtr<OtherValue, OtherDeleter> &&other) {
    mPtr = std::move(other.mPtr);
    return *this;
  }

  [[nodiscard]] pointer release() noexcept { return mPtr.release(); }

  template <typename... Args> void reset(Args &&...args) noexcept { mPtr.reset(std::forward<Args>(args)...); }

  void swap(DeepPtr &other) noexcept { mPtr.swap(other.mPtr); }

  auto &operator*() noexcept { return mPtr.operator*(); }

  auto &operator*() const noexcept { return mPtr.operator*(); }

  auto *operator->() noexcept { return mPtr.operator->(); }

  auto *operator->() const noexcept { return mPtr.operator->(); }

  [[nodiscard]] auto *get() const noexcept { return mPtr.get(); }

  [[nodiscard]] auto &get_deleter() noexcept { return mPtr.get_deleter(); }

  [[nodiscard]] auto &get_deleter() const noexcept { return mPtr.get_deleter(); }

  operator bool() const noexcept { return mPtr != nullptr; }

  template <typename... Other> auto operator<=>(const DeepPtr<Other...> &other) const noexcept { return mPtr.get() <=> other.mPtr.get(); }

  template <typename... Other> bool operator==(const DeepPtr<Other...> &other) const noexcept { return mPtr.get() == other.mPtr.get(); }

  template <typename... Other> bool operator!=(const DeepPtr<Other...> &other) const noexcept { return mPtr.get() != other.mPtr.get(); }

  auto operator<=>(std::nullptr_t) const noexcept { return mPtr.get() <=> nullptr; }

  bool operator==(std::nullptr_t) const noexcept { return mPtr.get() == nullptr; }

  bool operator!=(std::nullptr_t) const noexcept { return mPtr.get() != nullptr; }

  explicit operator std::unique_ptr<Value, Deleter> &() noexcept { return mPtr; }

private:
  std::unique_ptr<Value, Deleter> mPtr;
};

template <typename Value> DeepPtr(Value *) -> DeepPtr<Value>;

} // namespace mi
