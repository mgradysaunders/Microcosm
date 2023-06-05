#include "Microcosm/Serializer"
#include "testing.h"

#include <list>
#include <map>
#include <vector>

struct StaticObject {
  float foo{};
  std::string bar{};
  void onSerialize(auto &serializer) { serializer <=> foo <=> bar; }
};

struct StaticArrayLikeConstantSize : mi::ArrayLike<StaticArrayLikeConstantSize> {
  StaticArrayLikeConstantSize() = default;
  StaticArrayLikeConstantSize(auto... values) : values{values...} {}
  MI_ARRAY_LIKE_CONSTEXPR_DATA(&values[0]);
  MI_ARRAY_LIKE_STATIC_CONSTEXPR_SIZE(4);
  int values[4] = {};
};

static int DynamicCtorCalls = 0, DynamicDtorCalls = 0, DynamicSerializeCalls = 0;

struct DynamicObject : public mi::Serializable {
  DynamicObject() { DynamicCtorCalls++; }
  ~DynamicObject() override { DynamicDtorCalls++; }
};

struct DynamicObjectBranch final : public DynamicObject {
  MI_DECLARE_DYNAMIC_SERIALIZABLE(DynamicObjectBranch) { serializer <=> objects, DynamicSerializeCalls++; }
  std::map<std::string, mi::RefPtr<DynamicObject>> objects;
};

struct DynamicObjectLeaf final : public DynamicObject {
  DynamicObjectLeaf() = default;
  DynamicObjectLeaf(double value) : value(value) {}
  MI_DECLARE_DYNAMIC_SERIALIZABLE(DynamicObjectLeaf) { serializer <=> value, DynamicSerializeCalls++; }
  double value{};
};

TEST_CASE("Serializer") {
  SUBCASE("Static serialization") {
    auto stream = std::make_shared<std::stringstream>();
    {
      mi::StandardSerializer serializer(static_cast<const std::shared_ptr<std::ostream> &>(stream));
      std::map<int, std::optional<std::string>> intToOptionalStr;
      intToOptionalStr[11] = std::nullopt;
      intToOptionalStr[12] = "Hello, world!";
      std::variant<double, std::vector<double>> doubleOrVectorOfDouble1 = 2.0;
      std::variant<double, std::vector<double>> doubleOrVectorOfDouble2 = std::vector{2.0, 2.5, 3.0};
      std::list<std::tuple<int, char, float>> listOfIntCharFloat;
      listOfIntCharFloat.emplace_back(std::make_tuple(1, 'A', 1.5f));
      listOfIntCharFloat.emplace_back(std::make_tuple(2, 'B', 2.5f));
      listOfIntCharFloat.emplace_back(std::make_tuple(3, 'C', 3.5f));
      StaticObject myStruct1 = {3, "Hello, world!"};
      StaticArrayLikeConstantSize myStruct2 = {5, 6, 7, 8};
      serializer <=> intToOptionalStr;
      serializer <=> doubleOrVectorOfDouble1;
      serializer <=> doubleOrVectorOfDouble2;
      serializer <=> listOfIntCharFloat;
      serializer <=> myStruct1;
      serializer <=> myStruct2;
    }
    {
      mi::StandardSerializer serializer(static_cast<const std::shared_ptr<std::istream> &>(stream));
      std::map<int, std::optional<std::string>> intToOptionalStr;
      std::variant<double, std::vector<double>> doubleOrVectorOfDouble1;
      std::variant<double, std::vector<double>> doubleOrVectorOfDouble2;
      std::list<std::tuple<int, char, float>> listOfIntCharFloat;
      StaticObject myStruct1;
      StaticArrayLikeConstantSize myStruct2;
      serializer <=> intToOptionalStr;
      serializer <=> doubleOrVectorOfDouble1;
      serializer <=> doubleOrVectorOfDouble2;
      serializer <=> listOfIntCharFloat;
      serializer <=> myStruct1;
      serializer <=> myStruct2;
      CHECK(intToOptionalStr.at(11) == std::nullopt);
      CHECK(intToOptionalStr.at(12) == "Hello, world!");
      CHECK((doubleOrVectorOfDouble1.index() == 0 && std::get<0>(doubleOrVectorOfDouble1) == 2.0));
      CHECK((doubleOrVectorOfDouble2.index() == 1 && std::get<1>(doubleOrVectorOfDouble2) == std::vector{2.0, 2.5, 3.0}));
      auto itr = listOfIntCharFloat.begin();
      CHECK(*itr++ == std::make_tuple(1, 'A', 1.5f));
      CHECK(*itr++ == std::make_tuple(2, 'B', 2.5f));
      CHECK(*itr++ == std::make_tuple(3, 'C', 3.5f));
      CHECK(myStruct1.foo == 3);
      CHECK(myStruct1.bar == "Hello, world!");
      CHECK(myStruct2.values[0] == 5);
      CHECK(myStruct2.values[1] == 6);
      CHECK(myStruct2.values[2] == 7);
      CHECK(myStruct2.values[3] == 8);
    }
  }
  SUBCASE("Dynamic serialization") {
    auto stream = std::make_shared<std::stringstream>();
    {
      auto branch1 = mi::make_ref<DynamicObjectBranch>();
      auto branch2 = mi::make_ref<DynamicObjectBranch>();
      auto leaf1 = mi::make_ref<DynamicObjectLeaf>(2.784);
      auto leaf2 = mi::make_ref<DynamicObjectLeaf>(-1.993);
      auto leaf3 = mi::make_ref<DynamicObjectLeaf>(-4.761);
      branch1->objects["Leaf1"] = leaf1;
      branch1->objects["Leaf2"] = leaf2;
      branch2->objects["Leaf3"] = leaf3;
      branch2->objects["Leaf1"] = leaf1;
      branch2->objects["Branch1"] = branch1;
      mi::StandardSerializer serializer(static_cast<const std::shared_ptr<std::ostream> &>(stream));
      serializer <=> branch2;
      CHECK(DynamicCtorCalls == DynamicSerializeCalls);
    }
    {
      DynamicCtorCalls = 0;
      DynamicDtorCalls = 0;
      mi::StandardSerializer serializer(static_cast<const std::shared_ptr<std::istream> &>(stream));
      mi::RefPtr<DynamicObject> object = serializer.deserialize<mi::RefPtr<DynamicObject>>();
      CHECK(DynamicCtorCalls == 5);
      auto *branch2 = dynamic_cast<DynamicObjectBranch *>(object.get());
      auto *branch1 = dynamic_cast<DynamicObjectBranch *>(branch2->objects.at("Branch1").get());
      auto *leaf1 = dynamic_cast<DynamicObjectLeaf *>(branch1->objects.at("Leaf1").get());
      auto *leaf2 = dynamic_cast<DynamicObjectLeaf *>(branch1->objects.at("Leaf2").get());
      auto *leaf3 = dynamic_cast<DynamicObjectLeaf *>(branch2->objects.at("Leaf3").get());
      CHECK(leaf1->value == 2.784);
      CHECK(leaf2->value == -1.993);
      CHECK(leaf3->value == -4.761);
      CHECK(leaf1 == dynamic_cast<DynamicObjectLeaf *>(branch1->objects.at("Leaf1").get())); // Pointer equality preserved!
    }
    CHECK(DynamicCtorCalls == DynamicDtorCalls);
  }
}
