#include "Microcosm/Render/More/Shape/Disk"
#include "Microcosm/Plane"

namespace mi::render {

BoundBox3d Disk::box() const noexcept {
  return BoundBox3d(Vector3d(-mRadius, -mRadius, mOffset - 1e-7), Vector3d(+mRadius, +mRadius, mOffset + 1e-7));
}

std::optional<double> Disk::intersect(Ray3d ray, Manifold &manifold) const noexcept {
  if (auto param = Plane3d({0, 0, 1}, {0, 0, mOffset}).rayCast(ray)) {
    Vector3d point{ray(*param)};
    if (double rad{hypot(point[0], point[1])}; rad < mRadius + 1e-7) {
      point[2] = mOffset;
      manifold = manifoldOf(point);
      return param;
    }
  }
  return std::nullopt;
}

std::optional<double> Disk::nearestTo(Vector3d referencePoint, Manifold &manifold) const noexcept {
  Vector3d point{referencePoint};
  if (double rad{hypot(point[0], point[1])}; rad > mRadius) {
    point[0] *= mRadius / rad;
    point[1] *= mRadius / rad;
  }
  point[2] = mOffset;
  double dist{distance(point, referencePoint)};
  if (dist < manifold.nearestDistance) {
    manifold = manifoldOf(point);
    manifold.nearestDistance = dist;
    return dist;
  } else {
    return std::nullopt;
  }
}

Manifold Disk::manifoldOf(Vector3d point) const noexcept {
  Manifold manifold;
  manifold.point = point;
  double radius{hypot(point[0], point[1])};
  manifold.correct.parameters[0] = radius / mRadius;
  manifold.correct.parameters[1] = nonnegativeAtan2(point[1], point[0]) / TwoPi;
  manifold.correct.tangents[0] = Vector3d(+point[0], +point[1], 0) * (mRadius / radius);
  manifold.correct.tangents[1] = Vector3d(-point[1], +point[0], 0) * TwoPi;
  manifold.correct.normal = {0, 0, 1}; // TODO Is this consistent with the tangents?
  manifold.shading = manifold.correct;
  return manifold;
}

} // namespace mi::render
