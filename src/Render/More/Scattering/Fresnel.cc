#include "Microcosm/Render/More/Scattering/Fresnel"

namespace mi::render {

/* TODO Not entirely sure if this is correct. Followed what I found online and it seems to match up to
        the standard FresnelTerms in the case of a single layer, which is good at least. */
FresnelTerms
FresnelTerms::forLayers(double cosThetaI, double eta0, IteratorRange<const Layer *> layers, double waveLen) noexcept {
  Matrix2cd transferMs = {{1.0, 0.0}, {0.0, 1.0}};
  Matrix2cd transferMp = {{1.0, 0.0}, {0.0, 1.0}};
  complex<double> lastCosTheta{cosThetaI};
  complex<double> lastEta{eta0};
  for (const auto &[tau, eta] : layers) {
    FresnelTerms terms{lastCosTheta, lastEta / eta};
    complex<double> delta{tau * eta * terms.cosThetaT / waveLen};
    Matrix2cd propagate{{exp(-TwoPi * 1i * delta), 0.0}, {0.0, exp(+TwoPi * 1i * delta)}};
    transferMs = dot(dot(propagate, Matrix2cd{{1.0, terms.Rs}, {terms.Rs, 1.0}}) / terms.Ts, transferMs);
    transferMp = dot(dot(propagate, Matrix2cd{{1.0, terms.Rp}, {terms.Rp, 1.0}}) / terms.Tp, transferMp);
    lastCosTheta = terms.cosThetaT, lastEta = eta;
  }
  FresnelTerms terms;
  terms.eta = eta0 / lastEta;
  terms.cosThetaI = cosThetaI;
  terms.cosThetaT = lastCosTheta;
  terms.Rs = transferMs(1, 0) / transferMs(0, 0), terms.Ts = 1.0 / transferMs(0, 0);
  terms.Rp = transferMp(1, 0) / transferMp(0, 0), terms.Tp = 1.0 / transferMp(0, 0);
  return terms;
}

FresnelR schlickFresnelR(double eta) {
  return [eta](Spectrum &f, double cosThetaF, bool isOutsideF) { //
    f *= schlickApproximation(cosThetaF, isOutsideF ? eta : 1 / eta);
  };
}

FresnelR dielectricFresnelR(double eta) {
  return [eta](Spectrum &f, double cosThetaF, bool isOutsideF) { //
    f *= FresnelTerms(cosThetaF, isOutsideF ? eta : 1 / eta).powerR();
  };
}

FresnelR dielectricFresnelR(Spectrum eta) {
  return [eta = std::move(eta)](Spectrum &f, double cosThetaF, bool isOutsideF) {
    for (size_t i = 0; i < f.size(); i++) f[i] *= FresnelTerms(cosThetaF, isOutsideF ? eta[i] : 1 / eta[i]).powerR();
  };
}

FresnelR conductiveFresnelR(ComplexSpectrum eta) {
  return [eta = std::move(eta)](Spectrum &f, double cosThetaF, bool isOutsideF) {
    for (size_t i = 0; i < f.size(); i++) f[i] *= isOutsideF ? FresnelTerms(cosThetaF, eta[i]).powerR() : 0;
  };
}

} // namespace mi::render
