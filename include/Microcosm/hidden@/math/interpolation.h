/*-*- C++ -*-*/
#pragma once

namespace mi {

/// Linear interpolation.
[[nodiscard, strong_inline]] constexpr auto lerp(auto t, auto &&valueA, auto &&valueB) noexcept { return (1 - t) * auto_forward(valueA) + t * auto_forward(valueB); }

/// Linear interpolation. If called with only 2 arguments, then build a lambda.
[[nodiscard, strong_inline]] constexpr auto lerp(auto &&valueA, auto &&valueB) noexcept {
  return [valueA = auto_forward(valueA), //
          valueB = auto_forward(valueB)](auto t) constexpr { return lerp(t, valueA, valueB); };
}

/// Linear interpolation undo-er.
[[nodiscard, strong_inline]] constexpr auto unlerp( //
  std::floating_point auto value, std::floating_point auto valueA, std::floating_point auto valueB) noexcept {
  return valueA == valueB ? 0 : (value - valueA) / (valueB - valueA);
}

/// Linearly space fractions from zero (inclusive) to one (exclusive).
template <std::floating_point Float = double> [[nodiscard, strong_inline]] constexpr auto linspace(int count) noexcept {
  return std::views::iota(0, count) | std::views::transform([=, factor = Float(1) / Float(count)](int i) constexpr noexcept { return factor * i; });
}

template <typename Value> struct Exclusive {
  constexpr Exclusive() noexcept = default;
  constexpr Exclusive(Value value) noexcept : value(value) {}
  [[nodiscard]] constexpr explicit operator Value() const noexcept { return value; }
  Value value;
};

template <typename Value> struct to_float<Exclusive<Value>> : using_type<to_float_t<Value>> {};

template <typename ValueA, typename ValueB> requires((concepts::arithmetic<ValueA> || concepts::match<ValueA, Exclusive>) && (concepts::arithmetic<ValueB> || concepts::match<ValueB, Exclusive>))
[[nodiscard, strong_inline]] constexpr auto linspace(int count, ValueA valueA, ValueB valueB) noexcept {
  using FloatA = to_float_t<ValueA>;
  using FloatB = to_float_t<ValueB>;
  using Float = std::common_type_t<FloatA, FloatB>;
  constexpr int includeA = concepts::match<ValueA, Exclusive> ? 0 : 1;
  constexpr int excludeA = concepts::match<ValueA, Exclusive> ? 1 : 0;
  constexpr int excludeB = concepts::match<ValueB, Exclusive> ? 1 : 0;
  return std::views::iota(excludeA, excludeA + count) | std::views::transform([=, factor = Float(1) / Float(count - includeA + excludeB)](int i) constexpr noexcept { //
           return lerp(factor * i, Float(valueA), Float(valueB));
         });
}

/// Hermite interpolation.
[[nodiscard]] constexpr auto hermite(auto t, auto &&valueA, auto &&slopeA, auto &&slopeB, auto &&valueB) noexcept {
  auto u = 1 - t;
  auto u2 = u * u;
  auto t2 = t * t;
  return ((u2 * (1 + 2 * t)) * auto_forward(valueA) + (u2 * t) * auto_forward(slopeA)) + //
         ((t2 * (1 + 2 * u)) * auto_forward(valueB) - (t2 * u) * auto_forward(slopeB));
}

/// Catmull-Rom interpolation.
[[nodiscard]] constexpr auto catmullRom(auto t, auto &&valueP, auto &&valueA, auto &&valueB, auto &&valueN) noexcept {
  return hermite(
    t, valueA,                           //
    (valueB - auto_forward(valueP)) / 2, //
    (auto_forward(valueN) - valueA) / 2, valueB);
}

namespace ease {

[[nodiscard]] constexpr auto Identity() noexcept {
  return [](auto t) constexpr noexcept { return t; };
}

[[nodiscard]] constexpr auto Clamp() noexcept {
  return [](auto t) constexpr noexcept { return clamp(t, 0, 1); };
}

[[nodiscard]] constexpr auto Repeat() noexcept {
  return [](auto t) constexpr noexcept { return t - fastFloor(t); };
}

[[nodiscard]] constexpr auto Mirror() noexcept {
  return [](auto t) constexpr noexcept {
    int i = fastFloor(t);
    t = t - i;
    return (i & 1) ? 1 - t : t;
  };
}

[[nodiscard]] constexpr auto Lerp(auto easingA, auto easingB) noexcept {
  return [=](auto t) constexpr noexcept { return (1 - t) * easingA(t) + t * easingB(t); };
}

[[nodiscard]] constexpr auto Feed(auto easingA, auto easingB) noexcept {
  return [=](auto t) constexpr noexcept { return easingA(easingB(t)); };
}

[[nodiscard]] constexpr auto StartToStop(auto easing) noexcept {
  return [=](auto t) constexpr noexcept { return 1 - easing(1 - t); };
}

[[nodiscard]] constexpr auto SmoothStart(auto power) noexcept {
  return [=](auto t) constexpr noexcept {
    if constexpr (std::integral<decltype(power)>) {
      return nthPow(t, power);
    } else {
      return pow(t, power);
    }
  };
}

[[nodiscard]] constexpr auto SmoothStop(auto power) noexcept { return StartToStop(SmoothStart(power)); }

[[nodiscard]] constexpr auto Smooth(auto power) noexcept { return Lerp(SmoothStart(power), SmoothStop(power)); }

[[nodiscard]] constexpr auto Smooth(auto powerA, auto powerB) noexcept { return Lerp(SmoothStart(powerA), SmoothStop(powerB)); }

[[nodiscard]] constexpr auto ExpSmoothStart(auto power) noexcept {
  return [=](auto t) noexcept { return exp((1 - 1 / max(t, 0)) / power); };
}

[[nodiscard]] constexpr auto ExpSmoothStop(auto power) noexcept { return StartToStop(ExpSmoothStart(power)); }

[[nodiscard]] constexpr auto ExpSmooth(auto power) noexcept { return Lerp(ExpSmoothStart(power), ExpSmoothStop(power)); }

[[nodiscard]] constexpr auto ExpSmooth(auto powerA, auto powerB) noexcept { return Lerp(ExprSmoothStart(powerA), ExpSmoothStop(powerB)); }

[[nodiscard]] constexpr auto TrigSmoothStart() noexcept {
  return [](auto t) noexcept { return 1 - cosPi(t / 2); };
}

[[nodiscard]] constexpr auto TrigSmoothStop() noexcept { return StartToStop(TrigSmoothStart()); }

[[nodiscard]] constexpr auto TrigSmooth() noexcept {
  return [](auto t) noexcept { return (1 - cosPi(t)) / 2; };
}

[[nodiscard]] constexpr auto ThereAndBack(auto tArrive, auto tDepart) noexcept {
  return [=](auto t) constexpr noexcept -> decltype(t) {
    if (t < 0) return 0;
    if (t > 1) return 0;
    if (t < tArrive) return t / tArrive;
    if (t < tDepart) return 1;
    return 1 - (t - tDepart) / (1 - tDepart);
  };
}

[[nodiscard]] constexpr auto ThereAndBack(auto tPause) noexcept { return ThereAndBack(0.5 - tPause / 2, 0.5 + tPause / 2); }

#if 0
[[nodiscard]] inline auto mish(auto x) noexcept { return x * tanh(log1p(exp(x))); }

struct MishOvershoot {
  template <std::floating_point Float> [[nodiscard]] Float operator()(Float t) const noexcept {
    t = saturate(t);
    if (t == 0) return 0;
    if (t == 1) return 1;
    constexpr Float mish1 = Float(0.865098388267); // mish(1) = tanh(log(1 + e))
    return lerp(expSmooth(t), expSmoothStart(Float(1.5) * t), lerp(t, t, 1 - mish(-Float(2.125) * t / (1 - t * t)) / mish1));
  }
};
#endif

} // namespace ease

template <std::floating_point Float, typename Value = Float> struct Springy {
public:
  constexpr Springy() noexcept = default;

  constexpr Springy(Value value, Value speed = {}) noexcept : mValue(value), mSpeed(speed), mTargetValue(value) {}

  [[nodiscard]] constexpr Value value() const noexcept { return mValue; }

  [[nodiscard]] constexpr Value speed() const noexcept { return mSpeed; }

  constexpr void setValue(Value newValue, Value newSpeed = {}) noexcept {
    mValue = newValue;
    mSpeed = newSpeed;
    mTargetValue = newValue;
  }

  /// Set the frequency and damping.
  ///
  /// The frequency is given in cycles per second, and is usually described as "the natural
  /// undamped frequency of the oscillator". The damping coefficient is a unitless ratio that
  /// determines where the behavior falls between oscillation and exponential decay.
  ///
  /// - If damping = 0, the spring oscillates forever and never loses any energy. This is unadvisable since this is a
  ///   discrete simulation. The behavior will probably diminish or explode sooner or later. (Haven't tested this extensively
  ///   since it is not under the umbrella of intended use cases here.)
  /// - If damping < 1, the spring is "underdamped". It oscillates, but loses amplitude and gradually returns to rest.
  /// - If damping = 1, the spring is "critically damped" and returns to rest as quickly as possible without oscillating.
  /// - If damping > 1, the spring is "overdamped". It smoothly pursues the target without oscillating. The larger the
  ///   damping, the longer it takes for the spring to reach the target.
  ///
  /// \note
  /// Even if damping is greater than or equal to 1, the spring still overshoots the target if the response coefficient
  /// is also greater than 1.
  ///
  /// \see setResponse()
  ///
  constexpr void setFrequencyAndDamping(Float frequency, Float damping) noexcept {
    mCoeffK1 = damping / (constants::Pi<Float> * frequency);
    mCoeffK2 = 1 / sqr(constants::TwoPi<Float> * frequency);
  }

  /// Set the frequency per half-life of an underdamped system.
  ///
  /// This is an alternative for `setFrequencyAndDamping()` that accepts the local frequency
  /// (number of oscillations or cycles) per half-life of the exponential decay of an underdamped
  /// spring system.
  ///
  constexpr void setUnderdampedFrequencyPerHalfLife(Float frequency, Float halfLife) noexcept {
    Float damping = (constants::LnTwo<Float> / halfLife) / //
                    (constants::TwoPi<Float> * frequency);
    damping /= sqrt(1 + sqr(damping));
    setFrequencyAndDamping(frequency / sqrt(1 - sqr(damping)), damping);
  }

  /// Set the response coefficient.
  ///
  /// To understand this value, it is useful to describe how it changes the behavior
  /// in the case of responding to the unit step function. So, the spring is initially
  /// at rest at the value of zero. Then the target value instantly jumps from zero to
  /// one. Then:
  /// - If response = 0, the spring accelerates continously from rest.
  /// - If response > 0, the spring reacts instantly, such that its velocity changes discontinously.
  /// - If response > 1, the spring reacts instantly, and will always overshoot the target value.
  /// - If response < 0, the spring reacts instantly, and effectively anticipates the motion by first
  ///   traveling in the opposite direction.
  ///
  constexpr void setResponse(Float response) noexcept { mCoeffR = response / 2; }

  constexpr void update(Float deltaTime, const Value &targetValue, const Value &targetSpeed) noexcept {
    if (deltaTime > 0) [[likely]] {
      mValue += deltaTime * mSpeed;
      mSpeed += deltaTime * (targetValue - mValue + mCoeffK1 * (mCoeffR * targetSpeed - mSpeed)) / max(mCoeffK2, Float(1.1) * deltaTime * (Float(0.25) * deltaTime + Float(0.5)));
    }
    mTargetValue = targetValue;
  }

  constexpr void update(Float deltaTime, const Value &targetValue) noexcept { update(deltaTime, targetValue, (targetValue - mTargetValue) / deltaTime); }

  [[nodiscard]] constexpr operator Value() const noexcept { return mValue; }

private:
  Float mCoeffK1{0};
  Float mCoeffK2{0};
  Float mCoeffR{0};
  Value mValue{};       ///< The current value.
  Value mSpeed{};       ///< The current speed.
  Value mTargetValue{}; ///< The last target value, needed to compute target speed incrementally.
};

} // namespace mi
