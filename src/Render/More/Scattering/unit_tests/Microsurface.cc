#include "Microcosm/Render/More/Scattering/Microsurface"
#include "testing.h"

namespace mir = mi::render;

TEST_CASE("Microsurface") {
  mir::Microsurface surface{{0.94027, 0.42551}, mir::normalMicrosurfaceHeight};
  SUBCASE("Projected area and normal PDF") {
    auto BruteForceProjectedArea = [&](mi::Vector3d omegaO) {
      mir::LowDiscrepancySequence2d seq;
      double value{0};
      for (int i = 0; i < 500000; i++) {
        mi::Vector3d omegaM{mir::cosineHemisphereSample(seq())};
        value += mi::max(mi::dot(omegaO, omegaM), 0) * surface.normalPDF(omegaM) / (mir::OneOverPi * mi::abs(omegaM[2]));
      }
      return value /= 500000;
    };
    mi::Vector3d omegaO{mi::normalize(mi::Vector3d(-0.10526, +0.09481, +0.24307))};
    CHECK(BruteForceProjectedArea(omegaO) == Approx(surface.projectedArea(omegaO)).epsilon(1e-2));
  }
  SUBCASE("Height-specific G1 and height-averaged G1 and G2") {
    auto BruteForceG1{[&](mi::Vector3d omegaO) {
      double value{0};
      for (double h0 : mi::linspace(5000, -8, +8)) // -8σ to +8σ
        value += surface.shadowG1(omegaO, h0) * surface.heightPDF(h0);
      return value * 16.0 / 5000.0;
    }};
    auto BruteForceG2{[&](mi::Vector3d omegaO, mi::Vector3d omegaI) {
      double value{0};
      for (double h0 : mi::linspace(5000, -8, +8)) // -8σ to +8σ
        value += surface.shadowG1(mir::upperHemisphere(omegaO), h0 * mi::copysign(1.0, omegaO[2])) *
                 surface.shadowG1(mir::upperHemisphere(omegaI), h0 * mi::copysign(1.0, omegaI[2])) * surface.heightPDF(h0);
      return value * 16.0 / 5000.0;
    }};
    mi::Vector3d omegaO{mi::normalize(mi::Vector3d(+0.1304, -0.4676, +0.1269))};
    mi::Vector3d omegaI{mi::normalize(mi::Vector3d(-0.4171, -0.1891, +0.5099))};
    CHECK(BruteForceG1(omegaO) == Approx(surface.shadowG1(omegaO)).epsilon(1e-4));
    CHECK(BruteForceG2(omegaO, +omegaI) == Approx(surface.shadowG2(omegaO, +omegaI)).epsilon(1e-4));
    CHECK(BruteForceG2(omegaO, -omegaI) == Approx(surface.shadowG2(omegaO, -omegaI)).epsilon(1e-4));
  }
  SUBCASE("Visible height CDF and G1") {
    mi::Vector3d omega{mi::normalize(mi::Vector3d(+0.7, -1.8, +2.8))}; // Upper hemisphere now
    double h0{-0.318};
    auto visibleHeightCDF{[&](double h1) { return surface.visibleHeightCDF(omega, h0, h1); }};
    CHECK(visibleHeightCDF(mir::Inf) < 1);
    CHECK(visibleHeightCDF(mir::Inf) > 0);
    CHECK(visibleHeightCDF(mir::Inf) == Approx(1 - surface.shadowG1(omega, h0)));
  }
  SUBCASE("Visible height PDF, CDF, and sampling routine") {
    mi::Vector3d omega{mi::normalize(mi::Vector3d(-1.7, +0.3, -0.8))};
    double h0{0.447};
    auto visibleHeightPDF{[&](double h1) { return surface.visibleHeightPDF(omega, h0, h1); }};
    auto visibleHeightCDF{[&](double h1) { return surface.visibleHeightCDF(omega, h0, h1); }};
    auto visibleHeightSample{[&](double sampleU) { return surface.visibleHeightSample(sampleU, omega, h0); }};
    double sampleU{0.566}, h1{visibleHeightSample(sampleU)};
    // Note that the looking direction is pointing downward (z < 0) into the microsurface. That being the case, the
    // likelihood of intersection is increasing with respect to decreasing height, and thus the sampling routine produces
    // heights strictly below h0 and the CDF is legitimately decreasing. This is why we oddly have to negate the derivatives.
    CHECK(h1 < h0);
    CHECK(visibleHeightPDF(h0 + 1e-3) == 0); // No probability of intersecting behind us.
    CHECK(visibleHeightPDF(h1) == Approx(-ApproxDerivative(h1, 1e-5, visibleHeightCDF)).epsilon(1e-4));
    CHECK(visibleHeightPDF(h1) == Approx(-1.0 / ApproxDerivative(sampleU, 1e-5, visibleHeightSample)).epsilon(1e-4));
    CHECK(visibleHeightCDF(h1) == Approx(sampleU).epsilon(1e-4));
  }
  SUBCASE("Smith transmission") {
    auto BruteForceTransmission{[&](mi::Vector3d omega, double h0, double h1) {
      double sinTheta{mi::hypot(omega[0], omega[1])};
      double cotTheta{omega[2] / sinTheta};
      double value{0};
      for (double tau : mi::linspace(5000, 0.0, (h1 - h0) / cotTheta))
        value += surface.smithExtinction(-omega, h0 + tau * cotTheta) / sinTheta;
      return mi::exp(-value * (h1 - h0) / cotTheta / 5000);
    }};
    mi::Vector3d omega{mi::normalize(mi::Vector3d(+0.7, +0.8, -0.4))};
    double h0{+0.812};
    double h1{-0.332};
    CHECK(BruteForceTransmission(+omega, h0, h1) == Approx(surface.smithTransmission(+omega, h0, h1)).epsilon(1e-4));
    CHECK(BruteForceTransmission(-omega, h1, h0) == Approx(surface.smithTransmission(-omega, h1, h0)).epsilon(1e-4));
  }
  SUBCASE("Specular reflection and refraction") {
    SUBCASE("Reflection") {
      mi::Vector3d omegaO{mi::normalize(mi::Vector3d(+0.10248, -0.11391, +0.25018))};
      mi::Vector3d omegaI{mi::normalize(mi::Vector3d(-0.29638, -0.41583, +0.71014))};
      auto forwardTerms = surface.specularReflection(omegaO, omegaI);
      auto reverseTerms = surface.specularReflection(omegaI, omegaO);
      CHECK(forwardTerms.value / omegaI[2] == Approx(reverseTerms.value / omegaO[2]));
      CHECK(forwardTerms.bidirPDF.forward == Approx(reverseTerms.bidirPDF.reverse));
      CHECK(forwardTerms.bidirPDF.reverse == Approx(reverseTerms.bidirPDF.forward));
    }
    SUBCASE("Refraction") {
      mi::Vector3d omegaO{mi::normalize(mi::Vector3d(+0.10248, -0.11391, +0.25018))};
      mi::Vector3d omegaI{mi::normalize(mi::Vector3d(-0.29638, -0.41583, -0.71014))};
      double cosThetaO{mi::abs(omegaO[2])};
      double cosThetaI{mi::abs(omegaI[2])};
      double etaO{1.10427};
      double etaI{1.76886};
      auto forwardTerms = surface.specularRefraction(omegaO, omegaI, etaO, etaI);
      auto reverseTerms = surface.specularRefraction(omegaI, omegaO, etaI, etaO);
      CHECK(forwardTerms.value / cosThetaI == Approx(reverseTerms.value / cosThetaO));
      CHECK(forwardTerms.bidirPDF.forward == Approx(reverseTerms.bidirPDF.reverse));
      CHECK(forwardTerms.bidirPDF.reverse == Approx(reverseTerms.bidirPDF.forward));
    }
  }
}
