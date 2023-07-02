/*-*- C++ -*-*/
#pragma once

#include "../utility/ArrayLike.h"
#include <vector>

namespace mi {

template <std::integral Int, typename Node, Int Node::*Next = &Node::next> struct IntrusivePoolVector : ArrayLike<IntrusivePoolVector<Int, Node, Next>> {
public:
  static constexpr Int None{-1};

public:
  MI_ARRAY_LIKE_DATA(mNodes.data())

  MI_ARRAY_LIKE_SIZE(mNodes.size())

  [[nodiscard]] size_t numActive() const noexcept { return mNumActive; }

  void clear() noexcept { mNodes.clear(), mNextFree = None, mNumActive = 0; }

  Int allocate() {
    if (mNextFree == None) {
      size_t next = mNodes.size();
      mNodes.resize(next == 0 ? 32 : 2 * next);
      for (size_t node = next; node < mNodes.size(); node++) mNodes[node].*Next = Int(node + 1);
      mNodes.back().*Next = None;
      mNextFree = Int(next);
    }
    mNumActive++;
    size_t node = size_t(mNextFree);
    mNextFree = mNodes[node].*Next, mNodes[node] = Node();
    return node;
  }

  void deallocate(Int node) noexcept {
    mNodes[node].~Node();
    mNodes[node].*Next = mNextFree, mNextFree = node;
    mNumActive--;
  }

private:
  std::vector<Node> mNodes;

  Int mNextFree{None};

  size_t mNumActive{0};
};

} // namespace mi
