#include "Microcosm/Render/More/Scattering/Diffuse"

namespace mi::render {

LambertBSDF::LambertBSDF(Spectrum valueR, Spectrum valueT) noexcept : mValueR(std::move(valueR)), mValueT(std::move(valueT)) {
  if (mValueR.empty()) mValueR = spectrumZerosLike(mValueT);
  if (mValueT.empty()) mValueT = spectrumZerosLike(mValueR);
  double weightR = mValueR.sum();
  double weightT = mValueT.sum();
  mProbR = finiteOr(weightR / (weightR + weightT), 1.0);
}

BidirPDF LambertBSDF::scatter(Vector3d omegaO, Vector3d omegaI, Spectrum &f) const noexcept {
  double cosThetaO{abs(omegaO[2])};
  double cosThetaI{abs(omegaI[2])};
  if (isSameHemisphere(omegaO, omegaI)) {
    f.assign(OneOverPi * cosThetaI * mValueR);
    return BidirPDF{OneOverPi * cosThetaI * mProbR, OneOverPi * cosThetaO * mProbR};
  } else {
    f.assign(OneOverPi * cosThetaI * mValueT);
    return BidirPDF{OneOverPi * cosThetaI * (1 - mProbR), OneOverPi * cosThetaO * (1 - mProbR)};
  }
}

BidirPDF LambertBSDF::scatterSample(Vector2d sampleU, Vector3d omegaO, Vector3d &omegaI, Spectrum &ratio) const noexcept {
  if (sampleU[0] < mProbR) {
    sampleU[0] /= mProbR;
    omegaI = cosineHemisphereSample(sampleU), omegaI[2] = copysign(omegaI[2], +omegaO[2]);
    DoesntAlias(ratio) *= mValueR * (1 / mProbR);
    double cosThetaO{abs(omegaO[2])};
    double cosThetaI{abs(omegaI[2])};
    return BidirPDF{OneOverPi * cosThetaI * mProbR, OneOverPi * cosThetaO * mProbR};
  } else {
    sampleU[0] -= mProbR, sampleU[0] /= 1 - mProbR;
    omegaI = cosineHemisphereSample(sampleU), omegaI[2] = copysign(omegaI[2], -omegaO[2]);
    DoesntAlias(ratio) *= mValueT * (1 / (1 - mProbR));
    double cosThetaO{abs(omegaO[2])};
    double cosThetaI{abs(omegaI[2])};
    return BidirPDF{OneOverPi * cosThetaI * (1 - mProbR), OneOverPi * cosThetaO * (1 - mProbR)};
  }
}

OrenNayarBRDF::OrenNayarBRDF(Spectrum valueR, Spectrum sigma) noexcept : mValueR(std::move(valueR)), mCoeffA(sigma.shape), mCoeffB(sigma.shape) {
  for (size_t i = 0; i < sigma.size(); i++) {
    mCoeffA[i] = 1;
    mCoeffB[i] = 0;
    if (double s = 0.33 / sqr(sigma[i]); isfinite(s)) {
      mCoeffA[i] = (0.5 + s) / (1 + s);
      mCoeffB[i] = 0.45 / (1 + (3.0 / 11.0) * s);
    }
  }
}

BidirPDF OrenNayarBRDF::scatter(Vector3d omegaO, Vector3d omegaI, Spectrum &f) const noexcept {
  if (isSameHemisphere(omegaO, omegaI)) {
    double cosThetaO{abs(omegaO[2])};
    double cosThetaI{abs(omegaI[2])};
    double productX{omegaO[0] * omegaI[0]};
    double productY{omegaO[1] * omegaI[1]};
    double fraction{finiteOrZero(max(productX + productY, 0.0) / max(cosThetaO, cosThetaI))};
    f.assign(OneOverPi * cosThetaI * (mCoeffA + fraction * mCoeffB) * mValueR);
    return BidirPDF{OneOverPi * cosThetaI, OneOverPi * cosThetaO};
  } else {
    f = 0;
    return BidirPDF{};
  }
}

BidirPDF OrenNayarBRDF::scatterSample(Vector2d sampleU, Vector3d omegaO, Vector3d &omegaI, Spectrum &ratio) const noexcept {
  omegaI = cosineHemisphereSample(sampleU), omegaI[2] = copysign(omegaI[2], +omegaO[2]);
  double cosThetaO{abs(omegaO[2])};
  double cosThetaI{abs(omegaI[2])};
  double productX{omegaO[0] * omegaI[0]};
  double productY{omegaO[1] * omegaI[1]};
  double fraction{finiteOrZero(max(productX + productY, 0.0) / max(cosThetaO, cosThetaI))};
  DoesntAlias(ratio) *= (mCoeffA + fraction * mCoeffB) * mValueR;
  return BidirPDF{OneOverPi * cosThetaI, OneOverPi * cosThetaO};
}

DisneyDiffuseBRDF::DisneyDiffuseBRDF(Spectrum valueR, Spectrum retro, Spectrum sheen, Spectrum roughness) noexcept : mValueR(std::move(valueR)), mRetro(std::move(retro)), mSheen(std::move(sheen)), mRoughness(std::move(roughness)) {
  if (mRetro.empty()) mRetro = spectrumZerosLike(mValueR);
  if (mSheen.empty()) mSheen = spectrumZerosLike(mValueR);
  if (mRoughness.empty()) mRoughness = spectrumZerosLike(mValueR);
}

BidirPDF DisneyDiffuseBRDF::scatter(Vector3d omegaO, Vector3d omegaI, Spectrum &f) const noexcept {
  if (isSameHemisphere(omegaO, omegaI)) {
    Vector3d omegaM{reflectionHalfDirection(omegaO, omegaI)};
    double cosThetaO{abs(omegaO[2])}, schlickO{nthPow(1 - cosThetaO, 5)};
    double cosThetaI{abs(omegaI[2])}, schlickI{nthPow(1 - cosThetaI, 5)};
    double cosThetaM{abs(omegaM[2])}, schlickM{nthPow(1 - cosThetaM, 5)};
    Spectrum roughR{2 * sqr(dot(omegaO, omegaM)) * mRoughness};
    f.assign(OneOverPi * cosThetaI * ((1 - 0.5 * schlickO) * (1 - 0.5 * schlickI) * mValueR + (schlickO + schlickI - schlickO * schlickI * (1 - roughR)) * roughR * mRetro + schlickM * mSheen));
    return BidirPDF{OneOverPi * cosThetaI, OneOverPi * cosThetaO};
  } else {
    f = 0;
    return BidirPDF{};
  }
}

BidirPDF DisneyDiffuseBRDF::scatterSample(Vector2d sampleU, Vector3d omegaO, Vector3d &omegaI, Spectrum &ratio) const noexcept {
  omegaI = cosineHemisphereSample(sampleU), omegaI[2] = copysign(omegaI[2], +omegaO[2]);
  Vector3d omegaM{reflectionHalfDirection(omegaO, omegaI)};
  double cosThetaO{abs(omegaO[2])}, schlickO{nthPow(1 - cosThetaO, 5)};
  double cosThetaI{abs(omegaI[2])}, schlickI{nthPow(1 - cosThetaI, 5)};
  double cosThetaM{abs(omegaM[2])}, schlickM{nthPow(1 - cosThetaM, 5)};
  Spectrum roughR{2 * sqr(dot(omegaO, omegaM)) * mRoughness};
  DoesntAlias(ratio) *= ((1 - 0.5 * schlickO) * (1 - 0.5 * schlickI) * mValueR + (schlickO + schlickI - schlickO * schlickI * (1 - roughR)) * roughR * mRetro + schlickM * mSheen);
  return BidirPDF{OneOverPi * cosThetaI, OneOverPi * cosThetaO};
}

} // namespace mi::render
