#include "Microcosm/Render/More/Scattering/Media"
#include "Microcosm/Render/More/Scattering/Phase"

namespace mi::render {

void HomogeneousMedium::transmission(Random &, Ray3d ray, Spectrum &tr) const {
  DoesntAlias(tr) *= exp(-mSigmaT * min(ray.segmentLength(), constants::Max<double>));
}

std::optional<VolumeScattering> HomogeneousMedium::transmissionSample(Random &random, Ray3d ray, Spectrum &ratio) const {
  // Normalize the ray to guarantee the length of the direction vector is one and the minimum
  // parameter is zero.
  ray = normalize(ray);

  // 1. Randomly sample scattering coefficient sigmaS.
  // 2. Randomly sample scattering distance according an exponential distribution with sigmaS as the rate parameter.
  double sigmaS{mSigmaS[random.generateIndex(mSigmaS.size())]};
  double hitDistance{-log1p(-random.generate1()) / sigmaS};
  double maxDistance{ray.maxParam};

  // If the scattering distance happens to be within range, the volume scatters the ray before the surface
  // it would otherwise hit.
  if (hitDistance < maxDistance) {
    // Keep in mind that we sampled sigmaS uniformly randomly from the spectrum of scattering coefficients. Therefore
    // the density is the mean of the different exponential scattering densities we could have formed this way. And the
    // exponential Probability Density Function (PDF) is P(d) = λ exp(-λ d) where λ is the rate parameter.
    double density{stats::mean(mSigmaS * exp(-mSigmaS * hitDistance))};
    if (!isPositiveAndFinite(density)) [[unlikely]] {
      ratio = 0;
      return {};
    }

    // Scale ratio by the usual Monte Carlo ratio: The function evaluation divided by the probability density
    // associated with the function evaluation. The function here is the path transmission, which we compute directly
    // as exp(-mSigmaT * hitDistance) instead of calling transmission().
    DoesntAlias(ratio) *= exp(-mSigmaT * hitDistance) / density;

    // We also have to account for something we do not ordinarily account for in the surface scattering case. Light
    // bouncing around in a volume essentially entails evaluating exponential decays of spatially-integrated functions (our
    // absorption and scattering coefficients). Differentially, this means that the scattering coefficient ends up in front
    // of the scattering integral.
    DoesntAlias(ratio) *= mSigmaS;

    VolumeScattering volumeScattering;
    volumeScattering.position = ray(hitDistance);
    if (mScatteringProvider) volumeScattering.scattering = mScatteringProvider(volumeScattering.position);
    return volumeScattering;
  } else {
    // If the scattering distance is not in range, then we hit the surface instead. The probability of hitting the surface
    // is discrete (so not really a density), and equal to one minus the probability of scattering in range. The exponential
    // Cumulative Distribution Function (CDF) is what we want, which is C(d) = 1 - exp(-λd). But again we really want one minus
    // the probability of scattering before the maximum distance, so the calculation is just exp(-mSigmaS * maxDistance).
    DoesntAlias(ratio) *= exp(-mSigmaT * maxDistance) * finiteOrZero(1 / stats::mean(exp(-mSigmaS * maxDistance)));
    return {};
  }
}

void HeterogeneousDeltaTrackingMedium::transmission(Random &random, Ray3d ray, Spectrum &tr) const {
  if (auto params = mBoundBox.rayCast(ray)) {
    // Restrict ray parameter range, normalize the ray, and declare distance tracking variables.
    ray.minParam = max(ray.minParam, params->first);
    ray.maxParam = min(ray.maxParam, params->second);
    ray = normalize(ray);
    double hitDistance{0};
    double maxDistance{ray.maxParam};

    // Allocate spectra for the volume coefficients.
    Spectrum sigmaS{spectrumZerosLike(tr)};
    Spectrum sigmaA{spectrumZerosLike(tr)};
    double invMaxSigmaT{1 / mMaxSigmaT};

    // Calculate transmission with ratio tracking. Essentially what we're doing is randomly sampling events
    // according to our majorant extinction, then accumulating the probability of null-scattering at each
    // event.
    while (true) {
      if (hitDistance += -log1p(-random.generate1()) * invMaxSigmaT; hitDistance < maxDistance) {
        mSigmaProvider(ray(hitDistance), -ray.direction, sigmaS, sigmaA);
        DoesntAlias(tr) *= 1 - (sigmaS + sigmaA) * invMaxSigmaT;
      } else {
        break;
      }
    }
  }
}

std::optional<VolumeScattering> HeterogeneousDeltaTrackingMedium::transmissionSample( //
  Random &random, Ray3d ray, Spectrum &ratio) const {                                 //
  if (auto params = mBoundBox.rayCast(ray)) {
    // Restrict ray parameter range, normalize the ray, and declare distance tracking variables.
    ray.minParam = max(ray.minParam, params->first);
    ray.maxParam = min(ray.maxParam, params->second);
    ray = normalize(ray);
    double hitDistance{0};
    double maxDistance{ray.maxParam};

    // Allocate spectra for the volume coefficients.
    Spectrum sigmaS{spectrumZerosLike(ratio)};
    Spectrum sigmaA{spectrumZerosLike(ratio)};
    double invMaxSigmaT{1 / mMaxSigmaT};

    // Calculate transmission sample with delta tracking. Note that the calculation here is fully
    // spectral (unlike, e.g., PBRT which assumes total extinction is wavelength independent), so
    // we do not see as much term cancellation as other implementations. Moreover, the code is
    // intentionally left unsimplified because reducing the terms makes it way less obvious what
    // is actually happening.
    while (true) {
      if (hitDistance += -log1p(-random.generate1()) * invMaxSigmaT; hitDistance < maxDistance) {
        mSigmaProvider(ray(hitDistance), -ray.direction, sigmaS, sigmaA);
        Spectrum sigmaN = mMaxSigmaT - (sigmaS + sigmaA); // Null scattering coefficient.
        Spectrum probN = sigmaN * invMaxSigmaT;           // Null scattering probability.

        if (auto i{random.generateIndex(probN.size())}; random.generate1() < 1 - probN[i]) {
          // We intersected in the medium, so update the ratio accordingly. Note that there are some "invisible"
          // or implicitly cancelled terms in the right hand side. We're really multiplying by the transmission over
          // the probability of sampling the distance, and further dividing out the probability of scattering versus
          // null-scattering.
          DoesntAlias(ratio) *= sigmaS * invMaxSigmaT / stats::mean(1 - probN);

          VolumeScattering volumeScattering;
          volumeScattering.position = ray(hitDistance);
          if (mScatteringProvider) volumeScattering.scattering = mScatteringProvider(volumeScattering.position);
          return volumeScattering;
        } else {
          // It is important to remark that null scattering is still "scattering" as far as the math
          // is concerned, so we have to update the ratio in the same way as the scattering case,
          // except with sigmaN instead of sigmaS. However, in non-spectral implementations like PBRT,
          // the numerator and denominator work out to be equivalent, so they simply ignore null-scattering
          // ratio updates.
          DoesntAlias(ratio) *= sigmaN * invMaxSigmaT / stats::mean(probN);
        }
      } else {
        break;
      }
    }
  }
  return {};
}

} // namespace mi::render
