#include "Microcosm/Render/Scattering"

namespace mi::render {

Scattering &Scattering::operator*=(Spectrum weight) {
  mScatter = //
    [weight, scatter = std::move(mScatter)](auto &self, Random &random, Vector3d omegaO, Vector3d omegaI, Spectrum &f) {
      BidirPDF density{scatter(self, random, omegaO, omegaI, f)};
      DoesntAlias(f) *= weight;
      return density;
    };
  mScatterSample = //
    [weight = std::move(weight), scatterSample = std::move(mScatterSample)](
      auto &self, Random &random, Vector3d omegaO, Vector3d &omegaI, Spectrum &ratio, bool &isDelta) {
      BidirPDF density{scatterSample(self, random, omegaO, omegaI, ratio, isDelta)};
      DoesntAlias(ratio) *= weight;
      return density;
    };
  return *this;
}

Scattering &Scattering::operator*=(double weight) {
  mScatter = //
    [=, scatter = std::move(mScatter)](auto &self, Random &random, Vector3d omegaO, Vector3d omegaI, Spectrum &f) {
      BidirPDF density{scatter(self, random, omegaO, omegaI, f)};
      DoesntAlias(f) *= weight;
      return density;
    };
  mScatterSample =                                  //
    [=, scatterSample = std::move(mScatterSample)]( //
      auto &self, Random &random, Vector3d omegaO, Vector3d &omegaI, Spectrum &ratio, bool &isDelta) {
      BidirPDF density{scatterSample(self, random, omegaO, omegaI, ratio, isDelta)};
      DoesntAlias(ratio) *= weight;
      return density;
    };
  return *this;
}

BidirPDF ScatteringMixture::scatter(Random &random, Vector3d omegaO, Vector3d omegaI, Spectrum &f) const {
  BidirPDF density{};
  f = 0;
  Spectrum fTerm{spectrumZerosLike(f)};
  for (size_t i = 0; i < mTerms.size(); i++) {
    BidirPDF densityTerm{mTerms[i].scattering.scatter(random, omegaO, omegaI, fTerm)};
    DoesntAlias(f) += mTerms[i].coefficient * fTerm;
    density.forward += mTerms[i].probability * densityTerm.forward;
    density.reverse += mTerms[i].probability * densityTerm.reverse;
  }
  return density;
}

BidirPDF ScatteringMixture::scatterSample( //
  Random &random, Vector3d omegaO, Vector3d &omegaI, Spectrum &ratio, bool &isDelta) const {
  double sampleU{random.generate1()};
  for (size_t i = 0; i < mTerms.size(); i++) {
    if (i + 1 == mTerms.size() || sampleU < mTerms[i].probability) {
      Spectrum unused{spectrumLike(ratio, 1.0)}; // We're going to recalculate the total ratio below.
      void(mTerms[i].scattering.scatterSample(random, omegaO, omegaI, unused, isDelta));
      break;
    } else {
      sampleU -= mTerms[i].probability;
    }
  }
  Spectrum f{spectrumZerosLike(ratio)};
  BidirPDF density{scatter(random, omegaO, omegaI, f)};
  DoesntAlias(ratio) *= f / density.forward;
  return density;
}

} // namespace mi::render
