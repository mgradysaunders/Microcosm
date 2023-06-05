#include "Microcosm/Geometry/DynamicBVH"

namespace mi::geometry {

template <size_t N> void DynamicBVH<N>::privateInsert(Int leaf) {
  if (mRoot == None) {
    mRoot = leaf;
    mNodes[mRoot].parent = None;
    return;
  }
  Box leafBox = mNodes[leaf].box;
  Int node = mRoot;
  while (mNodes[node].isBranch()) {
    Node &nodeRef = mNodes[node];
    Node &childARef = mNodes[nodeRef.childA];
    Node &childBRef = mNodes[nodeRef.childB];
    float area = nodeRef.box.hyperArea();
    float combinedArea = (leafBox | nodeRef.box).hyperArea();
    float cost = 2 * combinedArea;
    float costChild0 = 2 * (combinedArea - area);
    float costChild1 = 2 * (combinedArea - area);
    costChild0 += (leafBox | childARef.box).hyperArea();
    costChild1 += (leafBox | childBRef.box).hyperArea();
    if (childARef.isBranch()) costChild0 -= childARef.box.hyperArea();
    if (childBRef.isBranch()) costChild1 -= childBRef.box.hyperArea();
    if (cost < costChild0 && cost < costChild1)
      break;
    else
      node = costChild0 < costChild1 ? nodeRef.childA : nodeRef.childB;
  }
  Int oldParent = mNodes[node].parent;
  Int newParent = mNodes.allocate();
  Node &newParentRef = mNodes[newParent];
  newParentRef.box = leafBox | mNodes[node].box;
  newParentRef.parent = oldParent;
  newParentRef.childA = node;
  newParentRef.childB = leaf;
  newParentRef.height = mNodes[node].height + 1;
  mNodes[node].parent = newParent;
  mNodes[leaf].parent = newParent;
  if (oldParent != None) {
    if (mNodes[oldParent].childA == node)
      mNodes[oldParent].childA = newParent;
    else
      mNodes[oldParent].childB = newParent;
  } else
    mRoot = newParent; // Sibling was root

  // Traverse back to root, rebalancing and updating heights.
  node = mNodes[leaf].parent;
  while (node != None) {
    node = privateBalance(node);
    Node &nodeRef = mNodes[node];
    Node &childARef = mNodes[nodeRef.childA];
    Node &childBRef = mNodes[nodeRef.childB];
    nodeRef.height = max(childARef.height, childBRef.height);
    nodeRef.height++;
    nodeRef.box = childARef.box | childBRef.box;
    node = nodeRef.parent;
  }
}

template <size_t N> void DynamicBVH<N>::privateRemove(Int leaf) {
  if (mRoot == leaf) {
    mRoot = None;
    return;
  }
  Int parent = mNodes[leaf].parent;
  Int sibling = mNodes[parent].childA == leaf ? mNodes[parent].childB : mNodes[parent].childA;
  if (Int grandparent = mNodes[parent].parent; grandparent != None) {
    if (mNodes[grandparent].childA == parent)
      mNodes[grandparent].childA = sibling;
    else
      mNodes[grandparent].childB = sibling;
    mNodes[sibling].parent = grandparent;
    mNodes[parent].height = -1;
    mNodes.deallocate(parent);

    Int node = grandparent;
    while (node != None) {
      node = privateBalance(node);
      Node &nodeRef = mNodes[node];
      Node &childARef = mNodes[nodeRef.childA];
      Node &childBRef = mNodes[nodeRef.childB];
      nodeRef.box = childARef.box | childBRef.box;
      nodeRef.height = max(childARef.height, childBRef.height);
      nodeRef.height++;
      node = nodeRef.parent;
    }
  } else {
    mRoot = sibling;
    mNodes[sibling].parent = None;
    mNodes[parent].height = -1;
    mNodes.deallocate(parent);
  }
}

template <size_t N> typename DynamicBVH<N>::Int DynamicBVH<N>::privateBalance(Int node) {
  // Rotate B above its parent A.
  auto rotate = [&](Int nodeB, Int nodeA) {
    Node &nodeARef = mNodes[nodeA];
    Node &nodeBRef = mNodes[nodeB];
    Node &nodeCRef = mNodes[nodeARef.childA == nodeB ? nodeARef.childB : nodeARef.childA];
    Int nodeD = nodeBRef.childA;
    Int nodeE = nodeBRef.childB;
    if (mNodes[nodeD].height < mNodes[nodeE].height) std::swap(nodeD, nodeE);
    Node &nodeDRef = mNodes[nodeD];
    Node &nodeERef = mNodes[nodeE];
    nodeBRef.childA = nodeA;
    nodeBRef.childB = nodeD;
    nodeBRef.parent = nodeARef.parent;
    nodeARef.parent = nodeB;
    if (nodeBRef.parent != None) {
      if (mNodes[nodeBRef.parent].childA == nodeA)
        mNodes[nodeBRef.parent].childA = nodeB;
      else
        mNodes[nodeBRef.parent].childB = nodeB;
    } else
      mRoot = nodeB;
    if (nodeARef.childA == nodeB)
      nodeARef.childA = nodeE;
    else
      nodeARef.childB = nodeE;
    nodeERef.parent = nodeA;
    nodeARef.box = nodeCRef.box | nodeERef.box;
    nodeBRef.box = nodeARef.box | nodeDRef.box;
    nodeARef.height = 1 + max(nodeCRef.height, nodeERef.height);
    nodeBRef.height = 1 + max(nodeARef.height, nodeDRef.height);
  };
  // Do tree rotation if necessary.
  Node &nodeRef = mNodes[node];
  if (nodeRef.isLeaf() || nodeRef.height < 2) return node;
  Int childA = nodeRef.childA;
  Int childB = nodeRef.childB;
  Int imbalance = mNodes[childB].height - mNodes[childA].height;
  if (imbalance > 1) {
    rotate(childB, node);
    return childB;
  }
  if (imbalance < -1) {
    rotate(childA, node);
    return childA;
  }
  return node;
}

template class DynamicBVH<2>;

template class DynamicBVH<3>;

} // namespace mi::geometry
