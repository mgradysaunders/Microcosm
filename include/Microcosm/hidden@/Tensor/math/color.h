#pragma once

namespace mi {

namespace concepts {

template <typename Value>
concept tensor_rgb_or_rgba = (tensor_with_shape<Value, 3> || tensor_with_shape<Value, 4>)&&arithmetic<value_type_t<Value>>;

} // namespace concepts

/// Encode linear RGB as sRGB.
template <concepts::arithmetic Arith, std::floating_point Float = to_float_t<Arith>> //
[[nodiscard]] inline Arith encodeSRGB(Arith value) noexcept {
  if constexpr (std::floating_point<Arith>) {
    if (value <= 0) return 0;
    if (value >= 1) return 1;
    return value <= Float(3.1308e-3) ? Float(12.92) * value : Float(1.055) * std::pow(value, 1 / Float(2.4)) - Float(0.055);
  } else {
    return Arith(encodeSRGB(saturate(value / 255.0f)) * 255.0f);
  }
}

template <typename Value> requires(concepts::tensor_rgb_or_rgba<Value>) [[nodiscard]] inline auto encodeSRGB(Value &&value) {
  auto result = value.execute();
  result[0] = encodeSRGB(result[0]);
  result[1] = encodeSRGB(result[1]);
  result[2] = encodeSRGB(result[2]);
  return result;
}

/// Decode linear RGB from sRGB.
template <concepts::arithmetic Arith, std::floating_point Float = to_float_t<Arith>> //
[[nodiscard]] inline Float decodeSRGB(Arith value) noexcept {
  if constexpr (std::floating_point<Arith>) {
    if (value <= 0) return 0;
    if (value >= 1) return 1;
    return value <= Float(4.0450e-2) ? value / Float(12.92) : std::pow((value + Float(0.055)) / Float(1.055), Float(2.4));
  } else {
    return Arith(decodeSRGB(saturate(value / 255.0f)) * 255.0f);
  }
}

template <typename Value> requires(concepts::tensor_rgb_or_rgba<Value>) [[nodiscard]] inline auto decodeSRGB(Value &&value) {
  auto result = value.execute();
  result[0] = decodeSRGB(result[0]);
  result[1] = decodeSRGB(result[1]);
  result[2] = decodeSRGB(result[2]);
  return result;
}

/// XYZ triple to RGB triple.
///
/// \note
/// This uses standard CIE parameters:
/// - \f$ C_r = (0.7350, 0.2650) \f$,
/// - \f$ C_g = (0.2740, 0.7170) \f$,
/// - \f$ C_b = (0.1670, 0.0090) \f$, and
/// - \f$ W = (1, 1, 1) \f$.
///
/// \see
/// [Bruce Lindbloom's page][1].
/// [1]: http://brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
///
template <std::floating_point Float = double>
[[nodiscard, strong_inline]] constexpr auto convertXYZToRGB(const Vector3<Float> &value) noexcept {
  const Matrix3<Float> matrix = {
    {Float(+3.2404500), Float(-1.537140), Float(-0.498532)}, //
    {Float(-0.9692660), Float(+1.876010), Float(+0.041556)}, //
    {Float(+0.0556434), Float(-0.204026), Float(+1.057230)}};
  return dot(matrix, value);
}

/// RGB triple to XYZ triple.
///
/// \note
/// This uses standard CIE parameters:
/// - \f$ C_r = (0.7350, 0.2650) \f$,
/// - \f$ C_g = (0.2740, 0.7170) \f$,
/// - \f$ C_b = (0.1670, 0.0090) \f$, and
/// - \f$ W = (1, 1, 1) \f$.
///
/// \see
/// [Bruce Lindbloom's page][1].
/// [1]: http://brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
///
template <std::floating_point Float = double>
[[nodiscard, strong_inline]] constexpr auto convertRGBToXYZ(const Vector3<Float> &value) noexcept {
  const Matrix3<Float> matrix = {
    {Float(0.412456), Float(0.357576), Float(0.180438)}, //
    {Float(0.212673), Float(0.715152), Float(0.072175)}, //
    {Float(0.019334), Float(0.119192), Float(0.950304)}};
  return dot(matrix, value);
}

template <std::floating_point Float = double>
[[nodiscard, strong_inline]] constexpr auto convertRGBToLuminance(const Vector3<Float> &value) noexcept {
  return dot(Vector3<Float>{Float(0.212673), Float(0.715152), Float(0.072175)}, value);
}

/// RGB to XYZ conversion matrix.
///
/// \param[in] vectorCr  Chromaticity of reference red.
/// \param[in] vectorCg  Chromaticity of reference green.
/// \param[in] vectorCb  Chromaticity of reference blue.
/// \param[in] vectorW   XYZ reference white.
///
/// \see
/// [Bruce Lindbloom's page][1].
/// [1]: http://brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
///
template <std::floating_point Float = float>
[[nodiscard]] inline Matrix3<Float> convertRGBToXYZ(
  const Vector2<Float> &vectorCr, const Vector2<Float> &vectorCg, const Vector2<Float> &vectorCb,
  const Vector3<Float> &vectorW) noexcept {
  Matrix3<Float> matrixA{
    from_cols,                                                                        //
    Vector3<Float>{vectorCr[0] / vectorCr[1], 1, (1 - vectorCr.sum()) / vectorCr[1]}, //
    Vector3<Float>{vectorCg[0] / vectorCg[1], 1, (1 - vectorCg.sum()) / vectorCg[1]}, //
    Vector3<Float>{vectorCb[0] / vectorCb[1], 1, (1 - vectorCb.sum()) / vectorCb[1]}};
  Vector3<Float> vectorS = dot(inverse(matrixA), vectorW);
  matrixA.row(0) *= vectorS;
  matrixA.row(1) *= vectorS;
  matrixA.row(2) *= vectorS;
  return matrixA;
}

/// XYZ triple to xyY triple.
template <std::floating_point Float = double>
[[nodiscard, strong_inline]] constexpr auto convertXYZToXYY(const Vector3<Float> &value) noexcept {
  auto factor = value.sum();
  auto result = value;
  result[2] = result[1];
  if (factor > 0) {
    result[0] /= factor;
    result[1] /= factor;
  }
  return result;
}

/// xyY triple to XYZ triple.
template <std::floating_point Float = double>
[[nodiscard, strong_inline]] constexpr auto convertXYYToXYZ(const Vector3<Float> &value) noexcept {
  auto factor = value[1];
  auto result = value;
  result = {result[2] * result[0], result[2], result[2] * (1 - result[0] - result[1])};
  if (factor > 0) {
    result[0] /= factor;
    result[2] /= factor;
  }
  return result;
}

/// \name Color-blindness
///
/// These helpers allow us to simulate the three forms of color-blindness corresponding to the Long,
/// Medium, and Short wavelength cones found in the human eye.
///
/// 1. Protanopia, red-green color blindness from defective L cones.
/// 2. Deuteranopia, red-green color blindness from defective M cones.
/// 3. Tritanopia, blue-yellow color blindness from defective S cones.
///
/// \see
/// https://daltonlens.org/understanding-cvd-simulation/
///
/// \{

template <std::floating_point Float = double>
[[nodiscard, strong_inline]] constexpr Vector3<Float> convertXYZToLMS(const Vector3<Float> &value) noexcept {
  const Matrix3<Float> matrix = {
    {Float(+0.15514), Float(+0.54312), Float(-0.03286)},
    {Float(-0.15514), Float(+0.45684), Float(+0.03286)},
    {Float(+0.00000), Float(+0.00000), Float(+0.01608)}};
  return dot(matrix, value);
}

template <std::floating_point Float = double>
[[nodiscard, strong_inline]] constexpr Vector3<Float> convertLMSToXYZ(const Vector3<Float> &value) noexcept {
  const Matrix3<Float> matrix = {
    {Float(2.94481291), Float(-3.50097799), Float(13.17218215)},
    {Float(1.00004000), Float(+1.00004000), Float(0.000000000)},
    {Float(0.00000000), Float(+0.00000000), Float(62.18905473)}};
  return dot(matrix, value);
}

template <std::floating_point Float = double>
[[nodiscard, strong_inline]] constexpr Vector3<Float> simulateProtanLMS(const Vector3<Float> &value) noexcept {
  return {Float(2.02344377) * value[1] - Float(2.52580405) * value[2], value[1], value[2]};
}

template <std::floating_point Float = double>
[[nodiscard, strong_inline]] constexpr Vector3<Float> simulateDeutanLMS(const Vector3<Float> &value) noexcept {
  return {value[0], Float(0.49420696) * value[0] + Float(1.24826995) * value[2], value[2]};
}

template <std::floating_point Float = double>
[[nodiscard, strong_inline]] constexpr Vector3<Float> simulateTritanLMS(const Vector3<Float> &value) noexcept {
  if (Float(0.34478) * value[0] - Float(0.65518) * value[1] >= 0)
    return {value[0], value[1], Float(-0.00257) * value[0] + Float(0.05366) * value[1]};
  else
    return {value[0], value[1], Float(-0.06011) * value[0] + Float(0.16299) * value[1]};
}

/// \}

/// Contrast as per the Accessible Perceptual Contrast Algorithm.
///
/// \param[in] foregroundY  The foreground luminance.
/// \param[in] backgroundY  The background luminance.
///
/// \note
/// If the background is brighter than the foreground, the result will be positive.
/// If the foreground is brighter than the background, the result will be negative.
/// Even after accounting for the sign difference, this function is not exactly
/// symmetric in the arguments!
///
/// \see
/// https://github.com/Myndex/SAPC-APCA/blob/master/src/JS/SAPC_0_98G_4g_minimal.js
///
template <std::floating_point Float> [[nodiscard]] inline Float contrastAPCA(Float foregroundY, Float backgroundY) noexcept {
  auto softClampToBlack = [](Float valueY) noexcept {
    valueY = max(valueY, 0);
    valueY = valueY > Float(0.022) ? valueY : valueY + pow(Float(0.022) - valueY, Float(1.414));
    return valueY;
  };
  foregroundY = softClampToBlack(foregroundY);
  backgroundY = softClampToBlack(backgroundY);
  Float contrast = 0;
  if (abs(backgroundY - foregroundY) > Float(0.0005)) {
    if (backgroundY > foregroundY) { // White-on-black
      contrast = Float(1.14) * (pow(backgroundY, Float(0.56)) - pow(foregroundY, Float(0.57)));
      contrast = contrast < Float(+0.001)      ? 0
                 : contrast < Float(+0.035991) ? contrast * Float(0.24981245311)
                                               : contrast - Float(0.027);

    } else { // Black-on-white
      contrast = Float(1.14) * (pow(backgroundY, Float(0.65)) - pow(foregroundY, Float(0.62)));
      contrast = contrast > Float(-0.001)      ? 0
                 : contrast > Float(-0.035991) ? contrast * Float(0.24981245311)
                                               : contrast + Float(0.027);
    }
  }
  return 100 * contrast;
}

/// XYZ triple to LAB triple.
template <std::floating_point Float = double>
[[nodiscard]] inline Vector3<Float> convertXYZToLAB(const Vector3<Float> &value) noexcept {
  auto remap = [](Float t) {
    if (t > Float(216.0 / 24389.0)) return std::cbrt(t);
    return (Float(210.25 / 27.0)) * t + Float(1.0 / 7.25);
  };
  auto result = value;
  result[0] = remap(result[0]);
  result[1] = remap(result[1]);
  result[2] = remap(result[2]);
  result = {
    Float(116) * result[1] - 16,          //
    Float(500) * (result[0] - result[1]), //
    Float(200) * (result[1] - result[2])};
  return result;
}

/// LAB triple to XYZ triple.
template <std::floating_point Float = double>
[[nodiscard]] inline Vector3<Float> convertLABToXYZ(const Vector3<Float> &value) noexcept {
  auto remap = [](Float t) {
    if (t > Float(6.0 / 29.0)) return t * t * t;
    return Float(27.0 / 210.25) * t - Float(27.0 / 1524.3125);
  };
  auto result = value;
  auto resultY = (result[0] + 16) / 116;
  result = {remap(resultY + result[1] / 500), remap(resultY), remap(resultY - result[2] / 200)};
  return result;
}

/// RGB triple to LAB triple.
template <std::floating_point Float = double>
[[nodiscard]] inline Vector3<Float> convertRGBToLAB(const Vector3<Float> &value) noexcept {
  return convertXYZToLAB<Float>(convertRGBToXYZ<Float>(value));
}

/// LAB triple to RGB triple.
template <std::floating_point Float = double>
[[nodiscard]] inline Vector3<Float> convertLABToRGB(const Vector3<Float> &value) noexcept {
  return convertXYZToRGB<Float>(convertLABToXYZ<Float>(value));
}

/// LAB triple to LCH triple.
template <std::floating_point Float = double>
[[nodiscard]] inline Vector3<Float> convertLABToLCH(const Vector3<Float> &value) noexcept {
  return {value[0], hypot(value[1], value[2]), atan2(value[2], value[1])};
}

/// LCH triple to LAB triple.
template <std::floating_point Float = double>
[[nodiscard]] inline Vector3<Float> convertLCHToLAB(const Vector3<Float> &value) noexcept {
  return {value[0], value[1] * cos(value[2]), value[1] * sin(value[2])};
}

/// Correlated color temperature (CCT) to chromaticity.
template <std::floating_point Float> [[nodiscard]] inline Vector2<Float> convertCCTToXY(Float kelvin) noexcept {
  double t = 1.0 / kelvin;
  double x = kelvin < 4000 ? ((-0.2661239e9 * t - 0.2343589e6) * t + 0.8776956e3) * t + 0.179910
                           : ((-3.0258469e9 * t + 2.1070379e6) * t + 0.2226347e3) * t + 0.240390;
  double y = kelvin < 2222   ? ((-1.1063814 * x - 1.34811020) * x + 2.18555832) * x - 0.20219683
             : kelvin < 4000 ? ((-0.9549476 * x - 1.37418593) * x + 2.09137015) * x - 0.16748867
                             : ((+3.0817580 * x - 5.87338670) * x + 3.75112997) * x - 0.37001483;
  return {Float(x), Float(y)};
}

/// Chromaticity to correlated color temperature (CCT).
template <std::floating_point Float = double> [[nodiscard]] inline Float convertXYToCCT(const Vector2<Float> &value) noexcept {
  double n = (value[0] - 0.3366) / (value[1] - 0.1735);
  double kelvin = -9.4986315e+02;
  kelvin += 6.25380338e+03 * std::exp(-n / 0.92159);
  kelvin += 2.87059900e+01 * std::exp(-n / 0.20039);
  kelvin += 4.00000000e-05 * std::exp(-n / 0.07125);
  if (!(kelvin < 50000)) {
    n = (value[0] - 0.3356) / (value[1] - 0.1691);
    kelvin = 3.628448953e+04;
    kelvin += 2.280000000e-03 * std::exp(-n / 0.07861);
    kelvin += 5.453500000e-36 * std::exp(-n / 0.01543);
  }
  return Float(std::fmax(kelvin, 0));
}

/// Blackbody radiation as predicted by Planck's law. [MW/sr/m^2/μm]
template <std::floating_point Float> [[nodiscard]] inline Float blackbodyRadiance(Float waveLen, Float kelvin) noexcept {
  if (!(waveLen > 0))
    return 0;
  else {
    constexpr Float c0 = Float(1.19104290768681554502861912e+02L);
    constexpr Float c1 = Float(1.43877729954300303744214349e+04L);
    return c0 / (nthPow(waveLen, 5) * expm1(c1 / (kelvin * waveLen)));
  }
}

template <std::floating_point Float>
[[nodiscard]] inline Float blackbodyRadianceNormalized(Float waveLen, Float kelvin) noexcept {
  return blackbodyRadiance(waveLen, kelvin) / blackbodyRadiance(Float(2897.771955 / kelvin), kelvin);
}

/// Fit of CIE 1931 X by Wyman et al.
///
/// \see [This publication][1] by Wyman, Sloan, and Shirley.
/// [1]: http://jcgt.org/published/0002/02/01/
///
template <std::floating_point Float> [[nodiscard]] inline Float wymanFit1931X(Float waveLen) noexcept {
  Float t1 = waveLen - Float(0.4420);
  Float t2 = waveLen - Float(0.5998);
  Float t3 = waveLen - Float(0.5011);
  t1 *= std::signbit(t1) ? Float(62.4) : Float(37.4);
  t2 *= std::signbit(t2) ? Float(26.4) : Float(32.3);
  t3 *= std::signbit(t3) ? Float(49.0) : Float(38.2);
  return Float(0.362) * std::exp(Float(-0.5) * t1 * t1) + Float(1.056) * std::exp(Float(-0.5) * t2 * t2) -
         Float(0.065) * std::exp(Float(-0.5) * t3 * t3);
}

/// Fit of CIE 1931 Y by Wyman et al.
///
/// \see [This publication][1] by Wyman, Sloan, and Shirley.
/// [1]: http://jcgt.org/published/0002/02/01/
///
template <std::floating_point Float> [[nodiscard]] inline Float wymanFit1931Y(Float waveLen) noexcept {
  Float t1 = waveLen - Float(0.5688);
  Float t2 = waveLen - Float(0.5309);
  t1 *= std::signbit(t1) ? Float(21.3) : Float(24.7);
  t2 *= std::signbit(t2) ? Float(61.3) : Float(32.2);
  return Float(0.821) * std::exp(Float(-0.5) * t1 * t1) + Float(0.286) * std::exp(Float(-0.5) * t2 * t2);
}

/// Fit of CIE 1931 Z by Wyman et al.
///
/// \see [This publication][1] by Wyman, Sloan, and Shirley.
/// [1]: http://jcgt.org/published/0002/02/01/
///
template <std::floating_point Float> [[nodiscard]] inline Float wymanFit1931Z(Float waveLen) noexcept {
  Float t1 = waveLen - Float(0.4370);
  Float t2 = waveLen - Float(0.4590);
  t1 *= std::signbit(t1) ? Float(84.5) : Float(27.8);
  t2 *= std::signbit(t2) ? Float(38.5) : Float(72.5);
  return Float(1.217) * std::exp(Float(-0.5) * t1 * t1) + Float(0.681) * std::exp(Float(-0.5) * t2 * t2);
}

template <std::floating_point Float = double>
[[nodiscard]] inline Vector3<Float> tonemapReinhard(Vector3<Float> value) noexcept {
  value = max(value, Vector3<Float>(0));
  return saturate(value / (1 + convertRGBToLuminance(value)));
}

template <std::floating_point Float = double> [[nodiscard]] inline Vector3<Float> tonemapHejl(Vector3<Float> value) noexcept {
  value = max(value, Vector3<Float>(0));
  Vector3<Float> numer = value * (Float(1.425) * value + Float(0.05)) + Float(0.004);
  Vector3<Float> denom = value * (Float(1.425) * value + Float(0.60)) + Float(0.0491);
  return saturate(numer / denom - Float(0.0821));
}

template <std::floating_point Float = double> [[nodiscard]] inline Vector3<Float> tonemapACES(Vector3<Float> value) noexcept {
  // https://64.github.io/tonemapping/
  value = max(value, Vector3<Float>(0));
  const Matrix3<Float> matrixRGBToRRT = {
    {Float(0.59719), Float(0.35458), Float(0.04823)},
    {Float(0.07600), Float(0.90834), Float(0.01566)},
    {Float(0.02840), Float(0.13383), Float(0.83777)}};
  const Matrix3<Float> matrixODTToRGB = {
    {Float(+1.60475), Float(-0.53108), Float(-0.07367)},
    {Float(-0.10208), Float(+1.10813), Float(-0.00605)},
    {Float(-0.00327), Float(-0.07276), Float(+1.07602)}};
  // RRT = Reference Rendering Transform
  // ODT = Output Device Transform
  auto valueRRT = dot(matrixRGBToRRT, value);
  auto valueODT = (valueRRT * (valueRRT + Float(0.0245786)) - Float(0.000090537)) /
                  (valueRRT * (Float(0.983729) * valueRRT + Float(0.4329510)) + Float(0.238081));
  return saturate(dot(matrixODTToRGB, valueODT));
}

} // namespace mi
