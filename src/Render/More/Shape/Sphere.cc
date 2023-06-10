#include "Microcosm/Render/More/Shape/Sphere"

namespace mi::render {

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

} // namespace mi::render
