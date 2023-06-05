#include "testing.h"

TEST_CASE("math") {
  SUBCASE("Distributions") {
    auto ContinuousDistributionChecks = [](auto distr) {
      mi::Pcg32 prng = PRNG();
      auto distributionPDF{[&](double value) { return distr.distributionPDF(value); }};
      auto distributionCDF{[&](double value) { return distr.distributionCDF(value); }};
      auto distributionSample{[&](double sampleU) { return distr.distributionSample(sampleU); }};
      double sampleU{0.627}, value{distributionSample(sampleU)};
      CHECK(distributionCDF(value) == Approx(sampleU).epsilon(1e-5));
      CHECK(distributionPDF(value) == Approx(ApproxDerivative(value, 1e-5, distributionCDF)).epsilon(1e-4));
      CHECK(distributionPDF(value) == Approx(1.0 / ApproxDerivative(sampleU, 1e-5, distributionSample)).epsilon(1e-4));
      CHECK(distributionPDF(-mi::constants::Inf<double>) == 0.0);
      CHECK(distributionPDF(+mi::constants::Inf<double>) == 0.0);
      CHECK(distributionCDF(-mi::constants::Inf<double>) == 0.0);
      CHECK(distributionCDF(+mi::constants::Inf<double>) == 1.0);
      for (double moreSampleU : mi::randomize<double>(prng, 32) | std::views::transform(mi::lerp(0.1, 0.9))) {
        CHECK(distributionCDF(distributionSample(moreSampleU)) == Approx(moreSampleU).epsilon(1e-6));
      }
    };
    SUBCASE("Uniform") { ContinuousDistributionChecks(mi::distributions::Uniform(2, 7)); }
    SUBCASE("Normal") {
      mi::distributions::Normal distr(-3, 5); // mu, sigma
      ContinuousDistributionChecks(distr);
      CHECK(distr.distributionCDF(-3) == doctest::Approx(0.5));
      CHECK(distr.distributionCDF(-3 + 5) - distr.distributionCDF(-3 - 5) == doctest::Approx(0.68).epsilon(0.01));
      CHECK(distr.distributionCDF(-3 + 10) - distr.distributionCDF(-3 - 10) == doctest::Approx(0.95).epsilon(0.01));
      CHECK(distr.distributionCDF(-3 + 15) - distr.distributionCDF(-3 - 15) == doctest::Approx(0.997).epsilon(0.01));
    }
    SUBCASE("Cauchy") { ContinuousDistributionChecks(mi::distributions::Cauchy(-0.4, 1.9)); }
    SUBCASE("Logistic") { ContinuousDistributionChecks(mi::distributions::Logistic(-2.7, 0.4)); }
    SUBCASE("HyperbolicSecant") { ContinuousDistributionChecks(mi::distributions::HyperbolicSecant(+5.1, 3.2)); }
    SUBCASE("Exponential") { ContinuousDistributionChecks(mi::distributions::Exponential(4.2)); }
  }
  SUBCASE("Interpolation") {
    CHECK(mi::lerp(0.5, 4, 8) == 6);
    CHECK(mi::lerp(0.0, 7, 9) == 7);
    CHECK(mi::lerp(1.0, 7, 9) == 9);
    CHECK(mi::lerp(13, 17)(0.7) == doctest::Approx((1 - 0.7) * 13 + 0.7 * 17));
    CHECK(mi::unlerp(18.0, 10.0, 20.0) == doctest::Approx(0.8));
    CHECK(mi::unlerp(18.0, 10.0, 10.0) == 0.0); // No explosion
    CHECK(std::ranges::size(mi::linspace(8)) == 8);
    CHECK(mi::linspace(8)[0] == 0.0);
    CHECK(mi::linspace(8)[7] == 0.875);
    CHECK(std::ranges::size(mi::linspace(8, mi::Exclusive(0.0), 1.0)) == 8);
    CHECK(std::ranges::size(mi::linspace(8, 0.0, mi::Exclusive(1.0))) == 8);
    CHECK(mi::linspace(8, 0.0, mi::Exclusive(1.0))[0] == 0.0);
    CHECK(mi::linspace(8, 0.0, mi::Exclusive(1.0))[7] == 0.875);
    CHECK(mi::linspace(8, mi::Exclusive(0.0), 1.0)[0] == 0.125);
    CHECK(mi::linspace(8, mi::Exclusive(0.0), 1.0)[7] == 1.0);
    CHECK(std::ranges::size(mi::linspace(8, mi::Exclusive(0.0), mi::Exclusive(1.0))) == 8);
    CHECK(mi::linspace(7, mi::Exclusive(0.0), mi::Exclusive(1.0))[0] == 0.125);
    CHECK(mi::linspace(7, mi::Exclusive(0.0), mi::Exclusive(1.0))[6] == 0.875);
    SUBCASE("Hermite") {
      mi::Pcg32 prng = PRNG();
      double valueA = mi::lerp(mi::randomize<double>(prng), -4, 4);
      double slopeA = mi::lerp(mi::randomize<double>(prng), -4, 4);
      double slopeB = mi::lerp(mi::randomize<double>(prng), -4, 4);
      double valueB = mi::lerp(mi::randomize<double>(prng), -4, 4);
      CHECK(mi::hermite(0.0, valueA, slopeA, slopeB, valueB) == valueA);
      CHECK(mi::hermite(1.0, valueA, slopeA, slopeB, valueB) == valueB);
      /* CHECK(mi::hermiteDerivative(0.0, valueA, slopeA, slopeB, valueB) == slopeA);
         CHECK(mi::hermiteDerivative(1.0, valueA, slopeA, slopeB, valueB) == slopeB); */
    }
    SUBCASE("Catmull-Rom") {
      // TODO
    }
#if 0
      SUBCASE("Easing") {
        auto DerivativeIsCorrect = [](auto easing) -> bool {
          mi::Pcg32 prng = PRNG();
          for (double fraction : mi::randomize<double>(prng, 32) | std::views::transform(mi::lerp(0.1, 0.9))) {
            double derivative = easing.derivative(fraction);
            double estimate = (easing(fraction + 1e-4) - easing(fraction - 1e-4)) / 2e-4;
            if (derivative != doctest::Approx(estimate).epsilon(5e-4)) return false;
          }
          return true;
        };
        CHECK(DerivativeIsCorrect(mi::ease::SmoothStart<3>()));
        CHECK(DerivativeIsCorrect(mi::ease::SmoothStop<2>()));
        CHECK(DerivativeIsCorrect(mi::ease::Smooth<4, 7>()));
        CHECK(DerivativeIsCorrect(mi::ease::ExpSmoothStart(0.7)));
        CHECK(DerivativeIsCorrect(mi::ease::ExpSmoothStop(0.5)));
        CHECK(DerivativeIsCorrect(mi::ease::ExpSmooth(0.6, 0.8)));
        CHECK(DerivativeIsCorrect(mi::ease::CosSmoothStart()));
        CHECK(DerivativeIsCorrect(mi::ease::CosSmoothStop()));
        CHECK(DerivativeIsCorrect(mi::ease::CosSmooth()));
      }
#endif
  }
}

