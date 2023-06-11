#include "Microcosm/Render/More/Scattering/Phase"

namespace mi::render {

BidirPDF IsotropicPhaseWithOneParameter::scatter(Vector3d omegaO, Vector3d omegaI, Spectrum &f) const {
  double phase{0};
  double cosThetaP{dot(omegaO, -omegaI)};
  for (size_t i = 0; i < mParam.size(); i++) {
    double term = mPhase(mParam[i], cosThetaP);
    f[i] = term;
    phase += term;
  }
  phase /= mParam.size();
  return BidirPDF{phase, phase};
}

BidirPDF IsotropicPhaseWithOneParameter::scatterSample(Vector2d sampleU, Vector3d omegaO, Vector3d &omegaI, Spectrum &ratio) const {
  double param{mParam[spectrumIndexSample(mParam.size(), sampleU[0])]};
  double cosThetaP{mPhaseSample(param, sampleU[0])};
  double sinThetaP{sqrt(1 - sqr(cosThetaP))};
  omegaI = normalize(dot(Matrix3d::orthonormalBasis(-omegaO), Vector3d(sinThetaP * cos(TwoPi * sampleU[1]), sinThetaP * sin(TwoPi * sampleU[1]), cosThetaP)));
  double phase{0};
  for (size_t i = 0; i < mParam.size(); i++) {
    double term = mPhase(mParam[i], cosThetaP);
    phase += term;
    ratio[i] *= term;
  }
  phase /= mParam.size(), ratio *= 1 / phase;
  return BidirPDF{phase, phase};
}

} // namespace mi::render
