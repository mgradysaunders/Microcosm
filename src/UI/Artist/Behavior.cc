#include "Microcosm/UI/Artist/Behavior"
#include "Microcosm/UI/Artist/Fill"
#include "Microcosm/UI/Artist/Icon"

namespace mi::ui {

BehaviorArtist &BehaviorArtist::drawButton(const ButtonBehavior &behavior) {
  const auto &theme = mCtx.theme;
  float hovered = behavior.hovered.fraction();
  float pressed = behavior.pressed.fraction();
  FillArtist(mCtx, mDrawCmds)
    .withFillColor(theme.neutral[1].fade(lerp(pressed, 0.0f, 0.2f)))
    .withStrokeColor(theme.neutral[4].fade(lerp(pressed, lerp(hovered, 0.4f, 0.6f), 0.8f)))
    .withStrokeWidth(0.5f)
    .withCornerRadius(3)
    .withCornerResolution(6)
    .nextCorner(behavior.rect.northEast())
    .nextCorner(behavior.rect.northWest())
    .nextCorner(behavior.rect.southWest())
    .nextCorner(behavior.rect.southEast())
    .finish();
  mDrawCmds.commit(mCtx);
  return *this;
}

BehaviorArtist &BehaviorArtist::drawButtonIcon(const ButtonBehavior &behavior, IconFunc iconFunc) {
  const auto &theme = mCtx.theme;
  float hovered = behavior.hovered.fraction();
  float pressed = behavior.pressed.fraction();
  FillArtist(mCtx, mDrawCmds)
    .withFillColor(Vector4b())
    .withStrokeColor(theme.neutral[4].fade(lerp(hovered, 0.0f, 0.4f)))
    .withStrokeWidth(0.5f)
    .withCornerRadius(3)
    .withCornerResolution(6)
    .nextCorner(behavior.rect.northEast())
    .nextCorner(behavior.rect.northWest())
    .nextCorner(behavior.rect.southWest())
    .nextCorner(behavior.rect.southEast())
    .finish();
  mDrawCmds.commit(mCtx);
  mCtx.push();
  mCtx->model.scale(behavior.rect.minExtent() * lerp(pressed, lerp(hovered, 0.4f, 0.425f), 0.375f));
  mCtx->model.translate(behavior.rect.center());
  std::invoke(iconFunc, IconArtist(mCtx, mDrawCmds).withStrokeColor(theme.neutral[6].lerpTo(hovered, theme.neutral[8])));
  mDrawCmds.commit(mCtx);
  mCtx.pop();
  return *this;
}

BehaviorArtist &BehaviorArtist::drawToggleButtonCheckbox(const ToggleButtonBehavior &behavior) {
  const auto &theme = mCtx.theme;
  drawButton(behavior);
  if (behavior.checked) {
    mCtx.push();
    mCtx->model.scale(behavior.rect.minExtent() / 2);
    mCtx->model.translate(behavior.rect.center());
    float hovered = behavior.hovered.fraction();
    float checked = behavior.checked.fraction();
    Color checkColor = theme.neutral[7].lerpTo(hovered, theme.neutral[9]).fade(checked);
    IconArtist(mCtx, mDrawCmds).withStrokeColor(checkColor).check();
    mDrawCmds.commit(mCtx);
    mCtx.pop();
  }
  return *this;
}

BehaviorArtist &BehaviorArtist::drawToggleButtonSwitch(const ToggleButtonBehavior &behavior) {
  // TODO Redo
  drawButton(behavior);
  float hovered = behavior.hovered.fraction();
  float checked = behavior.checked.fraction();
  float switchPosition = lerp(checked, 0.25f, 0.75f);
  Rect switchRect = {behavior.rect(switchPosition - 0.1f, 0.15f), behavior.rect(switchPosition + 0.1f, 0.85f)};
  FillArtist(mCtx, mDrawCmds)
    .withFillColor(mCtx.theme.neutral[1].lerpTo(hovered, mCtx.theme.neutral[4]).lerpTo(checked, mCtx.theme.primary[7]))
    .withCornerRadius(2)
    .nextCorner(switchRect.northEast())
    .nextCorner(switchRect.northWest())
    .nextCorner(switchRect.southWest())
    .nextCorner(switchRect.southEast())
    .finish();
  mDrawCmds.commit(mCtx);
  return *this;
}

BehaviorArtist &BehaviorArtist::drawSlider(const SliderBehavior &behavior) { 
  // TODO Redo
  const auto &theme = mCtx.theme;
  float hovered = behavior.hovered.fraction();
  float pressed = behavior.pressed.fraction();
  StrokeArtist(mCtx, mDrawCmds)
    .withStrokeColor(theme.neutral[1])
    .withStrokeWidth(1)
    .moveTo(behavior.position0)
    .lineTo(behavior.position1)
    .finish(/*roundCaps=*/true)
    .withStrokeColor(theme.neutral[4].lerpTo(max(hovered, pressed), theme.neutral[7]))
    .withStrokeWidth(4)
    .moveTo(behavior.position() - Vector2d(0, 2))
    .lineTo(behavior.position() + Vector2d(0, 2))
    .finish(/*roundCaps=*/true);
  return *this;
}

} // namespace mi::ui
