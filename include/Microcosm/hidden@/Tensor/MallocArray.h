#pragma once

#include "./common.h"

namespace mi {

template <typename Value, size_t SmallSize = 0> struct MallocArray final : ArrayLike<MallocArray<Value, SmallSize>> {
private:
  struct SmallStorage {
    [[nodiscard, strong_inline]] auto size() const noexcept requires(SmallSize != 0) { return mData.size(); }

    [[nodiscard, strong_inline]] auto data() const noexcept requires(SmallSize != 0) { return mData.data(); }

    [[nodiscard, strong_inline]] auto data() noexcept requires(SmallSize != 0) { return mData.data(); }

    void swap(SmallStorage &other) { std::swap(mData, other.mData); }

    std::array<Value, SmallSize> mData;
  };

  struct LargeStorage {
    LargeStorage() noexcept = default;

    LargeStorage(const LargeStorage &other) noexcept {
      mData = static_cast<Value *>(std::malloc(sizeof(Value) * other.mSize));
      mSize = other.mSize;
      std::memcpy(mData, other.mData, sizeof(Value) * mSize);
    }

    LargeStorage(LargeStorage &&other) noexcept : mData(other.mData), mSize(other.mSize) {
      other.mData = nullptr;
      other.mSize = 0;
    }

    ~LargeStorage() { clear(); }

    LargeStorage &operator=(const LargeStorage &other) noexcept {
      if (this != &other) [[likely]] {
        this->~LargeStorage();
        new (this) LargeStorage(other);
      }
      return *this;
    }

    LargeStorage &operator=(LargeStorage &&other) noexcept {
      this->~LargeStorage();
      new (this) LargeStorage(std::move(other));
      return *this;
    }

    [[nodiscard, strong_inline]] auto size() const noexcept { return mSize; }

    [[nodiscard, strong_inline]] auto data() const noexcept { return mData; }

    [[nodiscard, strong_inline]] auto data() noexcept { return mData; }

    void resize(size_t newSize) noexcept {
      if (newSize == 0) {
        clear();
        return;
      }
      mData = static_cast<Value *>(std::realloc(mData, sizeof(Value) * newSize));
      mSize = newSize;
    }

    void clear() noexcept { std::free(mData), mData = nullptr, mSize = 0; }

    void swap(LargeStorage &other) noexcept {
      std::swap(mData, other.mData);
      std::swap(mSize, other.mSize);
    }

    Value *mData{nullptr};

    size_t mSize{0};
  };

public:
  MallocArray() noexcept = default;

  MallocArray(std::initializer_list<Value> values) noexcept {
    resize(values.size());
    std::copy(values.begin(), values.end(), mData);
  }

  MallocArray(std::in_place_t, Value *newData, size_t newSize) noexcept : mData(newData), mSize(newSize) {
    mLarge.mData = newData;
    mLarge.mSize = newSize;
    // If we support small storage and the given data is small enough, then copy it into the
    // small storage and free the original pointer.
    if constexpr (SmallSize != 0) {
      if (mSize <= SmallSize) {
        mData = mSmall.data();
        mLarge.mData = nullptr;
        mLarge.mSize = 0;
        std::copy(newData, newData + newSize, mSmall.data());
        std::free(newData);
      }
    }
  }

  MallocArray(const MallocArray &other) noexcept {
    resize(other.size());
    std::memcpy(mData, other.mData, sizeof(Value) * mSize);
  }

  MallocArray(MallocArray &&other) noexcept : mData(other.mData), mSize(other.mSize), mLarge(std::move(other.mLarge)) {
    if constexpr (SmallSize != 0) {
      mSmall = std::move(other.mSmall);
      cacheDataPointer();
    }
    other.mData = nullptr;
    other.mSize = 0;
  }

  [[strong_inline]] ~MallocArray() { clear(); }

  [[strong_inline]] MallocArray &operator=(const MallocArray &other) {
    if (this != &other) [[likely]] {
      this->~MallocArray();
      new (this) MallocArray(other);
    }
    return *this;
  }

  [[strong_inline]] MallocArray &operator=(MallocArray &&other) {
    this->~MallocArray();
    new (this) MallocArray(std::move(other));
    return *this;
  }

  MI_ARRAY_LIKE_DATA(mData)

  MI_ARRAY_LIKE_SIZE(mSize)

  void resize(size_t newSize) {
    if (newSize == 0) [[unlikely]] {
      clear();
      return;
    }
    if constexpr (SmallSize != 0) {
      if (newSize <= SmallSize) {
        mLarge.clear();
        mData = mSmall.data();
        mSize = newSize;
        return;
      }
    }
    mLarge.resize(newSize);
    mData = mLarge.data();
    mSize = newSize;
    std::fill(mData, mData + mSize, Value());
  }

  void clear() noexcept { mData = nullptr, mSize = 0, mLarge.clear(); }

  void swap(MallocArray &other) {
    if constexpr (SmallSize != 0) {
      std::swap(mSize, other.mSize);
      mLarge.swap(other.mLarge);
      mSmall.swap(other.mSmall);
      this->cacheDataPointer();
      other.cacheDataPointer();
    } else {
      std::swap(mData, other.mData);
      std::swap(mSize, other.mSize);
      mLarge.swap(other.mLarge);
    }
  }

  void onSerialize(auto &serializer) {
    serializer <=> mSize;
    if (serializer.reading()) resize(mSize);
    serializer.readOrWrite(mData, mSize, sizeof(Value));
  }

private:
  Value *mData{nullptr};

  size_t mSize{0};

  LargeStorage mLarge{};

  conditional_member_t<SmallSize != 0, SmallStorage> mSmall{};

  [[nodiscard]] bool isSmall() const noexcept requires(SmallSize != 0) { return mSize <= SmallSize; }

  void cacheDataPointer() noexcept requires(SmallSize != 0) {
    mData = mSize == 0 ? nullptr : mSize <= SmallSize ? mSmall.data() : mLarge.data();
  }
};

} // namespace mi
