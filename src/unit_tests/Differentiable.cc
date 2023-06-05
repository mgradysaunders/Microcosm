#include "Microcosm/Differentiable"
#include "testing.h"

TEST_CASE("Differentiable") {
  SUBCASE("Scalar functions") {
    auto Function1 = [](auto t) { return t * mi::log(mi::exp(t + 0.5 * mi::cos(8 * t)) + 1); };
    auto Function2 = [](auto t) { return (mi::pow(t, 3.6) - mi::pow(0.2, t)) / mi::hypot(1, t); };
    auto BruteForceCheck = [](auto &&func, double t, double dt = 1e-6) {
      CHECK(
        func(mi::Differentiable<double>(t, 1)).deriv() ==
        Approx((func(t + dt / 2) - func(t - dt / 2)) / dt).epsilon(5 * dt));
    };
    BruteForceCheck(Function1, -0.674);
    BruteForceCheck(Function1, +4.227);
    BruteForceCheck(Function1, -3.173);
    BruteForceCheck(Function2, +1.447);
    BruteForceCheck(Function2, +5.871);
  }
}
