#include "Microcosm/Quadrature"
#include "testing.h"

TEST_CASE("Quadrature") {
  mi::Quadrature<7> quad7;
  mi::Quadrature<52> quad52;
  mi::Quadrature<103> quad103;
  mi::Quadrature<518> quad518;
  mi::AdaptiveSimpsonQuadrature adaptive;
  SUBCASE("Cubic") {
    auto f = [](double x) { return x * x * x - 2 * x * x - 3 * x + 5; };
    CHECK(quad7(+2.0, +8.0, f) == Approx(624).epsilon(1e-4));
    CHECK(quad52(+1.0, -6.0, f) == Approx(380.916666).epsilon(1e-4));
    CHECK(adaptive(+1.0, -6.0, f) == Approx(380.916666).epsilon(1e-4));
  }
  SUBCASE("Transcendental") {
    auto f = [](double x) {
      double u = std::cos(x);
      return 1 + std::exp(-u * u) * std::sin(x * u);
    };
    CHECK(quad103(+1.5, +4.8, f) == Approx(2.26931542).epsilon(1e-3));
    CHECK(quad518(-7.2, -2.0, f) == Approx(6.20949359).epsilon(1e-3));
    CHECK(adaptive(-7.2, -2.0, f) == Approx(6.20949359).epsilon(1e-3));
  }
  SUBCASE("Transcendental with infinite limits") {
    mi::distributions::Normal distr(1.5, 3.3);
    auto f = [=](double x) { return distr.distributionPDF(x); };
    auto g = [=](double x) { return distr.distributionCDF(x); };
    double inf = mi::constants::Inf<double>;
    CHECK(quad103(-inf, +inf, f) == Approx(+1).epsilon(1e-3));
    CHECK(quad103(+inf, -inf, f) == Approx(-1).epsilon(1e-3));
    CHECK(quad518(-inf, +2.2, f) == Approx(+g(2.2)).epsilon(1e-3));
    CHECK(quad518(+0.7, +inf, f) == Approx(-g(0.7) + 1).epsilon(1e-3));
  }
}
