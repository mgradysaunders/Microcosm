#include "Microcosm/Render/More/Shape/TriangleMesh"
#include "Microcosm/Render/More/Shape/Primitives"

namespace mi::render {

void TriangleMesh::clear() noexcept { *this = TriangleMesh(); }

void TriangleMesh::build() {
  if (positions.rows() == 0) {
    triangleBVH = {};
    return;
  }
  validate();
  geometry::ImmutableBVH3::Items items;
  items.reserve(numTris());
  for (size_t i = 0; i < numTris(); i++) {
    auto &item = items.emplace_back();
    item.index = i;
    item.box |= Vector3f(positions.row(3 * i + 0));
    item.box |= Vector3f(positions.row(3 * i + 1));
    item.box |= Vector3f(positions.row(3 * i + 2));
    item.boxCenter = item.box.center();
  }
  triangleBVH.build(4, items);
  auto reorderPerFaceVertex = [&](auto &values) {
    std::decay_t<decltype(values)> newValues{values.shape};
    for (size_t i = 0; i < items.size(); i++) {
      newValues.row(3 * i + 0).assign(values.row(3 * items[i].index + 0));
      newValues.row(3 * i + 1).assign(values.row(3 * items[i].index + 1));
      newValues.row(3 * i + 2).assign(values.row(3 * items[i].index + 2));
    }
    values = std::move(newValues);
  };
  auto reorderPerFace = [&](auto &values) {
    std::decay_t<decltype(values)> newValues{values.shape};
    for (size_t i = 0; i < items.size(); i++) newValues[i] = values[items[i].index];
    values = std::move(newValues);
  };
  reorderPerFaceVertex(positions);
  if (texcoords) reorderPerFaceVertex(*texcoords);
  if (normals) reorderPerFaceVertex(*normals);
  if (tangents) reorderPerFaceVertex(*tangents);
  if (materials) reorderPerFace(*materials);
}

void TriangleMesh::build(const geometry::Mesh &mesh) {
  clear();
  size_t i{0};
  size_t n{0};
  for (auto &face : mesh.faces)
    if (face.count >= 3) n += face.count - 2;
  positions.resize(3 * n);
  if (mesh.texcoords) texcoords.emplace(with_shape, 3 * n);
  if (mesh.normals) normals.emplace(with_shape, 3 * n);
  materials.emplace(with_shape, n);
  for (auto &face : mesh.faces) {
    for (uint32_t j = 1; j + 1 < face.count; j++) {
      positions.row(3 * i + 0).assign(mesh.positions(face, 0));
      positions.row(3 * i + 1).assign(mesh.positions(face, j));
      positions.row(3 * i + 2).assign(mesh.positions(face, j + 1));
      if (mesh.texcoords) {
        texcoords->row(3 * i + 0).assign(mesh.texcoords(face, 0));
        texcoords->row(3 * i + 1).assign(mesh.texcoords(face, j));
        texcoords->row(3 * i + 2).assign(mesh.texcoords(face, j + 1));
      }
      if (mesh.normals) {
        normals->row(3 * i + 0).assign(mesh.normals(face, 0));
        normals->row(3 * i + 1).assign(mesh.normals(face, j));
        normals->row(3 * i + 2).assign(mesh.normals(face, j + 1));
      }
      (*materials)[i] = face.metadata.material;
      ++i;
    }
  }
  build();
}

void TriangleMesh::validate() {
  auto validateSameSizeAsPositions = [&](auto &values, const char *name) {
    if (values && values->rows() != positions.rows())
      throw Error(std::runtime_error(
        "Triangle mesh validation failed! ({} positions, but {} {})"_format(positions.rows(), values->rows(), name)));
  };
  validateSameSizeAsPositions(texcoords, "texcoords");
  validateSameSizeAsPositions(normals, "normals");
  validateSameSizeAsPositions(tangents, "tangents");
  if (materials && materials->size() != numTris())
    throw Error(std::runtime_error(
      "Triangle mesh validation failed! ({} triangles, but {} materials)"_format(numTris(), materials->size())));
}

std::optional<double> TriangleMesh::intersect(Ray3d ray, Manifold &manifold) const noexcept {
  std::optional<double> rayParam;
  triangleBVH.visitRayCast(ray, [&](auto node) -> bool {
    for (uint32_t i = node.first; i < node.first + node.count; i++) {
      Triangle triangle{
        Vector3d(positions.row(3 * i)),     //
        Vector3d(positions.row(3 * i + 1)), //
        Vector3d(positions.row(3 * i + 2))};
      if (auto param = triangle.intersect(ray, manifold)) {
        ray.maxParam = *param, rayParam = param;
        manifold.primitiveIndex = i;
      }
    }
    return true; // Continue.
  });
  if (rayParam) {
    interpolateShading(manifold);
  }
  return rayParam;
}

std::optional<double> TriangleMesh::nearestTo(Vector3d referencePoint, Manifold &manifold) const noexcept {
  auto todo{GrowableMaxHeap<std::pair<double, const geometry::ImmutableBVH3::Node *>, 64>{}};
  auto dist{manifold.nearestDistance};
  if (!triangleBVH.nodes.empty()) {
    auto root{&triangleBVH.nodes[0]};
    auto rootDist{distance(referencePoint, root->box.clamp(referencePoint))};
    if ((rootDist < dist)) todo.push({rootDist, root});
  }
  while (!todo.empty()) {
    auto [nodeDist, node] = todo.pop();
    if (!(nodeDist < dist)) continue;
    if (node->isBranch()) {
      auto childA{node + 1};
      auto childB{node + node->right};
      auto childDistA{distance(referencePoint, childA->box.clamp(referencePoint))};
      auto childDistB{distance(referencePoint, childB->box.clamp(referencePoint))};
      if (!(childDistA <= childDistB)) {
        std::swap(childA, childB);
        std::swap(childDistA, childDistB);
      }
      if (childDistB < dist) todo.push({childDistB, childB});
      if (childDistA < dist) todo.push({childDistA, childA});
    } else {
      for (uint32_t i = node->first; i < node->first + node->count; i++) {
        Triangle triangle{
          Vector3d(positions.row(3 * i)),     //
          Vector3d(positions.row(3 * i + 1)), //
          Vector3d(positions.row(3 * i + 2))};
        if (auto thisDist = triangle.nearestTo(referencePoint, manifold)) {
          dist = *thisDist;
          manifold.primitiveIndex = i;
        }
      }
    }
  }
  if (!(dist < manifold.nearestDistance)) {
    return std::nullopt;
  } else {
    manifold.nearestDistance = dist;
    interpolateShading(manifold);
    return dist;
  }
}

void TriangleMesh::interpolateShading(Manifold &manifold) const noexcept {
  auto &correct{manifold.correct};
  auto &shading{manifold.shading};
  shading = correct;
  if (!(!texcoords && !normals && !tangents)) {
    size_t i{manifold.primitiveIndex};
    Vector3d barycentric{
      1 - correct.parameters.sum(), //
      correct.parameters[0],        //
      correct.parameters[1]};
    if (texcoords) {
      Vector2d texcoord0{texcoords->row(3 * i + 0)};
      Vector2d texcoord1{texcoords->row(3 * i + 1)};
      Vector2d texcoord2{texcoords->row(3 * i + 2)};
      shading.parameters = barycentric[0] * texcoord0 + barycentric[1] * texcoord1 + barycentric[2] * texcoord2;
      if (!tangents) {
        try {
          Matrix2d matrixA;
          matrixA.row(0).assign(texcoord1 - texcoord0);
          matrixA.row(1).assign(texcoord2 - texcoord0);
          Matrix2x3d matrixB;
          matrixB.row(0).assign(correct.tangents[0]);
          matrixB.row(1).assign(correct.tangents[1]);
          Matrix2x3d matrixX = DecompLU{matrixA}.solve(matrixB);
          shading.tangents[0] = matrixX.row(0);
          shading.tangents[1] = matrixX.row(1);
        } catch (...) {
          // If the LU decomposition fails, the UV map is degenerate. Just leave the shading-tangents the same
          // as the correct tangents.
        }
      }
    }
    if (normals) {
      shading.normal = normalize(
        barycentric[0] * Vector3d(normals->row(3 * i + 0)) + //
        barycentric[1] * Vector3d(normals->row(3 * i + 1)) + //
        barycentric[2] * Vector3d(normals->row(3 * i + 2)));
    }
    if (tangents) {
      shading.tangents[0] = normalize(
        barycentric[0] * Vector3d(tangents->row(3 * i + 0)) + //
        barycentric[1] * Vector3d(tangents->row(3 * i + 1)) + //
        barycentric[2] * Vector3d(tangents->row(3 * i + 2)));
      shading.tangents[1] = normalize(cross(shading.normal, shading.tangents[0]));
    }
  }
}

} // namespace mi::render
