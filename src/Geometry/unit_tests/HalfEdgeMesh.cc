#include "Microcosm/Geometry/HalfEdgeMesh"
#include "testing.h"

[[nodiscard]] static bool ValidateLinkage(mi::geometry::HalfEdgeMesh &mesh) {
  for (auto vert : mesh.allVerts()) {
    if (vert->next && vert->next->prev != vert) return false;
    if (vert->prev && vert->prev->next != vert) return false;
    if (vert->edge == nullptr || vert->edge->vert != vert) return false;
  }
  for (auto edge : mesh.allEdges()) {
    if (edge->next && edge->next->prev != edge) return false;
    if (edge->prev && edge->prev->next != edge) return false;
    if (edge->twin == nullptr || edge->twin->twin != edge) return false;
    if (edge->loop.next == nullptr || edge->loop.next->loop.prev != edge) return false;
    if (edge->loop.prev == nullptr || edge->loop.prev->loop.next != edge) return false;
    if (edge->vert == nullptr || !mesh.edgesOf(edge->vert).contains(edge)) return false;
    if (edge->face == nullptr && edge->twin->face == nullptr) return false;
  }
  return true;
}

TEST_CASE("HalfEdgeMesh") {
  SUBCASE("Basic Plane") {
    // Construct a basic plane with 1 subdivision in U and V each. That should give us
    // this:
    //   * ------- * ------- *
    //   |         |         |
    //   |         |         |
    //   |         |         |
    //   * ------- * ------- *
    //   |         |         |
    //   |         |         |
    //   |         |         |
    //   * ------- * ------- *
    auto plane = mi::geometry::HalfEdgeMesh(mi::geometry::Mesh::makePlane(1, 1));
    CHECK(plane.numVerts() == 9);
    CHECK(plane.numFaces() == 4);
    CHECK(plane.numEdges() == 12 * 2);
    CHECK(ValidateLinkage(plane));

    auto middle = plane.allVerts().findIfOrElse(nullptr, [&](auto vert) { return vert->isRegular(); });
    CHECK(middle);
    CHECK(middle->valence() == 4);
    CHECK(middle->angleDefect() == Approx(0));
    CHECK(middle->edgeByIndex(0)->interiorAngle() == Approx(90.0_degrees));
    for (auto vert : plane.allVerts())
      if (vert != middle) CHECK(vert->isBoundary());

    {
      CHECK(middle->vertByIndex(0)->valence() == 3);
      CHECK(middle->vertByIndex(0)->angleDefect() == Approx(0));
      auto boundaryEdge = plane.edgesOf(middle->vertByIndex(0)).findIfOrElse(nullptr, [&](auto edge) { //
        return edge->isBoundary();
      });
      CHECK(boundaryEdge);
      CHECK(boundaryEdge->boundaryLength() == 8);
    }

    {
      // Now split one of the edges outgoing from the middle, which should give us this:
      //   * ------- Y ------- *
      //   |         |         |
      //   |         |         |
      //   |         |         |
      //   * ------- A -- * -- B
      //   |         |         |
      //   |         |         |
      //   |         |         |
      //   * ------- X ------- *
      auto vertA = middle;
      auto vertB = middle->vertByIndex(2); // Could pick any index here.
      auto vertX = middle->vertByIndex(2 - 1);
      auto vertY = middle->vertByIndex(2 + 1);
      auto newEdge = plane.splitEdgeInsertVert(middle->edgeByIndex(2), 0.5f, /*relative=*/true);
      auto newVert = newEdge->vert;
      CHECK(plane.numVerts() == 10);
      CHECK(plane.numFaces() == 4);
      CHECK(plane.numEdges() == 13 * 2);
      CHECK(plane.vertsOf(newEdge->face).size() == 5);
      CHECK(plane.vertsOf(newEdge->twin->face).size() == 5);
      CHECK(mi::isNear<1e-7f>(newVert->position, 0.5f * (vertA->position + vertB->position)));
      CHECK(ValidateLinkage(plane));

      // And now dissolve the edge and merge the faces:
      //   * ------- Y ------- *
      //   |         |         |
      //   |         |         |
      //   |         |         |
      //   * ------- A         B
      //   |         |         |
      //   |         |         |
      //   |         |         |
      //   * ------- X ------- *
      auto face = plane.dissolveEdgeMergeFaces(newEdge);
      CHECK(vertA->valence() == 3);
      CHECK(vertB->valence() == 2);
      CHECK(vertA->isBoundary() == false);
      CHECK(vertB->isBoundary() == true);
      CHECK(plane.numVerts() == 9);
      CHECK(plane.numFaces() == 3);
      CHECK(plane.numEdges() == 11 * 2);
      CHECK(plane.edgesOf(face).size() == 6);
      CHECK(ValidateLinkage(plane));

      // And now separate:
      //   * ------- Y    * ------- *
      //   |         |    |         |
      //   |         |    |         |
      //   |         |    |         |
      //   * ------- A    *         B
      //   |         |    |         |
      //   |         |    |         |
      //   |         |    |         |
      //   * ------- X    * ------- *
      auto separateResults = plane.separate({vertX, vertA, vertY});
      CHECK(plane.numVerts() == 12);
      CHECK(plane.numFaces() == 3);
      CHECK(plane.numEdges() == 13 * 2);
      for (auto [inVert, outVert] : separateResults) {
        CHECK(inVert->isBoundary());
        CHECK(outVert->isBoundary());
      }

      // And finally triangulate.
      plane.triangulate();
      CHECK(plane.numVerts() == 12);
      CHECK(plane.numFaces() == 8);
      CHECK(plane.numEdges() == 18 * 2);
      CHECK(plane.findIslands().size() == 2);
      CHECK(ValidateLinkage(plane));
    }
  }
}
