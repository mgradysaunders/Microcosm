#include "Microcosm/UI/Rect"

namespace mi::ui {

std::optional<Anchor> Rect::clickHitTest(float radius, Vector2f cursor, Vector2f &hitPoint) const noexcept {
  float minX = lowerX(), minY = lowerY();
  float maxX = upperX(), maxY = upperY();
  if (!(minX - radius < cursor[0] && cursor[0] < maxX + radius && //
        minY - radius < cursor[1] && cursor[1] < maxY + radius))
    return std::nullopt;
  hitPoint[0] = clamp(cursor[0], minX, maxX);
  hitPoint[1] = clamp(cursor[1], minY, maxY);
  // Test the points first, then the edges.
  for (Anchor anchor : {Anchor::NE, Anchor::NW, Anchor::SE, Anchor::SW, Anchor::N, Anchor::S, Anchor::E, Anchor::W}) {
    float hitPointX = hitPoint[0];
    float hitPointY = hitPoint[1];
    if ((anchor & Anchor::MaskNS) == Anchor::N && !((hitPointY = maxY) - radius < cursor[1])) continue;
    if ((anchor & Anchor::MaskNS) == Anchor::S && !((hitPointY = minY) + radius > cursor[1])) continue;
    if ((anchor & Anchor::MaskEW) == Anchor::E && !((hitPointX = maxX) - radius < cursor[0])) continue;
    if ((anchor & Anchor::MaskEW) == Anchor::W && !((hitPointX = minX) + radius > cursor[0])) continue;
    hitPoint[0] = hitPointX;
    hitPoint[1] = hitPointY;
    return anchor;
  }
  if (contains(cursor)) {
    hitPoint = center();
    return Anchor::Center;
  } else {
    return std::nullopt;
  }
}

} // namespace mi::ui
