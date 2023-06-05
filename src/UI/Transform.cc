#include "Microcosm/UI/Transform"

namespace mi::ui {

Transform::Transform(const Matrix2f &matrix) noexcept {
  mForward.linear().assign(matrix);
  mInverse.linear().assign(::mi::inverse(matrix));
}

Transform::Transform(const Matrix2f &matrix, const Vector2f &affine) noexcept : Transform(matrix) {
  mForward.col(2).assign(affine);
  mInverse.col(2).assign(dot(mInverse.linear(), affine));
}

} // namespace mi::ui
