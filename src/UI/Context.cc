#include "Microcosm/UI/Context"

namespace mi::ui {

#if 0
Vector2f Context::mouseDragOffset(Mouse::Button btn) const noexcept {
  return canvasToModel().applyLinear(mouse.dragOffset(btn));
}

float Context::mouseDistanceOnCanvas(Vector2f localPosition) const noexcept {
  return fastLength(modelToCanvas().applyAffine(localPosition) - mouse.position);
}

float Context::mouseDistanceOnCanvas(Line2f localLine) const noexcept {
  Line2f lineSegment = {
    modelToCanvas().applyAffine(localLine[0]), //
    modelToCanvas().applyAffine(localLine[1])};
  return fastLength(lineSegment.nearestTo(mouse.position, /*clampToSeg=*/true).point - mouse.position);
}
#endif

} // namespace mi::ui
