/*-*- C++ -*-*/
#pragma once

#include "../utility/algorithm.h"
#include <memory>

namespace mi {

/// A heap-allocated memory arena.
template <typename Alloc = std::allocator<std::byte>> class MemoryArena {
public:
  using allocator_type = Alloc;
  using allocator_traits = std::allocator_traits<Alloc>;

  MemoryArena(size_t blockSize = 0, const Alloc &alloc = Alloc()) noexcept
    : mBlockSize(blockSize), //
      mFree(alloc),          //
      mFull(alloc), mAlloc(alloc) {
    // Round up to 256 byte interval.
    mBlockSize += 255u;
    mBlockSize &= ~255u;
    if (mBlockSize == 0) mBlockSize = 65536;
    // Allocate initial block, reserve blocks.
    mBlock.size = mBlockSize;
    mBlock.begin = mAlloc.allocate(mBlock.size);
    mBlock.offset = 0;
    mFree.reserve(4);
    mFull.reserve(4);
  }

  MemoryArena(const MemoryArena &) = delete;

  MemoryArena(MemoryArena &&other) noexcept
    : mBlockSize(steal(other.mBlockSize)), //
      mBlock(steal(other.mBlock)),         //
      mFree(std::move(other.mFree)),       //
      mFull(std::move(other.mFull)),       //
      mAlloc(std::move(other.mAlloc)) {}

  MemoryArena(MemoryArena &&other, const Alloc &alloc) : mAlloc(alloc) {
    // We need to be able to deallocate the pointers we steal!
    if (mAlloc != other.alloc) throw Error(std::invalid_argument("Incompatible allocators!"));
    mBlockSize = steal(other.mBlockSize);
    mBlock = steal(other.mBlock);
    mFree = std::move(other.mFree);
    mFull = std::move(other.mFull);
  }

  ~MemoryArena() {
    mAlloc.deallocate(mBlock.begin, mBlock.size);
    for (auto &each : mFree) mAlloc.deallocate(each.begin, each.size);
    for (auto &each : mFull) mAlloc.deallocate(each.begin, each.size);
  }

  MemoryArena &operator=(const MemoryArena &) = delete;

  MemoryArena &operator=(MemoryArena &&other) noexcept {
    mBlockSize = steal(other.mBlockSize);
    mBlock = steal(other.mBlock);
    mFree = std::move(other.mFree);
    mFull = std::move(other.mFull);
    if constexpr (allocator_traits::propagate_on_container_move_assignment::value) mAlloc = std::move(other.mAlloc);
    return *this;
  }

public:
  /// Allocate bytes.
  [[nodiscard]] void *allocate(size_t size) {
    // Round up to 16 byte interval.
    size = (size + 15) & ~15;
    if (size == 0) return nullptr;

    if (mBlock.size < mBlock.offset + size) {
      mFull.emplace_back(mBlock);
      if (mFree.empty() || mFree.back().size < size) {
        // Allocate block.
        mBlock.size = std::max(mBlockSize, size);
        mBlock.begin = mAlloc.allocate(mBlock.size);
        mBlock.offset = 0;
      } else {
        // Use free block.
        mBlock = mFree.back();
        mFree.pop_back();
      }
    }
    std::byte *pos = mBlock.begin + mBlock.offset;
    mBlock.offset += size;
    return static_cast<void *>(pos);
  }

  /// Allocate given type.
  template <typename Value> [[nodiscard]] Value *allocate(size_t count = 1) { return static_cast<Value *>(allocate(sizeof(Value) * count)); }

  /// Clear.
  void clear() {
    // Clear current block.
    mBlock.offset = 0;
    // Convert full blocks to free blocks.
    mFree.reserve(mFree.size() + mFull.size());
    for (auto &block : mFull) {
      mFree.push_back(block);
      mFree.back().offset = 0;
    }
    mFull.clear();
  }

  /// Clear and deallocate.
  void reset() {
    mBlock.offset = 0;
    for (auto &each : mFree) mAlloc.deallocate(each.begin, each.size);
    for (auto &each : mFull) mAlloc.deallocate(each.begin, each.size);
    mFree.clear();
    mFull.clear();
  }

  void swap(MemoryArena &other) {
    if (this != &other) {
      std::swap(mBlockSize, other.mBlockSize);
      std::swap(mBlock, other.mBlock);
      std::swap(mFree, other.mFree);
      std::swap(mFull, other.mFull);
      if constexpr (allocator_traits::propagate_on_container_swap::value) std::swap(mAlloc, other.mAlloc);
    }
  }

private:
  /// A memory block.
  struct Block {
    std::byte *begin; ///< Pointer to bytes.
    size_t offset;    ///< Offset.
    size_t size;      ///< Size.
  };

  size_t mBlockSize = 0;
  Block mBlock;

  template <typename Value> using RebindAlloc = typename allocator_traits::template rebind_alloc<Value>;
  std::vector<Block, RebindAlloc<Block>> mFree;
  std::vector<Block, RebindAlloc<Block>> mFull;
  RebindAlloc<std::byte> mAlloc;
};

/// A standard-compatible memory arena allocator.
template <typename Value, typename Alloc = std::allocator<std::byte>> class MemoryArenaAllocator {
public:
  using value_type = Value;
  using propagate_on_container_copy_assignment = std::true_type;
  using propagate_on_container_move_assignment = std::true_type;
  using propagate_on_container_swap = std::true_type;
  using is_always_equal = std::false_type;

public:
  MemoryArenaAllocator(size_t blockSize = 0, const Alloc &alloc = Alloc()) : mArena(new MemoryArena<Alloc>(blockSize, alloc)) {}

  template <typename Other> MemoryArenaAllocator(const MemoryArenaAllocator<Other, Alloc> &other) : mArena(other.mArena) {}

  template <typename Other> MemoryArenaAllocator(MemoryArenaAllocator<Other, Alloc> &&other) : mArena(std::move(other.mArena)) {}

  template <typename Other> MemoryArenaAllocator &operator=(const MemoryArenaAllocator<Other, Alloc> &other) {
    if (this != &other) {
      this->mArena = other.mArena;
    }
    return *this;
  }

  template <typename Other> MemoryArenaAllocator &operator=(MemoryArenaAllocator<Other, Alloc> &&other) {
    this->mArena = std::move(other.mArena);
    return *this;
  }

  void clear() { mArena->clear(); }

  void reset() { mArena->reset(); }

  [[nodiscard]] Value *allocate(size_t n) { return static_cast<Value *>(mArena->allocate(sizeof(Value) * n)); }

  void deallocate(Value *, size_t) {}

  template <typename Other> bool operator==(const MemoryArenaAllocator<Other, Alloc> &other) const { return mArena.get() == other.mArena.get(); }

  template <typename Other> bool operator!=(const MemoryArenaAllocator<Other, Alloc> &other) const { return mArena.get() != other.mArena.get(); }

private:
  std::shared_ptr<MemoryArena<Alloc>> mArena;

  template <typename, typename> friend class MemoryArenaAllocator;
};

} // namespace mi

template <typename Alloc> inline void *operator new(size_t size, mi::MemoryArena<Alloc> &arena) { return arena.allocate(size); }

template <typename Alloc> inline void *operator new[](size_t size, mi::MemoryArena<Alloc> &arena) { return arena.allocate(size); }

template <typename Alloc> inline void operator delete(void *, mi::MemoryArena<Alloc> &) noexcept {}

template <typename Alloc> inline void operator delete[](void *, mi::MemoryArena<Alloc> &) noexcept {}
