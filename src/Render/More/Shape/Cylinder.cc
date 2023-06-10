#include "Microcosm/Render/More/Shape/Cylinder"

namespace mi::render {

BoundBox3d Cylinder::box() const noexcept {
  return BoundBox3d(
    Vector3d(-mRadius, -mRadius, min(mHeightA, mHeightB)), //
    Vector3d(+mRadius, +mRadius, max(mHeightA, mHeightB)));
}

std::optional<double> Cylinder::intersect(Ray3d ray, Manifold &manifold) const noexcept {
  Vector2d origin{ray.origin};
  Vector2d direction{ray.direction};
  double invRadiusSq{1 / sqr(mRadius)};
  double coeffA{invRadiusSq * dot(direction, direction)};
  double coeffB{invRadiusSq * dot(direction, origin) * 2};
  double coeffC{invRadiusSq * dot(origin, origin) - 1};
  double minHeight{min(mHeightA, mHeightB) - 1e-7};
  double maxHeight{max(mHeightA, mHeightB) + 1e-7};
  for (double param : solveQuadratic(coeffA, coeffB, coeffC)) {
    if (ray.isInRange(param)) {
      if (Vector3d point{ray(param)}; minHeight < point[2] && point[2] < maxHeight) {
        double rad{hypot(point[0], point[1])}; // Reproject.
        point[0] *= mRadius / rad;
        point[1] *= mRadius / rad;
        manifold = manifoldOf(point);
        return param;
      }
    }
  }
  return std::nullopt;
}

std::optional<double> Cylinder::nearestTo(Vector3d referencePoint, Manifold &manifold) const noexcept {
  Vector3d point{referencePoint};
  double rad{hypot(point[0], point[1])};
  point[0] *= mRadius / rad;
  point[1] *= mRadius / rad;
  point[2] = clamp(point[2], min(mHeightA, mHeightB), max(mHeightA, mHeightB));
  if (double dist{distance(point, referencePoint)}; dist < manifold.nearestDistance) {
    manifold = manifoldOf(point);
    manifold.nearestDistance = dist;
    return dist;
  } else {
    return std::nullopt;
  }
}

Manifold Cylinder::manifoldOf(Vector3d point) const noexcept {
  Manifold manifold;
  manifold.point = point;
  double pointX{point[0]};
  double pointY{point[1]};
  manifold.correct.parameters[0] = unlerp(point[2], mHeightA, mHeightB);
  manifold.correct.parameters[1] = nonnegativeAtan2(pointY, pointX) / TwoPi;
  manifold.correct.tangents[0] = Vector3d(0.0, 0.0, mHeightB - mHeightA);
  manifold.correct.tangents[1] = Vector3d(-pointY, +pointX, 0.0) * TwoPi;
  manifold.correct.normal = fastNormalize(Vector3d(+pointX, +pointY, 0.0) * -copysign(1.0, mHeightB - mHeightA));
  manifold.shading = manifold.correct;
  return manifold;
}

} // namespace mi::render
