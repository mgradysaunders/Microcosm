#include "Microcosm/Render/MLT"

namespace mi::render {

void MLTRandom::nextIteration() {
  mIteration++;
  mIsLargeStep = double(mRandom) < mLargeStepProbability;
  mSampleCount = mSequenceCount = 0;
}

void MLTRandom::nextSequence() {
  mSampleCount = 0;
  mSequenceCount++;
  if (mSequences.size() < mSequenceCount) [[unlikely]] {
    mSequences.resize(mSequenceCount);
    mSequences.back().reserve(32);
  }
}

double MLTRandom::nextSample() {
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
    active.value +=
      distributions::Normal(0, mSmallStepSigma * sqrt(mIteration - active.iteration)).distributionSample(double(mRandom));
    active.value -= floor(active.value);
    active.iteration = mIteration;
  }
  return active.value;
}

void MLTRandom::finish(bool accept) {
  if (accept) {
    // If necessary, remember iteration of last large step.
    if (mIsLargeStep) mIterationOfLastLargeStep = mIteration;
  } else {
    // Rewind all of the samples that were modified this iteration.
    for (auto &sequence : mSequences)
      for (auto &sample : sequence)
        if (sample.active.iteration == mIteration) sample.load();
    // Rewind the iteration.
    --mIteration;
  }
}

} // namespace mi::render
