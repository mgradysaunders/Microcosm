#include "Microcosm/Geometry/Delaunator"

namespace mi::geometry {

void Delaunator::clear() noexcept {
  verts.clear();
  faces.clear();
  facesForEdge.clear();
  boundaryEdges.clear();
}

void Delaunator::build() {
  faces.clear();
  facesForEdge.clear();
  boundaryEdges.clear();
  if (verts.size() < 3) return;

  // Center.
  Vector2d center{};
  for (Vector2f &vert : verts)
    if (allTrue(isfinite(vert))) center += vert;
  center /= verts.size();

  // Sort by distance.
  using VertQueue = std::vector<std::pair<double, Int>>;
  VertQueue vertQueue;
  vertQueue.reserve(verts.size());
  for (Vector2f &vert : verts) vertQueue.emplace_back(distanceSquare(vert, center), &vert - &verts[0]);
  std::sort(
    vertQueue.begin(), //
    vertQueue.end(),   //
    [](const auto &lhs, const auto &rhs) { return lhs.first < rhs.first; });
  auto vertItr = vertQueue.begin();

  // Form first triangle.
  Face firstFace;
  double firstFaceArea = 0;
  while (vertItr + 2 < vertQueue.end()) {
    firstFace[0] = vertItr[0].second;
    firstFace[1] = vertItr[1].second;
    firstFace[2] = vertItr[2].second;
    firstFaceArea = signedArea(firstFace);
    if (std::fabs(firstFaceArea) > 1e-9 * vertQueue.back().first) break;
    ++vertItr;
  }

  // First triangle initialization failed?
  if (vertItr + 3 >= vertQueue.end()) throw Error(std::runtime_error("First face initialization failed!"));

  // First triangle area negative?
  if (firstFaceArea < 0) firstFace.flipWinding();

  // Add first triangle.
  faces.reserve(verts.size());
  faces.push_back(firstFace);
  Edge firstEdges[3] = {{firstFace[0], firstFace[1]}, {firstFace[1], firstFace[2]}, {firstFace[2], firstFace[0]}};
  facesForEdge[firstEdges[0]].push(0);
  facesForEdge[firstEdges[1]].push(0);
  facesForEdge[firstEdges[2]].push(0);
  boundaryEdges.insert(firstEdges[0]);
  boundaryEdges.insert(firstEdges[1]);
  boundaryEdges.insert(firstEdges[2]);

  // Add vert, update boundary.
  auto addVert = [&](Int vertX) {
    std::set<Edge> boundaryEdgesToAdd;
    std::vector<Edge> boundaryEdgesToRemove;
    if (!allTrue(isfinite(verts[vertX]))) return; // Ignore
    for (Edge edge : boundaryEdges) {
      Int vertA = edge[0];
      Int vertB = edge[1];
      if (signedArea(Face(vertX, vertA, vertB)) < 0) {
        // Form counter-clockwise triangle.
        Int faceF = faces.size();
        facesForEdge[edge].push(faceF);
        facesForEdge[Edge(vertX, vertA)].push(faceF);
        facesForEdge[Edge(vertB, vertX)].push(faceF);
        faces.emplace_back(vertX, vertB, vertA);

        // Record boundary updates.
        boundaryEdgesToAdd.insert(Edge(vertA, vertX));
        boundaryEdgesToAdd.insert(Edge(vertX, vertB));
        boundaryEdgesToRemove.push_back(edge);
      }
    }
    for (Edge edge : boundaryEdgesToRemove) boundaryEdges.erase(boundaryEdges.find(edge));
    for (Edge edge : boundaryEdgesToAdd)
      if (!facesForEdge[edge].full()) boundaryEdges.insert(edge);
  };

  // ... and flip edges as needed to maintain Delaunay condition.
  auto andFlipEdges = [&] {
    std::set<Edge> flip;
    for (const auto &[edge, facePair] : facesForEdge) flip.insert(edge);
    while (!flip.empty()) {
      std::set<Edge> next;
      for (Edge edge : flip) {
        auto itr = facesForEdge.find(edge);
        if (delaunayCondition(itr)) continue;
        // Flip edge.
        Int vertA = itr->first[0], faceF = itr->second[0];
        Int vertB = itr->first[1], faceG = itr->second[1];
        Int vertP = faces[faceF].opposite(itr->first);
        Int vertQ = faces[faceG].opposite(itr->first);
        faces[faceF] = Face(vertP, vertA, vertQ);
        faces[faceG] = Face(vertP, vertQ, vertB);
        facesForEdge.erase(itr);
        facesForEdge[Edge(vertP, vertQ)] = FacePair(faceF, faceG);
        facesForEdge[Edge(vertP, vertB)].replace(faceF, faceG);
        facesForEdge[Edge(vertQ, vertA)].replace(faceG, faceF);
        if (signedArea(faces[faceF]) < 0) {
          faces[faceF].flipWinding();
          faces[faceG].flipWinding();
        }

        // Maybe flip neighboring edges on next iteration.
        next.insert(Edge(vertP, vertB));
        next.insert(Edge(vertQ, vertA));
        next.insert(Edge(vertP, vertA));
        next.insert(Edge(vertQ, vertB));
      }
      flip.swap(next);
    }
  };

  // Add remaining verts.
  for (vertItr += 3; vertItr < vertQueue.end(); vertItr++) {
    addVert(vertItr->second);
    andFlipEdges();
  }

  for (Edge edge : boundaryEdges) {
    FacePair &facePair = facesForEdge.at(edge);
    assert(facePair[0] == None || facePair[1] == None);
    if (facePair[0] == None) std::swap(facePair[0], facePair[1]);

    // Force boundary edge to be first edge in triangle.
    Face &face = faces[facePair[0]];
    if (
      face[0] != edge[0] || //
      face[1] != edge[1]) {
      face.cycle();
      if (
        face[0] != edge[0] || //
        face[1] != edge[1])
        face.cycle();
    }
    assert(face[0] == edge[0] && face[1] == edge[1]);
  }
}

float Delaunator::signedArea(Face face) const noexcept {
  const Vector2f &vertA = verts[face[0]];
  const Vector2f &vertB = verts[face[1]];
  const Vector2f &vertC = verts[face[2]];
  return cross(vertB - vertA, vertC - vertA) / 2;
}

float Delaunator::oppositeAngle(Face face, Edge edge) const noexcept {
  const Vector2f &vertA = verts[edge[0]];
  const Vector2f &vertB = verts[edge[1]];
  const Vector2f &vertC = verts[face.opposite(edge)];
  return angleBetween(vertA - vertC, vertB - vertC);
}

bool Delaunator::delaunayCondition(Edge edge) const noexcept {
  auto itr = facesForEdge.find(edge);
  if (itr == facesForEdge.end()) return true;
  return delaunayCondition(itr);
}

bool Delaunator::delaunayCondition(EdgeIterator edge) const noexcept {
  if (edge == facesForEdge.end() || !edge->second.full()) return true;
  float phi0 = abs(oppositeAngle(faces[edge->second[0]], edge->first));
  float phi1 = abs(oppositeAngle(faces[edge->second[1]], edge->first));
  return abs(phi0) + abs(phi1) < 180.1_degreesf;
}

} // namespace mi::geometry
