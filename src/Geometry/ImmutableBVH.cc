#include "Microcosm/Geometry/ImmutableBVH"

namespace mi::geometry {

/// An immutable bounding volume hierarchy builder.
template <size_t N> class ImmutableBVHBuilder {
public:
  using BVH = ImmutableBVH<N>;
  using Box = typename BVH::Box;
  using Item = typename BVH::Item;
  using Items = typename BVH::Items;

  struct Node {
    Box box{};            ///< Box.
    Node *left{nullptr};  ///< If branch, left child.
    Node *right{nullptr}; ///< If branch, right child.
    ssize_t splitAxis{0}; ///< If branch, split axis.
    ssize_t firstItem{0}; ///< If leaf, first item index.
    ssize_t itemCount{0}; ///< If leaf, item count.
  };

  Node *root{};
  ssize_t leafLimit{4};
  ssize_t nodeCount{0};
  MemoryArena<> nodeArena{};

public:
  /// Build.
  void build(Items &items) {
    ssize_t firstItem{0};
    root = buildRange(firstItem, IteratorRange(items.data(), items.size()));
    assert(firstItem == ssize_t(items.size()));
  }

  /// Build range recursively.
  [[nodiscard]] Node *buildRange(ssize_t &firstItem, IteratorRange<Item *> items) {
    Node *node{new (nodeArena) Node()};
    nodeCount++;
    Box box, boxCenter;
    for (const Item &item : items) box |= item.box, boxCenter |= item.boxCenter;
    ssize_t splitAxis{static_cast<ssize_t>(argmax(boxCenter.extent()))};
    ssize_t itemCount{static_cast<ssize_t>(items.size())};
    if (itemCount <= leafLimit) { // Leaf?
      *node = {box, nullptr, nullptr, 0, firstItem, itemCount};
      firstItem += itemCount;
    } else {
      Item *split{findSplitSAH(boxCenter, splitAxis, items)};
      Node *child0{buildRange(firstItem, {items.begin(), split})};
      Node *child1{buildRange(firstItem, {split, items.end()})};
      *node = {box, child0, child1, splitAxis, 0, 0};
    }
    return node;
  }

  /// Find split using surface area heuristic.
  [[nodiscard]] static Item *findSplitSAH(const Box &boxCenter, ssize_t splitAxis, IteratorRange<Item *> items) {
    constexpr ssize_t Nbins = 8;
    using Bin = std::pair<Box, ssize_t>;
    if (boxCenter.lower()[splitAxis] == boxCenter.upper()[splitAxis]) return findSplitEqualCounts(splitAxis, items);

    auto itemIndex = [&](const Item &item) {
      float factor = unlerp(
        item.boxCenter[splitAxis],    //
        boxCenter.lower()[splitAxis], //
        boxCenter.upper()[splitAxis]);
      return std::min<ssize_t>(Nbins * factor, Nbins - 1);
    };

    // Initialize bins.
    std::array<Bin, Nbins> bins{};
    for (const Item &item : items) {
      ssize_t index{itemIndex(item)};
      bins[index].first |= item.box;
      bins[index].second++;
    }

    // Initialize sweeps.
    std::array<Bin, Nbins - 1> sweepL{};
    std::array<Bin, Nbins - 1> sweepR{};
    {
      auto sweepLItr = sweepL.begin(), binsLItr = bins.begin();
      auto sweepRItr = sweepR.rbegin(), binsRItr = bins.rbegin();
      *sweepLItr++ = *binsLItr++;
      *sweepRItr++ = *binsRItr++;
      for (; sweepLItr < sweepL.end(); ++sweepLItr, ++binsLItr, ++sweepRItr, ++binsRItr) {
        sweepLItr->first = (sweepLItr - 1)->first | binsLItr->first;
        sweepRItr->first = (sweepRItr - 1)->first | binsRItr->first;
        sweepLItr->second = (sweepLItr - 1)->second + binsLItr->second;
        sweepRItr->second = (sweepRItr - 1)->second + binsRItr->second;
      }
    }

    // Compute costs to find best split index.
    float minCost{constants::Inff};
    ssize_t minCostIndex{0};
    for (ssize_t costIndex = 0; costIndex < Nbins - 1; ++costIndex) {
      if (auto cost{
            sweepL[costIndex].first.hyperArea() * sweepL[costIndex].second +
            sweepR[costIndex].first.hyperArea() * sweepR[costIndex].second};
          minCost > cost) {
        minCost = cost;
        minCostIndex = costIndex;
      }
    }

    // Partition.
    if (auto best = std::partition(items.begin(), items.end(), [&](auto &item) { return itemIndex(item) <= minCostIndex; });
        best != items.begin() && best != items.end())
      return best;
    else
      return findSplitEqualCounts(splitAxis, items);
  }

  /// Find split using equal counts.
  [[nodiscard]] static Item *findSplitEqualCounts(ssize_t splitAxis, IteratorRange<Item *> items) {
    Item *split = items.begin() + items.size() / 2;
    std::nth_element(items.begin(), split, items.end(), [=](const Item &item0, const Item &item1) -> bool {
      return item0.boxCenter[splitAxis] < item1.boxCenter[splitAxis];
    });
    return split;
  }

  /// Collapse.
  static void collapse(Node *from, auto &nodes) {
    assert(from);
    auto &node = nodes.emplace_back();
    node.box = from->box;
    if (from->itemCount > 0) {
      assert(!from->left);
      assert(!from->right);
      node.first = from->firstItem;
      node.count = from->itemCount;
      node.split = from->splitAxis;
    } else {
      collapse(from->left, nodes);
      node.right = &nodes.back() - &node + 1;
      node.count = 0;
      node.split = from->splitAxis;
      collapse(from->right, nodes);
    }
  }
};

template <size_t N> inline void ImmutableBVH<N>::build(int leafLimit, Items &items) {
  if (leafLimit < 1) leafLimit = 1;

  // Run builder.
  ImmutableBVHBuilder<N> builder;
  builder.leafLimit = leafLimit;
  builder.build(items);

  // Collapse.
  nodes.clear();
  nodes.reserve(builder.nodeCount);
  ImmutableBVHBuilder<N>::collapse(builder.root, nodes);
}

template class ImmutableBVH<2>;
template class ImmutableBVH<3>;

} // namespace mi::geometry
