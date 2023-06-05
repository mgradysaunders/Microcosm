#include "Microcosm/UI/Theme"

namespace mi::ui {

Theme::Theme() {
  neutral.initialize(Color::fromWeb("LavenderBlush"));
  neutralVariant.initialize(Color::fromWeb("MistyRose"));
  primary.initialize(Color::fromWeb("Coral"));
  secondary.initialize(Color::fromWeb("Plum"));
}

void Theme::ColorFamily::initialize(const Color &referenceColor) {
  Vector3f valueLCH = referenceColor.toLCH();
  Vector3f valueLCH0 = {5, valueLCH[1], valueLCH[2] - 10.0_degrees};
  Vector3f valueLCH1 = {99, valueLCH[1], valueLCH[2] + 10.0_degrees};
  initialize(valueLCH0, valueLCH1);
}

void Theme::ColorFamily::initialize(const Vector3f &valueLCH0, const Vector3f &valueLCH1) {
  std::vector<Color> ramp = Color::rampFromLCH(valueLCH0, valueLCH1, 10, true);
  std::copy(ramp.begin(), ramp.end(), colors.begin());
}

} // namespace mi::ui
