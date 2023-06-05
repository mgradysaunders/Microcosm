#include "Microcosm/Render/More/Scattering/Microsurface"

namespace mi::render {

Vector2d GGXMicrosurfaceSlope::visibleSlopeSample(Vector2d sampleU, double cosThetaO) const noexcept {
  if (cosThetaO > 0.99999) return Vector2d::polar(sqrt(sampleU[0] / (1 - sampleU[0])), TwoPi * sampleU[1]);
  double sinThetaO{sqrt(1 - sqr(cosThetaO))};
  double tanThetaO{sinThetaO / cosThetaO};
  double mu{sampleU[0] * (1 + 1 / cosThetaO) - 1}, nu{1 / (1 - sqr(mu))};
  double discrim{safeSqrt(nu * (sqr(mu) - (1 - nu) * sqr(tanThetaO)))};
  double slopeX0{-nu * tanThetaO - discrim};
  double slopeX1{-nu * tanThetaO + discrim};
  double slopeX{mu < 0 || slopeX1 * sinThetaO > cosThetaO ? slopeX0 : slopeX1};
  double slopeY{1};
  if (sampleU[1] > 0.5)
    sampleU[1] = saturate(2 * sampleU[1] - 1);
  else
    sampleU[1] = saturate(1 - 2 * sampleU[1]), slopeY = -1;
  slopeY *= sqrt(1 + sqr(slopeX));
  slopeY *= (sampleU[1] * (sampleU[1] * (sampleU[1] * 0.273850 - 0.733690) + 0.463410)) /
            (sampleU[1] * (sampleU[1] * (sampleU[1] * 0.093073 + 0.309420) - 1.000000) + 0.597999);
  return {slopeX, slopeY};
}

Vector2d BeckmannMicrosurfaceSlope::visibleSlopeSample(Vector2d sampleU, double cosThetaO) const noexcept {
  if (sampleU[0] < 1e-6) sampleU[0] = 1e-6;
  if (cosThetaO < -0.99999) cosThetaO = -0.99999;
  if (cosThetaO > +0.99999) return Vector2d::polar(sqrt(-log1p(-sampleU[0])), TwoPi * sampleU[1]);
  double sinThetaO{sqrt(1 - sqr(cosThetaO))};
  double cotThetaO{cosThetaO / sinThetaO};
  auto visibleCDF{[&](double a) { return 0.5 * OneOverSqrtPi * sinThetaO * exp(-sqr(a)) + 0.5 * cosThetaO * erfc(-a); }};
  if (double visibleCDFNorm = 1 / visibleCDF(cotThetaO); isfinite(visibleCDFNorm)) {
    // Improved initial guess lifted from PBRT-v3 source.
    double thetaO{acos(cosThetaO)};
    double xMax{erf(cotThetaO)};
    double xMin{-1};
    double x{-0.0564};
    x = thetaO * x + 0.4265;
    x = thetaO * x - 0.876;
    x = thetaO * x + 1;
    x = xMax - (1 + xMax) * pow(1 - sampleU[0], x);
    // Do numerical inversion.
    if (solveNewton(
          x, xMin, xMax, sampleU[0], 1e-6,
          [&](double x) { return x >= cotThetaO ? 1 : visibleCDFNorm * visibleCDF(erfInverse(x)); },
          [&](double x) { return visibleCDFNorm / 2 * (cosThetaO - erfInverse(x) * sinThetaO); }))
      return {erfInverse(x), erfInverse(2 * sampleU[1] - 1)};
  }
  return {};
}

const MicrosurfaceHeight uniformMicrosurfaceHeight = UniformMicrosurfaceHeight();

const MicrosurfaceHeight normalMicrosurfaceHeight = NormalMicrosurfaceHeight();

const MicrosurfaceSlope ggxMicrosurfaceSlope = GGXMicrosurfaceSlope();

const MicrosurfaceSlope beckmannMicrosurfaceSlope = BeckmannMicrosurfaceSlope();

double Microsurface::normalPDF(Vector3d omegaM) const noexcept {
  if (!isUpperHemisphere(omegaM)) [[unlikely]]
    return 0;
  double cos2ThetaM{sqr(omegaM[2])};
  double cos4ThetaM{sqr(cos2ThetaM)};
  return finiteOrZero(slopePDF(convertNormalToSlope(omegaM)) / saturate(cos4ThetaM));
}

double Microsurface::visibleNormalPDF(Vector3d omegaO, Vector3d omegaM) const noexcept {
  if (!isUpperHemisphere(omegaM)) [[unlikely]]
    return 0;
  double cosThetaO{dot(omegaO, omegaM)};
  double areaRatio{max(cosThetaO, 0) / projectedArea(omegaO)};
  return areaRatio > 0 ? finiteOrZero(areaRatio * normalPDF(omegaM)) : 0;
}

Vector3d Microsurface::visibleNormalSample(Vector2d sampleU, Vector3d omegaO) const noexcept {
  Vector3d omega11{normalize(Vector3d(mRoughness[0] * omegaO[0], mRoughness[1] * omegaO[1], omegaO[2]))};
  Vector2d slope11{mSlope.visibleSlopeSample(sampleU, omega11[2])};
  double phi{atan2(omega11[1], omega11[0])};
  double sinPhi{sin(phi)};
  double cosPhi{cos(phi)};
  Vector2d slope{
    mRoughness[0] * (cosPhi * slope11[0] - sinPhi * slope11[1]), //
    mRoughness[1] * (sinPhi * slope11[0] + cosPhi * slope11[1])};
  if (!allTrue(isfinite(slope))) [[unlikely]] {
    return omegaO[2] == 0 ? normalize(omegaO) : Vector3d{0, 0, 1};
  } else {
    return convertSlopeToNormal(slope);
  }
}

double Microsurface::visibleHeightPDF(Vector3d omega, double h0, double h1) const noexcept {
  if (h0 < h1 && omega[2] < 0) return 0; // Increasing height but pointing down?
  if (h0 > h1 && omega[2] > 0) return 0; // Decreasing height but pointing up?
  double smithLambdaOmega{smithLambda(omega)};
  return finiteOrZero(
    abs(smithLambdaOmega) * mHeight.heightPDF(h1) * pow(mHeight.heightCDF(h0), smithLambdaOmega) /
    pow(mHeight.heightCDF(h1), 1 + smithLambdaOmega));
}

double Microsurface::visibleHeightCDF(Vector3d omega, double h0, double h1) const noexcept {
  if (h0 < h1 && omega[2] < 0) return 0; // Increasing height but pointing down?
  if (h0 > h1 && omega[2] > 0) return 0; // Decreasing height but pointing up?
  return saturate(1 - finiteOrZero(pow(mHeight.heightCDF(h0) / mHeight.heightCDF(h1), smithLambda(omega))));
}

double Microsurface::visibleHeightSample(double sampleU, Vector3d omega, double h0) const noexcept {
  if (abs(omega[2]) < 0.00001) return h0;
  if (omega[2] < -0.99999) return mHeight.heightSample(mHeight.heightCDF(h0) * sampleU);
  if (omega[2] > +0.99999) return Inf;
  if (sampleU >= 1 - shadowG1(omega, h0)) return Inf;
  return mHeight.heightSample(mHeight.heightCDF(h0) / pow(1 - sampleU, 1 / smithLambda(omega)));
}

Microsurface::SpecularTerms Microsurface::specularReflection(Vector3d omegaO, Vector3d omegaI) const noexcept {
  if (isLowerHemisphere(omegaO)) omegaO *= -1, omegaI *= -1;
  if (isLowerHemisphere(omegaI)) [[unlikely]]
    return {};
  Vector3d omegaM{reflectionHalfDirection(omegaO, omegaI)};
  double normalTermOver4{0.25 * normalPDF(omegaM)};
  double smithLambdaOmegaO{smithLambda(omegaO)}, projectedAreaOmegaO{(1 + smithLambdaOmegaO) * omegaO[2]};
  double smithLambdaOmegaI{smithLambda(omegaI)}, projectedAreaOmegaI{(1 + smithLambdaOmegaI) * omegaI[2]};
  double shadowing{1 / (1 + smithLambdaOmegaO + smithLambdaOmegaI)};
  return SpecularTerms{
    finiteOrZero(normalTermOver4 * shadowing / omegaO[2]),
    BidirPDF{
      finiteOrZero(normalTermOver4 / projectedAreaOmegaO), //
      finiteOrZero(normalTermOver4 / projectedAreaOmegaI)}};
}

Microsurface::SpecularTerms Microsurface::specularRefraction(Vector3d omegaO, Vector3d omegaI, double eta) const noexcept {
  if (isLowerHemisphere(omegaO)) omegaO *= -1, omegaI *= -1;
  if (isUpperHemisphere(omegaI)) [[unlikely]]
    return {};
  Vector3d omegaM{upperHemisphere(refractionHalfDirection(omegaO, omegaI, eta))};
  double cosThetaO{dot(omegaO, omegaM)};
  double cosThetaI{dot(omegaI, omegaM)};
  if (!(cosThetaO > 0 && cosThetaI < 0)) [[unlikely]]
    return {};
  double normalTerm{normalPDF(omegaM)};
  double forwardJacobian{refractionHalfVectorJacobian(omegaO, omegaI, eta)};
  double reverseJacobian{refractionHalfVectorJacobian(omegaI, omegaO, 1 / eta)};
  double smithLambdaOmegaO{smithLambda(+omegaO)}, projectedAreaOmegaO{(1 + smithLambdaOmegaO) * +omegaO[2]};
  double smithLambdaOmegaI{smithLambda(-omegaI)}, projectedAreaOmegaI{(1 + smithLambdaOmegaI) * -omegaI[2]};
  double shadowing{std::beta(1 + smithLambdaOmegaO, 1 + smithLambdaOmegaI)};
  return SpecularTerms{
    finiteOrZero(normalTerm * forwardJacobian * +cosThetaO * shadowing / omegaO[2] * eta), // Note: symmetrized by eta!
    BidirPDF{
      finiteOrZero(normalTerm * forwardJacobian * +cosThetaO / projectedAreaOmegaO),
      finiteOrZero(normalTerm * reverseJacobian * -cosThetaI / projectedAreaOmegaI)}};
}

} // namespace mi::render
