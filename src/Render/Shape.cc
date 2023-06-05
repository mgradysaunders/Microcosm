#include "Microcosm/Render/Shape"

namespace mi::render {

BoundBox3d ShapeGroup::box() const {
  return BoundBox3d(mShapes, [](const Shape &shape) -> BoundBox3d { return shape.box(); });
}

std::optional<double> ShapeGroup::intersect(Ray3d ray, Manifold &manifold) const {
  std::optional<double> best{};
  for (auto &shape : mShapes) {
    if (shape.box().rayCast(ray)) {
      if (auto param = shape.intersect(ray, manifold)) {
        ray.maxParam = *param, best = param;
      }
    }
  }
  return best;
}

std::optional<double> ShapeGroup::nearestTo(Vector3d referencePoint, Manifold &manifold) const {
  std::optional<double> best{};
  for (auto &shape : mShapes) {
    if (auto distToBox = distance(referencePoint, shape.box().clamp(referencePoint)); distToBox < manifold.nearestDistance) {
      if (auto distToShape = shape.nearestTo(referencePoint, manifold)) {
        best = distToShape;
      }
    }
  }
  return best;
}

} // namespace mi::render
