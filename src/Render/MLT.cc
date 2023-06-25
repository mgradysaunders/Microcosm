#include "Microcosm/Render/MLT"

namespace mi::render {

double PSMLTRandom::nextSample() {
  auto &sequence{mSequences[mSequenceCount - 1]};
  mSampleCount++;
  if (sequence.size() < mSampleCount) sequence.resize(mSampleCount);
  auto &sample{sequence[mSampleCount - 1]};
  auto &active{sample.active};
  if (active.iteration < mIterationOfLastLargeStep) {
    active.iteration = mIterationOfLastLargeStep;
    active.value = double(mRandom);
  }
  sample.save();
  if (mIsLargeStep) {
    active.value = double(mRandom);
    active.iteration = mIteration;
  } else {
    active.value += distributions::Normal(0, mSmallStepSigma * sqrt(mIteration - active.iteration)).distributionSample(double(mRandom));
    active.value -= floor(active.value);
    active.iteration = mIteration;
  }
  return active.value;
}

void PSMLTRandom::finishAndAccept() {
  // If necessary, remember iteration of last large step.
  if (mIsLargeStep) mIterationOfLastLargeStep = mIteration;
}

void PSMLTRandom::finishAndReject() {
  // Rewind all of the samples that were modified this iteration.
  for (auto &sequence : mSequences)
    for (auto &sample : sequence)
      if (sample.active.iteration == mIteration) sample.load();
  // Rewind the iteration.
  --mIteration;
}

void PSMLTIntegrator::operator()(const RandomSampler &randomSampler, const Recorder &recorder) const {
  const bool printProgress{mOptions.printProgress};
  const size_t seed{mOptions.seed};
  const size_t minBounces{mOptions.minBounces};
  const size_t maxBounces{mOptions.maxBounces};
  const size_t numBootstrapPaths{mOptions.numBootstrapPaths};
  const size_t numBootstrapBounces{maxBounces - minBounces + 1};
  const size_t numMutations{mOptions.numMutations};
  const size_t numChains{mOptions.numChains};

  auto doRandomSample = [&](Random &random, size_t numBounces) -> std::optional<Contribution> {
    random.as<PSMLTRandom>().nextSequence();
    size_t depth{numBounces + 2};
    size_t depthFromCamera{static_cast<size_t>(random.generateIndex(depth + 1))};
    size_t depthFromLight{depth - depthFromCamera};
    std::swap(depthFromCamera, depthFromLight);
    if (auto contribution{randomSampler(random, depthFromCamera, depthFromLight)}; //
        contribution && contribution->subpathFromCamera.size() == depthFromCamera && contribution->subpathFromLight.size() == depthFromLight) {
      // Account for the probability of the specific depth combination we asked for.
      contribution->pathL *= depth + 1;
      contribution->pathI *= depth + 1;
      return contribution;
    }
    return std::nullopt;
  };

  std::vector<double> bootstrapValues(numBootstrapPaths * ((maxBounces - minBounces) + 1), 0.0);
#pragma omp parallel for schedule(dynamic)
  for (size_t pathIndex = 0; pathIndex < numBootstrapPaths; pathIndex++) {
    for (size_t bounceIndex = minBounces; bounceIndex <= maxBounces; bounceIndex++) {
      size_t bootstrapIndex{pathIndex * numBootstrapBounces + bounceIndex};
      Random random{PSMLTRandom{ExtendedPcg32<>{std::seed_seq{seed, 0xA7CBE565UL, 0x6AF93C73UL, bootstrapIndex, 0xE5C6FB2CUL, 0x24718FB5UL}}, mOptions.smallStepSigma, mOptions.largeStepProbability}};
      if (std::optional<Contribution> contribution{doRandomSample(random, bounceIndex)}) bootstrapValues[bootstrapIndex] = contribution->pathI;
    }
  }
  double overallValue{0};
  for (double value : bootstrapValues) overallValue += value;
  overallValue /= numBootstrapPaths;
  distributions::Discrete bootstrap(std::move(bootstrapValues));

  Progress progress{"Rendering", numMutations};
#pragma omp parallel for schedule(dynamic)
  for (size_t chainIndex = 0; chainIndex < numChains; chainIndex++) {
    ExtendedPcg32<> otherRandom{std::seed_seq{chainIndex, 0x3D6411FFUL, 0xDE44B7D2UL, seed, 0xE9F523E9UL, 0xD64CFEEEUL}};
    const size_t bootstrapIndex{static_cast<size_t>(bootstrap(otherRandom))};
    const size_t numBounces{bootstrapIndex % numBootstrapBounces + minBounces};
    const size_t numMutationsStep0{((chainIndex + 0) * numMutations) / numChains};
    const size_t numMutationsStep1{((chainIndex + 1) * numMutations) / numChains};
    const size_t numChainMutations{min(numMutationsStep1, numMutations) - numMutationsStep0};
    Random random{PSMLTRandom{ExtendedPcg32<>{std::seed_seq{seed, 0xA7CBE565UL, 0x6AF93C73UL, bootstrapIndex, 0xE5C6FB2CUL, 0x24718FB5UL}}, mOptions.smallStepSigma, mOptions.largeStepProbability}};
    std::optional<Contribution> CCurr{doRandomSample(random, numBounces)};
    std::optional<Contribution> CNext;
    if (!CCurr) [[unlikely]] {
      throw Error(std::logic_error("Bootstrap contribution should have been non-null!"));
    }
    for (size_t mutationIndex = 0; mutationIndex < numChainMutations; mutationIndex++) {
      random.as<PSMLTRandom>().nextIteration();
      double accept{0};
      if (CNext = doRandomSample(random, numBounces)) accept = fmin(1.0, CNext->pathI / CCurr->pathI);
      if (accept > 0) recorder(*CNext, overallValue / CNext->pathI * accept);
      if (accept < 1) recorder(*CCurr, overallValue / CCurr->pathI * (1 - accept));
      if (randomize<double>(otherRandom) < accept) {
        CCurr = std::move(CNext);
        random.as<PSMLTRandom>().finishAndAccept();
      } else {
        random.as<PSMLTRandom>().finishAndReject();
      }
      progress.increment();
    }
  }
}

} // namespace mi::render
