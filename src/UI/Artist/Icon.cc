#include "Microcosm/UI/Artist/Icon"

namespace mi::ui {

IconArtist &IconArtist::minus(float theta) {
  const Vector2f offsetX{0.7f * unitCircle(theta)};
  stroke().moveTo(-offsetX).lineTo(+offsetX).finish(/*roundCaps=*/true);
  return *this;
}

IconArtist &IconArtist::plus(float theta) {
  const Vector2f offsetX{0.7f * unitCircle(theta)};
  const Vector2f offsetY{hodge(offsetX)};
  stroke().moveTo(-offsetX).lineTo(+offsetX).finish(/*roundCaps=*/true);
  stroke().moveTo(-offsetY).lineTo(+offsetY).finish(/*roundCaps=*/true);
  return *this;
}

IconArtist &IconArtist::check() {
  const float size{0.6f};
  const Vector2f pointA{size, size - 0.2f};
  const Vector2f pointB{Vector2f(-size, -size - 0.2f) * 0.5f};
  const Vector2f pointC{Vector2f(-size, +size) * 0.5f + pointB};
  stroke().moveTo(pointA).lineTo(pointB).lineTo(pointC).finish(/*roundCaps=*/true);
  return *this;
}

IconArtist &IconArtist::chevron(float theta) {
  const Vector2f axis{unitCircle(theta)};
  stroke().arrowHead(0.3f * axis, axis, 0.6f);
  return *this;
}

IconArtist &IconArtist::arrow(float theta) {
  const Vector2f axis{unitCircle(theta)};
  const Vector2f offset{0.7f * axis};
  stroke().moveTo(-offset).lineTo(offset).arrowHead(offset, axis, 0.4f);
  return *this;
}

IconArtist &IconArtist::twoWayArrow(float theta) {
  const Vector2f axis{unitCircle(theta)};
  const Vector2f offset{0.7f * axis};
  stroke().moveTo(-offset).lineTo(+offset).arrowHead(+offset, +axis, 0.4f).arrowHead(-offset, -axis, 0.4f);
  return *this;
}

IconArtist &IconArtist::fourWayArrow(float theta) {
  const Vector2f axisX{unitCircle(theta)};
  const Vector2f axisY{hodge(axisX)};
  const Vector2f offsetX{0.8f * axisX};
  const Vector2f offsetY{0.8f * axisY};
  stroke()
    .moveTo(-offsetX)
    .lineTo(+offsetX)
    .arrowHead(+offsetX, +axisX, 0.2f)
    .arrowHead(-offsetX, -axisX, 0.2f)
    .moveTo(-offsetY)
    .lineTo(+offsetY)
    .arrowHead(+offsetY, +axisY, 0.2f)
    .arrowHead(-offsetY, -axisY, 0.2f);
  return *this;
}

IconArtist &IconArtist::arrowIntoBox(float theta, bool outOfBox) {
  const Vector2f axisX{unitCircle(theta)};
  const Vector2f axisY{hodge(axisX)};
  const Vector2f pointA{-0.7f * axisY + 0.25f * axisX};
  const Vector2f pointB{-0.7f * axisY + 0.7f * axisX};
  const Vector2f pointC{+0.7f * axisY + 0.7f * axisX};
  const Vector2f pointD{+0.7f * axisY + 0.25f * axisX};
  stroke()
    .moveTo(pointA)
    .roundCornerTo(pointB, pointC, 0.1f)
    .roundCornerTo(pointC, pointD, 0.1f)
    .lineTo(pointD)
    .finish(/*roundCorners=*/true)
    .moveTo((outOfBox ? +0.4f : -0.7f) * axisX)
    .lineTo((outOfBox ? -0.7f : +0.4f) * axisX)
    .arrowHead(0.5f);
  return *this;
}

IconArtist &IconArtist::refresh() {
  const float radius{0.6f};
  const float thetaA{-45.0_degreesf};
  const float thetaB{-340.0_degreesf};
  stroke().arcTo({0, 0}, radius, thetaA, thetaB, /*numSegments=*/60).arrowHead(radius * unitCircle(thetaB), {1, -1}, 0.4f);
  return *this;
}

IconArtist &IconArtist::undo() { // TODO
  return *this;
}

IconArtist &IconArtist::redo() { // TODO
  return *this;
}

IconArtist &IconArtist::zoomIn() {
  const float radius{0.5f};
  const Vector2f center{-0.25f, 0.25f};
  stroke()
    .circle(center, radius)
    .moveTo(center + radius * Vector2f(constants::OneOverSqrtTwo<float>, -constants::OneOverSqrtTwo<float>))
    .lineTo(center + radius * Vector2f(constants::OneOverSqrtTwo<float>, -constants::OneOverSqrtTwo<float>) * 2.4f)
    .finish(/*roundCapFirst=*/false, /*roundCapLast=*/true)
    .moveTo(center - Vector2f(0.5f * radius, 0))
    .lineTo(center + Vector2f(0.5f * radius, 0))
    .finish(/*roundCaps=*/true)
    .moveTo(center - Vector2f(0, 0.5f * radius))
    .lineTo(center + Vector2f(0, 0.5f * radius))
    .finish(/*roundCaps=*/true);
  return *this;
}

IconArtist &IconArtist::zoomOut() {
  const float radius{0.5f};
  const Vector2f center{-0.25f, 0.25f};
  stroke()
    .circle(center, radius)
    .moveTo(center + radius * Vector2f(constants::OneOverSqrtTwo<float>, -constants::OneOverSqrtTwo<float>))
    .lineTo(center + radius * Vector2f(constants::OneOverSqrtTwo<float>, -constants::OneOverSqrtTwo<float>) * 2.4f)
    .finish(/*roundCapFirst=*/false, /*roundCapLast=*/true)
    .moveTo(center - Vector2f(0.5f * radius, 0))
    .lineTo(center + Vector2f(0.5f * radius, 0))
    .finish(/*roundCaps=*/true);
  return *this;
}

IconArtist &IconArtist::search() {
  const float radius{0.5f};
  const Vector2f center{-0.25f, 0.25f};
  stroke()
    .circle(center, radius)
    .moveTo(center + radius * Vector2f(constants::OneOverSqrtTwo<float>, -constants::OneOverSqrtTwo<float>))
    .lineTo(center + radius * Vector2f(constants::OneOverSqrtTwo<float>, -constants::OneOverSqrtTwo<float>) * 2.4f)
    .finish(/*roundCapFirst=*/false, /*roundCapLast=*/true);
  return *this;
}

IconArtist &IconArtist::trash() {
  const float sizeX{0.5f};
  const float sizeY{0.6f};
  const Vector2f pointA{-sizeX, +sizeY};
  const Vector2f pointB{-sizeX, -sizeY};
  const Vector2f pointC{+sizeX, -sizeY};
  const Vector2f pointD{+sizeX, +sizeY};
  stroke()
    .moveTo(pointA)
    .roundCornerTo(pointB, pointC, 0.1f)
    .roundCornerTo(pointC, pointD, 0.1f)
    .lineTo(pointD)
    .finish()
    .moveTo({-sizeX * 1.4f, sizeY})
    .lineTo({+sizeX * 1.4f, sizeY})
    .finish(/*roundCaps=*/true)
    .moveTo({-sizeX * 0.5f, sizeY + 0.1f})
    .lineTo({+sizeX * 0.5f, sizeY + 0.1f})
    .finish(/*roundCaps=*/true);
  return *this;
}

IconArtist &IconArtist::letter() {
  const Rect rect{Anchor::Center, {}, {1.4f, 1.0f}};
  stroke()
    .rectangle(rect, 0.0f, 0.0f, 0.1f, 0.1f)
    .moveTo(rect(0.0f, 1.0f))
    .lineTo(rect(0.5f, 0.4f))
    .lineTo(rect(1.0f, 1.0f))
    .finish(/*roundCaps=*/true);
  return *this;
}

IconArtist &IconArtist::folder() { // TODO
  return *this;
}

IconArtist &IconArtist::document() { // TODO
  return *this;
}

IconArtist &IconArtist::eye() {
  const float radius{0.88f};
  const float height{0.40f};
  const float thetaA{asin(height / radius)};
  const float thetaB{180.0_degreesf - thetaA};
  stroke()
    .arcTo({0, +height * 1.0f}, radius, -thetaB, -thetaA, /*numSegments=*/40)
    .arcTo({0, -height * 1.0f}, radius, +thetaA, +thetaB, /*numSegments=*/40)
    .finishCloseLoop();
  stroke().circle({0, 0}, 0.2f).finish();
  return *this;
}

IconArtist &IconArtist::padlock(float lockFraction) {
  const Rect rect{Anchor::Center, {0.0f, -0.3f}, {1.2f, 0.8f}};
  const Vector2f pointA{rect(0.3f, 1.0f)};
  const Vector2f pointB{rect(0.7f, 1.0f)};
  const Vector2f pointC{0.5f * pointA + 0.5f * pointB};
  const float barHeight{0.4f * lerp(lockFraction, 1.0f, 0.5f)};
  stroke()
    .rectangle(rect, 0.2f)
    .moveTo(pointA)
    .lineTo(pointA + Vector2f(0, barHeight))
    .arcTo(pointC + Vector2f(0, barHeight), fastLength(pointC - pointA), 180.0_degreesf, 0.0_degreesf)
    .lineTo(pointB + Vector2f(0, barHeight * (1 - lockFraction)))
    .finish(/*roundCapFirst=*/false, /*roundCapLast=*/lockFraction < 1);
  return *this;
}

IconArtist &IconArtist::key() {
  const float radius{0.3f};
  const Vector2f center{-0.4f, 0.0f};
  const Vector2f pointA{-0.4f + radius, 0.0f};
  const Vector2f pointB{0.7f, 0.0f};
  stroke()
    .circle(center, radius)
    .moveTo(pointA)
    .lineTo(pointB)
    .finish(/*roundCapFirst=*/false, /*roundCapLast=*/true)
    .moveTo(lerp(0.9f, pointA, pointB))
    .lineTo(lerp(0.9f, pointA, pointB) - Vector2f(0, 0.8f * radius))
    .finish(/*roundCapFirst=*/false, /*roundCapLast=*/true)
    .moveTo(lerp(0.6f, pointA, pointB))
    .lineTo(lerp(0.6f, pointA, pointB) - Vector2f(0, 1.2f * radius))
    .finish(/*roundCapFirst=*/false, /*roundCapLast=*/true);
  return *this;
}

IconArtist &IconArtist::sun() {
  stroke().circle({}, 0.3f);
  for (Vector2f cosSinTheta : unitCircleLinspace(8, 0.0_degreesf, Exclusive(360.0_degreesf)))
    stroke() //
      .moveTo(0.6f * cosSinTheta)
      .lineTo(0.8f * cosSinTheta)
      .finish(/*roundCaps=*/true);
  return *this;
}

IconArtist &IconArtist::snowflake() {
  stroke().circle({}, 0.3f);
  for (Vector2f cosSinTheta : unitCircleLinspace(6, 30.0_degreesf, Exclusive(390.0_degreesf))) {
    const Vector2f axisX{cosSinTheta};
    const Vector2f axisY{hodge(axisX)};
    stroke()
      .moveTo(0.3f * axisX)
      .lineTo(0.7f * axisX)
      .finish(/*roundCapFirst=*/false, /*roundCapLast=*/true)
      .moveTo(0.6f * axisX + 0.2f * axisY)
      .lineTo(0.5f * axisX)
      .lineTo(0.6f * axisX - 0.2f * axisY)
      .finish(/*roundCaps=*/true);
  }
  return *this;
}

IconArtist &IconArtist::hash() {
  const float extent{0.5f};
  const float space{0.2f};
  stroke().moveTo({-space, -extent}).lineTo({-space, +extent}).finish(/*roundCaps=*/true);
  stroke().moveTo({+space, -extent}).lineTo({+space, +extent}).finish(/*roundCaps=*/true);
  stroke().moveTo({-extent, -space}).lineTo({+extent, -space}).finish(/*roundCaps=*/true);
  stroke().moveTo({-extent, +space}).lineTo({+extent, +space}).finish(/*roundCaps=*/true);
  return *this;
}

IconArtist &IconArtist::musicSharp() {
  const float extent{0.5f};
  const float space{0.2f};
  const float slant{0.1f};
  stroke().moveTo({-1.25f * space, -extent - slant}).lineTo({-1.25f * space, +extent - slant}).finish(/*roundCaps=*/true);
  stroke().moveTo({+1.25f * space, -extent + slant}).lineTo({+1.25f * space, +extent + slant}).finish(/*roundCaps=*/true);
  stroke().moveTo({-extent, -space - 2 * slant}).lineTo({+extent, -space + 2 * slant}).finish(/*roundCaps=*/true);
  stroke().moveTo({-extent, +space - 2 * slant}).lineTo({+extent, +space + 2 * slant}).finish(/*roundCaps=*/true);
  return *this;
}

IconArtist &IconArtist::musicFlat() {
  stroke()
    .moveTo({-0.2f, 0.6f})
    .lineTo({-0.2f, -0.6f})
    .curveTo(
      {-0.2f + 0.8f, -0.6f}, //
      {-0.2f + 0.8f, +0.4f}, //
      {-0.2f + 0.0f, -0.4f})
    .finish(/*roundCapFirst=*/true, /*roundCapLast=*/false);
  return *this;
}

IconArtist &IconArtist::gene() {
  auto armA = [&](float t) -> Vector2f { return 0.7f * Vector2f(+0.5f * tanh(3.25f * (2 * t - 1)), 2 * t - 1); };
  auto armB = [&](float t) -> Vector2f { return 0.7f * Vector2f(-0.5f * tanh(3.25f * (2 * t - 1)), 2 * t - 1); };
  stroke().functionTo(32, armA).finish(/*roundCap=*/true);
  stroke().functionTo(16, [&](float t) -> Vector2f { return armB(lerp(t, 0.0f, 0.4f)); }).finish(/*roundCap=*/true);
  stroke().functionTo(16, [&](float t) -> Vector2f { return armB(lerp(t, 0.6f, 1.0f)); }).finish(/*roundCap=*/true);
  return *this;
}

IconArtist &IconArtist::bug() {
  const float sizeX{0.35f};
  const float sizeY{0.45f};
  const Vector2f pointA{-sizeX, +sizeY};
  const Vector2f pointB{-sizeX, -sizeY};
  const Vector2f pointC{+sizeX, -sizeY};
  const Vector2f pointD{+sizeX, +sizeY};
  stroke()
    .moveTo(0.5f * pointA + 0.5f * pointD)
    .roundCornerTo(pointA, pointB, 0.25f)
    .roundCornerTo(pointB, pointC, 0.25f)
    .roundCornerTo(pointC, pointD, 0.25f)
    .roundCornerTo(pointD, pointA, 0.25f)
    .finishCloseLoop()
    .moveTo(0.5f * pointA + 0.5f * pointD + Vector2f(0.5f * sizeX, 0))
    .arcTo(0.5f * pointA + 0.5f * pointD, 0.5f * sizeX, 0.0f, 180.0_degreesf)
    .finish();
  auto addLeg = [&](Vector2f point, int signX, int signY) {
    const Vector2f offsetX{0.3f * signX, 0.0f};
    const Vector2f offsetY{0.0f, 0.3f * signY};
    if (signY == 0) // Center leg?
      stroke()      //
        .moveTo(point)
        .lineTo(point + 1.5f * offsetX)
        .finish(/*roundCapFirst=*/false, /*roundCapLast=*/true);
    else
      stroke()
        .moveTo(point)
        .curveTo(point + offsetX, point + offsetX + offsetY)
        .finish(/*roundCapFirst=*/false, /*roundCapLast=*/true);
  };
  addLeg(lerp(0.20f, pointA, pointB), -1, +1);
  addLeg(lerp(0.20f, pointD, pointC), +1, +1);
  addLeg(lerp(0.45f, pointA, pointB), -1, 0);
  addLeg(lerp(0.45f, pointD, pointC), +1, 0);
  addLeg(lerp(0.70f, pointA, pointB), -1, -1);
  addLeg(lerp(0.70f, pointD, pointC), +1, -1);
  return *this;
}

IconArtist &IconArtist::leaf() { // TODO
  return *this;
}

IconArtist &IconArtist::tree() { // TODO
  return *this;
}

IconArtist &IconArtist::gear() { // TODO Clean up
  auto curveFunction = [&](float t) -> Vector2f {
    float theta = 359.0_degreesf * t + 10.0_degreesf;
    float r = 1 + 0.15f * tanh(1 / 0.15f * sin(8 * theta));
    return 0.7f * r * unitCircle(theta);
  };
  stroke().functionTo(100, curveFunction).finishCloseLoop().circle({}, 0.3f, 50);
  return *this;
}

IconArtist &IconArtist::dice() {
  const Rect rect{Anchor::Center, {}, {1.4f, 1.4f}};
  stroke()
    .rectangle(rect, 0.3f)
    .circle(rect(0.3f, 0.3f), 0.125f)
    .circle(rect(0.5f, 0.5f), 0.125f)
    .circle(rect(0.7f, 0.7f), 0.125f);
  return *this;
}

IconArtist &IconArtist::male() {
  const Vector2f center{-0.3f, -0.3f};
  stroke()
    .circle(center, 0.4f)
    .moveTo(center + 0.4f * Vector2f(constants::OneOverSqrtTwo<float>, constants::OneOverSqrtTwo<float>))
    .lineTo(center + 1.2f * Vector2f(constants::OneOverSqrtTwo<float>, constants::OneOverSqrtTwo<float>))
    .arrowHead(0.2f);
  return *this;
}

IconArtist &IconArtist::female() {
  const float radius{0.4f};
  const Vector2f center{0.0f, 0.3f};
  stroke()
    .circle(center, radius)
    .moveTo(center + Vector2f{0, -radius})
    .lineTo({0, -0.7f})
    .finish(/*roundCaps=*/true)
    .moveTo({-0.3f, -0.5f})
    .lineTo({+0.3f, -0.5f})
    .finish(/*roundCaps=*/true);
  return *this;
}

IconArtist &IconArtist::nonbinary() {
  const float radius{0.4f};
  const Vector2f center{0.0f, -0.3f};
  stroke()
    .circle(center, radius)
    .moveTo(center + Vector2f{0, radius})
    .lineTo({0, 0.6f})
    .finish()
    .moveTo({-0.2f, 0.6f + -0.2f})
    .lineTo({+0.2f, 0.6f + +0.2f})
    .finish(/*roundCaps=*/true)
    .moveTo({-0.2f, 0.6f + +0.2f})
    .lineTo({+0.2f, 0.6f + -0.2f})
    .finish(/*roundCaps=*/true);
  return *this;
}

IconArtist &IconArtist::infinity() {
  auto artist = stroke();
  for (Vector2f cosSinTheta : unitCircleLinspace(80, -75.0_degreesf, 255.0_degreesf)) {
    float cosTheta = cosSinTheta[0];
    float sinTheta = cosSinTheta[1];
    artist.lineTo(0.7f * Vector2f(cosTheta, cosTheta * sinTheta));
  }
  artist.finish(/*roundCaps=*/true);
  return *this;
}

IconArtist &IconArtist::partial() {
  auto curveFunction = [&](float t) -> Vector2f {
    float theta = lerp(t, -5.7f, 2.7f);
    return 0.6f * Vector2f(
                    +0.66f * cos(theta + 3.3f) - 0.132f * sin(theta + 5.8f),
                    -0.62f * sin(theta + 2.8f) - 0.376f * cos(0.5f * theta + 5.76f) - 0.1f);
  };
  stroke().functionTo(60, curveFunction).finish(/*roundCaps=*/true);
  return *this;
}

IconArtist &IconArtist::forAll() {
  const float size{0.6f};
  stroke()
    .moveTo({-size, size})
    .lineTo({0.0f, -size})
    .lineTo({size, size})
    .finish(/*roundCaps=*/true)
    .moveTo({-size / 2, 0.0f})
    .lineTo({+size / 2, 0.0f});
  return *this;
}

} // namespace mi::ui
