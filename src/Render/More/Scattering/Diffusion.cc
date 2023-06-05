#include "Microcosm/Render/More/Scattering/Diffusion"

namespace mi::render {

[[nodiscard]] static double DisneyDiffusionDistancePDF(double d) noexcept { return d < 0 ? 0 : (exp(-d) + exp(-d / 3)) / 4; }

[[nodiscard]] static double DisneyDiffusionDistanceCDF(double d) noexcept {
  return d <= 0 ? 0 : 1 - (exp(-d) + 3 * exp(-d / 3)) / 4;
}

Spectrum DisneyDiffusion::profile(double d) const noexcept {
  Spectrum value{mRadius.shape};
  for (size_t i = 0; i < value.size(); i++)
    value[i] = finiteOrZero(DisneyDiffusionDistancePDF(d / mRadius[i]) / (TwoPi * mRadius[i]));
  return value;
}

double DisneyDiffusion::distancePDF(double d) const noexcept {
  return DisneyDiffusionDistancePDF(d / mRadiusForPDF) / mRadiusForPDF;
}

double DisneyDiffusion::distanceSample(double sampleU) const noexcept {
  if (double d = max(20 * sampleU, 0.0001);
      solveNewton(d, 0.0001, 20, sampleU, 1e-4, DisneyDiffusionDistanceCDF, DisneyDiffusionDistancePDF))
    return mRadiusForPDF * d;
  else
    return 0;
}

DisneyDiffusion DisneyDiffusion::fromAlbedoVMFP(const Spectrum &albedo, Spectrum radius) noexcept {
  for (size_t i = 0; i < radius.size(); i++) {
    radius[i] /= (7 * nthPow(abs(albedo[i] - 0.8), 3) + 1.85 - albedo[i]);
    radius[i] = max(radius[i], 0);
  }
  return {std::move(radius)};
}

DisneyDiffusion DisneyDiffusion::fromAlbedoSMFP(const Spectrum &albedo, Spectrum radius) noexcept {
  for (size_t i = 0; i < radius.size(); i++) {
    radius[i] /= (100 * nthPow(albedo[i] - 0.33, 4) + 3.5);
    radius[i] = max(radius[i], 0);
  }
  return {std::move(radius)};
}

} // namespace mi::render
