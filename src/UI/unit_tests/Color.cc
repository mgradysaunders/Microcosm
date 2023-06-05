#include "Microcosm/UI/Color"
#include "testing.h"

TEST_CASE("Color") {
  SUBCASE("Verify some web colors") {
    CHECK(mi::ui::Color::fromWeb("Coral").toHex() == 0xFF7F50'FFu);
    CHECK(mi::ui::Color::fromWeb("Firebrick").toHex() == 0xB22222'FFu);
    CHECK(mi::ui::Color::fromWeb("SlateBlue").toHex() == 0x6A5ACD'FFu);
    CHECK(mi::ui::Color::fromWeb("SandyBrown").toHex() == 0xF4A460'FFu);
    CHECK(mi::ui::Color::fromWeb("Chartreuse").toHex() == 0x7FFF00'FFu);
    CHECK(mi::ui::Color::fromWeb("Aquamarine").toHex() == 0x7FFFD4'FFu);
    CHECK(mi::ui::Color::fromWeb("DeepSkyBlue").toHex() == 0x00BFFF'FFu);
  }
  SUBCASE("Verify round-trip conversion") {
    mi::ui::Color color = mi::ui::Color::fromWeb("DarkOrchid");
    CHECK(color.toHex() == 0x9932CC'FFu);
    CHECK(color.toHex() == mi::ui::Color::fromXYZ(color.toXYZ()).toHex());
    CHECK(color.toHex() == mi::ui::Color::fromLAB(color.toLAB()).toHex());
    CHECK(color.toHex() == mi::ui::Color::fromLCH(color.toLCH()).toHex());
    CHECK(color.toWeb() == "DarkOrchid");
  }
  SUBCASE("Verify CIEDE2000 distance") {
    // https://hajim.rochester.edu/ece/sites/gsharma/ciede2000/ciede2000noteCRNA.pdf 
    auto distanceBetweenLABColors = [](const mi::Vector3f &valueLAB0, const mi::Vector3f &valueLAB1) {
      return mi::ui::Color::fromLAB(valueLAB0).distanceTo(mi::ui::Color::fromLAB(valueLAB1));
    };
    CHECK(
      distanceBetweenLABColors({50.000f, 2.6772f, -79.7751f}, {50.000f, 0.0000f, -82.7485f}) ==
      doctest::Approx(2.0425).epsilon(5e-4));
    CHECK(
      distanceBetweenLABColors({2.0776f, 0.0795f, -1.1350f}, {0.9033f, -0.0636f, -0.5514f}) ==
      doctest::Approx(0.9082).epsilon(5e-4));
  }
}
