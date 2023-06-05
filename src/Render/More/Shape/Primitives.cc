#include "Microcosm/Render/More/Shape/Primitives"
#include "Microcosm/Line"
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

BoundBox3d Sphere::box() const noexcept {
  return BoundBox3d(Vector3d(-mRadius, -mRadius, -mRadius), Vector3d(+mRadius, +mRadius, +mRadius));
}

std::optional<double> Sphere::intersect(Ray3d ray, Manifold &manifold) const noexcept {
  double invRadiusSq{1 / sqr(mRadius)};
  double coeffA{invRadiusSq * dot(ray.direction, ray.direction)};
  double coeffB{invRadiusSq * dot(ray.direction, ray.origin) * 2};
  double coeffC{invRadiusSq * dot(ray.origin, ray.origin) - 1};
  for (double param : solveQuadratic(coeffA, coeffB, coeffC)) {
    if (ray.isInRange(param)) {
      manifold = manifoldOf(mRadius * fastNormalize(ray(param)));
      return param;
    }
  }
  return std::nullopt;
}

std::optional<double> Sphere::nearestTo(Vector3d referencePoint, Manifold &manifold) const noexcept {
  Vector3d point{fastNormalize(referencePoint) * mRadius};
  if (double dist{distance(point, referencePoint)}; dist < manifold.nearestDistance) {
    manifold = manifoldOf(point);
    manifold.nearestDistance = dist;
    return dist;
  } else {
    return std::nullopt;
  }
}

double Sphere::solidAnglePDF(Vector3d referencePoint, Manifold manifold) const noexcept {
  if (double len{length(referencePoint)}; len > mRadius) {
    if (Manifold unused; intersect(shadowRayBetween(referencePoint, manifold.point), unused)) [[unlikely]]
      return 0;
    double sinThetaMax{mRadius / len};
    double cosThetaMax{safeSqrt(1 - sqr(sinThetaMax))};
    return uniformConePDF(cosThetaMax);
  } else {
    return Primitive::solidAnglePDF(referencePoint, manifold);
  }
}

double Sphere::solidAngleSample(Vector2d sampleU, Vector3d referencePoint, Manifold &manifold) const noexcept {
  if (double len{length(referencePoint)}; len > mRadius) {
    double sinThetaMax{mRadius / len};
    double cosThetaMax{safeSqrt(1 - sqr(sinThetaMax))};
    Ray3d ray;
    ray.origin = referencePoint;
    ray.direction = dot(Matrix3d::orthonormalBasis(-referencePoint / len), uniformConeSample(sampleU, cosThetaMax));
    if (!intersect(ray, manifold)) [[unlikely]]
      return 0;
    return uniformConePDF(cosThetaMax);
  } else {
    return Primitive::solidAngleSample(sampleU, referencePoint, manifold);
  }
}

Manifold Sphere::manifoldOf(Vector3d point) const noexcept {
  Manifold manifold;
  manifold.point = point;
  double cosTheta{clamp(point[2] / mRadius, -1, 1)}, sinTheta{sqrt(1 - sqr(cosTheta))};
  double cotTheta{finiteOrZero(cosTheta / sinTheta)};
  manifold.correct.parameters[0] = acos(cosTheta) / Pi;
  manifold.correct.parameters[1] = nonnegativeAtan2(point[1], point[0]) / TwoPi;
  manifold.correct.tangents[0] = Vector3d(cotTheta * point[0], cotTheta * point[1], -mRadius * sinTheta) * Pi;
  manifold.correct.tangents[1] = Vector3d(-point[1], point[0], 0) * TwoPi;
  manifold.correct.normal = point * (1 / mRadius);
  manifold.shading = manifold.correct;
  return manifold;
}

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
