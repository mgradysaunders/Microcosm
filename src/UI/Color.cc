#include "Microcosm/UI/Color"
#include <iostream>

namespace mi::ui {

Color Color::fromHex(uint32_t valueRGBA) noexcept {
  return {
    decodeSRGB((0xFF & (valueRGBA >> 24)) / 255.0f), //
    decodeSRGB((0xFF & (valueRGBA >> 16)) / 255.0f), //
    decodeSRGB((0xFF & (valueRGBA >> 8)) / 255.0f),  //
    (0xFF & (valueRGBA)) / 255.0f};
}

Color Color::fromXYZ(Vector3f valueXYZ, float valueA) noexcept { return {convertXYZToRGB<float>(valueXYZ), valueA}; }

Color Color::fromLAB(Vector3f valueLAB, float valueA) noexcept {
  return fromXYZ(Vector3f(0.950489f, 1.0f, 1.08884f) * convertLABToXYZ<float>(valueLAB), valueA);
}

Color Color::fromLCH(Vector3f valueLCH, float valueA) noexcept { return fromLAB(convertLCHToLAB<float>(valueLCH), valueA); }

struct NamedColor {
  std::string_view groupName;
  std::string_view name;
  uint8_t valueR{};
  uint8_t valueG{};
  uint8_t valueB{};
};

static const NamedColor sNamedColors[] = {
  {"Pink", "Pink", 255, 192, 203},
  {"Pink", "LightPink", 255, 182, 193},
  {"Pink", "HotPink", 255, 105, 180},
  {"Pink", "DeepPink", 255, 20, 147},
  {"Pink", "PaleVioletRed", 219, 112, 147},
  {"Pink", "MediumVioletRed", 199, 21, 133},
  {"Red", "LightSalmon", 255, 160, 122},
  {"Red", "Salmon", 250, 128, 114},
  {"Red", "DarkSalmon", 233, 150, 122},
  {"Red", "LightCoral", 240, 128, 128},
  {"Red", "IndianRed", 205, 92, 92},
  {"Red", "Crimson", 220, 20, 60},
  {"Red", "Firebrick", 178, 34, 34},
  {"Red", "DarkRed", 139, 0, 0},
  {"Red", "Red", 255, 0, 0},
  {"Orange", "OrangeRed", 255, 69, 0},
  {"Orange", "Tomato", 255, 99, 71},
  {"Orange", "Coral", 255, 127, 80},
  {"Orange", "DarkOrange", 255, 140, 0},
  {"Orange", "Orange", 255, 165, 0},
  {"Yellow", "Yellow", 255, 255, 0},
  {"Yellow", "LightYellow", 255, 255, 224},
  {"Yellow", "LemonChiffon", 255, 250, 205},
  {"Yellow", "LightGoldenrodYellow", 250, 250, 210},
  {"Yellow", "PapayaWhip", 255, 239, 213},
  {"Yellow", "Moccasin", 255, 228, 181},
  {"Yellow", "PeachPuff", 255, 218, 185},
  {"Yellow", "PaleGoldenrod", 238, 232, 170},
  {"Yellow", "Khaki", 240, 230, 140},
  {"Yellow", "DarkKhaki", 189, 183, 107},
  {"Yellow", "Gold", 255, 215, 0},
  {"Brown", "Cornsilk", 255, 248, 220},
  {"Brown", "BlanchedAlmond", 255, 235, 205},
  {"Brown", "Bisque", 255, 228, 196},
  {"Brown", "NavajoWhite", 255, 222, 173},
  {"Brown", "Wheat", 245, 222, 179},
  {"Brown", "Burlywood", 222, 184, 135},
  {"Brown", "Tan", 210, 180, 140},
  {"Brown", "RosyBrown", 188, 143, 143},
  {"Brown", "SandyBrown", 244, 164, 96},
  {"Brown", "Goldenrod", 218, 165, 32},
  {"Brown", "DarkGoldenrod", 184, 134, 11},
  {"Brown", "Peru", 205, 133, 63},
  {"Brown", "Chocolate", 210, 105, 30},
  {"Brown", "SaddleBrown", 139, 69, 19},
  {"Brown", "Sienna", 160, 82, 45},
  {"Brown", "Brown", 165, 42, 42},
  {"Brown", "Maroon", 128, 0, 0},
  {"Green", "DarkOliveGreen", 85, 107, 47},
  {"Green", "Olive", 128, 128, 0},
  {"Green", "OliveDrab", 107, 142, 35},
  {"Green", "YellowGreen", 154, 205, 50},
  {"Green", "LimeGreen", 50, 205, 50},
  {"Green", "Lime", 0, 255, 0},
  {"Green", "LawnGreen", 124, 252, 0},
  {"Green", "Chartreuse", 127, 255, 0},
  {"Green", "GreenYellow", 173, 255, 47},
  {"Green", "SpringGreen", 0, 255, 127},
  {"Green", "MediumSpringGreen", 0, 250, 154},
  {"Green", "LightGreen", 144, 238, 144},
  {"Green", "PaleGreen", 152, 251, 152},
  {"Green", "DarkSeaGreen", 143, 188, 143},
  {"Green", "MediumAquamarine", 102, 205, 170},
  {"Green", "MediumSeaGreen", 60, 179, 113},
  {"Green", "SeaGreen", 46, 139, 87},
  {"Green", "ForestGreen", 34, 139, 34},
  {"Green", "Green", 0, 128, 0},
  {"Green", "DarkGreen", 0, 100, 0},
  {"Cyan", "Aqua", 0, 255, 255},
  {"Cyan", "Cyan", 0, 255, 255},
  {"Cyan", "LightCyan", 224, 255, 255},
  {"Cyan", "PaleTurquoise", 175, 238, 238},
  {"Cyan", "Aquamarine", 127, 255, 212},
  {"Cyan", "Turquoise", 64, 224, 208},
  {"Cyan", "MediumTurquoise", 72, 209, 204},
  {"Cyan", "DarkTurquoise", 0, 206, 209},
  {"Cyan", "LightSeaGreen", 32, 178, 170},
  {"Cyan", "CadetBlue", 95, 158, 160},
  {"Cyan", "DarkCyan", 0, 139, 139},
  {"Cyan", "Teal", 0, 128, 128},
  {"Blue", "LightSteelBlue", 176, 196, 222},
  {"Blue", "PowderBlue", 176, 224, 230},
  {"Blue", "LightBlue", 173, 216, 230},
  {"Blue", "SkyBlue", 135, 206, 235},
  {"Blue", "LightSkyBlue", 135, 206, 250},
  {"Blue", "DeepSkyBlue", 0, 191, 255},
  {"Blue", "DodgerBlue", 30, 144, 255},
  {"Blue", "CornflowerBlue", 100, 149, 237},
  {"Blue", "SteelBlue", 70, 130, 180},
  {"Blue", "RoyalBlue", 65, 105, 225},
  {"Blue", "Blue", 0, 0, 255},
  {"Blue", "MediumBlue", 0, 0, 205},
  {"Blue", "DarkBlue", 0, 0, 139},
  {"Blue", "Navy", 0, 0, 128},
  {"Blue", "MidnightBlue", 25, 25, 112},
  {"Violet", "Lavender", 230, 230, 250},
  {"Violet", "Thistle", 216, 191, 216},
  {"Violet", "Plum", 221, 160, 221},
  {"Violet", "Violet", 238, 130, 238},
  {"Violet", "Orchid", 218, 112, 214},
  {"Violet", "Magenta", 255, 0, 255},
  {"Violet", "Fuchsia", 255, 0, 255},
  {"Violet", "MediumOrchid", 186, 85, 211},
  {"Violet", "MediumPurple", 147, 112, 219},
  {"Violet", "BlueViolet", 138, 43, 226},
  {"Violet", "DarkViolet", 148, 0, 211},
  {"Violet", "DarkOrchid", 153, 50, 204},
  {"Violet", "DarkMagenta", 139, 0, 139},
  {"Violet", "Purple", 128, 0, 128},
  {"Violet", "Indigo", 75, 0, 130},
  {"Violet", "DarkSlateBlue", 72, 61, 139},
  {"Violet", "SlateBlue", 106, 90, 205},
  {"Violet", "MediumSlateBlue", 123, 104, 238},
  {"White", "White", 255, 255, 255},
  {"White", "Snow", 255, 250, 250},
  {"White", "Honeydew", 240, 255, 240},
  {"White", "MintCream", 245, 255, 250},
  {"White", "Azure", 240, 255, 255},
  {"White", "AliceBlue", 240, 248, 255},
  {"White", "GhostWhite", 248, 248, 255},
  {"White", "WhiteSmoke", 245, 245, 245},
  {"White", "Seashell", 255, 245, 238},
  {"White", "Beige", 245, 245, 220},
  {"White", "OldLace", 253, 245, 230},
  {"White", "FloralWhite", 255, 250, 240},
  {"White", "Ivory", 255, 255, 240},
  {"White", "AntiqueWhite", 250, 235, 215},
  {"White", "Linen", 250, 240, 230},
  {"White", "LavenderBlush", 255, 240, 245},
  {"White", "MistyRose", 255, 228, 225},
  {"Gray", "Gainsboro", 220, 220, 220},
  {"Gray", "LightGray", 211, 211, 211},
  {"Gray", "Silver", 192, 192, 192},
  {"Gray", "DarkGray", 169, 169, 169},
  {"Gray", "Gray", 128, 128, 128},
  {"Gray", "DimGray", 105, 105, 105},
  {"Gray", "LightSlateGray", 119, 136, 153},
  {"Gray", "SlateGray", 112, 128, 144},
  {"Gray", "DarkSlateGray", 47, 79, 79},
  {"Gray", "Black", 0, 0, 0}};

Color Color::fromWeb(std::string_view name) noexcept {
  for (const auto &namedColor : sNamedColors)
    if (namedColor.name == name)
      return {
        decodeSRGB(namedColor.valueR / 255.0f), //
        decodeSRGB(namedColor.valueG / 255.0f), //
        decodeSRGB(namedColor.valueB / 255.0f), 1.0f};
  return {};
}

uint32_t Color::toHex() const noexcept {
  return (fastRound<uint32_t>(255 * encodeSRGB(mValue[0])) << 24) | //
         (fastRound<uint32_t>(255 * encodeSRGB(mValue[1])) << 16) | //
         (fastRound<uint32_t>(255 * encodeSRGB(mValue[2])) << 8) |  //
         (fastRound<uint32_t>(255 * saturate(mValue[3])));
}

Vector3f Color::toXYZ() const noexcept { return convertRGBToXYZ<float>(Vector3f(mValue)); }

Vector3f Color::toLAB() const noexcept { return convertXYZToLAB<float>(toXYZ() / Vector3f(0.950489f, 1.0f, 1.08884f)); }

Vector3f Color::toLCH() const noexcept { return convertLABToLCH<float>(toLAB()); }

std::string_view Color::toWeb() const noexcept {
  float bestError = constants::Inf<float>;
  const NamedColor *bestColor = nullptr;
  for (const auto &namedColor : sNamedColors) {
    float error = distanceTo(Color{
      decodeSRGB(namedColor.valueR / 255.0f), //
      decodeSRGB(namedColor.valueG / 255.0f), //
      decodeSRGB(namedColor.valueB / 255.0f), //
      1.0f});
    if (bestError > error) {
      bestError = error;
      bestColor = &namedColor;
    }
  }
  assert(bestColor);
  return bestColor->name;
}

float Color::distanceTo(const Color &other) const noexcept {
  auto ChromaMapping = [](double C) { return sqrt(1 / (1 + nthPow(25 / C, 7))); };
  auto [L1, a1, b1] = Vector3d(this->toLAB());
  auto [L2, a2, b2] = Vector3d(other.toLAB());
  double C1 = hypot(a1, b1);
  double C2 = hypot(a2, b2);
  double C = (C1 + C2) / 2;
  double G = (1 - ChromaMapping(C)) / 2;
  double a1Prime = (1 + G) * a1, C1Prime = hypot(a1Prime, b1), h1Prime = finiteOrZero(atan2(b1, a1Prime));
  double a2Prime = (1 + G) * a2, C2Prime = hypot(a2Prime, b2), h2Prime = finiteOrZero(atan2(b2, a2Prime));
  if (h1Prime < 0) h1Prime += 360.0_degrees;
  if (h2Prime < 0) h2Prime += 360.0_degrees;
  while (h1Prime + 180.0_degrees < h2Prime) h1Prime += 360.0_degrees;
  while (h1Prime - 180.0_degrees > h2Prime) h1Prime -= 360.0_degrees;
  double hPrime = (h1Prime + h2Prime) / 2;
  double LPrime = (L1 + L2) / 2;
  double deltaL = (L2 - L1) / (1 + 0.015 * sqr(LPrime - 50) / sqrt(20 + sqr(LPrime - 50)));
  double CPrime = (C1Prime + C2Prime) / 2;
  double deltaC = (C2Prime - C1Prime) / (1 + 0.045 * CPrime);
  double deltaH = (2 * sqrt(C1Prime * C2Prime) * sin((h2Prime - h1Prime) / 2)) / //
                  (1 + 0.015 * CPrime *
                         (1 +                                     //
                          -0.17 * cos(hPrime - 30.0_degrees) +    //
                          +0.24 * cos(2 * hPrime) +               //
                          +0.32 * cos(3 * hPrime + 6.0_degrees) + //
                          -0.20 * cos(4 * hPrime - 63.0_degrees)));
  double deltaTheta = 30.0_degrees * exp(-sqr((hPrime - 275.0_degrees) / 25.0_degrees));
  return sqrt(sqr(deltaL) + sqr(deltaC) + sqr(deltaH) - 2 * ChromaMapping(CPrime) * sin(2 * deltaTheta) * deltaC * deltaH);
}

float Color::contrast(const Color &other) const noexcept { return contrastAPCA<float>(luminance(), other.luminance()); }

Color Color::simulate(Anomaly anomaly, float severity) const noexcept {
  Vector3f valueLMS0 = convertXYZToLMS<float>(toXYZ());
  Vector3f valueLMS1 = valueLMS0;
  switch (anomaly) {
  case Anomaly::Protan: valueLMS1 = simulateProtanLMS<float>(valueLMS0); break;
  case Anomaly::Deutan: valueLMS1 = simulateDeutanLMS<float>(valueLMS0); break;
  case Anomaly::Tritan: valueLMS1 = simulateTritanLMS<float>(valueLMS0); break;
  default: break;
  }
  return fromXYZ(convertLMSToXYZ<float>(lerp(severity, valueLMS0, valueLMS1)), alpha());
}

std::vector<Color>
Color::rampFromLAB(const Vector3f &valueLAB0, const Vector3f &valueLAB1, size_t numColors, bool optimizePerception) {
  return rampFromContour([&](float t) { return fromLAB(lerp(t, valueLAB0, valueLAB1)); }, numColors, optimizePerception);
}

std::vector<Color>
Color::rampFromLCH(const Vector3f &valueLCH0, const Vector3f &valueLCH1, size_t numColors, bool optimizePerception) {
  return rampFromContour([&](float t) { return fromLCH(lerp(t, valueLCH0, valueLCH1)); }, numColors, optimizePerception);
}

std::vector<Color>
Color::rampFromContour(const std::function<Color(float)> &colorContour, size_t numColors, bool optimizePerception) {
  Color colorA = colorContour(0.0f);
  Color colorB = colorContour(1.0f);
  switch (numColors) {
  case 0: return {};
  case 1: return {colorA};
  case 2: return {colorA, colorB};
  default: break;
  }
  std::vector<Color> colors(numColors);
  colors[0] = colorA, colors[numColors - 1] = colorB;
  Color protanColorA = colorA.simulateProtan(), protanColorB = colorB.simulateProtan();
  Color deutanColorA = colorA.simulateDeutan(), deutanColorB = colorB.simulateDeutan();
  Color tritanColorA = colorA.simulateTritan(), tritanColorB = colorB.simulateTritan();
  for (size_t i = 1; i + 1 < numColors; i++) { //
    float param = float(i) / numColors;
    if (optimizePerception) {
      float bestError = constants::Inf<float>;
      Color bestColor;
      for (size_t j = 1; j < 512; j++) {
        auto errorTerms = [param](const Color &color, const Color &colorA, const Color &colorB) {
          float distanceToA = color.distanceTo(colorA);
          float distanceToB = color.distanceTo(colorB);
          float distanceParam = distanceToA / (distanceToA + distanceToB);
          float contrastParamA = color.contrast(colorA) / colorB.contrast(colorA);
          float contrastParamB = color.contrast(colorB) / colorA.contrast(colorB);
          return sqr(distanceParam - param) + 0.5f * (sqr(contrastParamA - param) + sqr(contrastParamB - (1 - param)));
        };
        Color color = colorContour(j / 512.0f);
        double error = 0;
        error += errorTerms(color, colorA, colorB);
        error += 0.333 * errorTerms(color.simulateProtan(), protanColorA, protanColorB);
        error += 0.333 * errorTerms(color.simulateDeutan(), deutanColorA, deutanColorB);
        error += 0.333 * errorTerms(color.simulateTritan(), tritanColorA, tritanColorB);
        if (bestError > error) {
          bestError = error;
          bestColor = color;
        }
      }
      colors[i] = bestColor;
    } else {
      colors[i] = colorContour(param);
    }
  }
  return colors;
}

Color Color::over(const Color &other) const noexcept {
  auto [colorA, alphaA] = this->detachAlpha();
  auto [colorB, alphaB] = other.detachAlpha();
  float alphaC = lerp(alphaA, alphaB, 1), invAlphaC = finiteOrZero(1 / alphaC);
  return {Vector3f(invAlphaC * lerp(alphaA, alphaB * colorB, colorA)), alphaC};
}

Color::operator Vector3b() const noexcept {
  return {
    fastRound<uint8_t>(255.0f * encodeSRGB(mValue[0])), //
    fastRound<uint8_t>(255.0f * encodeSRGB(mValue[1])), //
    fastRound<uint8_t>(255.0f * encodeSRGB(mValue[2]))};
}

Color::operator Vector4b() const noexcept {
  return {
    fastRound<uint8_t>(255.0f * encodeSRGB(mValue[0])), //
    fastRound<uint8_t>(255.0f * encodeSRGB(mValue[1])), //
    fastRound<uint8_t>(255.0f * encodeSRGB(mValue[2])), //
    fastRound<uint8_t>(255.0f * saturate(mValue[3]))};
}

} // namespace mi::ui
