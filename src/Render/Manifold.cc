#include "Microcosm/Render/Manifold"

namespace mi::render {

Quaterniond Manifold::TangentSpace::orthonormalLocalToWorld() const noexcept {
  Vector3d axisZ = normalize(normal);
  Vector3d axisX = normalize(tangents[0] - dot(tangents[0], axisZ) * axisZ);
  Vector3d axisY = normalize(cross(axisZ, axisX));
  if (!isPositiveAndFinite(lengthSquare(axisZ))) return Quaterniond(1);
  if (!isPositiveAndFinite(lengthSquare(axisX))) return Quaterniond(Matrix3d::orthonormalBasis(axisZ));
  return Quaterniond(Matrix3d(from_cols, axisX, axisY, axisZ));
}

void Manifold::TangentSpace::calculateNormalFromTangents() noexcept { normal = normalize(cross(tangents[0], tangents[1])); }

void Manifold::TangentSpace::calculateTangentsFromNormal() noexcept {
  auto matrix = Matrix3d::orthonormalBasis(normalize(normal));
  tangents[0] = matrix.col(0);
  tangents[1] = matrix.col(1);
}

void Manifold::TangentSpace::flattenTangentsToNormal() noexcept {
  auto projector = normalize(normal);
  tangents[0] -= dot(tangents[0], projector) * projector;
  tangents[1] -= dot(tangents[1], projector) * projector;
}

void Manifold::TangentSpace::perturbWithLocalNormal(Vector3d localNormal) noexcept {
  if (localNormal[2] < 0) localNormal = -localNormal;
  if (isPositiveAndFinite(lengthSquare(localNormal))) {
    Quaterniond rotation = Quaterniond::rotate(Vector3d(0, 0, 1), localNormal);
    tangents[0] = rotation.applyLinear(tangents[0]);
    tangents[1] = rotation.applyLinear(tangents[1]);
    normal = rotation.applyNormal(normal);
  }
}

void Manifold::TangentSpace::perturbWithLocalAzimuthRotation(double localRotation) noexcept {
  Quaterniond rotation = Quaterniond::rotate(localRotation, normal);
  tangents[0] = rotation.applyLinear(tangents[0]);
  tangents[1] = rotation.applyLinear(tangents[1]);
}

} // namespace mi::render
