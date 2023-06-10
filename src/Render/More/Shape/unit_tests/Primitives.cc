#include "Microcosm/Render/More/Shape/Primitives"
#include "testing.h"

TEST_CASE("Primitives") {
  auto VerifyParameterization = [](const auto &primitive, mi::Vector2d parameters) {
    mi::render::Manifold manifold{primitive.parameterization(parameters)};
    mi::render::Manifold manifoldDX(primitive.parameterization(parameters + mi::Vector2d(1e-7, 0)));
    mi::render::Manifold manifoldDY(primitive.parameterization(parameters + mi::Vector2d(0, 1e-7)));
    mi::Vector3d tangentX = (manifoldDX.point - manifold.point) / 1e-7;
    mi::Vector3d tangentY = (manifoldDY.point - manifold.point) / 1e-7;
    CHECK(mi::isNear<1e-5>(manifold.correct.tangents[0], tangentX));
    CHECK(mi::isNear<1e-5>(manifold.correct.tangents[1], tangentY));
    CHECK(mi::isNear<1e-6>(manifold.correct.parameters, parameters));
  };
  SUBCASE("Parameterization") {
    SUBCASE("Disk") { VerifyParameterization(mi::render::Disk{1.456}, {0.3, 0.4}); }
    SUBCASE("Sphere") { VerifyParameterization(mi::render::Sphere{2.331}, {0.7, 0.2}); }
    SUBCASE("Cylinder") { VerifyParameterization(mi::render::Cylinder{0.337, -1.211, 0.506}, {0.6, 0.1}); }
  }
  SUBCASE("Triangle") {
    mi::render::Triangle triangle{
      {0.22840985, 0.80467911, -0.09984538}, //
      {0.25033788, -0.53025618, 0.22119837}, //
      {-0.64113522, -0.7732893, -0.76438761}};
    mi::Vector3d center{(triangle[0] + triangle[1] + triangle[2]) / 3.0};
    mi::Vector3d normal{mi::cross(triangle[1] - triangle[0], triangle[2] - triangle[0])};
    SUBCASE("Intersect") {
      mi::render::Manifold manifold{};
      mi::Ray3d ray{center + 148.312 * normal, -83.409 * normal};
      std::optional<double> param{triangle.intersect(ray, manifold)};
      CHECK(param.has_value());
      CHECK(mi::isNear<1e-6>(ray(*param), center));
      CHECK(mi::isNear<1e-6>(ray(*param), manifold.point));
      CHECK(triangle.intersect({triangle[0] + 679.66 * normal, -947.21 * normal}, manifold).has_value());
      CHECK(triangle.intersect({triangle[1] + 315.90 * normal, -553.09 * normal}, manifold).has_value());
      CHECK(triangle.intersect({triangle[2] + 353.47 * normal, -148.31 * normal}, manifold).has_value());
    }
  }
}
