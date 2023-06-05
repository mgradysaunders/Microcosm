#include "Microcosm/Geometry/HalfEdgeMesh"
#include "Microcosm/Quaternion"
#include <deque>

namespace mi::geometry {

void HalfEdgeMesh::initialize(const Mesh &mesh) {
  clear();
  const auto &positions{mesh.positions};
  const auto &texcoords{mesh.texcoords};
  const auto &faces{mesh.faces};
  auto verts{std::vector<Vert *>(positions.size())};
  auto edges{std::map<std::pair<Vert *, Vert *>, Edge *>()};
  // Allocate vertexes.
  for (size_t i = 0; i < positions.size(); i++) verts[i] = mElems.allocate<Vert>(), verts[i]->position = positions[i];
  // Allocate faces.
  for (size_t i = 0; i < faces.size(); i++) {
    if (faces[i].count < 3) [[unlikely]] // Ignore blatantly non-manifold faces.
      continue;
    Edge *last{nullptr};
    Face *face{mElems.allocate<Face>()};
    for (uint32_t j = 0; j < faces[i].count; j++) {
      // Lookup adjacent vertexes in the face.
      auto &vertA{verts[positions.f[faces[i][j + 0]]]};
      auto &vertB{verts[positions.f[faces[i][j + 1]]]};

      // Lookup or allocate the pair of half-edges between vertex A and vertex B.
      auto &edge{edges[std::make_pair(vertA, vertB)]};
      auto &twin{edges[std::make_pair(vertB, vertA)]};
      if (!edge) edge = mElems.allocate<Edge>();
      if (!twin) twin = mElems.allocate<Edge>();

      // If the main half-edge is already initialized, then there are either more than two faces meeting here, and/or there
      // is a mismatch in the winding of the faces (as in something like a Mobius strip).
      if (edge->face) throw Error(std::runtime_error("Tried to build topology for non-manifold mesh!"));

      // Link the half-edges to each other, and to the vertexes in question. Also link the main half-edge to the
      // face we are building.
      edge->twin = twin, edge->vert = vertA, vertA->edge = edge, edge->face = face;
      twin->twin = edge, twin->vert = vertB;

      // If this is the very first iteration of the loop, then store the initial half-edge in the face. We'll use
      // it to complete the linkage at the end. If it is *not* the first iteration of the loop, then link to the half-edge
      // we initialized in the last iteration.
      if (j == 0) face->edge = edge;
      if (j != 0) linkLoop(last, edge);
      last = edge; // Remember the last half-edge for the next iteration.

      // If applicable, look up the texture coordinate.
      if (texcoords) edge->texcoord = texcoords.v[texcoords.f[faces[i][j]]];
    }
    // Complete the loop.
    linkLoop(last, face->edge);
  }
  // Interior edges should have correct connectivity by now. However, boundary edges are still isolated. The
  // boundary edges are those edges which have no associated face. Boundaries should still form closed loops.
  // We visit each boundary edge and walk around its associated vertex clockwise (stepping over interior
  // edges) until we find the boundary edge that logically precedes, then link them together.
  for (Edge *edge : allEdges()) {
    if (edge->face) continue; // Skip non-boundary edges.

    // If the mesh is manifold, which we require that it is, each vertex should be either interior or be
    // on exactly one logical boundary (something weird, like two faces sharing a vertex without sharing any
    // edges, is not allowed). Moreover, if the vertex is on exactly one logical boundary, there should be exactly
    // one half-edge in the valence such that edge->face is nullptr, which is the edge we are currently dealing
    // with. We store this edge in vert->edge further below. All of this is to say, if we find that vert->edge
    // is another unique boundary edge, then the mesh is non-manifold.
    if (Vert *vert = edge->vert; vert->edge != edge && !vert->edge->face) {
      throw Error(std::runtime_error("Tried to build topology for non-manifold mesh!"));
    } else {
      vert->edge = edge;

      // Boundary edges have not yet had texture coordinates initialized, since there are no corresponding
      // faces in the given mesh. So copy over the texcoordinate from the other side.
      edge->texcoord = edge->twin->loop.next->texcoord;

      // Link to the logically preceding boundary edge.
      linkLoop(findBoundaryAroundVertCW(edge)->twin, edge);
    }
  }
  // Cache all geometric quantities.
  cache();
}

void HalfEdgeMesh::cache() {
  // Note: Verts must be cached after faces because the vert normal depends on the surrounding face normals.
  for (auto [face, i] : ranges::enumerate(allFaces())) face->cache(), face->index = i;
  for (auto [vert, i] : ranges::enumerate(allVerts())) vert->cache(), vert->index = i;
}

HalfEdgeMesh::operator Mesh() const {
  Mesh mesh;
  mesh.faces.reserve(numFaces());
  mesh.positions.v.reserve(numVerts());
  mesh.texcoords.v.reserve(numVerts());
  for (auto vert : allVerts()) mesh.positions.v.push_back(vert->position);
  for (auto face : allFaces()) {
    for (auto vert : vertsOf(face)) mesh.positions.f.push_back(vert->index);
    for (auto edge : edgesOf(face)) {
      mesh.texcoords.f.push_back(mesh.texcoords.v.size());
      mesh.texcoords.v.push_back(edge->texcoord);
    }
    mesh.faces.push_back({mesh.indexCount, uint32_t(face->count)});
    mesh.indexCount += face->count;
  }
  mesh.calculateNormals();
  return mesh;
}

size_t HalfEdgeMesh::Vert::valence() noexcept { //
  return edgesOf(this).size();
}

bool HalfEdgeMesh::Vert::isBoundary() noexcept {
  for (auto edge : edgesOf(this))
    if (edge->isBoundary()) return true;
  return false;
}

auto HalfEdgeMesh::Vert::edgeByIndex(int i) noexcept -> Edge * {
  EdgeOfVertIterator walk = edge;
  for (; i > 0; --i) ++walk;
  for (; i < 0; ++i) --walk;
  return *walk;
}

float HalfEdgeMesh::Vert::angleDefect() noexcept {
  float expectedAngleSum = constants::TwoPi<float>;
  float angleSum = 0;
  for (auto edge : edgesOf(this)) {
    if (edge->face) [[likely]] {
      angleSum += edge->interiorAngle();
    } else {
      // The edge does not have a face, so this is on a
      // boundary and the expected angle sum is therefore
      // only a half-turn.
      expectedAngleSum = constants::Pi<float>;
    }
  }
  return expectedAngleSum - angleSum;
}

Vector2f HalfEdgeMesh::Vert::averageUV() noexcept {
  Vector2f numer;
  float denom{0};
  for (auto edge : edgesOf(this)) {
    numer += edge->texcoord;
    denom += 1.0f;
  }
  return denom > 0 ? numer / denom : Vector2f{};
}

void HalfEdgeMesh::Vert::cache() noexcept {
  normal = Vector3f{};
  for (auto face : facesOf(this))
    if (face) normal += face->normal * face->area;
  normal = normalize(normal);
  tangentSpace = arbitraryTangentSpace(normal);
}

bool HalfEdgeMesh::Face::isBoundary() noexcept {
  for (auto edge : edgesOf(this))
    if (edge->isBoundary()) return true;
  return false;
}

auto HalfEdgeMesh::Face::edgeByIndex(int i) noexcept -> Edge * {
  EdgeOfFaceIterator walk = edge;
  for (; i > 0; --i) ++walk;
  for (; i < 0; ++i) --walk;
  return *walk;
}

auto HalfEdgeMesh::Face::vertPositions() noexcept -> MatrixNx3f {
  MatrixNx3f matrix{with_shape, count};
  for (auto [vert, i] : ranges::enumerate(vertsOf(this))) matrix.row(i).assign(vert->position);
  return matrix;
}

auto HalfEdgeMesh::Face::vertNormals() noexcept -> MatrixNx3f {
  MatrixNx3f matrix{with_shape, count};
  for (auto [vert, i] : ranges::enumerate(vertsOf(this))) matrix.row(i).assign(vert->normal);
  return matrix;
}

auto HalfEdgeMesh::Face::gradient() noexcept -> Matrix3xNf {
  Matrix3xNf matrix{with_shape, count};
  for (auto [edge, i] : ranges::enumerate(edgesOf(this))) matrix.col(i).assign(edge->gradient());
  return matrix;
}

auto HalfEdgeMesh::Face::sharp() noexcept -> Matrix3xNf {
  Matrix3xNf matrix{with_shape, count};
  for (auto [edge, i] : ranges::enumerate(edgesOf(this))) matrix.col(i).assign(edge->sharp());
  return matrix;
}

auto HalfEdgeMesh::Face::flat() noexcept -> MatrixNx3f {
  MatrixNx3f matrix{with_shape, count};
  for (auto [edge, i] : ranges::enumerate(edgesOf(this))) matrix.row(i).assign(edge->flat());
  return matrix;
}

auto HalfEdgeMesh::Face::innerProduct(float lambda) noexcept -> MatrixNxNf {
  auto matrixU = sharp();
  auto matrixP = MatrixNxNf(-dot(flat(), matrixU));
  diag(matrixP) += 1.0f;
  return area * dot(transpose(matrixU), matrixU).execute() + lambda * dot(transpose(matrixP), matrixP).execute();
}

auto HalfEdgeMesh::Face::laplacian(float lambda) noexcept -> MatrixNxNf {
  auto matrixM = innerProduct(lambda);
  auto matrixL = MatrixNxNf(with_shape, count, count);
  for (int32_t i = 0; i < count; i++) {
    for (int32_t j = 0; j < count; j++) {
      int32_t iU = i, iV = (i + 1) % count;
      int32_t jU = j, jV = (j + 1) % count;
      matrixL(iV, jV) = matrixM(iU, jU) + matrixM(iV, jV) - (matrixM(iV, jU) + matrixM(iU, jV));
    }
  }
  return matrixL;
}

auto HalfEdgeMesh::Face::vectorLaplacian(float lambda) noexcept -> MatrixNxNf {
  std::vector<Matrix2x2f> connections{size_t(count)};
  for (auto [vert, i] : ranges::enumerate(vertsOf(this))) connections[i] = leviCivitaConnection(vert, this);
  MatrixNxNf matrixL0 = laplacian(lambda);
  MatrixNxNf matrixL1{with_shape, 2 * count, 2 * count};
  for (int32_t i = 0; i < count; i++) {
    matrixL1(2 * i + 0, 2 * i + 0) = matrixL0(i, i);
    matrixL1(2 * i + 1, 2 * i + 1) = matrixL0(i, i);
    for (int32_t j = i + 1; j < count; j++) {
      const Matrix2x2f &connectionI = connections[i];
      const Matrix2x2f &connectionJ = connections[j];
      Matrix2x2f connectionIJ = dot(transpose(connectionI), connectionJ);
      matrixL1(Slice(2 * i, 2 * i + 2), Slice(2 * j, 2 * j + 2)).assign(matrixL0(i, j) * connectionIJ);
      matrixL1(Slice(2 * j, 2 * j + 2), Slice(2 * i, 2 * i + 2)).assign(matrixL0(i, j) * transpose(connectionIJ));
    }
  }
  return matrixL1;
}

auto HalfEdgeMesh::Face::shape() noexcept -> Matrix2x2f {
  Matrix3x3f gradientOfNormals = dot(gradient(), vertNormals());
  Matrix3x3f gradientOfNormalsSymmetric = 0.5f * (gradientOfNormals + transpose(gradientOfNormals));
  return dot(transpose(tangentSpace), gradientOfNormalsSymmetric, tangentSpace);
}

auto HalfEdgeMesh::Face::principalCurvatures() noexcept -> PrincipalCurvatures {
  Matrix2x2f matrixS = shape();
  DecompSVD<float, TensorShape<2, 2>, /*EnableU=*/true, /*EnableV=*/false> decomp{matrixS};
  return {decomp.singularValue(1), decomp.singularValue(0), decomp.singularVectorU(1), decomp.singularVectorU(0)};
}

auto HalfEdgeMesh::Face::perimeter() noexcept -> float {
  double perimeterSum = 0;
  for (auto edge : edgesOf(this)) perimeterSum += length(edge->vector());
  return perimeterSum;
}

void HalfEdgeMesh::Face::cache() noexcept {
  center = Vector3f{};
  normal = Vector3f{};
  count = 0;
  for (auto vert : vertsOf(this)) center += vert->position, count += 1;
  center /= float(count);
  for (auto edge : edgesOf(this)) {
    const Vector3f &currPosition{edge->position()};
    const Vector3f &nextPosition{edge->twin->position()};
    normal += cross(currPosition - center, nextPosition - center);
  }
  auto [normalLen, normalDir] = lengthAndDirection(normal);
  area = normalLen / 2;
  normal = normalDir;
  tangentSpace = arbitraryTangentSpace(normal);
}

size_t HalfEdgeMesh::Edge::boundaryLength() noexcept {
  if (!isBoundary()) return 0;
  return EdgesOfEdgeLoop(EdgeOfEdgeLoopIterator{!face ? this : twin}).size();
}

auto HalfEdgeMesh::leviCivitaConnection(Vert *vert, Face *face) noexcept -> Matrix2x2f {
  return dot(transpose(face->tangentSpace), Matrix3x3f(Quaternionf::rotate(vert->normal, face->normal)), vert->tangentSpace);
}

auto HalfEdgeMesh::leviCivitaConnection(Face *face, Vert *vert) noexcept -> Matrix2x2f {
  return dot(transpose(vert->tangentSpace), Matrix3x3f(Quaternionf::rotate(face->normal, vert->normal)), face->tangentSpace);
}

bool HalfEdgeMesh::removeFaceIfInvalid(Face *face) {
  if (face && edgesOf(face).size() == 2) {
    auto edgeA{face->edge};
    auto edgeB{face->edge->loop.next};
    edgeA->vert->edge = edgeA->twin->loop.next;
    edgeB->vert->edge = edgeB->twin->loop.next;
    linkTwin(edgeA->twin, edgeB->twin);
    mElems.deallocate(edgeA);
    mElems.deallocate(edgeB);
    mElems.deallocate(face);
    return true;
  }
  return false;
}

auto HalfEdgeMesh::collapseEdgeMergeVerts(Edge *edge) -> Vert * {
  if (!edge) [[unlikely]]
    return nullptr;
  // If the edge connecting the vertexes is a boundary edge, then we can merge them without breaking
  // as long as the boundary is longer than three edges in total, otherwise we would be collapsing an
  // isolated triangle to an isolated edge.
  //
  // If the edge connecting the vertexes is *NOT* a boundary edge, but both vertexes are themselves
  // on the boundary, then merging them together is going to invalidate our manifold assumptions.
  // Essentially we'd be trying to do something like this:
  //
  //    * ---------- *
  //    |            |
  //    |            |
  //    A <--------> B
  //    |            |
  //    |            |
  //    * ---------- *
  auto edgeA{edge}, edgeB{edge->twin};
  auto vertA{edgeA->vert}, vertB{edgeB->vert};
  auto faceA{edgeA->face}, faceB{edgeB->face};
  if (edgeA->isBoundary() ? edgeA->boundaryLength() < 4 : vertA->isBoundary() && vertB->isBoundary())
    throw Error(std::runtime_error("Collapse would result in non-manifold topology!"));

  // For every edge that points to vertex B, update it so it points to vertex A instead.
  for (auto each : edgesOf(vertB)) each->vert = vertA;

  // We are going to remove both edges A and B, so make sure that vertex A as well as faces A and B
  // do not point to them.
  vertA->edge = edgeA->loop.prev;
  if (faceA) faceA->edge = edgeA->loop.prev;
  if (faceB) faceB->edge = edgeB->loop.prev;

  // Extract edges A and B from their list structure by linking the previous and next edges
  // to each other.
  linkLoop(edgeA->loop.prev, edgeA->loop.next);
  linkLoop(edgeB->loop.prev, edgeB->loop.next);

  // Now we are safe to deallocate everything.
  mElems.deallocate(edgeA);
  mElems.deallocate(edgeB);
  mElems.deallocate(vertB);

  // And if necessary, remove invalidated faces.
  removeFaceIfInvalid(faceA);
  removeFaceIfInvalid(faceB);
  return vertA;
}

auto HalfEdgeMesh::dissolveEdgeMergeFaces(Edge *edge) -> Face * {
  if (!edge) [[unlikely]]
    return nullptr;
  auto faceA{edge->face};
  auto faceB{edge->twin->face};
  if (edge->isBoundary()) [[unlikely]]
    return faceA ? faceA : faceB;

  // Reduce the edge chain first. If the simplification ends up collapsing one of the faces, return the other.
  edge = reduceEdgeChain(edge);
  if (edge->face != faceA) return edge->twin->face;
  if (edge->twin->face != faceB) return edge->face;

  // Reroute topology.
  auto edgeA{edge};
  auto edgeB{edge->twin};
  faceA->edge = edgeA->loop.next;
  edgeA->vert->edge = edgeB->loop.next;
  edgeB->vert->edge = edgeA->loop.next;
  linkLoop(edgeB->loop.prev, edgeA->loop.next);
  linkLoop(edgeA->loop.prev, edgeB->loop.next);
  assignFaceToLoop(faceA, faceA->edge);

  // Deallocate.
  mElems.deallocate(edgeA);
  mElems.deallocate(edgeB);
  mElems.deallocate(faceB);
  return faceA;
}

auto HalfEdgeMesh::reduceEdgeChain(Edge *edge) -> Edge * {
  if (!edge || edge->isBoundary()) [[unlikely]]
    return edge;

  // If the faces share more than 1 edge, collapse them sequentially until there is just 1 left.
  auto face{edge->twin->face};
  auto last{edge};
  while (edge->loop.prev->twin->face == face) edge = edge->loop.prev;
  while (last->loop.next->twin->face == face && last != edge) last = last->loop.next;
  while (edge != last) edge = edge->loop.next, collapseEdgeMergeVerts(edge->loop.prev);

  // If necessary, remove invalidated faces.
  auto edgeA{edge};
  auto edgeB{edge->twin};
  if (removeFaceIfInvalid(edgeA->face)) return edgeB;
  if (removeFaceIfInvalid(edgeB->face)) return edgeA;
  return edge;
}

auto HalfEdgeMesh::splitEdgeInsertVert(Edge *edge, float factor, bool relative) -> Edge * {
  if (!edge) return nullptr;
  auto edgeA{edge};
  auto edgeB{edge->twin};
  if (!relative) factor /= distance(edgeA->vert->position, edgeB->vert->position);
  if (factor < 0) factor += 1; // Negative means complement.
  auto outputEdgeA{mElems.allocate<Edge>()};
  auto outputEdgeB{mElems.allocate<Edge>()};
  auto outputVert{mElems.allocate<Vert>()};
  outputVert->position = lerp(factor, edgeA->vert->position, edgeB->vert->position);
  edgeA->twin = outputEdgeB, outputEdgeB->twin = edgeA;
  linkTwin(edgeA, outputEdgeB);
  linkTwin(edgeB, outputEdgeA);
  linkLoop(edgeA, outputEdgeA, edgeA->loop.next);
  linkLoop(edgeB, outputEdgeB, edgeB->loop.next);
  outputEdgeA->texcoord = lerp(factor, outputEdgeA->loop.prev->texcoord, outputEdgeA->loop.next->texcoord);
  outputEdgeB->texcoord = lerp(factor, outputEdgeB->loop.next->texcoord, outputEdgeB->loop.prev->texcoord);
  outputEdgeA->face = edgeA->face, outputEdgeA->vert = outputVert;
  outputEdgeB->face = edgeB->face, outputEdgeB->vert = outputVert;
  outputVert->edge = outputEdgeA;
  return outputEdgeA;
}

auto HalfEdgeMesh::splitFaceInsertEdge(Vert *vertA, Vert *vertB) -> SplitFaceResult {
  // If vertA is nullptr, return failure.
  // If vertB is nullptr, return failure.
  // If there is an edge already, return failure.
  if (!vertA || !vertB || findEdge(vertA, vertB)) return {};

  // Find the face shared by the verts. If the verts do not share a
  // face, return failure.
  auto face = findFace(vertA, vertB);
  if (!face) return {};

  // Find the edges for each vert.
  Edge *edgeA = nullptr;
  Edge *edgeB = nullptr;
  for (auto edge : edgesOf(face)) {
    if (edge->vert == vertA) edgeA = edge;
    if (edge->vert == vertB) edgeB = edge;
  }
  assert(edgeA);
  assert(edgeB);

  // Allocate new elements.
  auto outputFace{mElems.allocate<Face>()};
  auto outputEdgeA{mElems.allocate<Edge>()};
  auto outputEdgeB{mElems.allocate<Edge>()};

  // Set up topology.
  auto prevEdgeA{edgeA->loop.prev};
  auto prevEdgeB{edgeB->loop.prev};
  outputEdgeA->vert = edgeA->vert, outputEdgeA->texcoord = edgeA->texcoord;
  outputEdgeB->vert = edgeB->vert, outputEdgeB->texcoord = edgeB->texcoord;
  linkTwin(outputEdgeA, outputEdgeB);
  linkLoop(prevEdgeA, outputEdgeA, edgeB);
  linkLoop(prevEdgeB, outputEdgeB, edgeA);
  assignFaceToLoop(outputFace, outputEdgeB);
  face->edge = outputEdgeA, outputEdgeA->face = face;

  // Return result information.
  return {face, outputFace, outputEdgeA};
}

auto HalfEdgeMesh::spinEdge(Edge *edge) -> Edge * {
  if (!edge || edge->isBoundary()) return edge;
  edge = reduceEdgeChain(edge);
  auto edgeA{edge};
  auto edgeB{edge->twin};
  auto vertA{edgeA->loop.next->loop.next->vert};
  auto vertB{edgeB->loop.next->loop.next->vert};
  dissolveEdgeMergeFaces(edge);
  return splitFaceInsertEdge(vertA, vertB);
}

auto HalfEdgeMesh::insertEdgeLoop(Edge *first, float factor, bool relative, InsertEdgeLoopMode mode) -> InsertEdgeLoopResult {
  if (!first) [[unlikely]]
    return {};
  std::deque<Edge *> edgesToSplit{first};
  bool closed = false;
  bool acrossQuad = mode == InsertEdgeLoopMode::AcrossQuad;
  auto shouldStop = [acrossQuad, first](Edge *walk) {
    return !walk->face || walk->face == first->face || (acrossQuad && edgesOf(walk->face).size() != 4);
  };
  // Walk around until we should stop.
  if (Edge *walk = first; walk && walk->face) do {
      walk = acrossQuad ? walk->loop.prev->loop.prev->twin : walk->loop.prev->twin;
      if (walk == first) {
        closed = true;
        break;
      }
      edgesToSplit.push_back(walk);
    } while (!shouldStop(walk));
  // If we did not end up where we started, walk around the other way until we should stop.
  if (Edge *walk = first; !closed) do {
      walk = acrossQuad ? walk->twin->loop.next->loop.next : walk->twin->loop.next;
      assert(walk != first);
      edgesToSplit.push_front(walk);
    } while (!shouldStop(walk));

  // Split the edges.
  std::vector<Edge *> outputEdges;
  for (Edge *&edge : edgesToSplit) outputEdges.emplace_back(splitEdgeInsertVert(edge, factor, relative));
  // Split the faces.
  std::vector<SplitFaceResult> outputFaces;
  for (auto [edgeA, edgeB] : ranges::adjacent<2>(outputEdges, closed))
    outputFaces.emplace_back(splitFaceInsertEdge(edgeA->vert, edgeB->vert));
  return InsertEdgeLoopResult{closed, std::move(outputEdges), std::move(outputFaces)};
}

auto HalfEdgeMesh::selectEdgeLoop(Edge *first) -> std::vector<Edge *> {
  std::deque<Edge *> edges;
  if (first) [[likely]] {
    auto walkForward = [&] {
      EdgeOfEdgeLoopIterator walk{first};
      while (walk) edges.emplace_back(*walk++);
      return walk.complete(); // Made it all the way back?
    };
    if (!walkForward()) {
      EdgeOfEdgeLoopIterator walk{first};
      while (--walk) edges.emplace_front(*walk);
    }
  }
  return std::vector<Edge *>(edges.begin(), edges.end());
}

auto HalfEdgeMesh::separateAtVert(Edge *edgeA, Edge *edgeB) -> SeparateAtVertResult {
  if (!edgeA || !edgeB || edgeA == edgeB || edgeA->vert != edgeB->vert) return {}; // Invalid arguments!

  // Simplify without loss of generality by swapping the arguments to guarantee that edge A is
  // never a boundary. If both edges are boundaries, then there is nothing to do.
  if (edgeA->isBoundary()) std::swap(edgeA, edgeB);
  if (edgeA->isBoundary()) return {edgeA->vert, nullptr};

  // If edge B is a counter-clockwise boundary, then it does not have a valid face pointer. Rewind
  // it once and flip it to obtain an interior edge that has a valid face pointer, and will also produce
  // an identical separation.
  if (edgeB->isBoundaryCCW()) edgeB = edgeB->loop.prev->twin;

  // Verify our assumptions at this point:
  assert(edgeA->face && edgeA->twin->face); // Edge A is interior.
  assert(edgeB->face);                      // Edge B at least has a face pointer.

  // Declare vertexes X and Y, where Y is the new vertex.
  auto vertX{edgeA->vert};
  auto vertY{mElems.allocate<Vert>()};
  vertX->edge = edgeA; // Vertex X will be associated with edge A.
  vertY->edge = edgeB; // Vertex Y will be associated with edge B.
  vertY->position = vertX->position;

  // Start with this:
  //   |-----(edgeF0)---->|
  //   |<----(edgeF1)-----|
  //
  // Separate by inserting new half edges between F0 and F1 like this:
  //   |-----(edgeF0)---->|
  //   |<----(edgeG0)-----|---*
  //                          |   G1 loops to G0, G0 and F1 share the same vertex. G1 is given a new vertex.
  //   |-----(edgeG1)---->|---*
  //   |<----(edgeF1)-----|
  auto separateEdge = [&](auto edge, auto vert) {
    auto edgeF0{edge};
    auto edgeF1{edge->twin};
    auto edgeG0{mElems.allocate<Edge>()};
    auto edgeG1{mElems.allocate<Edge>()};
    linkTwin(edgeF0, edgeG0);
    linkTwin(edgeF1, edgeG1);
    linkLoop(edgeG1, edgeG0);
    edgeG0->vert = edgeF1->vert;
    edgeG1->vert = vert;
    return std::make_pair(edgeG0, edgeG1);
  };

  // If necessary, repair non-manifold vertex with disjoint boundary regions that might arise from this
  // operation. In other words, we must guarantee that we do not end up with faces that share a vertex
  // without also sharing an edge. We resolve this by separating the non-manifold vertex into two, one
  // for each face.
  auto repairNonManifoldVert = [&](auto edgeC, auto edgeD) {
    auto edgeE{findBoundaryAroundVertCCW(edgeC)};
    auto edgeF{findBoundaryAroundVertCW(edgeD)};
    if (edgeE == edgeD) {
      assert(edgeF == edgeC);
      return;
    }
    linkLoop(edgeC->twin, edgeE);
    linkLoop(edgeF->twin, edgeD);
    auto vertP{edgeC->vert};
    auto vertQ{mElems.allocate<Vert>()};
    vertP->edge = edgeC;
    vertQ->edge = edgeD;
    vertQ->position = vertP->position;
    for (auto edge : edgesOf(vertP)) edge->vert = vertP;
    for (auto edge : edgesOf(vertQ)) edge->vert = vertQ;
  };

  auto edgeA0{edgeA}, edgeA1{edgeA->twin};
  auto edgeB0{edgeB}, edgeB1{edgeB->twin};
  auto [edgeC0, edgeC1] = separateEdge(edgeA0, vertY);
  if (edgeB0->isBoundary()) {
    // If edge B is a boundary, then we do not need to separate it. Instead, complete the operation
    // by linking the boundary edges we just created into the existing boundary.
    linkLoop(edgeC0, edgeB1->loop.next);
    linkLoop(edgeB1, edgeC1);
    repairNonManifoldVert(edgeA1, edgeC0);
  } else {
    // Otherwise, separate edge B in the same way we separated edge A. This creates a hole at
    // the vertex in question, so we link the new boundary edges to each other.
    auto [edgeD0, edgeD1] = separateEdge(edgeB0, vertX);
    linkLoop(edgeD0, edgeC1);
    linkLoop(edgeC0, edgeD1);
    repairNonManifoldVert(edgeA1, edgeC0);
    repairNonManifoldVert(edgeB1, edgeD0);
  }

  // Finally update the vertex pointers for all relevant edges.
  for (auto edge : edgesOf(vertX)) edge->vert = vertX;
  for (auto edge : edgesOf(vertY)) edge->vert = vertY;
  return {vertX, vertY};
}

auto HalfEdgeMesh::separate(const std::vector<Edge *> &edgeLoop) -> std::vector<SeparateAtVertResult> {
  std::vector<SeparateAtVertResult> results;
  results.reserve(edgeLoop.size());
  for (size_t i = 0; i < edgeLoop.size(); i++) {
    Edge *edgeA = edgeLoop[i];
    Edge *edgeB = edgeLoop[(i + 1) % edgeLoop.size()];
    if (auto result = separateAtVert(edgeA->twin, edgeB)) results.emplace_back(result);
  }
  return results;
}

auto HalfEdgeMesh::separate(const std::vector<Vert *> &edgeLoopVerts) -> std::vector<SeparateAtVertResult> {
  std::vector<SeparateAtVertResult> results;
  results.reserve(edgeLoopVerts.size());
  for (size_t i = 0; i < edgeLoopVerts.size(); i++) {
    Vert *vertA = edgeLoopVerts[i];
    Vert *vertB = edgeLoopVerts[(i + 1) % edgeLoopVerts.size()];
    Vert *vertC = edgeLoopVerts[(i + 2) % edgeLoopVerts.size()];
    if (auto result = separateAtVert(vertA, vertB, vertC)) results.emplace_back(result);
  }
  return results;
}

auto HalfEdgeMesh::triangulate(Face *face) -> std::vector<Face *> {
  std::vector<Face *> faces;
  if (face) [[likely]] {
    while (edgesOf(face).size() > 3) {
      Vert *vertA = face->edge->vert;
      Vert *vertB = face->edge->loop.next->loop.next->vert;
      Face *newFace = splitFaceInsertEdge(vertA, vertB);
      if (edgesOf(newFace).size() == 3) {
        faces.emplace_back(newFace);
      } else {
        faces.emplace_back(face);
        face = newFace;
      }
    }
  }
  faces.emplace_back(face);
  return faces;
}

void HalfEdgeMesh::triangulate() {
  std::vector<Face *> faces;
  for (auto face : allFaces()) faces.emplace_back(face);
  for (auto face : faces) triangulate(face);
  cache();
}

float HalfEdgeMesh::Island::area() const {
  double areaSum = 0;
  for (auto face : faces) areaSum += face->area;
  return areaSum;
}

Vector3f HalfEdgeMesh::Island::center() const {
  Vector3f result;
  if (!verts.empty()) [[likely]] {
    for (auto vert : verts) result += vert->position;
    result /= verts.size();
  }
  return result;
}

void HalfEdgeMesh::Island::centerAndAlignUV() const {
  auto texcoords = [&] {
    std::set<std::array<float, 2>> texcoords;
    for (auto edge : edges) texcoords.insert({edge->texcoord[0], edge->texcoord[1]});
    return std::vector<Vector2f>(texcoords.begin(), texcoords.end());
  }();
  if (!texcoords.empty()) [[likely]] {
    Vector2f center;
    Matrix2f covariance;
    for (auto texcoord : texcoords) center += texcoord;
    center /= texcoords.size();
    for (auto texcoord : texcoords) covariance += outer(texcoord - center, texcoord - center);
    covariance /= texcoords.size();
    DecompSVD<float, TensorShape<2, 2>, /*EnableU=*/true, /*EnableV=*/false> decomp{covariance};
    Matrix2f rotation = decomp.matrixU();
    for (auto edge : edges) {
      edge->texcoord -= center;
      edge->texcoord = dot(transpose(rotation), edge->texcoord);
    }
  }
}

auto HalfEdgeMesh::findIsland(Face *face) const -> Island {
  Island island;
  if (face) [[likely]] {
    std::set<Face *> faces;
    std::set<Face *> facesToAdd;
    facesToAdd.insert(face);
    while (!facesToAdd.empty()) {
      auto faceToAdd = facesToAdd.begin();
      for (auto each : facesOf(*faceToAdd))
        if (each && !facesToAdd.contains(each) && !faces.contains(each)) facesToAdd.insert(each);
      faces.insert(*faceToAdd);
      facesToAdd.erase(faceToAdd);
    }
    island.faces.insert(island.faces.end(), faces.begin(), faces.end());
  }
  std::set<Vert *> verts;
  std::set<Edge *> edges;
  std::set<Edge *> holes;
  for (auto each : island.faces) {
    for (auto edge : edgesOf(each)) {
      verts.insert(edge->vert);
      edges.insert(edge);
      if (!edge->twin->face) holes.insert(edge->twin);
    }
  }
  island.verts.insert(island.verts.end(), verts.begin(), verts.end());
  island.edges.insert(island.edges.end(), edges.begin(), edges.end());
  while (!holes.empty()) {
    auto edge0 = *holes.begin();
    auto edge = edge0;
    do {
      holes.erase(holes.find(edge));
    } while ((edge = edge->loop.next) != edge0);
    island.holes.emplace_back(edge0);
  }
  return island;
}

auto HalfEdgeMesh::findIslands() const -> std::vector<Island> {
  std::vector<Island> islands;
  islands.reserve(4);
  for (auto face : allFaces()) face->islandIndex = -1;
  for (auto face : allFaces()) {
    if (face->islandIndex == -1) {
      Island &island{islands.emplace_back(findIsland(face))};
      for (auto each : island.faces) each->islandIndex = islands.size() - 1;
    }
  }
  return islands;
}

auto HalfEdgeMesh::vertPositions() const -> MatrixNx3f {
  MatrixNx3f matrix{with_shape, numVerts()};
  for (auto [vert, i] : ranges::enumerate(allVerts())) matrix.row(i).assign(vert->position);
  return matrix;
}

auto HalfEdgeMesh::vertNormals() const -> MatrixNx3f {
  MatrixNx3f matrix{with_shape, numVerts()};
  for (auto [vert, i] : ranges::enumerate(allVerts())) matrix.row(i).assign(vert->normal);
  return matrix;
}

SparseMatrix HalfEdgeMesh::laplacian() const {
  SparseMatrix sparseMatrix(numVerts(), numVerts());
  for (auto face : allFaces()) {
    auto matrix = face->laplacian();
    for (int32_t i = 0; i < face->count; i++) {
      for (int32_t j = 0; j < face->count; j++) {
        if (matrix(i, j) != 0) {
          sparseMatrix(
            face->vertByIndex(i)->index, //
            face->vertByIndex(j)->index) += matrix(i, j);
        }
      }
    }
  }
  return sparseMatrix;
}

SparseMatrix HalfEdgeMesh::vectorLaplacian() const {
  SparseMatrix sparseMatrix(2 * numVerts(), 2 * numVerts());
  for (auto face : allFaces()) {
    auto matrix = face->vectorLaplacian();
    for (int32_t i = 0; i < face->count; i++) {
      for (int32_t j = 0; j < face->count; j++) {
        int32_t vI = face->vertByIndex(i)->index;
        int32_t vJ = face->vertByIndex(j)->index;
        if (float value = matrix(2 * i + 0, 2 * j + 0); value != 0) sparseMatrix(2 * vI + 0, 2 * vJ + 0) += value;
        if (float value = matrix(2 * i + 0, 2 * j + 1); value != 0) sparseMatrix(2 * vI + 0, 2 * vJ + 1) += value;
        if (float value = matrix(2 * i + 1, 2 * j + 0); value != 0) sparseMatrix(2 * vI + 1, 2 * vJ + 0) += value;
        if (float value = matrix(2 * i + 1, 2 * j + 1); value != 0) sparseMatrix(2 * vI + 1, 2 * vJ + 1) += value;
      }
    }
  }
  return sparseMatrix;
}

Vectorf HalfEdgeMesh::discretize(const ScalarFunction &function) const {
  Vectorf scalars{with_shape, numVerts()};
  for (auto vert : allVerts()) scalars[vert->index] = function(vert);
  return scalars;
}

Vectorf HalfEdgeMesh::discretize(const VectorFunction &function) const {
  Vectorf vectors{with_shape, 2 * numVerts()};
  for (auto vert : allVerts()) {
    Vector2f vector = function(vert);
    vectors[2 * vert->index + 0] = vector[0];
    vectors[2 * vert->index + 1] = vector[1];
  }
  return vectors;
}

Vectorf HalfEdgeMesh::solveLaplaceEquation(const ScalarConstraints &constraints) const {
  SparseMatrix matrixL = laplacian();
  Matrixf matrixB{with_shape, numVerts(), 1};
  for (const auto &[vert, constraint] : constraints) {
    matrixL.setRowToZero(vert->index);
    matrixL(vert->index, vert->index) = 1.0f;
    matrixB(vert->index, 0) = constraint;
  }
  // Use LU because the constraints interrupt the symmetry of the matrix.
  return matrixL.solveLU(matrixB).col(0);
}

Vectorf HalfEdgeMesh::solveLaplaceEquation(const VectorConstraints &constraints) const {
  SparseMatrix matrixL = vectorLaplacian();
  Matrixf matrixB{with_shape, 2 * numVerts(), 1};
  for (const auto &[vert, constraint] : constraints) {
    size_t i = vert->index;
    matrixL.setRowToZero(2 * i + 0);
    matrixL.setRowToZero(2 * i + 1);
    matrixL(2 * i + 0, 2 * i + 0) = 1.0f;
    matrixL(2 * i + 1, 2 * i + 1) = 1.0f;
    matrixB(2 * i + 0, 0) = constraint[0];
    matrixB(2 * i + 1, 0) = constraint[1];
  }
  // Use LU because the constraints interrupt the symmetry of the matrix.
  return matrixL.solveLU(matrixB).col(0);
}

void HalfEdgeMesh::evolveByCurvatureFlow(float tau, int numIterations) {
  while (numIterations-- > 0) {
    auto matrixA = laplacian();
    matrixA *= tau;
    matrixA.addIdentity();
    auto matrixX = matrixA.solveCholesky(vertPositions());
    for (auto [vert, i] : ranges::enumerate(allVerts())) {
      if (!vert->isBoundary()) vert->position.assign(matrixX(i));
    }
    cache();
  }
}

// TODO
#if 0
void HalfEdgeMesh::parameterizeByConformalEnergy() {
  auto islands = findIslands();
  for (Island &island : islands) {
    size_t vertIndex = 0;
    for (auto vert : island.verts) vert->index = vertIndex++;
    if (island.holes.empty()) throw Error(std::runtime_error("Auto-parameterization requires at least 1 hole!"));
    auto findBoundary = [&] {
      Edge *bestHole{nullptr};
      float bestHolePerimeter{0};
      for (auto hole : island.holes) {
        float holePerimeter{0};
        for (auto edge : EdgesOfFace(hole)) holePerimeter += length(edge->vector());
        if (bestHolePerimeter < holePerimeter) {
          bestHolePerimeter = holePerimeter;
          bestHole = hole;
        }
      }
      std::vector<Edge *> boundary;
      for (auto edge : EdgesOfFace(bestHole)) boundary.emplace_back(edge);
      return boundary;
    };
    auto boundary = findBoundary();
    SparseMatrix matrixL(2 * island.verts.size(), 2 * island.verts.size());
    SparseMatrix matrixB(2 * island.verts.size(), 2 * island.verts.size());
    SparseMatrix matrixE(2 * island.verts.size(), 2);
    auto faceLaplacian = [&](Face *face) {
      auto matrix = face->laplacian();
      for (int32_t i = 0; i < face->count; i++) {
        for (int32_t j = 0; j < face->count; j++) {
          int32_t vI = face->vertByIndex(i)->index;
          int32_t vJ = face->vertByIndex(j)->index;
          matrixL(2 * vI + 0, 2 * vJ + 0) += matrix(i, j);
          matrixL(2 * vI + 1, 2 * vJ + 1) += matrix(i, j);
        }
      }
    };
    auto holeLaplacian = [&](Edge *hole) {
      if (hole == boundary.front()) return;
      Face *face = mElems.allocate<Face>();
      assignFaceToLoop(face, hole);
      face->cache();
      faceLaplacian(face);
      assignFaceToLoop(nullptr, hole);
      mElems.deallocate(face);
    };
    for (auto face : island.faces) faceLaplacian(face);
    for (auto hole : island.holes) holeLaplacian(hole);
    for (auto edge : boundary) {
      int32_t i = edge->vert->index;
      int32_t j = edge->twin->vert->index;
      matrixL(2 * i + 0, 2 * j + 1) += 0.5f;
      matrixL(2 * i + 1, 2 * j + 0) -= 0.5f;
      matrixL(2 * j + 1, 2 * i + 0) += 0.5f;
      matrixL(2 * j + 0, 2 * i + 1) -= 0.5f;
      matrixB(2 * i + 0, 2 * i + 0) = 1;
      matrixB(2 * i + 1, 2 * i + 1) = 1;
      matrixE(2 * i + 0, 0) = 1;
      matrixE(2 * i + 1, 1) = 1;
    }
    matrixL.addIdentity(1e-6f);
    matrixE = matrixE.dot(matrixE.transpose());
    matrixE /= float(boundary.size());
    matrixB -= matrixE;
    auto [values, vectors] = matrixB.solveEigsCholesky(SparseMatrix::Order::Largest, 1, matrixL);
    for (auto vert : island.verts) {
      float texcoordU = vectors(2 * vert->index + 0, 0);
      float texcoordV = vectors(2 * vert->index + 1, 0);
      for (auto edge : edgesOf(vert)) edge->texcoord = {texcoordU, texcoordV};
    }
  }
  parameterizePackRectangles(islands);
  cache();
}

void HalfEdgeMesh::parameterizePackRectangles(const std::vector<Island> &islands) {
  for (const Island &island : islands) island.centerAndAlignUV();
  for (const Island &island : islands) {
    auto bound = island.boundBoxUV();
    auto scale = std::sqrt(island.area() / bound.area());
    for (auto edge : island.edges) edge->texcoord *= scale;
  }
}
#endif

template <typename Node, typename Cost, typename Heuristic, typename Neighbors>
static std::vector<Node *> findPathGeneric(
  Node *source,
  Node *target,
  const HalfEdgeMesh::FindPathOptions &options,
  Cost &&cost,
  Heuristic &&heuristic,
  Neighbors &&neighbors) {
  if (!source || !target) [[unlikely]]
    return {};
  int maxLength = options.maxLength > 0 ? options.maxLength : constants::Max<int>;
  float epsilon = options.epsilon;
  float oneOverExpectedLength = 0.0f;
  if (options.expectedLength > 0)
    oneOverExpectedLength = 1.0f / float(options.expectedLength);
  else if (options.maxLength > 0)
    oneOverExpectedLength = 1.0f / float(options.maxLength);
  struct Visit {
    int depth = 0;
    float costF = constants::Inf<float>; // Estimated cost
    float costG = constants::Inf<float>; // Actual cost
    std::map<Node *, Visit>::iterator prev{};
  };
  struct VisitPredicate {
    [[nodiscard]] bool operator()(std::map<Node *, Visit>::iterator itrA, std::map<Node *, Visit>::iterator itrB) const {
      return itrA->second.costF < itrB->second.costF;
    }
  };
  using Visits = std::map<Node *, Visit>;
  using VisitsTodo = GrowableHeap<typename Visits::iterator, 64, VisitPredicate>;
  Visits visits;
  VisitsTodo visitsTodo;
  visitsTodo.push(visits.insert({source, Visit{0, std::invoke(heuristic, source, target), 0, visits.end()}}).first);
  auto best = visits.end();
  while (!visitsTodo.empty()) {
    auto curr = visitsTodo.pop();
    if (curr->first == target) { // Found?
      if (options.exitAsSoonAsPossible) {
        best = curr;
        break;
      } else if (best == visits.end() || best->second.costG > curr->second.costG) {
        best = curr;
      }
    }
    if (curr->second.depth >= maxLength) continue;
    auto weight = 1 + epsilon * max(1 - (curr->second.depth + 1) * oneOverExpectedLength, 0);
    for (Node *neighbor : std::invoke(std::forward<Neighbors>(neighbors), curr->first)) {
      if (!neighbor) continue;
      auto next = visits.insert({neighbor, {}}).first;
      if (float costG = curr->second.costG + std::invoke(cost, curr->first, next->first); costG < next->second.costG) {
        next->second.costF = costG + weight * std::invoke(heuristic, next->first, target);
        next->second.costG = costG;
        next->second.depth = curr->second.depth + 1;
        next->second.prev = curr;
        if (std::find(visitsTodo.begin(), visitsTodo.end(), next) == visitsTodo.end()) visitsTodo.push(next);
      }
    }
  }
  if (best != visits.end()) {
    std::vector<Node *> path;
    path.reserve(best->second.depth);
    for (; best->second.prev != visits.end(); best = best->second.prev) path.emplace_back(best->first);
    path.emplace_back(source);
    std::reverse(path.begin(), path.end());
    assert(path.front() == source && path.back() == target);
    return path;
  }
  return {};
}

auto HalfEdgeMesh::findPath(Vert *source, Vert *target, const FindPathOptions &options) const -> std::vector<Vert *> {
  return findPathGeneric(
    source, target, options, //
    [](auto vertX, auto vertY) { return distanceSquare(vertX->position, vertY->position); },
    [](auto vertX, auto vertY) { return distanceSquare(vertX->position, vertY->position); },
    [](auto vertX) { return vertsOf(vertX); });
}

auto HalfEdgeMesh::findPath(Face *source, Face *target, const FindPathOptions &options) const -> std::vector<Face *> {
  return findPathGeneric(
    source, target, options, //
    [](auto faceX, auto faceY) {
      float numer = 0;
      float denom = 0;
      for (auto edge : edgesOf(faceX)) {
        if (edge->twin->face == faceY) {
          numer += distanceSquare(faceX->center, edge->center());
          numer += distanceSquare(faceY->center, edge->center());
          denom += 1;
        }
      }
      return denom == 0 ? constants::Inf<float> : numer / denom;
    },
    [](auto faceX, auto faceY) { return distanceSquare(faceX->center, faceY->center); },
    [](auto faceX) { return facesOf(faceX); });
}

void HalfEdgeMesh::VertQuery::build() {
  mVerts.clear();
  mVerts.reserve(mMesh.numVerts());
  for (Vert *vert : mMesh.allVerts()) mVerts.emplace_back(vert);
  mKDTree.build(mVerts, [](Vert *vert) { return vert->position; });
}

} // namespace mi::geometry
