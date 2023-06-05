#include "testing.h"

template <typename Which> static void TestPcg() {
  Which prng = {typename Which::state_type(getContextOptions()->rand_seed)};
  CHECK(prng(17) < 17);
  CHECK((prng + 103) - prng == 103);
}

TEST_CASE("random") {
  SUBCASE("Pcg16") { TestPcg<mi::Pcg16>(); }
  SUBCASE("Pcg32") { TestPcg<mi::Pcg32>(); }
  SUBCASE("Pcg64") { TestPcg<mi::Pcg64>(); }
}
