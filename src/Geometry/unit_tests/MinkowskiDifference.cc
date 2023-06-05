#include "Microcosm/Geometry/MinkowskiDifference"
#include "testing.h"

TEST_CASE("MinkowskiDifference") {
  SUBCASE("Ellipsoid Support (N = 2)") {
    mi::geometry::support_functions::Ellipsoid2 ellipsoid{
      mi::Vector2f(1.0f, 2.0f),  // Center
      mi::Vector2f(0.6f, 1.4f)}; // Radius
    mi::Vector2f normalDir = mi::normalize(mi::Vector2f(0.6723f, 0.1124f));
    mi::Vector2f support = ellipsoid(normalDir);
    float maxProjection = -mi::constants::Inff;
    for (float theta : mi::linspace(512, 0.0f, 360.0_degreesf)) {
      mi::Vector2f point = {
        ellipsoid.center[0] + ellipsoid.radius[0] * mi::cos(theta), //
        ellipsoid.center[1] + ellipsoid.radius[1] * mi::sin(theta)};
      maxProjection = mi::max(maxProjection, dot(point, normalDir));
    }
    CHECK(dot(support, normalDir) == doctest::Approx(maxProjection));
  }
}
