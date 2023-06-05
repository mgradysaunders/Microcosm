#include "doctest.h"
#include <cstring>
#include <iostream>
#include <sstream>

#include "Microcosm/Pcg"
#include "Microcosm/Serializer"
#include "Microcosm/math"
#include "Microcosm/utility"

using doctest::Approx;
using doctest::getContextOptions;

#define CHECK_WITH(expr, ...) \
  do {                        \
    auto expr;                \
    CHECK(__VA_ARGS__);       \
  } while (false)

inline mi::Pcg32 PRNG() { return {getContextOptions()->rand_seed}; }

template <std::floating_point Float, typename Func> inline auto ApproxDerivative(Float coord, Float eps, Func &&func) {
  auto valueA{std::invoke(func, coord + eps / 2)};
  auto valueB{std::invoke(func, coord - eps / 2)};
  return decltype(valueA)((valueA - valueB) / eps);
}

template <typename Value> inline bool isApproxEqual(const Value &valueA, const Value &valueB) {
  if constexpr (mi::concepts::match<Value, std::complex>) {
    return isApproxEqual(valueA.real(), valueB.real()) && isApproxEqual(valueA.imag(), valueB.imag());
  } else {
    return valueA == Approx(valueB).epsilon(1e-3);
  }
}

template <typename Value> inline bool isApproxEqualAfterIORoundTrip(const Value &value) {
  std::stringstream ss;
  ss << value;
  Value value2;
  ss >> value2;
  return isApproxEqual(value, value2);
}

template <typename Value> inline bool isMemcmpEqualAfterSerializeRoundTrip(Value value) {
  Value valueCopy{};
  auto stream = std::make_shared<std::stringstream>();
  auto serializerIn = mi::StandardSerializer(static_cast<const std::shared_ptr<std::ostream> &>(stream));
  auto serializerOut = mi::StandardSerializer(static_cast<const std::shared_ptr<std::istream> &>(stream));
  serializerIn <=> value;
  serializerOut <=> valueCopy;
  return std::memcmp(&value, &valueCopy, sizeof(Value)) == 0;
}
