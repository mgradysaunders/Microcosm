#include "Microcosm/Render/More/Scattering/Microflake"

namespace mi::render {

Microflake::Microflake(const Matrix3d &matrix) : mMatrix(matrix), mMatrixInv(inverse(matrix)), mMatrixDet(determinant(matrix)) {
  if (!isPositiveAndFinite(mMatrixDet)) [[unlikely]]
    throw Error(std::logic_error("Microflake distribution matrix must have positive determinant!"));
}

Vector3d Microflake::visibleNormalSample(Vector2d sampleU, Vector3d omegaO) const noexcept {
  Matrix3d alignO = Matrix3d::orthonormalBasis(omegaO);
  Matrix3d localS = dot(transpose(alignO), mMatrix, alignO);
  double f = localS(0, 1) * localS(2, 2) - localS(0, 2) * localS(2, 1);
  double g = localS(1, 1) * localS(2, 2) - localS(1, 2) * localS(2, 1);
  Matrix3d localR = {{sqrt(mMatrixDet * localS(2, 2)), f, localS(0, 2)}, {0, g, localS(1, 2)}, {0, 0, localS(2, 2) * sqrt(g)}};
  Vector3d omegaI = {
    sqrt(sampleU[0]) * cos(TwoPi * sampleU[1]), //
    sqrt(sampleU[0]) * sin(TwoPi * sampleU[1]), //
    sqrt(1 - sampleU[0])};
  omegaI = dot(localR, omegaI);
  omegaI = dot(alignO, omegaI);
  return normalize(omegaI);
}

BidirPDF SpecularMicroflakePhase::scatter(Vector3d omegaO, Vector3d omegaI, Spectrum &f) const noexcept {
  Vector3d omegaM{reflectionHalfDirection(omegaO, omegaI)};
  double normalPDF{mMicroflake.normalPDF(omegaM)};
  double forwardPhase{finiteOrZero(normalPDF / (4 * mMicroflake.projectedArea(omegaO)))};
  double reversePhase{finiteOrZero(normalPDF / (4 * mMicroflake.projectedArea(omegaI)))};
  f = forwardPhase;
  return {forwardPhase, reversePhase};
}

BidirPDF SpecularMicroflakePhase::scatterSample( //
  Vector2d sampleU, Vector3d omegaO, Vector3d &omegaI, Spectrum &) const noexcept {
  Vector3d omegaM{mMicroflake.visibleNormalSample(sampleU, omegaO)};
  omegaI = normalize(-omegaO + 2 * dot(omegaO, omegaM) * omegaM);
  double normalPDF{mMicroflake.normalPDF(omegaM)};
  return {
    finiteOrZero(normalPDF / (4 * mMicroflake.projectedArea(omegaO))),
    finiteOrZero(normalPDF / (4 * mMicroflake.projectedArea(omegaI)))};
}

BidirPDF DiffuseMicroflakePhase::scatter(Random &random, Vector3d omegaO, Vector3d omegaI, Spectrum &f) const noexcept {
  Vector3d omegaM = mMicroflake.visibleNormalSample(random, omegaO);
  double cosThetaO = max(dot(omegaO, omegaM), 0.0);
  double cosThetaI = max(dot(omegaI, omegaM), 0.0);
  f = OneOverPi * cosThetaI;
  return {
    OneOverPi * cosThetaI, //
    OneOverPi * cosThetaO};
}

BidirPDF DiffuseMicroflakePhase::scatterSample(Random &random, Vector3d omegaO, Vector3d &omegaI, Spectrum &) const noexcept {
  Vector3d omegaM = mMicroflake.visibleNormalSample(random, omegaO);
  omegaI = cosineHemisphereSample(random);
  omegaI = normalize(dot(Matrix3d::orthonormalBasis(omegaM), omegaI));
  double cosThetaO = max(dot(omegaO, omegaM), 0.0);
  double cosThetaI = max(dot(omegaI, omegaM), 0.0);
  return {
    OneOverPi * cosThetaI, //
    OneOverPi * cosThetaO};
}

} // namespace mi::render
