#include "Microcosm/Json"
#include "testing.h"

struct SomeStruct {
  struct Nested {
    float num = 0;
    std::vector<int> someInts;
    std::vector<int> someMoreInts;
    bool operator==(const Nested &) const = default;
    void jsonConversion(mi::Json::Conversion conversion) {
      conversion //
        .required("num", num)
        .optionalImplicit("someInts", someInts)
        .optionalImplicit("someMoreInts", someMoreInts);
    }
  };
  Nested nested;
  std::map<std::string, int> arbitraryLookup;
  bool operator==(const SomeStruct &) const = default;
  void jsonConversion(mi::Json::Conversion conversion) {
    conversion //
      .required("nested", nested)
      .required("arbitraryLookup", arbitraryLookup);
  }
};

TEST_CASE("Json") {
  SUBCASE("Basic usage") {
    mi::Json json;
    CHECK(bool(json) == false);
    CHECK(json.kind() == mi::Json::Kind::None);
    CHECK(json.size() == 0);
    json["name"] = "Foo";
    json["info"] = 27;
    CHECK(bool(json) == true);
    CHECK(json.kind() == mi::Json::Kind::Table);
    CHECK(json.size() == 2);
    CHECK(json["name"].kind() == mi::Json::Kind::String);
    CHECK(json["info"].kind() == mi::Json::Kind::Number);
    CHECK_NOTHROW(void(json.at("name")));
    CHECK_NOTHROW(void(json.at("info")));
    CHECK_THROWS(void(json.at("more_info"))); // Not defined yet
    CHECK(std::string(json["name"]) == "Foo");
    CHECK(int(json["info"]) == 27);
    CHECK(json == mi::Json::parse(json.render()));
  }

  SUBCASE("Conversion") {
    CHECK_THROWS(uint8_t(mi::Json(1000)));
    CHECK_THROWS(uint8_t(mi::Json(-1)));
    CHECK_NOTHROW(uint8_t(mi::Json(7)));
    SomeStruct someStruct;
    CHECK_NOTHROW(
      someStruct =
        mi::Json::parse(R"({"nested": {"num": 2.7, "someInts": [4, 5, 6]}, "arbitraryLookup": {"first": 11, "second": 17}})"));
    CHECK(someStruct.nested.num == Approx(2.7f));
    CHECK(someStruct.nested.someInts == std::vector<int>{4, 5, 6});
    CHECK(someStruct.nested.someMoreInts.empty());
    CHECK(someStruct.arbitraryLookup["first"] == 11);
    CHECK(someStruct.arbitraryLookup["second"] == 17);
    CHECK(someStruct == SomeStruct(mi::Json(someStruct)));
  }
}
