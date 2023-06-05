/*-*- C++ -*-*/
#pragma once

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

namespace mi {

template <typename Alloc = std::allocator<std::byte>> class MemoryPool {
public:
  using allocator_type = Alloc;

  using allocator_traits = std::allocator_traits<Alloc>;

  template <typename Value> using rebind_alloc = typename allocator_traits::template rebind_alloc<Value>;

  struct Block {
    Block *next = nullptr;
  };

  struct Chunk {
    Block *blocks = nullptr;
    size_t blockSize = 0;
  };

  MemoryPool(const Alloc &alloc = Alloc()) : mChunks(alloc), mAlloc(alloc) { mChunks.reserve(128); }

  MemoryPool(const MemoryPool &) = delete;

  ~MemoryPool() { clear(); }

  [[nodiscard]] void *allocate(size_t size) {
    if (size > MaxBlockSize) return mAlloc.allocate(size);
    if (size == 0) return nullptr;
    size_t pool = SizeToPool(size);
    if (!mUnused[pool]) {
      auto *blocks = reinterpret_cast<Block *>(mAlloc.allocate(ChunkSize));
      size_t blockSize = PoolToSize(pool);
      size_t blockCount = ChunkSize / blockSize;
      auto block = [&](size_t i) { return reinterpret_cast<Block *>(reinterpret_cast<std::byte *>(blocks) + i * blockSize); };
      for (size_t i = 0; i + 1 < blockCount; i++) block(i)->next = block(i + 1);
      block(blockCount - 1)->next = nullptr;
      mChunks.push_back(Chunk{blocks, blockSize});
      mUnused[pool] = blocks;
    }
    return std::exchange(mUnused[pool], mUnused[pool]->next);
  }

  template <typename Type, typename... Args> [[nodiscard]] Type *allocate(Args &&...args) {
    return new (allocate(sizeof(Type))) Type(std::forward<Args>(args)...);
  }

  void deallocate(void *ptr, size_t size) {
    if (size == 0 || ptr == nullptr) return;
    if (size > MaxBlockSize) {
      mAlloc.deallocate(static_cast<std::byte *>(ptr), size);
      return;
    }
    size_t pool = SizeToPool(size);
    auto *block = static_cast<Block *>(ptr);
    block->next = mUnused[pool];
    mUnused[pool] = block;
  }

  void deallocate(auto *ptr) { deallocate(ptr, sizeof(*ptr)); }

  void clear() {
    for (auto &chunk : mChunks) {
      mAlloc.deallocate(reinterpret_cast<std::byte *>(chunk.blocks), ChunkSize);
      chunk.blocks = nullptr;
      chunk.blockSize = 0;
    }
    mChunks.clear();
    std::fill(&mUnused[0], &mUnused[0] + PoolCount, nullptr);
  }

private:
  std::vector<Chunk, rebind_alloc<Chunk>> mChunks;

  rebind_alloc<std::byte> mAlloc;

  static constexpr size_t MaxBlockSize = 640;

  static constexpr size_t ChunkSize = 16384;

  static constexpr size_t PoolCount = 14;

  static size_t PoolToSize(size_t pool) noexcept {
    static const size_t sizes[] = {16, 32, 64, 96, 128, 160, 192, 224, 256, 320, 384, 448, 512, 640};
    return sizes[pool];
  }

  static size_t SizeToPool(size_t size) noexcept {
    static const size_t sizes[] = {16, 32, 64, 96, 128, 160, 192, 224, 256, 320, 384, 448, 512, 640};
    size_t pool = 0;
    while (size > sizes[pool] && pool < PoolCount) pool++;
    return pool;
  }

  Block *mUnused[PoolCount]{};
};

} // namespace mi
