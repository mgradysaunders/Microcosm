#include "Microcosm/miniz"
#include "testing.h"

TEST_CASE("miniz") {
  auto prng = PRNG();
  mi::miniz::Bytes bytes;
  bytes.resize(700233);
  for (auto &byte : bytes) byte = std::byte(prng(255));
  SUBCASE("Test deflate/inflate") {
    auto deflated = mi::miniz::deflate(bytes);
    auto inflated = mi::miniz::inflate(deflated);
    CHECK(bytes != deflated);
    CHECK(bytes == inflated);
  }
  SUBCASE("Test deflate/inflate on streams") {
    auto stream = std::make_shared<std::stringstream>();
    auto inflated = mi::miniz::Bytes();
    inflated.resize(bytes.size());
    CHECK_NOTHROW(mi::miniz::StreamDeflator(stream).write(bytes));
    CHECK_NOTHROW(mi::miniz::StreamInflator(stream).read(inflated.data(), inflated.size()));
    CHECK(bytes == inflated);
  }
}
