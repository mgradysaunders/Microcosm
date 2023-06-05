#include "Microcosm/Geometry/FCurve"
#include "Microcosm/Bezier"
#include "Microcosm/utility"

namespace mi::geometry {

FCurve::Result FCurve::operator()(float time, int &index) const {
  if (keyframes.empty()) return {};
  if (keyframes.size() == 1) return {keyframes.front().value, 0};
  float time0 = keyframes.front().time;
  float time1 = keyframes.back().time;
  if (time < time0 || time > time1) {
    switch (time < time0 ? wrapBefore : wrapAfter) {
    default:
    case Wrap::Clamp: return {time < time0 ? keyframes.front().value : keyframes.back().value, 0.0f};
    case Wrap::Repeat: time = repeat(time, time0, time1); break;
    case Wrap::Mirror: time = mirror(time, time0, time1); break;
    }
  }
  Result result{};
  sequentialLowerBoundIndex(index, keyframes, time, [](const Keyframe &key, float time) { return key.time < time; });
  if (index <= 0) index = 1;
  const Keyframe &key0 = keyframes[index - 1];
  const Keyframe &key1 = keyframes[index];
  float duration = key1.time - key0.time;
  if (
    !(int(key0.weightMode) & int(Weight::Out)) && //
    !(int(key1.weightMode) & int(Weight::In))) {
    float t = unlerp(time, key0.time, key1.time);
    Bezier1f<3> curve = {
      key0.value, key0.value + 0.333333333f * duration * key0.slopeOut, key1.value - 0.333333333f * duration * key1.slopeIn,
      key1.value};
    result.value = curve(t)[0];
    result.valueDeriv = finiteOrZero(curve.derivative()(t)[0] / duration);
  } else {
    double t = unlerp(time, key0.time, key1.time);
    double shift0 = 0.333333333;
    double shift1 = 0.333333333;
    if (int(key0.weightMode) & int(Weight::Out)) shift0 = key0.weightOut;
    if (int(key1.weightMode) & int(Weight::In)) shift1 = key1.weightIn;
    shift0 *= duration;
    shift1 *= duration;
    Bezier1d<3> curveX = {key0.time, key0.time + shift0, key1.time - shift1, key1.time};
    Bezier1d<3> curveY = {
      key0.value,                          //
      key0.value + shift0 * key0.slopeOut, //
      key1.value - shift1 * key1.slopeIn,  //
      key1.value};
    Bezier1d<2> derivX = curveX.derivative();
    Bezier1d<2> derivY = curveY.derivative();
    if (!solveNewton(
          t, 0, 1, lerp(t, key0.time, key1.time), 1e-7, //
          [&](double u) { return curveX(u)[0]; },       //
          [&](double u) { return derivX(u)[0]; })) [[unlikely]]
      throw Error(std::runtime_error("Failed to converge!"));
    result.value = curveY(t)[0];
    result.valueDeriv = finiteOrZero(derivY(t)[0] / derivX(t)[0]);
  }
  return result;
}

} // namespace mi::geometry
