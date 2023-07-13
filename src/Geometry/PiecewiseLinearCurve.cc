#include "Microcosm/Geometry/PiecewiseLinearCurve"
#include "Microcosm/utility"

namespace mi::geometry {

float PiecewiseLinearCurve::value(float coord) const noexcept {
  if (isnan(coord)) return coord;
  if (size() == 0) return 0;
  if (size() == 1) return mValues.front();
  if (isinf(coord)) {
    if (coord < 0 && wrapBefore == Wrap::Clamp) return mValues.front();
    if (coord > 0 && wrapAfter == Wrap::Clamp) return mValues.back();
    return constants::NaN<float>;
  }
  float fac = unlerp(coord, minCoord, maxCoord);
  if (fac < 0 || fac >= 1) switch (fac < 0 ? wrapBefore : wrapAfter) {
    default:
    case Wrap::Clamp: return fac < 0 ? mValues.front() : mValues.back();
    case Wrap::Repeat: fac = repeat(fac, 0.0f, 1.0f); break;
    case Wrap::Mirror: fac = mirror(fac, 0.0f, 1.0f); break;
    }
  fac *= size() - 1;
  int index0 = clamp(int(fac) + 0, 0, size() - 1);
  int index1 = clamp(int(fac) + 1, 0, size() - 1);
  fac -= index0;
  return lerp(fac, mValues[index0], mValues[index1]);
}

float PiecewiseLinearCurve::integral(float coord) const noexcept {
  if (isnan(coord)) return coord;
  if (size() == 0) return 0;
  if (size() == 1) return (coord - minCoord) * mValues.front();
  if (isinf(coord)) {
    float value0 = mValues.front();
    float value1 = mValues.back();
    if (coord < 0 && wrapBefore == Wrap::Clamp) return value0 == 0 ? 0 : coord * value0;
    if (coord > 0 && wrapAfter == Wrap::Clamp) return value1 == 0 ? integral() : coord * value1;
    return coord * integral();
  }
  float fac = unlerp(coord, minCoord, maxCoord);
  if (fac < 0 || fac >= 1) {
    int cycle = fastFloor(fac);
    switch (fac < 0 ? wrapBefore : wrapAfter) {
    default:
    case Wrap::Clamp: return fac < 0 ? (coord - minCoord) * mValues.front() : (coord - maxCoord) * mValues.back() + integral();
    case Wrap::Repeat: return integral(repeat(coord, minCoord, maxCoord)) + integral() * cycle;
    case Wrap::Mirror:
      if (cycle & 1)
        return integral() * cycle - integral(mirror(coord, minCoord, maxCoord)) + integral();
      else
        return integral() * cycle + integral(mirror(coord, minCoord, maxCoord));
    }
  }
  fac *= size() - 1;
  int index0 = clamp(int(fac) + 0, 0, size() - 1);
  int index1 = clamp(int(fac) + 1, 0, size() - 1);
  float value0 = mValues[index0];
  float value1 = mValues[index1];
  fac -= index0;
  return lerp(0.5f * fac, value0, value1) * fac * spacing() + mValueInts[index0];
}

float PiecewiseLinearCurve::minimum(float coordA, float coordB) const noexcept {
  if (!(coordA < coordB)) std::swap(coordA, coordB);
  if (coordA == coordB) return value(coordA);
  if (size() == 0) return 0;
  if (size() == 1) return mValues.front();
  float facA = unlerp(coordA, minCoord, maxCoord);
  float facB = unlerp(coordB, minCoord, maxCoord);
  if (facA < 0 && wrapBefore == Wrap::Clamp) facA = 0;
  if (facB < 0 && wrapBefore == Wrap::Clamp) facB = 0;
  if (facA > 1 && wrapAfter == Wrap::Clamp) facA = 1;
  if (facB > 1 && wrapAfter == Wrap::Clamp) facB = 1;
  if (!(facA < facB)) std::swap(facA, facB);
  int index0 = int(facA * (size() - 1)) + 1;
  int index1 = int(facB * (size() - 1));
  float result = constants::Inf<float>;
  for (int index = index0, count = 0; index <= index1 && count < size(); index++, count++) {
    int i = index;
    if (i < 0 || i >= size()) {
      switch (i < 0 ? wrapBefore : wrapAfter) {
      case Wrap::Clamp: i = min(max(i, 0), size() - 1); break;
      case Wrap::Repeat: i = repeat(i, size()); break;
      case Wrap::Mirror: i = mirror(i, size()); break;
      }
    }
    minimize(result, mValues[i]);
  }
  minimize(result, value(coordA));
  minimize(result, value(coordB));
  return result;
}

float PiecewiseLinearCurve::maximum(float coordA, float coordB) const noexcept {
  if (!(coordA < coordB)) std::swap(coordA, coordB);
  if (coordA == coordB) return value(coordA);
  if (size() == 0) return 0;
  if (size() == 1) return mValues.front();
  float facA = unlerp(coordA, minCoord, maxCoord);
  float facB = unlerp(coordB, minCoord, maxCoord);
  if (facA < 0 && wrapBefore == Wrap::Clamp) facA = 0;
  if (facB < 0 && wrapBefore == Wrap::Clamp) facB = 0;
  if (facA > 1 && wrapAfter == Wrap::Clamp) facA = 1;
  if (facB > 1 && wrapAfter == Wrap::Clamp) facB = 1;
  if (!(facA < facB)) std::swap(facA, facB);
  int index0 = int(facA * (size() - 1)) + 1;
  int index1 = int(facB * (size() - 1));
  float result = constants::Inf<float>;
  for (int index = index0, count = 0; index <= index1 && count < size(); index++, count++) {
    int i = index;
    if (i < 0 || i >= size()) {
      switch ((i < 0) ? wrapBefore : wrapAfter) {
      case Wrap::Clamp: i = min(max(i, 0), size() - 1); break;
      case Wrap::Repeat: i = repeat(i, size()); break;
      case Wrap::Mirror: i = mirror(i, size()); break;
      }
    }
    maximize(result, mValues[i]);
  }
  maximize(result, value(coordA));
  maximize(result, value(coordB));
  return result;
}

float PiecewiseLinearCurve::integralInverse(float valueInt) const noexcept {
  // TODO Handle valueInt if repeating or mirroring
  if (size() < 2) return 0;
  int index = 0;
  if (mValueIntsIncreasing)
    index = lowerBoundIndex(mValueInts, valueInt, std::less()) - 1;
  else if (mValueIntsDecreasing)
    index = upperBoundIndex(mValueInts, valueInt, std::greater()) - 1;
  else
    while (index + 1 < size() && !(valueInt >= mValueInts[index + 0] && valueInt <= mValueInts[index + 1])) index++;
  index = max(index, 0);
  index = min(index, size() - 2);
  // Solve inverse.
  double delta = spacing();
  double value0 = mValues[index];
  double value1 = mValues[index + 1];
  double b = 2 / (value1 - value0) * value0;
  double c = 2 / (value1 - value0) * (mValueInts[index] - valueInt) / delta;
  double t0 = -(b + copysign(sqrt(b * b - 4 * c), b)) / 2;
  double t1 = +(c / t0);
  return minCoord + delta * (index + (0 <= t0 && t0 < 1 ? t0 : t1));
}

} // namespace mi::geometry
