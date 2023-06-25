/*-*- C++ -*-*/
#pragma once

namespace mi {

template <std::floating_point Float, std::invocable<Float> Function>
[[nodiscard, strong_inline]] inline bool solveNewton(
  Float &coord,                                                               //
  std::type_identity_t<Float> minCoord, std::type_identity_t<Float> maxCoord, //
  std::type_identity_t<Float> target, std::type_identity_t<Float> tolerance,  //
  Function &&function, int maxIters = 16, int multiplicity = 1) {
  bool usedMinCoord{false};
  bool usedMaxCoord{false};
  Float prevValue{0};
  for (int numIters = 0; numIters < maxIters; numIters++) {
    auto [value, deriv] = std::invoke(std::forward<Function>(function), coord);
    value -= target;
    if (std::abs(value) < tolerance) {
      return true;
    } else {
      coord -= Float(multiplicity) * (value / deriv);
      if (coord < minCoord) [[unlikely]] {
        coord = minCoord;
        if (usedMinCoord) return true; // Don't spin forever!
        usedMinCoord = true;
      } else
        usedMinCoord = false;
      if (coord > maxCoord) [[unlikely]] {
        coord = maxCoord;
        if (usedMaxCoord) return true; // Don't spin forever!
        usedMaxCoord = true;
      } else
        usedMaxCoord = false;
      if (!std::isfinite(coord) || (numIters > 3 && std::abs(value) > std::abs(prevValue))) [[unlikely]]
        break;
      prevValue = value;
    }
  }
  return false;
}

template <std::floating_point Float, std::invocable<Float> FunctionF, std::invocable<Float> FunctionG>
[[nodiscard, strong_inline]] inline bool solveNewton(
  Float &coord,                                                               //
  std::type_identity_t<Float> minCoord, std::type_identity_t<Float> maxCoord, //
  std::type_identity_t<Float> target, std::type_identity_t<Float> tolerance,  //
  FunctionF &&functionF, FunctionG &&functionG, int maxIters = 16, int multiplicity = 1) {
  return solveNewton(
    coord, minCoord, maxCoord, target, tolerance,
    [functionF = std::forward<FunctionF>(functionF), //
     functionG = std::forward<FunctionG>(functionG)](Float x) -> std::pair<Float, Float> {
      return {functionF(x), functionG(x)};
    },
    maxIters, multiplicity);
}

} // namespace mi
