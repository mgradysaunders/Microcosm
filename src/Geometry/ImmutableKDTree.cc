#include "Microcosm/Geometry/ImmutableKDTree"

namespace mi::geometry {

template <size_t N> typename ImmutableKDTree<N>::Nearest ImmutableKDTree<N>::nearestTo(Point point) const {
  Nearest near;
  GrowableStack<const Node *> todo;
  if (!nodes.empty()) todo.push(&nodes[0]);
  while (!todo.empty()) {
    const Node *node = todo.pop();
    float dist = distanceSquare(node->point, point);
    if (near.dist > dist) {
      near.dist = dist;
      near.node = node;
    }
    float diff = node->point[node->axis] - point[node->axis];
    const Node *childA = node + node->left;
    const Node *childB = node + node->right;
    if (childA != node && (diff > 0 || diff * diff < near.dist)) todo.push(childA);
    if (childB != node && (diff < 0 || diff * diff < near.dist)) todo.push(childB);
  }
  if (isfinite(near.dist)) near.dist = sqrt(near.dist);
  return near;
}

template <size_t N> void ImmutableKDTree<N>::nearestTo(Point point, IteratorRange<Nearest *> near) const {
  if (near.size() == 0) return;
  if (near.size() == 1) {
    near[0] = nearestTo(point);
    return;
  }
  near.fill(Nearest());
  Nearest *heap = near.begin();
  GrowableStack<const Node *> todo;
  if (!nodes.empty()) todo.push(&nodes[0]);
  while (!todo.empty()) {
    const Node *node = todo.pop();
    float dist = distanceSquare(node->point, point);
    if (heap != near.end() || dist < near[0].dist) {
      if (heap == near.end()) std::pop_heap(near.begin(), heap--);
      heap->node = node;
      heap->dist = dist;
      std::push_heap(near.begin(), ++heap);
    }
    float diff = node->point[node->axis] - point[node->axis];
    const Node *childA = node + node->left;
    const Node *childB = node + node->right;
    if (childA != node && (heap != near.end() || diff > 0 || diff * diff < near[0].dist)) todo.push(childA);
    if (childB != node && (heap != near.end() || diff < 0 || diff * diff < near[0].dist)) todo.push(childB);
  }
  std::sort_heap(near.begin(), heap);
  for (auto &each : near)
    if (isfinite(each.dist)) each.dist = sqrt(each.dist);
}

template <size_t N> class ImmutableKDTBuilder {
public:
  using KDTree = ImmutableKDTree<N>;
  using Point = typename KDTree::Point;
  using Box = typename KDTree::Box;
  using Item = typename KDTree::Item;
  using Items = typename KDTree::Items;

  struct Node {
    Point point;
    Node *childA = nullptr;
    Node *childB = nullptr;
    ssize_t index = 0;
    ssize_t axis = 0;
  };

  Node *root = nullptr;
  MemoryArena<> nodeArena = {};

public:
  /// Build.
  void build(Items &items) { root = buildRange({items.data(), items.data() + items.size()}); }

  /// Build range recursively.
  Node *buildRange(IteratorRange<Item *> items) {
    if (items.size() == 0) return nullptr;
    Node *node = new (nodeArena) Node();
    if (items.size() == 1) {
      node->point = items[0].point;
      node->index = items[0].index;
      return node;
    }
    Box box;
    for (Item &item : items) box |= item.point;
    int axis = argmax(box.extent());
    Item *middle = items.begin() + items.size() / 2;
    std::nth_element(
      items.begin(), middle, items.end(), [&](const Item &lhs, const Item &rhs) { return lhs.point[axis] < rhs.point[axis]; });
    node->point = middle->point;
    node->index = middle->index;
    node->childA = buildRange({items.begin(), middle});
    node->childB = buildRange({middle + 1, items.end()});
    node->axis = axis;
    return node;
  }

  /// Collapse.
  static void collapse(Node *from, auto &nodes) {
    assert(from);
    auto &node = nodes.emplace_back();
    node.point = from->point;
    node.index = from->index;
    node.right = 0;
    node.left = 0;
    node.axis = from->axis;
    if (from->childA) {
      node.left = 1;
      collapse(from->childA, nodes);
    }
    if (from->childB) {
      node.right = &nodes.back() - &node + 1;
      collapse(from->childB, nodes);
    }
  }
};

template <size_t N> void ImmutableKDTree<N>::build(Items &items) {
  // Run builder.
  ImmutableKDTBuilder<N> builder;
  builder.build(items);

  // Collapse.
  nodes.clear();
  nodes.reserve(items.size());
  ImmutableKDTBuilder<N>::collapse(builder.root, nodes);
}

template class ImmutableKDTree<2>;
template class ImmutableKDTree<3>;

} // namespace mi::geometry
