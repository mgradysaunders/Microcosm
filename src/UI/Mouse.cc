#include "Microcosm/UI/Mouse"
#include "Microcosm/UI/Clock"
#include "Microcosm/UI/Screen"

namespace mi::ui {

void Mouse::start() noexcept {
  input.positionLastFrame = input.position;
  input.scroll = {};
  for (auto &button : buttons) {
    button.down = (button.down << 1) | (button.down & 1);
    button.near = (button.near << 1) | (button.near & 1);
  }
  cursorIcon = CursorIcon::Arrow; // Reset.
  positionLastFrame = position;
}

void Mouse::afterInput(const Clock &clock, const Screen &screen) noexcept {
  // Recalculate position from input position, then update velocity estimates.
  auto limitSpeed = [&](Vector2d vector) {
    if (double speed = fastLength(vector); speed > settings.maxSpeed) {
      vector /= speed;
      vector *= settings.maxSpeed;
    }
    return vector;
  };
  Vector2d prevPosition = position;
  position = screen.screenToCanvas().applyAffine(input.position);
  velocity = lerp(
    clock.expLerpFraction(settings.halfLifeForVelocity), velocity,
    limitSpeed(finiteOrZero(1.0 / clock.deltaTime) * (position - prevPosition)));
  // Handle button states.
  for (auto &button : buttons) {
    int64_t elapsed = clock.ticks - button.clickTicks;
    if (button.isJustDown()) {
      button.near = 1;
      button.clickOrder += elapsed < settings.multiClickThreshold ? 1 : 0;
      button.clickTicks = clock.ticks;
      button.downPosition = position;
    } else {
      if ((button.down & 0b11) == 0b00 && elapsed >= settings.multiClickThreshold) {
        button.clickOrder = 0;
      }
      if (distanceSquare(button.downPosition, position) > sqr(settings.dragRadius)) {
        button.near &= 0b1111'1110;
      }
    }
  }
  scroll = input.scroll;
}

} // namespace mi::ui
