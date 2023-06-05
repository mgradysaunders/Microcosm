#include "Microcosm/UI/Artist/Stroke"

namespace mi::ui {

StrokeArtist &StrokeArtist::curveTo(Vector2f positionA, Vector2f positionB, int numLines) {
  assert(mState);
  return functionTo(numLines, Bezier2f<2>(mState->position, positionA, positionB));
}

StrokeArtist &StrokeArtist::curveTo(Vector2f positionA, Vector2f positionB, Vector2f positionC, int numLines) {
  assert(mState);
  return functionTo(numLines, Bezier2f<3>(mState->position, positionA, positionB, positionC));
}

StrokeArtist &StrokeArtist::arcTo(Vector2f center, float radius, float thetaA, float thetaB, int numLines) {
  assert(mState);
  for (Vector2f cosSinTheta : unitCircleLinspace(numLines, Exclusive(thetaA), thetaB)) lineTo(center + radius * cosSinTheta);
  return *this;
}

StrokeArtist &StrokeArtist::arcTo(Vector2f position, int numLines) {
  assert(mState);
  const Vector2f positionA = mState->position;
  const Vector2f positionB = position;
  const Vector2f offset = positionB - positionA;
  const Vector2f normal = hodge(mState->direction);
  const float radius = dot(offset, offset) / (2 * dot(offset, normal));
  if (!isfinite(radius)) [[unlikely]]
    return lineTo(position);
  const Vector2f center = positionA + radius * normal;
  const Vector2f vectorA = positionA - center;
  const Vector2f vectorB = positionB - center;
  const float thetaA = atan2(vectorA[1], vectorA[0]);
  const float thetaB = thetaA + copysign(angleBetween(vectorA, vectorB), radius);
  for (Vector2f cosSinTheta : unitCircleLinspace(numLines, Exclusive(thetaA), thetaB, vectorA)) lineTo(center + cosSinTheta);
  return *this;
}

StrokeArtist &StrokeArtist::roundCornerTo(Vector2f positionA, Vector2f positionB, float radius, int numLines) {
  if (radius <= mWidth) [[unlikely]]
    return lineTo(positionA);
  const Vector2f position0 = mState->position;
  const Vector2f directionA = fastNormalize(position0 - positionA);
  const Vector2f directionB = fastNormalize(positionB - positionA);
  return lineTo(positionA + directionA * radius).arcTo(positionA + directionB * radius, numLines);
}

StrokeArtist &StrokeArtist::circle(Vector2f position, float radius, int numLines) {
  if (radius <= mWidth) [[unlikely]] {
    finish();
    mDrawCmds.emitCircleWithFringe(mCtx, Vtx(position).withTexcoord(mTexcoord).withColor(mColor), radius, numLines);
    return *this;
  } else {
    return finish().arcTo(position, radius, 0, 360.0_degreesf, numLines).finishCloseLoop();
  }
}

StrokeArtist &StrokeArtist::rectangle(Rect rect) {
  return finish()
    .moveTo(rect.northEast())
    .lineTo(rect.northWest())
    .lineTo(rect.southWest())
    .lineTo(rect.southEast())
    .finishCloseLoop();
}

StrokeArtist &StrokeArtist::rectangle(Rect rect, float radius, int numLines) {
  if (radius <= mWidth) [[unlikely]]
    return rectangle(rect);
  else
    return rectangle(rect, radius, radius, radius, radius, numLines);
}

StrokeArtist &StrokeArtist::rectangle(Rect rect, float radiusNE, float radiusNW, float radiusSW, float radiusSE, int numLines) {
  const Vector2f cornerNE = rect.northEast();
  const Vector2f cornerNW = rect.northWest();
  const Vector2f cornerSW = rect.southWest();
  const Vector2f cornerSE = rect.southEast();
  return finish()
    .moveTo(0.5f * (cornerSE + cornerNE))
    .roundCornerTo(cornerNE, cornerNW, radiusNE, numLines)
    .roundCornerTo(cornerNW, cornerSW, radiusNW, numLines)
    .roundCornerTo(cornerSW, cornerSE, radiusSW, numLines)
    .roundCornerTo(cornerSE, cornerNE, radiusSE, numLines)
    .finishCloseLoop();
}

StrokeArtist &StrokeArtist::arrowHead(Vector2f position, Vector2f direction, float headSize, float headHalfAngle) {
  float sizeX = headSize;
  float sizeY = headSize * tan(headHalfAngle);
  Vector2f axisX = fastNormalize(direction);
  Vector2f axisY = hodge(axisX);
  Vector2f positionAbove = position - sizeX * axisX + sizeY * axisY;
  Vector2f positionBelow = position - sizeX * axisX - sizeY * axisY;
  return moveTo(positionBelow).lineTo(position).lineTo(positionAbove).finish(/*roundCaps=*/true);
}

} // namespace mi::ui
