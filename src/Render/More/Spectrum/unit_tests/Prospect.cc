#include "Microcosm/Render/More/Spectrum/Prospect"
#include "Microcosm/BoundBox"
#include "testing.h"

TEST_CASE("Prospect") {
  mi::render::Prospect prospect;
  {
    prospect.neuralNetworkFitFromXYZ(mi::convertRGBToXYZ(mi::decodeSRGB(mi::Vector3d(238/255.,168/255.,72/255.))));
    auto [r1, t1] = prospect.convertToXYZ();
    auto a1 = mi::Vector3d(mi::encodeSRGB(mi::convertXYZToRGB<double>(r1+t1)) * 255);
    std::cout << a1[0] << ' ' << a1[1] << ' ' << a1[2] <<std::endl;
  }
#if 0
  auto prng = PRNG();
  mi::Vector3d a;
  mi::BoundBox3d b;
  for (int i = 0; i < 1000; i++) {
  prospect.chlorophylls = mi::randomize<double>(prng) * 40;
  prospect.anthocyanins = mi::randomize<double>(prng) * 20;
  prospect.carotenoids = mi::randomize<double>(prng) * 20;
  prospect.browns = mi::randomize<double>(prng) * 2;
    auto [r1, t1] = prospect.convertToXYZ();
    prospect.neuralNetworkFitFromXYZ(r1 + t1);
    auto [r2, t2] = prospect.convertToXYZ();
    auto a1 = mi::Vector3d(mi::encodeSRGB(mi::convertXYZToRGB<double>(r1)) * 255);
    auto a2 = mi::Vector3d(mi::encodeSRGB(mi::convertXYZToRGB<double>(r2)) * 255);
    b |= a1;
    a1 -= a2;
    a1 = mi::abs(a1);
    a += a1;
    std::cout << a1[0] << ' ' << a1[1] << ' ' << a1[2] <<std::endl;
  }
  a /= 1000;
  std::cout << a[0] << ' ' << a[1] << ' ' << a[2] << std::endl;
  std::cout << b[0][0] << ' ' << b[0][1] << ' ' << b[0][2] << std::endl;
  std::cout << b[1][0] << ' ' << b[1][1] << ' ' << b[1][2] << std::endl;
#endif
}
