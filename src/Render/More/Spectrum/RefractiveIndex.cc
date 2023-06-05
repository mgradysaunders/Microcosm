#include "Microcosm/Render/More/Spectrum/RefractiveIndex"

#include "RefractiveIndexMetals.h"

namespace mi::render {

DielectricRefractiveIndex refractiveIndexOfAir(double temperatureC, double pressurekPa) {
  double correction{(pressurekPa / 101.33) / (1 + 3.4785e-3 * (temperatureC - 15))};
  return {0.30, 1.50, [correction](const Spectrum &waveLens) {
            Spectrum values{waveLens.shape};
            for (size_t i = 0; i < waveLens.size(); i++)
              values[i] = 1 + correction * (6.4328e-5 +                                       //
                                            2.949810e-2 / (1.46e2 - 1.0 / sqr(waveLens[i])) + //
                                            0.025540e-2 / (0.46e2 - 1.0 / sqr(waveLens[i])));
            return values;
          }};
}

DielectricRefractiveIndex refractiveIndexOf(KnownGlass knownGlass) {
  static constexpr double SellmeierParams[][6] = {
    {1.039612120, 0.231792344, 1.010469450, 0.00600069867, 0.0200179144, 103.560653},
    {1.585149500, 0.143559385, 1.085212690, 0.00926681282, 0.0424489805, 105.613573},
    {1.123656620, 0.309276848, 0.881511957, 0.00644742752, 0.0222284402, 107.297751},
    {0.971247817, 0.216901417, 0.904651666, 0.00472301995, 0.0153575612, 168.681330},
    {2.000295470, 0.298926886, 1.806918430, 0.01214260170, 0.0538736236, 156.530829},
    {1.524818890, 0.187085527, 1.427290150, 0.01125475600, 0.0588995392, 129.141675},
    {1.621539020, 0.256287842, 1.644475520, 0.01222414570, 0.0595736775, 147.468793},
    {1.737596950, 0.313747346, 1.898781010, 0.01318870700, 0.0623068142, 155.236290}};
  return {0.30, 1.50, [sellmeierParams = &SellmeierParams[int(knownGlass)][0]](const Spectrum &waveLens) {
            Spectrum values{waveLens.shape};
            for (size_t i = 0; i < waveLens.size(); i++) {
              double invSqrWaveLen{1 / sqr(waveLens[i])};
              values[i] = sqrt(
                1 +                                                             //
                sellmeierParams[0] / (1 - sellmeierParams[3] * invSqrWaveLen) + //
                sellmeierParams[1] / (1 - sellmeierParams[4] * invSqrWaveLen) + //
                sellmeierParams[2] / (1 - sellmeierParams[5] * invSqrWaveLen));
            }
            return values;
          }};
}

ConductiveRefractiveIndex refractiveIndexOf(KnownMetal knownMetal) {
  double minWaveLen{MetalTableLookup[int(knownMetal)].second[0].first};
  double maxWaveLen{MetalTableLookup[int(knownMetal)].second[MetalTableLookup[int(knownMetal)].first - 1].first};
  auto metalTableSize{MetalTableLookup[int(knownMetal)].first};
  auto metalTableData{MetalTableLookup[int(knownMetal)].second};
  auto metalTable{IteratorRange(metalTableData, metalTableSize)};
  return {minWaveLen, maxWaveLen, [=](const Spectrum &waveLens) {
            ComplexSpectrum values{waveLens.shape};
            for (size_t i = 0; i < waveLens.size(); i++) {
              double waveLen = waveLens[i];
              if (minWaveLen <= waveLen && waveLen <= maxWaveLen) [[likely]] {
                auto [itrA, itrB] = surroundingPair(
                  metalTable, float(waveLen), [](auto &each, float waveLen) -> bool { return each.first < waveLen; });
                const auto &[waveLenA, valueA] = *itrA;
                const auto &[waveLenB, valueB] = *itrB;
                values[i] = lerp(unlerp(float(waveLen), waveLenA, waveLenB), valueA, valueB);
              }
            }
            return values;
          }};
}

} // namespace mi::render
