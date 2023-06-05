#include "Microcosm/UI/Transform"
#include "testing.h"

TEST_CASE("Transform") {
  SUBCASE("Usage") {
    mi::ui::Transform transform;
    CHECK(mi::allTrue(mi::Matrix2f(transform) == mi::Matrix2f::identity()));
    CHECK(mi::allTrue(transform.translation() == 0));
    CHECK(!transform.hasTranslation());
    CHECK(!transform.hasRotation());

    // Translation only affects affine.
    transform.translate({3, 2});
    CHECK(mi::allTrue(transform.applyLinear(mi::Vector2f(1, 1)) == mi::Vector2f(1, 1)));
    CHECK(mi::allTrue(transform.applyNormal(mi::Vector2f(1, 1)) == mi::Vector2f(1, 1)));
    CHECK(mi::allTrue(transform.applyAffine(mi::Vector2f(1, 1)) == mi::Vector2f(4, 3)));
    CHECK(mi::allTrue(transform.inverse().applyAffine(mi::Vector2f(4, 3)) == mi::Vector2f(1, 1)));
    CHECK(transform.hasTranslation());
    CHECK(!transform.hasRotation());

    // Apply some arbitrary rotation and scale.
    transform.rotate(+1.4f).scale({0.8f, 1.9f}).translate({-0.3f, -1.5f});
    transform.rotate(-2.1f).scale(-0.4f);
    CHECK(transform.hasTranslation());
    CHECK(transform.hasRotation());

    // Check that forward and inverse matrices dot to the identity.
    mi::Matrix4f forwardMatrix = mi::Matrix4f(transform);
    mi::Matrix4f inverseMatrix = mi::Matrix4f(transform.inverse());
    CHECK(mi::isNear<1e-5f>(dot(forwardMatrix, inverseMatrix), mi::Matrix4f::identity()));
    CHECK(mi::isNear<1e-5f>(dot(inverseMatrix, forwardMatrix), mi::Matrix4f::identity()));

    // Check that normals transform as expected.
    CHECK(mi::dot(transform.applyLinear({1, 0}), transform.applyLinear({0, 1})) != doctest::Approx(0));
    CHECK(mi::dot(transform.applyLinear({1, 0}), transform.applyNormal({0, 1})) == doctest::Approx(0));
  }
  SUBCASE("Angle and scale calculation") {
    // Check angle and scale inversion.
    CHECK(mi::ui::Transform().rotate(1.2f).scale(0.7f).angle() == doctest::Approx(1.2f));
    CHECK(mi::ui::Transform().rotate(1.2f).scale(0.7f).scale() == doctest::Approx(0.7f));
  }
  SUBCASE("Rectilinear mappings") {
    mi::ui::Rect rectA = {{+1, +2}, {+5, +6}};
    mi::ui::Rect rectB = {{-4, +3}, {-1, -8}};
    {
      /// Map the [0,1) square onto rectangle A.
      mi::ui::Transform transform;
      transform.rectilinearForward(rectA);
      CHECK(mi::isNear<1e-5f>(transform.applyAffine({0, 0}), rectA[0]));
      CHECK(mi::isNear<1e-5f>(transform.applyAffine({1, 1}), rectA[1]));
    }
    {
      /// Map rectangle A onto the [0,1) square.
      mi::ui::Transform transform;
      transform.rectilinearInverse(rectA);
      CHECK(mi::isNear<1e-5f>(transform.applyAffine(rectA[0]), mi::Vector2f(0, 0)));
      CHECK(mi::isNear<1e-5f>(transform.applyAffine(rectA[1]), mi::Vector2f(1, 1)));
    }
    {
      // Apply rectilinear change-of-coordinates to map rectangle A onto rectangle B.
      mi::ui::Transform transform;
      transform.rectilinear(rectA, rectB);
      CHECK(mi::isNear<1e-5f>(transform.applyAffine(rectA[0]), rectB[0]));
      CHECK(mi::isNear<1e-5f>(transform.applyAffine(rectA[1]), rectB[1]));
    }
  }
}
