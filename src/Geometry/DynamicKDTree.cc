#include "Microcosm/Geometry/DynamicKDTree"

namespace mi::geometry {

template <size_t N> typename DynamicKDTree<N>::Box DynamicKDTree<N>::region(Int node) const {
  Box box = mBox;
  Point &minPoint = box[0];
  Point &maxPoint = box[1];
  Vector<bool, N> minFlags{};
  Vector<bool, N> maxFlags{};
  Point point = mNodes[node].point;
  Int parent = None;
  for (parent = mNodes[node].parent; parent != None; parent = mNodes[parent].parent) {
    const Node &ref = mNodes[parent];
    if (point < ref) {
      minimize(maxPoint[ref.axis], ref.threshold());
      maxFlags[ref.axis] = true;
    } else {
      maximize(minPoint[ref.axis], ref.threshold());
      minFlags[ref.axis] = true;
    }
    if (allTrue(minFlags & maxFlags)) break;
  }
  return box;
}

template <size_t N> typename DynamicKDTree<N>::Nearest DynamicKDTree<N>::nearestTo(Point point) const {
  Nearest near;
  GrowableStack<Int> todo;
  if (mRoot != None) todo.push(mRoot);
  while (!todo.empty()) {
    Int node = todo.pop();
    const Node &nodeRef = mNodes[node];
    if (!nodeRef.dead) {
      float dist = distanceSquare(nodeRef.point, point);
      if (near.dist > dist) {
        near.dist = dist;
        near.node = node;
      }
    }
    float diff = nodeRef.point[nodeRef.axis] - point[nodeRef.axis];
    if (nodeRef.childA != None && (diff > 0 || diff * diff < near.dist)) todo.push(nodeRef.childA);
    if (nodeRef.childB != None && (diff < 0 || diff * diff < near.dist)) todo.push(nodeRef.childB);
  }
  if (isfinite(near.dist)) near.dist = sqrt(near.dist);
  return near;
}

template <size_t N> void DynamicKDTree<N>::nearestTo(Point point, IteratorRange<Nearest *> near) const {
  if (near.size() == 0) return;
  if (near.size() == 1) {
    near[0] = nearestTo(point);
    return;
  }
  near.fill(Nearest());
  Nearest *heap = near.begin();
  GrowableStack<Int> todo;
  if (mRoot != None) todo.push(mRoot);
  while (!todo.empty()) {
    Int node = todo.pop();
    const Node &nodeRef = mNodes[node];
    float dist = distanceSquare(nodeRef.point, point);
    if (heap != near.end() || dist < near[0].dist) {
      if (heap == near.end()) std::pop_heap(near.begin(), heap--);
      heap->node = node;
      heap->dist = dist;
      std::push_heap(near.begin(), ++heap);
    }
    float diff = nodeRef.point[nodeRef.axis] - point[nodeRef.axis];
    if (nodeRef.childA != None && (heap != near.end() || diff > 0 || diff * diff < near[0].dist)) todo.push(nodeRef.childA);
    if (nodeRef.childB != None && (heap != near.end() || diff < 0 || diff * diff < near[0].dist)) todo.push(nodeRef.childB);
  }
  std::sort_heap(near.begin(), heap);
  for (auto &each : near)
    if (isfinite(each.dist)) each.dist = sqrt(each.dist);
}

template <size_t N> typename DynamicKDTree<N>::Int DynamicKDTree<N>::privateSelectAxis(Int node) {
  auto &nodeRef = mNodes[node];
  auto cost = Point(constants::Inf<float>);
  for (Int axis = 0; axis < Int(N); axis++) {
    nodeRef.axis = axis;
    Box box = region(node), box0 = box, box1 = box;
    box0[0][axis] = nodeRef.threshold();
    box1[1][axis] = nodeRef.threshold();
    float cost0{box0.hyperArea() / box0.hyperVolume()};
    float cost1{box1.hyperArea() / box1.hyperVolume()};
    if (isfinite(cost0) && isfinite(cost1))
      cost[axis] = max(cost0, cost1);
    else
      cost[axis] = isfinite(cost0) ? cost0 : cost1;
  }
  return argmin(cost);
}

template <size_t N> void DynamicKDTree<N>::privateInsert(Int node) {
  mBox |= mNodes[node].point;
  if (mRoot == None) {
    mRoot = node;
    mNodes[mRoot].parent = None;
    return;
  }
  Int walk = mRoot;
  while (true) {
    Node &nodeRef = mNodes[node];
    Node &walkRef = mNodes[walk];
    if (walkRef.axis == -1) walkRef.axis = privateSelectAxis(walk);
    Int *childA = &walkRef.childA;
    Int *childB = &walkRef.childB;
    Int *child = nodeRef.point < walkRef ? childA : childB;
    if (*child == None) {
      *child = node;
      nodeRef.parent = walk;
      break;
    }
    walk = *child;
  }
  // Update heights.
  Int imbalance = 0;
  walk = mNodes[node].parent;
  while (walk != None) {
    Node &nodeRef = mNodes[walk];
    Int childA = nodeRef.childA;
    Int childB = nodeRef.childB;
    Int height0 = childA != None ? mNodes[childA].height : 0;
    Int height1 = childB != None ? mNodes[childB].height : 0;
    maximize(imbalance, std::abs(height1 - height0));
    nodeRef.height = std::max(height0, height1);
    nodeRef.height++;
    walk = nodeRef.parent;
  }
  if (mAutomaticRebalance && imbalance > 4) privateRebalance();
}

template <size_t N> bool DynamicKDTree<N>::privateRemove(Int node) {
  Node &nodeRef = mNodes[node];
  Int childA = nodeRef.childA;
  Int childB = nodeRef.childB;
  if (childA != None && childB != None) {
    nodeRef.dead = true;
    mDeadCount++;
    if (mAutomaticRebalance && mDeadCount > mNodeCount / 2) privateRebalance();
    return false;
  }
  Int child = childA != None ? childA : childB;
  Int parent = nodeRef.parent;
  if (parent != None) {
    Node &parentRef = mNodes[parent];
    if (parentRef.childA == node)
      parentRef.childA = child;
    else
      parentRef.childB = child;
  }
  if (child != None) mNodes[child].parent = parent;
  // Update heights.
  node = parent;
  while (node != None) {
    Node &nodeRef = mNodes[node];
    childA = nodeRef.childA;
    childB = nodeRef.childB;
    Int height0 = childA != None ? mNodes[childA].height : 0;
    Int height1 = childB != None ? mNodes[childB].height : 0;
    nodeRef.height = std::max(height0, height1);
    nodeRef.height++;
    node = nodeRef.parent;
  }
  return true;
}

template <size_t N> void DynamicKDTree<N>::privateRebalance() {
  std::vector<Int> nodes;
  nodes.reserve(mNodes.size());
  mBox = {};
  for (Int node = 0; node < Int(mNodes.size()); node++) {
    if (mNodes[node].dead) { // Dead?
      mNodes[node].height = -1;
      mNodes.deallocate(node);
      mNodeCount--;
      continue;
    }
    Node &nodeRef = mNodes[node];
    if (nodeRef.height >= 0) {
      nodeRef.parent = None;
      nodeRef.childA = None;
      nodeRef.childB = None;
      nodeRef.height = 0;
      nodeRef.axis = None;
      nodeRef.dead = false;
      nodes.push_back(node);
      mBox |= nodeRef.point;
    }
  }
  mDeadCount = 0;
  mRoot = privateRebalance({nodes.data(), nodes.data() + nodes.size()});
  mRebalanceCount++;
}

template <size_t N> typename DynamicKDTree<N>::Int DynamicKDTree<N>::privateRebalance(IteratorRange<Int *> nodes) {
  if (nodes.empty()) return None;
  if (nodes.size() == 1) return nodes[0];
  Box box;
  for (Int node : nodes) box |= mNodes[node].point;
  Int axis = argmax(box.extent());
  Int *middle = nodes.begin() + nodes.size() / 2;
  std::nth_element(
    nodes.begin(), middle, nodes.end(), [&](Int lhs, Int rhs) { return mNodes[lhs].point[axis] < mNodes[rhs].point[axis]; });
  Int childA = privateRebalance({nodes.begin(), middle});
  Int childB = privateRebalance({middle + 1, nodes.end()});
  Node &middleRef = mNodes[*middle];
  middleRef.axis = axis;
  middleRef.childA = childA;
  middleRef.childB = childB;
  middleRef.height = 0;
  Node &childARef = mNodes[childA];
  Node &childBRef = mNodes[childB];
  if (childA != None) {
    childARef.parent = *middle;
    middleRef.height = 1 + childARef.height;
  }
  if (childB != None) {
    childBRef.parent = *middle;
    if (middleRef.height < 1 + childBRef.height) middleRef.height = 1 + childBRef.height;
  }
  return *middle;
}

template class DynamicKDTree<2>;
template class DynamicKDTree<3>;

} // namespace mi::geometry
