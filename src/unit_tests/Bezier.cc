#include "Microcosm/Bezier"
#include "testing.h"

TEST_CASE("Bezier") {
  SUBCASE("Nearest") {
    mi::Pcg32 random;
    mi::Bezier3d<3> curve;
    curve[0] = mi::Vector3d(random) * 2 - 1;
    curve[1] = mi::Vector3d(random) * 2 - 1;
    curve[2] = mi::Vector3d(random) * 2 - 1;
    curve[3] = mi::Vector3d(random) * 2 - 1;

    auto bruteForceNearestTo = [&](mi::Vector3d point) {
      mi::Vector3d bestPoint;
      double bestDistSq{mi::constants::Inf<double>};
      for (size_t i = 0; i <= 10000; i++) {
        mi::Vector3d thisPoint{curve(i / 10000.0)};
        double thisDistSq{mi::distanceSquare(thisPoint, point)};
        if (bestDistSq > thisDistSq) {
          bestDistSq = thisDistSq;
          bestPoint = thisPoint;
        }
      }
      return bestPoint;
    };
    mi::Vector3d point{random};
    CHECK(mi::isNear<1e-3>(curve.nearestTo(point).point, bruteForceNearestTo(point)));
  }
}
