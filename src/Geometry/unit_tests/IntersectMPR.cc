#include "Microcosm/Geometry/IntersectMPR"
#include "Microcosm/Timer"
#include "testing.h"

TEST_CASE("IntersectMPR") {
  SUBCASE("Ellipses") {
    auto ellipseA = mi::geometry::support_functions::Ellipsoid3({1.0f, 2.1f, 0.1f}, {0.2f, 0.5f, 0.4f});
    auto ellipseB = mi::geometry::support_functions::Ellipsoid3({1.5f, 1.3f, 0.0f}, {1.0f, 0.8f, 0.6f});
    mi::geometry::IntersectMPR3 mpr;
    mpr.minkowskiDifference.centerA = ellipseA.center;
    mpr.minkowskiDifference.centerB = ellipseB.center;
    mpr.minkowskiDifference.supportFunctionA = ellipseA;
    mpr.minkowskiDifference.supportFunctionB = ellipseB;
    CHECK(mpr.run());
#if 0
    std::cout << 1e6 * mi::HighResolutionTimer::benchmark(5, [] {}, [&] { mpr.run(); }) << std::endl;
    using namespace mi::string_literals;
    auto center = mpr.penetrationCenter();
    auto offset = mpr.penetrationOffsetVector();
    std::cout << "{}"_format(mi::join(center, ", ")) << std::endl;
    std::cout << "{}"_format(mi::join(offset, ", ")) << std::endl;
#endif
  }
}
