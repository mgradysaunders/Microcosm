#include "Microcosm/Render/More/Shape/Follicle"
#include "Microcosm/Render/More/Shape/Primitives"

namespace mi::render {

BoundBox3d Follicle::box() const noexcept {
  BoundBox3d boundBox{curve};
  boundBox[0] -= 0.5 * max(radiusA, radiusB);
  boundBox[1] += 0.5 * max(radiusA, radiusB);
  return boundBox;
}

std::optional<double> Follicle::intersect(Ray3d ray, Manifold &manifold) const noexcept {
  double dirLength{fastLength(ray.direction)};
  Vector3d up{Matrix3d::orthonormalBasisDiscontinuous(ray.direction / dirLength).col(1)};
  DualQuaterniond localToWorld{DualQuaterniond::lookAt(ray.origin, ray.origin + ray.direction, up)};
  DualQuaterniond worldToLocal{inverse(localToWorld)};
  Follicle follicle{*this};
  follicle >>= worldToLocal;
  double minParam{dirLength * ray.minParam};
  double maxParam{dirLength * ray.maxParam};
  if (auto param = follicle.intersectWithZAxis(minParam, maxParam, manifold)) {
    manifold >>= localToWorld;
    return *param / dirLength;
  }
  return {};
}

std::optional<double> Follicle::intersectWithZAxis(double minParam, double maxParam, Manifold &manifold) const noexcept {
  if (BoundBox3d boundBox{box()};                        //
      anyTrue(boundBox[0] > Vector3d(0, 0, maxParam)) || //
      anyTrue(boundBox[1] < Vector3d(0, 0, minParam)))
    return {};
  auto nearXY{Bezier2d<3>(curve).nearestTo(Vector2d(0, 0))};
  auto normal{kind == Kind::Ribbon ? normalize(lerp(nearXY.param, normalA, normalB)) : Vector3d(0, 0, 1)};
  auto radius{lerp(nearXY.param, radiusA, radiusB) * abs(normal[2])};
  if (!(fastLength(nearXY.point) < radius)) return false;

  Vector3d nearPoint{curve(nearXY.param)};
  Vector3d nearDeriv{curve.derivative()(nearXY.param)};
  normal = normalize(normal - finiteOrZero(dot(normal, nearDeriv) / dot(nearDeriv, nearDeriv)) * nearDeriv);
  double hitZ{dot(normal, nearPoint) / normal[2]};
  if (kind == Kind::Tube) {
    Ray3d ray{-nearPoint, Vector3d(0, 0, 1)};
    Manifold unused;
    if (auto param = Sphere{radius}.intersect(ray, unused)) {
      hitZ = *param, normal = normalize(ray(hitZ));
    } else {
      return {};
    }
  }
  if (minParam <= hitZ && hitZ <= maxParam) {
    manifold.point = {0, 0, hitZ};
    manifold.correct.parameters[0] = nearXY.param;
    manifold.correct.parameters[1] = cross(Vector2d(nearDeriv), Vector2d(0, 0)) > 0
                                       ? 0.5 + 0.5 * fastLength(nearXY.point) / radius
                                       : 0.5 - 0.5 * fastLength(nearXY.point) / radius;
    manifold.correct.tangents[0] = nearDeriv;
    manifold.correct.tangents[1] = normalize(cross(normal, nearDeriv)) * radius;
    manifold.correct.normal = normal;
    manifold.shading = manifold.correct;
    return hitZ;
  }
  return {};
}

std::optional<double> Follicle::nearestTo(Vector3d referencePoint, Manifold &manifold) const noexcept { //
  // TODO
  return {};
}

} // namespace mi::render
