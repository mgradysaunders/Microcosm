#include "Microcosm/Render/More/Spectrum/Prospect"

#include "ProspectTable.h"

namespace mi::render {

auto Prospect::operator()(double waveLen) const noexcept -> Result<double> {
  if (!(waveLen >= 0.4 && waveLen <= 2.5)) return {};
  double factor{0.5 * (waveLen * 1000 - 400)};
  const int indexA{max(int(factor), 0)};
  const int indexB{min(indexA + 1, ProspectTableSize - 1)};
  const float *tableValuesA{&ProspectTable[indexA][0]};
  const float *tableValuesB{&ProspectTable[indexB][0]};
  factor -= indexA;
  double tableValues[7]{lerp(factor, tableValuesA[0], tableValuesB[0]), lerp(factor, tableValuesA[1], tableValuesB[1]),
                        lerp(factor, tableValuesA[2], tableValuesB[2]), lerp(factor, tableValuesA[3], tableValuesB[3]),
                        lerp(factor, tableValuesA[4], tableValuesB[4]), lerp(factor, tableValuesA[5], tableValuesB[5]),
                        lerp(factor, tableValuesA[6], tableValuesB[6])};
  double eta{tableValues[0]};
  double k{
    (tableValues[1] * chlorophylls + tableValues[2] * carotenoids + //
     tableValues[3] * anthocyanins + tableValues[4] * browns +      //
     tableValues[5] * water + tableValues[6] * dryMatter) /
    numLayers};
  double tau{k > 1e-6 ? (1 - k) * exp(-k) - sqr(k) * std::expint(-k) : 1.0};
  // Fifth order polynomial fit to the average transmittance for alpha=40 and alpha=90 degrees. The
  // range of relevant etas is between 1.2 and 1.6, where these functions are very well behaved and
  // mostly linear. These were obtained by NumPy's polyfit function for the domain in question, with
  // RMS on the order of 1e-7 to 1e-8.
  auto averageTransmittance40 = [](double eta) -> double {
    double tau = 0.04667554;
    tau = tau * eta - 0.38995679;
    tau = tau * eta + 1.34963974;
    tau = tau * eta - 2.43163031;
    tau = tau * eta + 2.13767169;
    tau = tau * eta + 0.28776448;
    return tau;
  };
  auto averageTransmittance90 = [](double eta) -> double {
    double tau = -0.17369388;
    tau = tau * eta + 1.3189973;
    tau = tau * eta - 4.02936997;
    tau = tau * eta + 6.21265658;
    tau = tau * eta - 4.99648418;
    tau = tau * eta + 2.66515836;
    return tau;
  };
  /*
  auto averageTransmittance = [](double eta, double alpha) -> double {
    auto ts = [=](double sin2AlphaI) {
      double cosAlphaI{sqrt(1 - sin2AlphaI)};
      double cosAlphaT{sqrt(1 - sin2AlphaI / sqr(eta))};
      return 4 * eta * cosAlphaI * cosAlphaT / sqr(cosAlphaI + eta * cosAlphaT);
    };
    auto tp = [=](double sin2AlphaI) {
      double cosAlphaI{sqrt(1 - sin2AlphaI)};
      double cosAlphaT{sqrt(1 - sin2AlphaI / sqr(eta))};
      return 4 * eta * cosAlphaI * cosAlphaT / sqr(eta * cosAlphaI + cosAlphaT);
    };
    double sin2Alpha{sqr(sin(alpha))};
    static const Quadrature<32> quadrature;
    return 0.5 * (quadrature(0, sin2Alpha, ts) + quadrature(0, sin2Alpha, tp)) / sin2Alpha;
  };
   */
  double tAlpha{averageTransmittance40(eta)}, rAlpha{1 - tAlpha};
  double t12{averageTransmittance90(eta)}, r12{1 - t12};
  double t21{t12 / sqr(eta)}, r21{1 - t21};
  double tA{tAlpha * tau * t21 / (1 - sqr(r21 * tau))};
  double rA{rAlpha + r21 * tau * tA};
  double t{t12 * tau * t21 / (1 - sqr(r21 * tau))};
  double r{r12 + r21 * tau * t};
  double tSub{0};
  double rSub{0};
  if (r + t > 1) {
    tSub = t / (t + (1 - t) * (numLayers - 1));
    rSub = 1 - tSub;
  } else {
    double d{sqrt((1 + r + t) * (1 + r - t) * (1 - r + t) * (1 - r - t))};
    double a{(1 + sqr(r) - sqr(t) + d) / (2 * r)};
    double b{(1 - sqr(r) + sqr(t) + d) / (2 * t)}, bNm1{pow(b, numLayers - 1)};
    rSub = a * (sqr(bNm1) - 1) / (sqr(a * bNm1) - 1);
    tSub = bNm1 * (sqr(a) - 1) / (sqr(a * bNm1) - 1);
  }
  return {rA + tA * rSub * t / (1 - rSub * r), tA * tSub / (1 - rSub * r)};
}

auto Prospect::convertToXYZ() const noexcept -> Result<Vector3d> {
  Vector3d totalR{};
  Vector3d totalT{};
  double totalY{};
  for (double waveLen : linspace(200, 0.4, 0.8)) {
    auto [valueR, valueT] = operator()(waveLen);
    Vector3d wymanXYZ = {wymanFit1931X(waveLen), wymanFit1931Y(waveLen), wymanFit1931Z(waveLen)};
    totalR += wymanXYZ * valueR;
    totalT += wymanXYZ * valueT;
    totalY += wymanXYZ[1];
  }
  totalR /= totalY;
  totalT /= totalY;
  return {totalR, totalT};
}

void Prospect::neuralNetworkFitFromXYZ(Vector3d totalAlbedoXYZ) noexcept {
  Vector3d vectorV0 = log(totalAlbedoXYZ);
  vectorV0 -= Vector3d(-1.593630074436e+00, -1.681355887300e+00, -2.716103526258e+00);
  vectorV0 /= Vector3d(+5.790135458997e-01, +5.609804784187e-01, +4.455119087513e-01);
  static constexpr auto DenseLayer1 = std::make_pair(
    Matrix<double, 3, 8>{
      {+4.255868196487e-01, +7.059755735099e-03, -2.880350828171e+00, +3.149514913559e+00, +1.176136136055e+00,
       +4.204098701477e+00, +4.914900302887e+00, +4.545198917389e+00},
      {-5.157529115677e-01, +1.517757475376e-01, +2.316906452179e+00, -3.214446306229e+00, -3.275548815727e-01,
       +6.587715148926e-01, -4.833834648132e+00, -7.702155590057e+00},
      {+8.066483497620e+00, -3.913561254740e-02, -3.408432722092e+00, -1.618979275227e-01, -4.898065924644e-01,
       -2.874665260315e+00, +3.269512951374e-01, +2.052333831787e+00}},
    Vector<double, 8>{
      +8.461952209473e+00, +7.365098595619e-02, -6.050422668457e+00, -2.461498022079e+00, +1.536008000374e+00,
      +1.119824409485e+01, -2.743183672428e-01, -6.194952487946e+00});
  Vector<double, 8> vectorV1 = softSign(dot(vectorV0, DenseLayer1.first) + DenseLayer1.second);
  static constexpr auto DenseLayer2 = std::make_pair(
    Matrix<double, 8, 8>{
      {-1.860881298780e-01, +2.766618877649e-02, +5.767352581024e+00, -2.287517356873e+01, -1.038822889328e+00,
       -8.565822243690e-02, +1.343659877777e+01, +8.876585364342e-01},
      {-1.098977565765e+00, +1.004273605347e+01, +8.320302367210e-01, -4.945674419403e+00, +3.273842334747e+00,
       +2.441589355469e+00, -4.174402058125e-01, -3.699733734131e+00},
      {-3.799892663956e-01, -6.708385944366e+00, -1.806248545647e+00, -1.974681913853e-01, +6.732019186020e-01,
       +5.845348358154e+00, -4.427663326263e+00, -1.276872992516e+00},
      {+2.167475037277e-02, +2.999928474426e+00, -2.761754274368e+00, +3.902226209641e+00, -1.837223172188e+00,
       -5.687524676323e-01, +2.335954189301e+00, +8.559446334839e+00},
      {-1.390638202429e-01, -1.228298187256e+00, -7.140570640564e+00, -2.214319944382e+00, +9.667071700096e-01,
       +4.267201423645e+00, -1.682564544678e+01, -2.062811613083e+00},
      {-7.706543803215e-01, +3.790049314499e+00, -3.294202566147e+00, +1.596813350916e-01, -2.811556816101e+00,
       +1.966592311859e+00, +9.484733343124e-01, +3.539476633072e+00},
      {-5.774016380310e-01, +6.561708450317e-01, -1.036312133074e-01, +2.111636877060e+00, -2.061820030212e+00,
       -1.593809247017e+00, -1.202746555209e-01, +2.312342971563e-01},
      {-7.110388278961e-01, +1.053862452507e+00, -1.647055864334e+00, +1.393852949142e+00, -3.178906440735e+00,
       -3.942429125309e-01, +2.912712812424e+00, +1.206018924713e+00}},
    Vector<double, 8>{
      -4.084906280041e-01, -8.246263504028e+00, -4.712520122528e+00, +5.609501600266e-01, -2.893151760101e+00,
      +1.906291961670e+00, -5.029565334320e+00, +4.909110069275e+00});
  Vector<double, 8> vectorV2 = softSign(dot(vectorV1, DenseLayer2.first) + DenseLayer2.second);
  static constexpr auto DenseLayer3 = std::make_pair(
    Matrix<double, 8, 4>{
      {+2.797585964203e+00, -4.921471476555e-01, -1.635502338409e+00, +1.333385229111e+00},
      {-3.558403730392e+00, +1.445028334856e-01, -6.646015495062e-02, -4.896520137787e+00},
      {+1.731114029884e+00, +1.417233109474e+00, -3.006833553314e+00, -9.182871580124e-01},
      {-2.545663155615e-02, -1.657092273235e-01, +5.557020187378e+00, +1.231660172343e-01},
      {+2.990534305573e-01, +2.448901176453e+00, +5.116696953773e-01, -5.405459403992e+00},
      {-5.072249174118e-01, -2.716009855270e+00, +3.395467758179e+00, +1.169359385967e-01},
      {-1.561922907829e+00, -2.185689449310e+00, -3.085556983948e+00, +9.928663969040e-01},
      {-4.806176722050e-01, +6.073828220367e+00, +3.494888246059e-01, -3.181103944778e+00}},
    Vector<double, 4>{+1.274151325226e+00, +4.656734168530e-01, -1.734789013863e-01, -4.524215698242e+00});
  Vector<double, 4> vectorV3 = exp(dot(vectorV2, DenseLayer3.first) + DenseLayer3.second);
  chlorophylls = vectorV3[0];
  anthocyanins = vectorV3[1];
  carotenoids = vectorV3[2];
  browns = vectorV3[3];
}

} // namespace mi::render
