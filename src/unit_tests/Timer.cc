#include "Microcosm/Timer"
#include "testing.h"

TEST_CASE("Timer") {
  mi::Timer<std::chrono::system_clock> timer;
  SUBCASE("Read after sleep") {
    mi::sleep(1000);
    CHECK(timer.nanoseconds() > int64_t(0.999 * 1e9));
    CHECK(timer.microseconds() > int64_t(0.999 * 1e6));
    CHECK(timer.milliseconds() > int64_t(0.999 * 1e3));
    CHECK(timer.seconds() > 0.999);
  }
  SUBCASE("Read again after reset") {
    timer.reset();
    CHECK(timer.nanoseconds() < int64_t(0.999 * 1e9));
    CHECK(timer.microseconds() < int64_t(0.999 * 1e6));
    CHECK(timer.milliseconds() < int64_t(0.999 * 1e3));
    CHECK(timer.seconds() < 0.999);
  }
}
