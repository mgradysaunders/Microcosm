#include "Microcosm/Render/More/Shape/Triangle"
#include "Microcosm/Line"

namespace mi::render {

BoundBox3d Triangle::box() const noexcept {
  BoundBox3d boundBox;
  boundBox |= mPoints[0];
  boundBox |= mPoints[1];
  boundBox |= mPoints[2];
  return boundBox;
}

std::optional<double> Triangle::intersect(Ray3d ray, Manifold &manifold) const noexcept {
  try {
    Matrix3d system;
    system.col(0).assign(mPoints[0] - ray.origin);
    system.col(1).assign(mPoints[1] - ray.origin);
    system.col(2).assign(mPoints[2] - ray.origin);
    Vector3d barycentric{DecompLU(system).solve(ray.direction)};
    if (double barycentricSum = barycentric.sum(); abs(barycentricSum) > constants::MinInv<double>) {
      if ((barycentric *= 1.0 / barycentricSum); allTrue(barycentric > -Eps)) {
        if (double rayParam = 1.0 / barycentricSum; ray.minParam <= rayParam && rayParam <= ray.maxParam) {
          manifold = {};
          manifold.point = barycentric[0] * mPoints[0] + barycentric[1] * mPoints[1] + barycentric[2] * mPoints[2];
          manifold.correct.parameters = {barycentric[1], barycentric[2]};
          manifold.correct.tangents[0] = mPoints[1] - mPoints[0];
          manifold.correct.tangents[1] = mPoints[2] - mPoints[0];
          manifold.correct.calculateNormalFromTangents();
          manifold.shading = manifold.correct;
          return rayParam;
        }
      }
    }
  } catch (...) {
    /* Fallthrough */
  }
  return std::nullopt;
}

std::optional<double> Triangle::nearestTo(Vector3d referencePoint, Manifold &manifold) const noexcept {
  // Initialize this result. If the distance squared to the nearest point on the plane (not
  // restricted to the interior region of the triangle) is farther away than the result so far,
  // then we can quit immediately.
  Vector3d normal{cross(mPoints[1] - mPoints[0], mPoints[2] - mPoints[0])};
  Vector3d projector{1.0 / dot(normal, normal) * normal};
  Vector3d projectee{referencePoint - dot(projector, referencePoint) * normal};
  double bestDist{manifold.nearestDistance};
  double thisDist{distance(referencePoint, projectee)};
  if (!(isfinite(thisDist) && thisDist < bestDist)) {
    return std::nullopt;
  }
  Vector3d barycentric{
    dot(projector, cross(mPoints[2] - mPoints[1], projectee - mPoints[1])),
    dot(projector, cross(mPoints[0] - mPoints[2], projectee - mPoints[2])),
    dot(projector, cross(mPoints[1] - mPoints[0], projectee - mPoints[0]))};
  auto success = [&] {
    manifold = {};
    manifold.point = barycentric[0] * mPoints[0] + barycentric[1] * mPoints[1] + barycentric[2] * mPoints[2];
    manifold.correct.parameters = {barycentric[1], barycentric[2]};
    manifold.correct.tangents[0] = mPoints[1] - mPoints[0];
    manifold.correct.tangents[1] = mPoints[2] - mPoints[0];
    manifold.correct.normal = fastNormalize(normal);
    manifold.shading = manifold.correct;
    manifold.nearestDistance = thisDist;
    return thisDist;
  };
  if (double barycentricSum = barycentric.sum(); abs(barycentricSum) > constants::MinInv<double>) {
    if ((barycentric *= 1.0 / barycentricSum); allTrue(barycentric > -Eps)) {
      return success();
    } else {
      // If any barycentric coordinate is negative, the projectee is outside the interior region
      // of the triangle. Loop around the perimeter to find the closest point on an edge or vertex.
      for (size_t i = 0; i < 3; i++) {
        Vector3d pointA{mPoints[i]};
        Vector3d pointB{mPoints[(i + 1) % 3]};
        auto edgeParam{Line3d(pointA, pointB).parameterOf(projectee, /*clampToSeg=*/true)};
        auto edgeDist{distance(referencePoint, lerp(edgeParam, pointA, pointB))};
        if (thisDist > edgeDist) {
          barycentric[(i + 0) % 3] = 1 - edgeParam;
          barycentric[(i + 1) % 3] = edgeParam;
          barycentric[(i + 2) % 3] = 0;
          thisDist = edgeDist;
        }
      }
      // This will be farther away than the point on the plane we calculated
      // initially, so it could be farther away that the current result passed in.
      if (thisDist < bestDist) return success();
    }
  }
  return std::nullopt;
}

Manifold Triangle::parameterization(Vector2d parameters) const noexcept {
  Manifold manifold;
  manifold.point = (1 - parameters.sum()) * mPoints[0] + parameters[0] * mPoints[1] + parameters[1] * mPoints[2];
  manifold.correct.parameters = parameters;
  manifold.correct.tangents[0] = mPoints[1] - mPoints[0];
  manifold.correct.tangents[1] = mPoints[2] - mPoints[0];
  manifold.correct.calculateNormalFromTangents();
  manifold.shading = manifold.correct;
  return manifold;
}
} // namespace mi::render
