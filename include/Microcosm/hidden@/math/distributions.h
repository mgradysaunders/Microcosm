#pragma once

namespace mi::distributions {

/// A uniform real probability distribution.
struct Uniform {
public:
  constexpr Uniform() noexcept = default;

  constexpr Uniform(double valueA, double valueB) noexcept : mValueA(valueA), mValueB(valueB) {
    if (mValueB < mValueA) std::swap(mValueA, mValueB);
  }

  [[nodiscard]] double distributionPDF(double value) const noexcept {
    return mValueA <= value && value < mValueB ? 1 / (mValueB - mValueA) : 0;
  }

  [[nodiscard]] double distributionCDF(double value) const noexcept { return saturate(unlerp(value, mValueA, mValueB)); }

  [[nodiscard]] double distributionSample(double sampleU) const noexcept { return lerp(saturate(sampleU), mValueA, mValueB); }

  [[nodiscard]] double operator()(auto &gen) const noexcept { return distributionSample(randomize<double>(gen)); }

private:
  double mValueA = 0;

  double mValueB = 1;
};

template <auto DistributionPDF, auto DistributionCDF, auto DistributionSample> struct WithMeanAndWidth {
public:
  constexpr WithMeanAndWidth() noexcept = default;

  constexpr WithMeanAndWidth(double mean, double width) noexcept : mMean(mean), mWidth(width) {}

  [[nodiscard]] double distributionPDF(double value) const noexcept {
    return finiteOr(DistributionPDF((value - mMean) / mWidth) / mWidth, 0.0);
  }

  [[nodiscard]] double distributionCDF(double value) const noexcept {
    return finiteOr(DistributionCDF((value - mMean) / mWidth), value > mMean ? 1.0 : 0.0);
  }

  [[nodiscard]] double distributionSample(double sampleU) const noexcept {
    return mMean + mWidth * DistributionSample(saturate(sampleU));
  }

  [[nodiscard]] double operator()(auto &gen) const noexcept { return distributionSample(randomize<double>(gen)); }

private:
  double mMean{0};

  double mWidth{1};
};

using Normal = WithMeanAndWidth<
  [](double value) -> double { //
    return constants::OneOverSqrtTwoPi<double> * exp(-0.5 * sqr(value));
  },
  [](double value) -> double { //
    return 0.5 * erf(constants::OneOverSqrtTwo<double> * value) + 0.5;
  },
  [](double sampleU) -> double { //
    return constants::SqrtTwo<double> * erfInverse(2 * sampleU - 1);
  }>;

using Cauchy = WithMeanAndWidth<
  [](double value) -> double { //
    return constants::OneOverPi<double> / (1 + sqr(value));
  },
  [](double value) -> double { //
    return constants::OneOverPi<double> * atan(value) + 0.5;
  },
  [](double sampleU) -> double { //
    return tan(constants::Pi<double> * (sampleU - 0.5));
  }>;

using Logistic = WithMeanAndWidth<
  [](double value) -> double { //
    return 0.25 / sqr(cosh(0.5 * value));
  },
  [](double value) -> double { //
    return 0.5 * tanh(0.5 * value) + 0.5;
  },
  [](double sampleU) -> double { //
    return log(sampleU / (1 - sampleU));
  }>;

using HyperbolicSecant = WithMeanAndWidth<
  [](double value) -> double { //
    return 0.5 / cosh(constants::PiOverTwo<double> * value);
  },
  [](double value) -> double { //
    return atan(exp(constants::PiOverTwo<double> * value)) / constants::PiOverTwo<double>;
  },
  [](double sampleU) -> double { //
    return log(tan(constants::PiOverTwo<double> * sampleU)) / constants::PiOverTwo<double>;
  }>;

/// An exponential probability distribution.
struct Exponential {
public:
  constexpr Exponential() noexcept = default;

  constexpr Exponential(double lambda) noexcept : mLambda(lambda) {}

  [[nodiscard]] double distributionPDF(double value) const noexcept {
    return signbit(value) ? 0 : mLambda * exp(-mLambda * min(value, constants::Max<double>));
  }

  [[nodiscard]] double distributionCDF(double value) const noexcept {
    return signbit(value) ? 0 : 1 - exp(-mLambda * min(value, constants::Max<double>));
  }

  [[nodiscard]] double distributionSample(double sampleU) const noexcept { return finiteOrZero(-log1p(-sampleU) / mLambda); }

  [[nodiscard]] double operator()(auto &gen) const noexcept { return distributionSample(randomize<double>(gen)); }

private:
  double mLambda = 1;
};

/// A discrete probability distribution.
struct Discrete {
public:
  Discrete() noexcept = default;

  Discrete(std::vector<double> weights) noexcept : mCMF(std::move(weights)) {
    for (int i = 1; i < size(); i++) mCMF[i] += mCMF[i - 1];
    for (int i = 0; i < size(); i++) mCMF[i] /= mCMF.back();
  }

  [[nodiscard]] int size() const noexcept { return int(mCMF.size()); }

  [[nodiscard]] bool isInRange(int i) const noexcept { return 0 <= i && i < size(); }

  /// The Probability Mass Function (PMF). This is the discrete probability of sampling the given integer.
  [[nodiscard]] double distributionPMF(int i) const noexcept { return isInRange(i) ? mCMF[i] - (i > 0 ? mCMF[i - 1] : 0) : 0; }

  /// The Cumulative Mass Function (CMF). This is the discrete probability of sampling less than or equal to the given integer.
  [[nodiscard]] double distributionCMF(int i) const noexcept { return isInRange(i) ? mCMF[i] : (i < 0 ? 0 : 1); }

  [[nodiscard]] int distributionSample(double sampleU) const noexcept {
    int i = std::distance(mCMF.begin(), std::lower_bound(mCMF.begin(), mCMF.end(), sampleU));
    if (i > size() - 1) i = size() - 1;
    return i;
  }

  [[nodiscard]] int operator()(auto &gen) const noexcept { return distributionSample(randomize<double>(gen)); }

  void onSerialize(auto &serializer) { serializer <=> mCMF; }

private:
  /// The cumulative probabilities.
  std::vector<double> mCMF;
};

} // namespace mi::distributions
