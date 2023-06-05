#include "Microcosm/Render/Scattering"
#include "Microcosm/Render/More/Scattering/Diffuse"
#include "testing.h"

TEST_CASE("Scattering") {
  SUBCASE("Bind usage") {
#if 0
    mi::Vector3d omegaO = mi::normalize(mi::Vector3d(1, 2, 3));
    mi::Vector3d omegaI = mi::normalize(mi::Vector3d(3, 2, 1));
    mi::render::Scattering scattering = mi::render::LambertBSDF({0.5, 0.5, 0.5}, {0.0, 0.0, 0.0});
    auto f = scattering.scatterBSDF(omegaO, omegaI);
    std::cout << sizeof(scattering) << std::endl;
#endif
  }
}

