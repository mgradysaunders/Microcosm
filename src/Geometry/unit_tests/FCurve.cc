#include "Microcosm/Geometry/FCurve"
#include "testing.h"

TEST_CASE("FCurve") {
  auto prng = PRNG();
  auto uniform = [&](float minValue, float maxValue) {
    return mi::lerp(std::generate_canonical<float, 32>(prng), minValue, maxValue);
  };
  mi::geometry::FCurve fcurve;
  for (int step = 0; step < 16; step++) {
    auto &key = fcurve.keyframes.emplace_back();
    key.weightMode = mi::geometry::FCurve::Weight(prng(4));
    key.time = step + uniform(0.01f, 0.99f);
    key.value = uniform(-2.0f, +2.0f);
    key.slopeIn = uniform(-4.0f, +4.0f);
    key.slopeOut = uniform(-4.0f, +4.0f);
    key.weightIn = uniform(0.01f, 0.99f);
    key.weightOut = uniform(0.01f, 0.99f);
  }
  int lastIndex = 0;
  fcurve.keyframes.front().time = 0.0f;
  fcurve.keyframes.back().time = 16.0f;
  auto bruteForceDeriv = [&](float t) { return (fcurve(t + 0.5e-4f).value - fcurve(t - 0.5e-4f).value) / 1e-4f; };
  for (int step = 0; step + 1 < 16; step++) {
    float time = mi::lerp(
      uniform(0.01f, 0.99f),       //
      fcurve.keyframes[step].time, //
      fcurve.keyframes[step + 1].time);
    CHECK(fcurve(time, lastIndex).valueDeriv == Approx(bruteForceDeriv(time)).epsilon(1e-2f));
  }
}
