#include "Microcosm/UI/Rect"
#include "testing.h"

TEST_CASE("Rect") {
  SUBCASE("Anchor") {
    CHECK(mi::ui::Anchor::NE == (mi::ui::Anchor::N | mi::ui::Anchor::E));
    CHECK(mi::ui::Anchor::NW == (mi::ui::Anchor::N | mi::ui::Anchor::W));
    CHECK(mi::ui::Anchor::SW == (mi::ui::Anchor::S | mi::ui::Anchor::W));
    CHECK(mi::ui::Anchor::SE == (mi::ui::Anchor::S | mi::ui::Anchor::E));
    CHECK(~mi::ui::Anchor::N == mi::ui::Anchor::S);
    CHECK(~mi::ui::Anchor::S == mi::ui::Anchor::N);
    CHECK(~mi::ui::Anchor::E == mi::ui::Anchor::W);
    CHECK(~mi::ui::Anchor::W == mi::ui::Anchor::E);
    CHECK(~mi::ui::Anchor::NE == mi::ui::Anchor::SW);
    CHECK(~mi::ui::Anchor::NW == mi::ui::Anchor::SE);
    CHECK(~mi::ui::Anchor::SW == mi::ui::Anchor::NE);
    CHECK(~mi::ui::Anchor::SE == mi::ui::Anchor::NW);
    CHECK(~mi::ui::Anchor::Center == mi::ui::Anchor::Center);
    CHECK(mi::allTrue(mi::ui::anchorToVector(mi::ui::Anchor::N) == mi::Vector2f(0, +1)));
    CHECK(mi::allTrue(mi::ui::anchorToVector(mi::ui::Anchor::S) == mi::Vector2f(0, -1)));
    CHECK(mi::allTrue(mi::ui::anchorToVector(mi::ui::Anchor::E) == mi::Vector2f(+1, 0)));
    CHECK(mi::allTrue(mi::ui::anchorToVector(mi::ui::Anchor::W) == mi::Vector2f(-1, 0)));
    CHECK(mi::allTrue(mi::ui::anchorToVector(mi::ui::Anchor::NE) == mi::Vector2f(+1, +1)));
    CHECK(mi::allTrue(mi::ui::anchorToVector(mi::ui::Anchor::NW) == mi::Vector2f(-1, +1)));
    CHECK(mi::allTrue(mi::ui::anchorToVector(mi::ui::Anchor::SW) == mi::Vector2f(-1, -1)));
    CHECK(mi::allTrue(mi::ui::anchorToVector(mi::ui::Anchor::SE) == mi::Vector2f(+1, -1)));
    CHECK(mi::length(mi::ui::anchorToDirection(mi::ui::Anchor::NE)) == doctest::Approx(1));
    CHECK(mi::length(mi::ui::anchorToDirection(mi::ui::Anchor::NW)) == doctest::Approx(1));
    CHECK(mi::length(mi::ui::anchorToDirection(mi::ui::Anchor::SW)) == doctest::Approx(1));
    CHECK(mi::length(mi::ui::anchorToDirection(mi::ui::Anchor::SE)) == doctest::Approx(1));
  }
  SUBCASE("Anchor construction") {
    for (auto anchor :
         {mi::ui::Anchor::Center,                                                     //
          mi::ui::Anchor::N, mi::ui::Anchor::S, mi::ui::Anchor::E, mi::ui::Anchor::W, //
          mi::ui::Anchor::NE, mi::ui::Anchor::NW, mi::ui::Anchor::SE, mi::ui::Anchor::SW}) {
      mi::ui::Rect rect{anchor, {1, 2}, {7, 12}};
      CHECK(mi::allTrue(rect[anchor] == mi::Vector2f(1, 2)));
      CHECK(mi::allTrue(rect.extent() == mi::Vector2f(7, 12)));
      CHECK(mi::allTrue(rect.northEdge().center() == rect.north()));
      CHECK(mi::allTrue(rect.southEdge().center() == rect.south()));
      CHECK(mi::allTrue(rect.eastEdge().center() == rect.east()));
      CHECK(mi::allTrue(rect.westEdge().center() == rect.west()));
      CHECK(rect.north()[1] == rect.top());
      CHECK(rect.south()[1] == rect.bottom());
      CHECK(rect.east()[0] == rect.right());
      CHECK(rect.west()[0] == rect.left());
    }
  }
  SUBCASE("Extent, perimeter, and area") {
    mi::ui::Rect rect{mi::ui::Anchor::Center, {}, {9, 4}};
    CHECK(rect.extentX() == 9);
    CHECK(rect.extentY() == 4);
    CHECK(rect.perimeter() == 2 * (9 + 4));
    CHECK(rect.area() == 9 * 4);
    CHECK(rect.flipX().extentX() == 9);             // Always absolute value!
    CHECK(rect.flipY().extentY() == 4);             // Always absolute value!
    CHECK(rect.flipX().perimeter() == 2 * (9 + 4)); // Always absolute value!
    CHECK(rect.flipY().area() == 9 * 4);            // Always absolute value!
  }
  SUBCASE("Alignment and margin") {
    mi::ui::Rect rect{mi::ui::Anchor::SE, {}, {9, 4}};
    mi::ui::Rect rectAlignedN = rect.alignY(mi::ui::Anchor::N, 1.0f);
    mi::ui::Rect rectAlignedS = rect.alignY(mi::ui::Anchor::S, 1.0f);
    mi::ui::Rect rectAlignedE = rect.alignX(mi::ui::Anchor::E, 1.0f);
    mi::ui::Rect rectAlignedW = rect.alignX(mi::ui::Anchor::W, 1.0f);
    CHECK(rectAlignedN.top() == 1);
    CHECK(rectAlignedN.bottom() == 1 - 4);
    CHECK(rectAlignedS.bottom() == 1);
    CHECK(rectAlignedS.top() == 1 + 4);
    CHECK(rectAlignedE.right() == 1);
    CHECK(rectAlignedE.left() == 1 - 9);
    CHECK(rectAlignedW.left() == 1);
    CHECK(rectAlignedW.right() == 1 + 9);
    CHECK(rect.marginX(1).extentX() == 11);
    CHECK(rect.marginY(1).extentY() == 6);
    CHECK(rect.marginX(-100).extentX() == 0); // Clamps at zero
    CHECK(rect.marginY(-100).extentY() == 0); // Clamps at zero
    CHECK(rect.margin(mi::ui::Anchor::E, 1).extentX() == 10);
    CHECK(rect.margin(mi::ui::Anchor::W, 1).extentX() == 10);
  }
  SUBCASE("Click hit-test") {
    mi::ui::Rect rect{mi::ui::Anchor::W, {}, {9, 4}};
    mi::Vector2f hitPoint;
    CHECK(rect.clickHitTest(0.25f, rect[mi::ui::Anchor::NE], hitPoint) == mi::ui::Anchor::NE);
    CHECK(rect.clickHitTest(0.25f, rect[mi::ui::Anchor::NW], hitPoint) == mi::ui::Anchor::NW);
    CHECK(rect.clickHitTest(0.25f, rect[mi::ui::Anchor::SW], hitPoint) == mi::ui::Anchor::SW);
    CHECK(rect.clickHitTest(0.25f, rect[mi::ui::Anchor::SE], hitPoint) == mi::ui::Anchor::SE);
    CHECK(rect.clickHitTest(0.25f, rect(1, 0.75f) + mi::Vector2f(0.1f, 0), hitPoint) == mi::ui::Anchor::E);
    CHECK(rect.clickHitTest(0.25f, rect(0, 0.25f) - mi::Vector2f(0.1f, 0), hitPoint) == mi::ui::Anchor::W);
  }
}
