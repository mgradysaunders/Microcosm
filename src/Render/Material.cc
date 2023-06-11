#include "Microcosm/Render/Material"

namespace mi::render {

void Material::clear() noexcept { *this = {}; }

BidirPDF Material::scatter(Random &random, Vector3d omegaO, Vector3d omegaI, Spectrum &f) const {
  if (!scattering) [[unlikely]]
    throwLogicErrorNoScattering("scatter");
  return scattering->scatter(random, omegaO, omegaI, f);
}

BidirPDF Material::scatterSample(Random &random, Vector3d omegaO, Vector3d &omegaI, Spectrum &ratio, bool &isDelta) const {
  if (!scattering) [[unlikely]]
    throwLogicErrorNoScattering("scatterSample");
  return scattering->scatterSample(random, omegaO, omegaI, ratio, isDelta);
}

void Material::throwLogicErrorNoScattering(const char *functionName) { throw Error(std::logic_error("Tried to call Material::{} on material without scattering functions!"_format(functionName))); }

} // namespace mi::render
