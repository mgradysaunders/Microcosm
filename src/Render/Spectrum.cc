#include "Microcosm/Render/Spectrum"

namespace mi::render {

Spectrum spectrumLinspace(size_t count, double minValue, double maxValue) noexcept {
  Spectrum values{with_shape, count};
  for (size_t i = 0; i < count; i++) values[i] = lerp((i + 0.5) / count, minValue, maxValue);
  return values;
}

Spectrum spectrumLinspace(size_t count, double minValue, double maxValue, Random &random) noexcept {
  Spectrum values{with_shape, count};
  for (size_t i = 0; i < count; i++) values[i] = lerp((i + double(random)) / count, minValue, maxValue);
  return values;
}

Vector3d convertSpectrumToXYZ(const Spectrum &waveLens, const Spectrum &values) noexcept {
  assert(waveLens.size() == values.size());
  Vector3d colorXYZ{};
  for (size_t i = 0; i < waveLens.size(); i++) {
    colorXYZ[0] += wymanFit1931X(waveLens[i]) * values[i];
    colorXYZ[1] += wymanFit1931Y(waveLens[i]) * values[i];
    colorXYZ[2] += wymanFit1931Z(waveLens[i]) * values[i];
  }
  return colorXYZ / waveLens.size();
}

Vector3d convertSpectrumToRGB(const Spectrum &waveLens, const Spectrum &values) noexcept { return convertXYZToRGB(convertSpectrumToXYZ(waveLens, values)); }

static constexpr float ConversionWaveLens[32] = //
  {+0.3800000f, +0.3909677f, +0.4019355f, +0.4129032f, +0.4238710f, +0.4348387f, +0.4458065f, +0.4567742f, +0.4677419f, +0.4787097f, +0.4896774f, +0.5006452f, +0.5116129f, +0.5225806f, +0.5335483f, +0.5445161f,
   +0.5554838f, +0.5664515f, +0.5774192f, +0.5883869f, +0.5993546f, +0.6103223f, +0.6212900f, +0.6322578f, +0.6432255f, +0.6541932f, +0.6651609f, +0.6761286f, +0.6870963f, +0.6980640f, +0.7090317f, +0.7200000f};
// Albedo: White, Cyan, Magenta, Yellow, Red, Green, Blue
// Illumination: White, Cyan, Magenta, Yellow, Red, Green, Blue
static constexpr float ConversionCurves[2][7][32] = //
  {{{+1.0618958f, +1.0615020f, +1.0614336f, +1.0622711f, +1.0622036f, +1.0625060f, +1.0623939f, +1.0624707f, +1.0625048f, +1.0624366f, +1.0620694f, +1.0613167f, +1.0610334f, +1.0613868f, +1.0614215f, +1.0620337f,
     +1.0625497f, +1.0624317f, +1.0625249f, +1.0624278f, +1.0624750f, +1.0625539f, +1.0625327f, +1.0623922f, +1.0623651f, +1.0625256f, +1.0612278f, +1.0594263f, +1.0599811f, +1.0602547f, +1.0601263f, +1.0606565f},
    {+1.0414628f, +1.0328661f, +1.0126146f, +1.0350461f, +1.0078661f, +1.0422280f, +1.0442597f, +1.0535238f, +1.0180776f, +1.0442730f, +1.0529362f, +1.0537034f, +1.0533901f, +1.0537783f, +1.0527093f, +1.0530449f,
     +1.0550555f, +1.0553674f, +1.0454307f, +0.6234895f, +0.1803807f, -0.0076304f, -0.0001522f, -0.0075102f, -0.0021709f, +0.0006592f, +0.0122788f, -0.0044670f, +0.0171198f, +0.0049211f, +0.0058763f, +0.0252594f},
    {+0.9942214f, +0.9898694f, +0.9829366f, +0.9962787f, +1.0198956f, +1.0166396f, +1.0220913f, +0.9965166f, +1.0097766f, +1.0215422f, +0.6403195f, +0.0025012f, +0.0065340f, +0.0028334f, -0.0000000f, -0.0090592f,
     +0.0033937f, -0.0030639f, +0.2220394f, +0.6314114f, +0.9748099f, +0.9720956f, +1.0173770f, +0.9987519f, +0.9470173f, +0.8525862f, +0.9489780f, +0.9475188f, +0.9959894f, +0.8630135f, +0.8915099f, +0.8486649f},
    {+0.0055741f, -0.0047983f, -0.0052537f, -0.0064571f, -0.0059694f, -0.0021837f, +0.0167811f, +0.0960964f, +0.2121736f, +0.3616913f, +0.5396101f, +0.7440881f, +0.9220957f, +1.0460304f, +1.0513825f, +1.0511992f,
     +1.0510530f, +1.0517397f, +1.0516043f, +1.0511944f, +1.0511590f, +1.0516613f, +1.0514039f, +1.0515941f, +1.0511460f, +1.0515124f, +1.0508871f, +1.0508924f, +1.0477493f, +1.0493273f, +1.0435964f, +1.0392281f},
    {+0.1657560f, +0.1184644f, +0.1240829f, +0.1137127f, +0.0789924f, +0.0322056f, -0.0107984f, +0.0180520f, +0.0053407f, +0.0136549f, -0.0059564f, -0.0018444f, -0.0105719f, -0.0029376f, -0.0107905f, -0.0080224f,
     -0.0022669f, +0.0070200f, -0.0081528f, +0.6077287f, +0.9883156f, +0.9939169f, +1.0039339f, +0.9923450f, +0.9992653f, +1.0084622f, +0.9835830f, +1.0085024f, +0.9745114f, +0.9854327f, +0.9349576f, +0.9871391f},
    {+0.0026494f, -0.0050175f, -0.0125472f, -0.0094555f, -0.0125261f, -0.0079171f, -0.0079956f, -0.0093559f, +0.0654686f, +0.3957288f, +0.7524402f, +0.9637648f, +0.9985443f, +0.9999298f, +0.9993908f, +0.9999437f,
     +0.9993912f, +0.9991124f, +0.9601958f, +0.6318628f, +0.2579740f, +0.0094015f, -0.0030798f, -0.0045230f, -0.0068933f, -0.0090352f, -0.0085914f, -0.0083691f, -0.0078686f, -0.0000084f, +0.0054301f, -0.0027746f},
    {+0.9920977f, +0.9887643f, +0.9953904f, +0.9952932f, +0.9918145f, +1.0002584f, +0.9996848f, +0.9998812f, +0.9850401f, +0.7902985f, +0.5608220f, +0.3313346f, +0.1369241f, +0.0189149f, -0.0000051f, -0.0004240f,
     -0.0004193f, +0.0017473f, +0.0037999f, -0.0005510f, -0.0000437f, +0.0075875f, +0.0257957f, +0.0381684f, +0.0494896f, +0.0495960f, +0.0498148f, +0.0398409f, +0.0305010f, +0.0212431f, +0.0069597f, +0.0041734f}},
   {{+1.1565232f, +1.1567225f, +1.1566203f, +1.1555783f, +1.1562176f, +1.1567674f, +1.1568023f, +1.1567677f, +1.1563563f, +1.1567055f, +1.1565135f, +1.1564336f, +1.1568023f, +1.1473148f, +1.1339318f, +1.1293876f,
     +1.1290516f, +1.0504864f, +1.0459696f, +0.9936669f, +0.9560167f, +0.9246748f, +0.9149994f, +0.8993947f, +0.8954252f, +0.8887057f, +0.8822284f, +0.8799831f, +0.8763524f, +0.8800037f, +0.8806567f, +0.8830470f},
    {+1.1334480f, +1.1266762f, +1.1346828f, +1.1357396f, +1.1356372f, +1.1361153f, +1.1362180f, +1.1364820f, +1.1355107f, +1.1364061f, +1.1360364f, +1.1360123f, +1.1354266f, +1.1363100f, +1.1355450f, +1.1353732f,
     +1.1349497f, +1.1111114f, +0.9059874f, +0.6116078f, +0.2953975f, +0.0959542f, -0.0116508f, -0.0121446f, -0.0111482f, -0.0119976f, -0.0050507f, -0.0079983f, -0.0094723f, -0.0055330f, -0.0045429f, -0.0125410f},
    {+1.0371892f, +1.0587543f, +1.0767272f, +1.0762707f, +1.0795289f, +1.0743644f, +1.0727029f, +1.0732447f, +1.0823761f, +1.0840546f, +0.9560757f, +0.5519789f, +0.0841911f, +0.0000879f, -0.0023086f, -0.0011248f,
     -0.0000000f, -0.0002727f, +0.0144665f, +0.2588312f, +0.5290800f, +0.9096662f, +1.0690571f, +1.0887326f, +1.0637622f, +1.0201813f, +1.0262197f, +1.0783086f, +0.9833385f, +1.0707246f, +1.0634248f, +1.0150876f},
    {+0.0027757f, +0.0039674f, -0.0001461f, +0.0003620f, -0.0002582f, -0.0000501f, -0.0002444f, -0.0000781f, +0.0496903f, +0.4851597f, +1.0295726f, +1.0333211f, +1.0368103f, +1.0364884f, +1.0365428f, +1.0368595f,
     +1.0365646f, +1.0363939f, +1.0367205f, +1.0365239f, +1.0361531f, +1.0348785f, +1.0042729f, +0.8421848f, +0.7375939f, +0.6585315f, +0.6053168f, +0.5954980f, +0.5941926f, +0.5651768f, +0.5606118f, +0.5822861f},
    {+0.0547112f, +0.0556091f, +0.0607559f, +0.0562329f, +0.0461699f, +0.0380128f, +0.0244242f, +0.0038984f, -0.0005608f, +0.0009649f, +0.0003734f, -0.0004337f, -0.0000935f, -0.0001235f, -0.0001452f, -0.0002005f,
     -0.0004994f, +0.0272551f, +0.1606741f, +0.3506979f, +0.5735747f, +0.7639209f, +0.8914447f, +0.9639461f, +0.9887946f, +0.9989745f, +0.9860514f, +0.9953250f, +0.9743348f, +0.9913436f, +0.9886629f, +0.9971386f},
    {+0.0251684f, +0.0394274f, +0.0062060f, +0.0071121f, +0.0002176f, +0.0000000f, -0.0216231f, +0.0156702f, +0.0028020f, +0.3249477f, +1.0164918f, +1.0329477f, +1.0321587f, +1.0358667f, +1.0151236f, +1.0338076f,
     +1.0371373f, +1.0361377f, +1.0229822f, +0.9691033f, -0.0051786f, +0.0011131f, +0.0066676f, +0.0007402f, +0.0215916f, +0.0051482f, +0.0014562f, +0.0001641f, -0.0064631f, +0.0102509f, +0.0423874f, +0.0212527f},
    {+1.0570490f, +1.0538467f, +1.0550494f, +1.0530407f, +1.0579931f, +1.0578439f, +1.0583133f, +1.0579712f, +1.0561885f, +1.0571399f, +1.0425795f, +0.3260309f, -0.0019256f, -0.0012959f, -0.0014357f, -0.0012964f,
     -0.0019227f, +0.0012621f, -0.0016095f, -0.0013030f, -0.0017667f, -0.0012325f, +0.0103168f, +0.0312845f, +0.0887739f, +0.1387362f, +0.1553507f, +0.1487848f, +0.1662426f, +0.1699761f, +0.1576974f, +0.1906909f}}};

Spectrum convertRGBToSpectrumAlbedo(const Spectrum &waveLens, const Vector3d &color) noexcept {
  Spectrum values{waveLens.shape};
  int orderA = (color[0] <= color[1] && color[0] <= color[2]) ? 0 : (color[1] <= color[2] && color[1] <= color[0]) ? 1 : 2;
  int orderB = (orderA + 1) % 3;
  int orderC = (orderA + 2) % 3;
  if (!(color[orderB] <= color[orderC])) {
    std::swap(orderB, orderC);
  }
  CubicInterpolator interpolator{&ConversionWaveLens[0], 32};
  for (auto &&[waveLen, value] : ranges::zip(waveLens, values)) {
    value = 0;
    if (waveLen >= ConversionWaveLens[0] && waveLen <= ConversionWaveLens[31]) {
      value += color[orderA] * interpolator(waveLen, &ConversionCurves[0][0][0]);                            // White
      value += (color[orderB] - color[orderA]) * interpolator(waveLen, &ConversionCurves[0][orderA + 1][0]); // CMY
      value += (color[orderC] - color[orderB]) * interpolator(waveLen, &ConversionCurves[0][orderC + 4][0]); // RGB
    }
    value *= 0.94;
  }
  return values;
}

Spectrum convertRGBToSpectrumIllumination(const Spectrum &waveLens, const Vector3d &color) noexcept {
  Spectrum values{waveLens.shape};
  int orderA = (color[0] <= color[1] && color[0] <= color[2]) ? 0 : (color[1] <= color[2] && color[1] <= color[0]) ? 1 : 2;
  int orderB = (orderA + 1) % 3;
  int orderC = (orderA + 2) % 3;
  if (!(color[orderB] <= color[orderC])) {
    std::swap(orderB, orderC);
  }
  CubicInterpolator interpolator{&ConversionWaveLens[0], 32};
  for (auto &&[waveLen, value] : ranges::zip(waveLens, values)) {
    value = 0;
    if (waveLen >= ConversionWaveLens[0] && waveLen <= ConversionWaveLens[31]) {
      value += color[orderA] * interpolator(waveLen, &ConversionCurves[0][0][0]);                            // White
      value += (color[orderB] - color[orderA]) * interpolator(waveLen, &ConversionCurves[0][orderA + 1][0]); // CMY
      value += (color[orderC] - color[orderB]) * interpolator(waveLen, &ConversionCurves[0][orderC + 4][0]); // RGB
    }
    value *= 0.86445;
  }
  return values;
}

Spectrum spectrumIlluminantD(const Spectrum &waveLens, const Vector2d &chromaticity) noexcept {
  static constexpr Vector3f Table[54] = //
    {{0.04f, 0.02f, 0.00f},     {6.00f, 4.50f, 2.00f},     {29.60f, 22.40f, 4.00f},   {55.30f, 42.00f, 8.50f},   {57.30f, 40.60f, 7.80f},  {61.80f, 41.60f, 6.70f},   {61.50f, 38.00f, 5.30f},   {68.80f, 42.40f, 6.10f},   {63.40f, 38.50f, 2.00f},
     {65.80f, 35.00f, 1.20f},   {94.80f, 43.40f, -1.10f},  {104.80f, 46.30f, -0.50f}, {105.90f, 43.90f, -0.70f}, {96.80f, 37.10f, -1.20f}, {113.90f, 36.70f, -2.60f}, {125.60f, 35.90f, -2.90f}, {125.50f, 32.60f, -2.80f}, {121.30f, 27.90f, -2.60f},
     {121.30f, 24.30f, -2.60f}, {113.50f, 20.10f, -1.80f}, {113.10f, 16.20f, -1.50f}, {110.80f, 13.20f, -1.30f}, {106.50f, 8.60f, -1.20f}, {108.80f, 6.10f, -1.00f},  {105.30f, 4.20f, -0.50f},  {104.40f, 1.90f, -0.30f},  {100.00f, 0.00f, 0.00f},
     {96.00f, -1.60f, 0.20f},   {95.10f, -3.50f, 0.50f},   {89.10f, -3.50f, 2.10f},   {90.50f, -5.80f, 3.20f},   {90.30f, -7.20f, 4.10f},  {88.40f, -8.60f, 4.70f},   {84.00f, -9.50f, 5.10f},   {85.10f, -10.90f, 6.70f},  {81.90f, -10.70f, 7.30f},
     {82.60f, -12.00f, 8.60f},  {84.90f, -14.00f, 9.80f},  {81.30f, -13.60f, 10.20f}, {71.90f, -12.00f, 8.30f},  {74.30f, -13.30f, 9.60f}, {76.40f, -12.90f, 8.50f},  {63.30f, -10.60f, 7.00f},  {71.70f, -11.60f, 7.60f},  {77.00f, -12.20f, 8.00f},
     {65.20f, -10.20f, 6.70f},  {47.70f, -7.80f, 5.20f},   {68.60f, -11.20f, 7.40f},  {65.00f, -10.40f, 6.80f},  {66.00f, -10.60f, 7.00f}, {61.00f, -9.70f, 6.40f},   {53.30f, -8.30f, 5.50f},   {58.90f, -9.30f, 6.10f},   {61.90f, -9.80f, 6.50f}};
  Spectrum values{waveLens.shape};
  double coeffM1{(-1.3515e+0 - 1.77030e+0 * chromaticity[0] + 5.91140e+0 * chromaticity[1]) / (+0.0241e+0 + 0.25620e+0 * chromaticity[0] - 0.73410e+0 * chromaticity[1])};
  double coeffM2{(+0.0300e+0 - 3.14424e+1 * chromaticity[0] + 3.00717e+1 * chromaticity[1]) / (+0.0241e+0 + 0.25620e+0 * chromaticity[0] - 0.73410e+0 * chromaticity[1])};
  for (auto &&[waveLen, value] : ranges::zip(waveLens, values)) {
    value = 0;
    if (waveLen >= 0.3 && waveLen <= 0.83) [[likely]] {
      auto param{100.0 * (waveLen - 0.3)};
      auto index{static_cast<int>(param)};
      Vector3d coeffS = catmullRom(
        param - index,                       //
        Vector3d(Table[max(index - 1, 0)]),  //
        Vector3d(Table[min(index + 0, 53)]), //
        Vector3d(Table[min(index + 1, 53)]), //
        Vector3d(Table[min(index + 2, 53)]));
      value = coeffS[0] + coeffM1 * coeffS[1] + coeffM2 * coeffS[2];
    }
  }
  return values;
}

Spectrum spectrumIlluminantF(const Spectrum &waveLens, int number) noexcept {
  static constexpr float Table[12][81] = //
    {{1.8700f,  2.3600f,  2.9400f,  3.4700f,  5.1700f,  19.4900f, 6.1300f,  6.2400f,  7.0100f,  7.7900f,  8.5600f,  43.6700f, 16.9400f, 10.7200f, 11.3500f, 11.8900f, 12.3700f, 12.7500f, 13.0000f, 13.1500f, 13.2300f,
      13.1700f, 13.1300f, 12.8500f, 12.5200f, 12.2000f, 11.8300f, 11.5000f, 11.2200f, 11.0500f, 11.0300f, 11.1800f, 11.5300f, 27.7400f, 17.0500f, 13.5500f, 14.3300f, 15.0100f, 15.5200f, 18.2900f, 19.5500f, 15.4800f,
      14.9100f, 14.1500f, 13.2200f, 12.1900f, 11.1200f, 10.0300f, 8.9500f,  7.9600f,  7.0200f,  6.2000f,  5.4200f,  4.7300f,  4.1500f,  3.6400f,  3.2000f,  2.8100f,  2.4700f,  2.1800f,  1.9300f,  1.7200f,  1.6700f,
      1.4300f,  1.2900f,  1.1900f,  1.0800f,  0.9600f,  0.8800f,  0.8100f,  0.7700f,  0.7500f,  0.7300f,  0.6800f,  0.6900f,  0.6400f,  0.6800f,  0.6900f,  0.6100f,  0.5200f,  0.4300f},
     {1.1800f,  1.4800f,  1.8400f,  2.1500f,  3.4400f,  15.6900f, 3.8500f,  3.7400f, 4.1900f, 4.6200f, 5.0600f, 34.9800f, 11.8100f, 6.2700f,  6.6300f,  6.9300f,  7.1900f,  7.4000f,  7.5400f,  7.6200f,  7.6500f,
      7.6200f,  7.6200f,  7.4500f,  7.2800f,  7.1500f,  7.0500f,  7.0400f,  7.1600f, 7.4700f, 8.0400f, 8.8800f, 10.0100f, 24.8800f, 16.6400f, 14.5900f, 16.1600f, 17.5600f, 18.6200f, 21.4700f, 22.7900f, 19.2900f,
      18.6600f, 17.7300f, 16.5400f, 15.2100f, 13.8000f, 12.3600f, 10.9500f, 9.6500f, 8.4000f, 7.3200f, 6.3100f, 5.4300f,  4.6800f,  4.0200f,  3.4500f,  2.9600f,  2.5500f,  2.1900f,  1.8900f,  1.6400f,  1.5300f,
      1.2700f,  1.1000f,  0.9900f,  0.8800f,  0.7600f,  0.6800f,  0.6100f,  0.5600f, 0.5400f, 0.5100f, 0.4700f, 0.4700f,  0.4300f,  0.4600f,  0.4700f,  0.4000f,  0.3300f,  0.2700f},
     {0.8200f,  1.0200f,  1.2600f,  1.4400f,  2.5700f,  14.3600f, 2.7000f,  2.4500f,  2.7300f, 3.0000f, 3.2800f, 31.8500f, 9.4700f,  4.0200f,  4.2500f,  4.4400f,  4.5900f,  4.7200f,  4.8000f,  4.8600f,  4.8700f,
      4.8500f,  4.8800f,  4.7700f,  4.6700f,  4.6200f,  4.6200f,  4.7300f,  4.9900f,  5.4800f, 6.2500f, 7.3400f, 8.7800f,  23.8200f, 16.1400f, 14.5900f, 16.6300f, 18.4900f, 19.9500f, 23.1100f, 24.6900f, 21.4100f,
      20.8500f, 19.9300f, 18.6700f, 17.2200f, 15.6500f, 14.0400f, 12.4500f, 10.9500f, 9.5100f, 8.2700f, 7.1100f, 6.0900f,  5.2200f,  4.4500f,  3.8000f,  3.2300f,  2.7500f,  2.3300f,  1.9900f,  1.7000f,  1.5500f,
      1.2700f,  1.0900f,  0.9600f,  0.8300f,  0.7100f,  0.6200f,  0.5400f,  0.4900f,  0.4600f, 0.4300f, 0.3900f, 0.3900f,  0.3500f,  0.3800f,  0.3900f,  0.3300f,  0.2800f,  0.2100f},
     {0.5700f,  0.7000f,  0.8700f,  0.9800f,  2.0100f,  13.7500f, 1.9500f,  1.5900f,  1.7600f,  1.9300f, 2.1000f, 30.2800f, 8.0300f,  2.5500f,  2.7000f,  2.8200f,  2.9100f,  2.9900f,  3.0400f,  3.0800f,  3.0900f,
      3.0900f,  3.1400f,  3.0600f,  3.0000f,  2.9800f,  3.0100f,  3.1400f,  3.4100f,  3.9000f,  4.6900f, 5.8100f, 7.3200f,  22.5900f, 15.1100f, 13.8800f, 16.3300f, 18.6800f, 20.6400f, 24.2800f, 26.2600f, 23.2800f,
      22.9400f, 22.1400f, 20.9100f, 19.4300f, 17.7400f, 16.0000f, 14.4200f, 12.5600f, 10.9300f, 9.5200f, 8.1800f, 7.0100f,  6.0000f,  5.1100f,  4.3600f,  3.6900f,  3.1300f,  2.6400f,  2.2400f,  1.9100f,  1.7000f,
      1.3900f,  1.1800f,  1.0300f,  0.8800f,  0.7400f,  0.6400f,  0.5400f,  0.4900f,  0.4600f,  0.4200f, 0.3700f, 0.3700f,  0.3300f,  0.3500f,  0.3600f,  0.3100f,  0.2600f,  0.1900f},
     {1.8700f,  2.3500f,  2.9200f,  3.4500f,  5.1000f,  18.9100f, 6.0000f,  6.1100f,  6.8500f,  7.5800f,  8.3100f,  40.7600f, 16.0600f, 10.3200f, 10.9100f, 11.4000f, 11.8300f, 12.1700f, 12.4000f, 12.5400f, 12.5800f,
      12.5200f, 12.4700f, 12.2000f, 11.8900f, 11.6100f, 11.3300f, 11.1000f, 10.9600f, 10.9700f, 11.1600f, 11.5400f, 12.1200f, 27.7800f, 17.7300f, 14.4700f, 15.2000f, 15.7700f, 16.1000f, 18.5400f, 19.5000f, 15.3900f,
      14.6400f, 13.7200f, 12.6900f, 11.5700f, 10.4500f, 9.3500f,  8.2900f,  7.3200f,  6.4100f,  5.6300f,  4.9000f,  4.2600f,  3.7200f,  3.2500f,  2.8300f,  2.4900f,  2.1900f,  1.9300f,  1.7100f,  1.5200f,  1.4300f,
      1.2600f,  1.1300f,  1.0500f,  0.9600f,  0.8500f,  0.7800f,  0.7200f,  0.6800f,  0.6700f,  0.6500f,  0.6100f,  0.6200f,  0.5900f,  0.6200f,  0.6400f,  0.5500f,  0.4700f,  0.4000f},
     {1.0500f,  1.3100f,  1.6300f,  1.9000f,  3.1100f,  14.8000f, 3.4300f,  3.3000f, 3.6800f, 4.0700f, 4.4500f, 32.6100f, 10.7400f, 5.4800f,  5.7800f,  6.0300f,  6.2500f,  6.4100f,  6.5200f,  6.5800f,  6.5900f,
      6.5600f,  6.5600f,  6.4200f,  6.2800f,  6.2000f,  6.1900f,  6.3000f,  6.6000f, 7.1200f, 7.9400f, 9.0700f, 10.4900f, 25.2200f, 17.4600f, 15.6300f, 17.2200f, 18.5300f, 19.4300f, 21.9700f, 23.0100f, 19.4100f,
      18.5600f, 17.4200f, 16.0900f, 14.6400f, 13.1500f, 11.6800f, 10.2500f, 8.9600f, 7.7400f, 6.6900f, 5.7100f, 4.8700f,  4.1600f,  3.5500f,  3.0200f,  2.5700f,  2.2000f,  1.8700f,  1.6000f,  1.3700f,  1.2900f,
      1.0500f,  0.9100f,  0.8100f,  0.7100f,  0.6100f,  0.5400f,  0.4800f,  0.4400f, 0.4300f, 0.4000f, 0.3700f, 0.3800f,  0.3500f,  0.3900f,  0.4100f,  0.3300f,  0.2600f,  0.2100f},
     {2.5600f,  3.1800f,  3.8400f,  4.5300f,  6.1500f,  19.3700f, 7.3700f,  7.0500f,  7.7100f,  8.4100f,  9.1500f,  44.1400f, 17.5200f, 11.3500f, 12.0000f, 12.5800f, 13.0800f, 13.4500f, 13.7100f, 13.8800f, 13.9500f,
      13.9300f, 13.8200f, 13.6400f, 13.4300f, 13.2500f, 13.0800f, 12.9300f, 12.7800f, 12.6000f, 12.4400f, 12.3300f, 12.2600f, 29.5200f, 17.0500f, 12.4400f, 12.5800f, 12.7200f, 12.8300f, 15.4600f, 16.7500f, 12.8300f,
      12.6700f, 12.4500f, 12.1900f, 11.8900f, 11.6000f, 11.3500f, 11.1200f, 10.9500f, 10.7600f, 10.4200f, 10.1100f, 10.0400f, 10.0200f, 10.1100f, 9.8700f,  8.6500f,  7.2700f,  6.4400f,  5.8300f,  5.4100f,  5.0400f,
      4.5700f,  4.1200f,  3.7700f,  3.4600f,  3.0800f,  2.7300f,  2.4700f,  2.2500f,  2.0600f,  1.9000f,  1.7500f,  1.6200f,  1.5400f,  1.4500f,  1.3200f,  1.1700f,  0.9900f,  0.8100f},
     {1.2100f,  1.5000f,  1.8100f,  2.1300f,  3.1700f,  13.0800f, 3.8300f,  3.4500f,  3.8600f,  4.4200f,  5.0900f,  34.1000f, 12.4200f, 7.6800f,  8.6000f,  9.4600f,  10.2400f, 10.8400f, 11.3300f, 11.7100f, 11.9800f,
      12.1700f, 12.2800f, 12.3200f, 12.3500f, 12.4400f, 12.5500f, 12.6800f, 12.7700f, 12.7200f, 12.6000f, 12.4300f, 12.2200f, 28.9600f, 16.5100f, 11.7900f, 11.7600f, 11.7700f, 11.8400f, 14.6100f, 16.1100f, 12.3400f,
      12.5300f, 12.7200f, 12.9200f, 13.1200f, 13.3400f, 13.6100f, 13.8700f, 14.0700f, 14.2000f, 14.1600f, 14.1300f, 14.3400f, 14.5000f, 14.4600f, 14.0000f, 12.5800f, 10.9900f, 9.9800f,  9.2200f,  8.6200f,  8.0700f,
      7.3900f,  6.7100f,  6.1600f,  5.6300f,  5.0300f,  4.4600f,  4.0200f,  3.6600f,  3.3600f,  3.0900f,  2.8500f,  2.6500f,  2.5100f,  2.3700f,  2.1500f,  1.8900f,  1.6100f,  1.3200f},
     {0.9000f,  1.1200f,  1.3600f,  1.6000f,  2.5900f,  12.8000f, 3.0500f,  2.5600f,  2.8600f,  3.3000f,  3.8200f,  32.6200f, 10.7700f, 5.8400f,  6.5700f,  7.2500f,  7.8600f,  8.3500f,  8.7500f,  9.0600f,  9.3100f,
      9.4800f,  9.6100f,  9.6800f,  9.7400f,  9.8800f,  10.0400f, 10.2600f, 10.4800f, 10.6300f, 10.7600f, 10.9600f, 11.1800f, 27.7100f, 16.2900f, 12.2800f, 12.7400f, 13.2100f, 13.6500f, 16.5700f, 18.1400f, 14.5500f,
      14.6500f, 14.6600f, 14.6100f, 14.5000f, 14.3900f, 14.4000f, 14.4700f, 14.6200f, 14.7200f, 14.5500f, 14.4000f, 14.5800f, 14.8800f, 15.5100f, 15.4700f, 13.2000f, 10.5700f, 9.1800f,  8.2500f,  7.5700f,  7.0300f,
      6.3500f,  5.7200f,  5.2500f,  4.8000f,  4.2900f,  3.8000f,  3.4300f,  3.1200f,  2.8600f,  2.6400f,  2.4300f,  2.2600f,  2.1400f,  2.0200f,  1.8300f,  1.6100f,  1.3800f,  1.1200f},
     {1.1100f,  0.6300f,  0.6200f,  0.5700f, 1.4800f,  12.1600f, 2.1200f,  2.7000f,  3.7400f,  5.1400f, 6.7500f, 34.3900f, 14.8600f, 10.4000f, 10.7600f, 10.6700f, 10.1100f, 9.2700f, 8.2900f, 7.2900f,  7.9100f,
      16.6400f, 16.7300f, 10.4400f, 5.9400f, 3.3400f,  2.3500f,  1.8800f,  1.5900f,  1.4700f,  1.8000f, 5.7100f, 40.9800f, 73.6900f, 33.6100f, 8.2400f,  3.3800f,  2.4700f,  2.1400f, 4.8600f, 11.4500f, 14.7900f,
      12.1600f, 8.9700f,  6.5200f,  8.8100f, 44.1200f, 34.5500f, 12.0900f, 12.1500f, 10.5200f, 4.4300f, 1.9500f, 2.1900f,  3.1900f,  2.7700f,  2.2900f,  2.0000f,  1.5200f,  1.3500f, 1.4700f, 1.7900f,  1.7400f,
      1.0200f,  1.1400f,  3.3200f,  4.4900f, 2.0500f,  0.4900f,  0.2400f,  0.2100f,  0.2100f,  0.2400f, 0.2400f, 0.2100f,  0.1700f,  0.2100f,  0.2200f,  0.1700f,  0.1200f,  0.0900f},
     {0.9100f,  0.6300f,  0.4600f, 0.3700f, 1.2900f,  12.6800f, 1.5900f,  1.7900f,  2.4600f,  3.3300f, 4.4900f, 33.9400f, 12.1300f, 6.9500f,  7.1900f, 7.1200f, 6.7200f, 6.1300f, 5.4600f, 4.7900f,  5.6600f,
      14.2900f, 14.9600f, 8.9700f, 4.7200f, 2.3300f,  1.4700f,  1.1000f,  0.8900f,  0.8300f,  1.1800f, 4.9000f, 39.5900f, 72.8400f, 32.6100f, 7.5200f, 2.8300f, 1.9600f, 1.6700f, 4.4300f, 11.2800f, 14.7600f,
      12.7300f, 9.7400f,  7.3300f, 9.7200f, 55.2700f, 42.5800f, 13.1800f, 13.1600f, 12.2600f, 5.1100f, 2.0700f, 2.3400f,  3.5800f,  3.0100f,  2.4800f, 2.1400f, 1.5400f, 1.3300f, 1.4600f, 1.9400f,  2.0000f,
      1.2000f,  1.3500f,  4.1000f, 5.5800f, 2.5100f,  0.5700f,  0.2700f,  0.2300f,  0.2100f,  0.2400f, 0.2400f, 0.2000f,  0.2400f,  0.3200f,  0.2600f, 0.1600f, 0.1200f, 0.0900f},
     {0.9600f,  0.6400f,  0.4500f, 0.3300f,  1.1900f,  12.4800f, 1.1200f,  0.9400f,  1.0800f,  1.3700f, 1.7800f, 29.0500f, 7.9000f,  2.6500f,  2.7100f, 2.6500f, 2.4900f, 2.3300f, 2.1000f, 1.9100f,  3.0100f,
      10.8300f, 11.8800f, 6.8800f, 3.4300f,  1.4900f,  0.9200f,  0.7100f,  0.6000f,  0.6300f,  1.1000f, 4.5600f, 34.4000f, 65.4000f, 29.4800f, 7.1600f, 3.0800f, 2.4700f, 2.2700f, 5.0900f, 11.9600f, 15.3200f,
      14.2700f, 11.8600f, 9.2800f, 12.3100f, 68.5300f, 53.0200f, 14.6700f, 14.3800f, 14.7100f, 6.4600f, 2.5700f, 2.7500f,  4.1800f,  3.4400f,  2.8100f, 2.4200f, 1.6400f, 1.3600f, 1.4900f, 2.1400f,  2.3400f,
      1.4200f,  1.6100f,  5.0400f, 6.9800f,  3.1900f,  0.7100f,  0.3000f,  0.2600f,  0.2300f,  0.2800f, 0.2800f, 0.2100f,  0.1700f,  0.2100f,  0.1900f, 0.1500f, 0.1000f, 0.0500f}};
  number = clamp(number, 1, 12);
  Spectrum values{waveLens.shape};
  for (auto &&[waveLen, value] : ranges::zip(waveLens, values)) {
    value = 0;
    if (waveLen >= 0.38 && waveLen <= 0.78) [[likely]] {
      auto param{200.0 * (waveLen - 0.38)};
      auto index{static_cast<int>(param)};
      value = catmullRom(
        param - index,                                 //
        double(Table[number - 1][max(index - 1, 0)]),  //
        double(Table[number - 1][min(index + 0, 81)]), //
        double(Table[number - 1][min(index + 1, 81)]), //
        double(Table[number - 1][min(index + 2, 81)]));
    }
  }
  return values;
}

Spectrum spectrumBlackbody(const Spectrum &waveLens, double kelvin) noexcept {
  Spectrum values{waveLens.shape};
  for (auto &&[waveLen, value] : ranges::zip(waveLens, values)) value = blackbodyRadianceNormalized(waveLen, kelvin);
  return values;
}

} // namespace mi::render