TEST_CASE_TEMPLATE("math", Float, float, double) {
  auto prng = PRNG();
  SUBCASE("Safe sqrt") {
    CHECK(mi::safeSqrt(+Float(2)) == mi::sqrt(+Float(2)));
    CHECK(mi::safeSqrt(-Float(2)) == 0);
  }
  SUBCASE("Saturate") {
    CHECK(mi::saturate(mi::constants::NaN<Float>) == Float(0));
    CHECK(mi::saturate(-mi::constants::Inf<Float>) == Float(0));
    CHECK(mi::saturate(+mi::constants::Inf<Float>) == Float(1));
  }
  SUBCASE("Machine epsilon") {
    CHECK(Float(1) == (Float(1) + mi::constants::MachineEps<Float>));
    CHECK(Float(1) != (Float(1) + mi::nextFloat(mi::constants::MachineEps<Float>)));
    CHECK(mi::isTiny(mi::constants::MachineEps<Float>, Float(1)));
    CHECK(mi::isTiny(Float(64) * mi::constants::MachineEps<Float>, Float(64)));
    CHECK(mi::isHuge(Float(64), Float(64) * mi::constants::MachineEps<Float>));
    CHECK_FALSE(mi::isTiny(mi::nextFloat(mi::constants::MachineEps<Float>), Float(1)));
    CHECK_FALSE(mi::isTiny(mi::nextFloat(Float(64) * mi::constants::MachineEps<Float>), Float(64)));
  }
  SUBCASE("Minimum squarable") {
    CHECK(Float(0) != mi::sqr(mi::constants::MinSqr<Float>));
    CHECK(Float(0) == mi::sqr(mi::prevFloat(mi::constants::MinSqr<Float>)));
  }
  SUBCASE("Minimum invertible") {
    CHECK(mi::isfinite(Float(1) / mi::constants::MinInv<Float>));
    CHECK_FALSE(mi::isfinite(Float(1) / mi::prevFloat(mi::constants::MinInv<Float>)));
  }
  SUBCASE("Next/previous increments") {
    for (Float value : mi::randomize<Float>(prng, 32) | std::views::transform(mi::lerp(-64, +64))) {
      CHECK(mi::nextFloat(value) == std::nextafter(value, +mi::constants::Inf<Float>));
      CHECK(mi::prevFloat(value) == std::nextafter(value, -mi::constants::Inf<Float>));
    }
    // No increment past infinity.
    CHECK(mi::nextFloat(+mi::constants::Inf<Float>) == +mi::constants::Inf<Float>);
    CHECK(mi::prevFloat(-mi::constants::Inf<Float>) == -mi::constants::Inf<Float>);
  }
  SUBCASE("Fast rounding functions") {
    for (Float value : mi::randomize<Float>(prng, 32) | std::views::transform(mi::lerp(-64, +64))) {
      if (mi::fastFract(value) == Float(0.5)) [[unlikely]]
        continue;
      CHECK(mi::fastFloor(value) == mi::floor(value));
      CHECK(mi::fastCeil(value) == mi::ceil(value));
      CHECK(mi::fastRound(value) == mi::round(value));
      CHECK(mi::fastTrunc(value) == mi::trunc(value));
    }
  }
  SUBCASE("Sine and cosine with pi-scaling") {
    for (Float value : mi::randomize<Float>(prng, 32) | std::views::transform(mi::lerp(-64, +64))) {
      auto [sinPiValue, cosPiValue] = mi::sinCosPi(value);
      CHECK(mi::sinPi(value) == Approx(mi::sin(mi::constants::Pi<Float> * value)));
      CHECK(mi::cosPi(value) == Approx(mi::cos(mi::constants::Pi<Float> * value)));
      CHECK(sinPiValue == Approx(mi::sin(mi::constants::Pi<Float> * value)));
      CHECK(cosPiValue == Approx(mi::cos(mi::constants::Pi<Float> * value)));
    }
  }
  SUBCASE("Erf inverse") {
    for (Float value : mi::randomize<Float>(prng, 32) | std::views::transform(mi::lerp(-16, 16))) {
      if (Float erfValue = mi::erf(value); mi::abs(erfValue) < Float(0.999)) {
        CHECK(mi::erfInverse(erfValue) == Approx(value).epsilon(1e-5));
      }
    }
  }
  SUBCASE("Quadratic roots") {
    auto roots = mi::solveQuadratic(Float(-3.2), Float(2.1), Float(1.5));
    CHECK(roots.size() == 2);
    CHECK(roots[0] == Approx(-0.431).epsilon(1e-3));
    CHECK(roots[1] == Approx(+1.087).epsilon(1e-3));
    CHECK(!mi::solveQuadratic(Float(+3.2), Float(2.1), Float(1.5)));
    CHECK(mi::solveQuadratic(Float(+1), Float(+4), Float(+4)).size() == 1);
    CHECK(mi::solveQuadratic(Float(+1), Float(-4), Float(+4)).size() == 1);
  }
  SUBCASE("Cubic roots") {
    CHECK(mi::solveCubic(Float(+1), Float(-1), Float(-1), Float(+1)).size() == 2);
    CHECK(mi::solveCubic(Float(+1), Float(-1), Float(-1), Float(+0.5)).size() == 3);
    CHECK(mi::solveCubic(Float(+1), Float(-1), Float(-1), Float(+1.5)).size() == 1);
  }
#if 0
  SUBCASE("Converger") {
    mi::Converger<Float> converger;
    converger.target = 7;
    Float t00 = 0.26, t01 = 1.96;
    Float t10 = 2.57, t11 = 3.41;
    Float t20 = 3.84, t21 = 6.10;
    auto f = [](Float x) {
      return mi::nthpow(x - 1, 3) * (x - 3) * (x - 4) + 7;
    };
    auto g = [](Float x) {
      return mi::nthpow(x - 1, 2) * (5 * x * x - 30 * x + 43);
    };
    CHECK(converger.newton(&t00, f, g, 3));
    CHECK(converger.newton(&t01, f, g, 3));
    CHECK(converger.newton(&t10, f, g));
    CHECK(converger.newton(&t11, f, g));
    CHECK(converger.newton(&t20, f, g));
    CHECK(converger.newton(&t21, f, g));
    CHECK(t00 == Approx(1).epsilon(1e-2));
    CHECK(t01 == Approx(1).epsilon(1e-2));
    CHECK(t10 == Approx(3).epsilon(1e-3));
    CHECK(t11 == Approx(3).epsilon(1e-3));
    CHECK(t20 == Approx(4).epsilon(1e-3));
    CHECK(t21 == Approx(4).epsilon(1e-3));
  }
#endif
}
