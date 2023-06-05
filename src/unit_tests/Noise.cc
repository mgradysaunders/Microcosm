#include "Microcosm/Noise"
#include "Microcosm/Differentiable"
#include "testing.h"

#include <iostream>

TEST_CASE("Noise") {
  auto noise = mi::noise_generators::Simplex<mi::Differentiable<double>, 2>();
  auto noise2 = mi::noise_generators::MusgraveFractal(noise, 6.73, 2.16, 0.53);
#if 0
  auto noise1 = mi::noise_generators::Circle<mi::Differentiable<double>>();
  auto noise2 = mi::noise_generators::CliffordTorus<mi::Differentiable<double>>()(noise1);
  auto value = noise2({mi::Differentiable<double>{0.4, 1}});
  auto deriv = mi::differentials(value);
  for (size_t i = 0; i < deriv.rows(); i++) {
    for (size_t j = 0; j < deriv.cols(); j++) {
      std::cout << deriv(i, j) << ' ';
    }
    std::cout << std::endl;
  }
#endif
}
