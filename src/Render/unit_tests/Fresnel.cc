#include "Microcosm/Render/More/Scattering/Fresnel"
#include "testing.h"

TEST_CASE("Fresnel") {
  SUBCASE("Usage") {
    double thetaI = 30.0_degrees;
    double thetaT = 23.5782_degrees;
    double etaI = 1.2;
    double etaT = 1.5;
    mi::render::FresnelTerms terms(mi::cos(thetaI), etaI, etaT);
    CHECK(terms.cosThetaT.real() == Approx(mi::cos(thetaT)).epsilon(1e-4));
    CHECK(terms.cosThetaT.imag() == 0);
    CHECK(terms.powerRs() == Approx(1.93205e-2).epsilon(1e-4));
    CHECK(terms.powerRp() == Approx(6.89695e-3).epsilon(1e-4));
    CHECK(terms.evanescentTransmission() == false);
    CHECK(mi::render::schlickApproximation(mi::cos(thetaI), etaI, etaT) == Approx(terms.powerR()).epsilon(1e-3));
  }
  SUBCASE("Trigonometry") {
    double thetaI = 42.199_degrees;
    double etaI = 1.1021;
    double etaT = 1.8872;
    mi::render::FresnelTerms terms(mi::cos(thetaI), etaI, etaT);
    double thetaT = mi::acos(terms.cosThetaT.real());
    CHECK(terms.cosThetaT.imag() == 0);
    CHECK(terms.Rs.real() == Approx(-mi::sin(thetaI - thetaT) / mi::sin(thetaI + thetaT)));
    CHECK(terms.Rp.real() == Approx(+mi::tan(thetaI - thetaT) / mi::tan(thetaI + thetaT)));
    CHECK(terms.powerRs() + terms.powerTs() == Approx(1.0));
    CHECK(terms.powerRp() + terms.powerTp() == Approx(1.0));
    CHECK(terms.evanescentTransmission() == false);
  }
  SUBCASE("Critical angle for Total Internal Reflection (TIR)") {
    double etaI = 1.5524;
    double etaT = 1.2233;
    double thetaC = mi::render::criticalAngle(etaI, etaT);
    CHECK(thetaC == Approx(51.9942_degrees).epsilon(1e-4));
    CHECK(mi::render::FresnelTerms(mi::cos(thetaC + 0.01), etaI, etaT).powerR() == Approx(1.0));
    CHECK(mi::render::FresnelTerms(mi::cos(thetaC + 0.01), etaI, etaT).evanescentTransmission() == true);
    CHECK(mi::render::FresnelTerms(mi::cos(thetaC - 0.01), etaI, etaT).evanescentTransmission() == false);
  }
  SUBCASE("Brewster angle") {
    double etaI = 1.1123;
    double etaT = 1.4721;
    double thetaB = mi::render::brewsterAngle(etaI, etaT);
    CHECK(thetaB == Approx(52.9258_degrees).epsilon(1e-4));
    CHECK(mi::render::FresnelTerms(mi::cos(thetaB), etaI, etaT).powerRp() == Approx(0.0));
  }
  SUBCASE("Film") {
    // Spot-check versus https://www.filmetrics.com/reflectance-calculator
    auto terms = mi::render::FresnelTerms::forLayers(mi::cos(30.0_degrees), 1.0, {{0.2, 1.2}, {0.0, 1.5}}, 0.632);
    CHECK(terms.powerRs() == Approx(0.01922).epsilon(1e-4));
    CHECK(terms.powerRp() == Approx(0.00861).epsilon(1e-4));
    CHECK(terms.powerTs() == Approx(0.98078).epsilon(1e-4));
    CHECK(terms.powerTp() == Approx(0.99138).epsilon(1e-4));
  }
}
