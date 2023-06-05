#include "Microcosm/Geometry/Mesh"
#include "testing.h"

TEST_CASE("Mesh") {
  constexpr float Pi = mi::constants::Pi<float>;
  constexpr float FourPi = 4 * Pi;
  constexpr float FourPiOverThree = FourPi / 3;

  SUBCASE("Area and volume") {
    SUBCASE("Sphere") {
      auto mesh = mi::geometry::Mesh::makeSphere(256, 128, 1.0f);
      CHECK(mesh.area() == Approx(FourPi).epsilon(1e-3f));
      CHECK(mesh.oneSideProjectedArea(mi::Vector3f::unitZ()) == Approx(Pi).epsilon(1e-3f));
      CHECK(mesh.twoSideProjectedArea(mi::Vector3f::unitZ()) == Approx(2 * Pi).epsilon(1e-3f));
      CHECK(mesh.volume() == Approx(FourPiOverThree).epsilon(1e-3f));
    }

    SUBCASE("Cube") {
      auto mesh = mi::geometry::Mesh::makeCube();
      CHECK(mesh.area() == Approx(24));
      CHECK(mesh.volume() == Approx(8));
    }
  }

  SUBCASE("Mass data") {
    SUBCASE("Sphere") {
      mi::Matrix3f correctInertia = {
        {0.4f * FourPiOverThree, 0.0f, 0.0f}, {0.0f, 0.4f * FourPiOverThree, 0.0f}, {0.0f, 0.0f, 0.4f * FourPiOverThree}};
      auto mesh = mi::geometry::Mesh::makeSphere(256, 128, 1.0f);
      mesh.translate(mi::Vector3f(1, 2, 3));
      auto massData = mesh.massData();
      CHECK(massData.mass == Approx(FourPiOverThree).epsilon(1e-3f));
      CHECK(mi::isNear<1e-4f>(massData.center, mi::Vector3f(1, 2, 3)));
      CHECK(mi::isNear<1e-3f>(massData.inertia, correctInertia));
    }

    SUBCASE("Sphere (Sphube)") {
      mi::Matrix3f correctInertia = {
        {0.4f * FourPiOverThree, 0.0f, 0.0f}, {0.0f, 0.4f * FourPiOverThree, 0.0f}, {0.0f, 0.0f, 0.4f * FourPiOverThree}};
      auto mesh = mi::geometry::Mesh::makeSphube(6);
      mesh.translate(mi::Vector3f(1, 2, 3));
      auto massData = mesh.massData();
      CHECK(massData.mass == Approx(FourPiOverThree).epsilon(1e-3f));
      CHECK(mi::isNear<1e-4f>(massData.center, mi::Vector3f(1, 2, 3)));
      CHECK(mi::isNear<1e-3f>(massData.inertia, correctInertia));
    }

    SUBCASE("Cube") {
      mi::Matrix3f correctInertia = {{5.33333f, 0.0f, 0.0f}, {0.0f, 5.33333f, 0.0f}, {0.0f, 0.0f, 5.33333f}};
      auto mesh = mi::geometry::Mesh::makeCube();
      auto massData = mesh.massData();
      CHECK(massData.mass == Approx(8));
      CHECK(mi::isNear<1e-5f>(massData.center, mi::Vector3f(0, 0, 0)));
      CHECK(mi::isNear<1e-5f>(massData.inertia, correctInertia));
    }

    SUBCASE("Rectangular prism with arbitrary rotation") {
      auto rotation = mi::Quaternionf::rotate(1.4f, mi::Vector3f(1, -1, 2));
      auto mesh = mi::geometry::Mesh::makeCube();
      mesh.scale(mi::Vector3f(1, 2, 3));
      mesh.transform(rotation);
      mesh.translate(mi::Vector3f(3, 4, 5));
      auto massData = mesh.massData();
      auto [matrixU, vectorS] = massData.principalInertia();
      CHECK(massData.mass == Approx(48));
      CHECK(vectorS[0] == Approx(208));
      CHECK(vectorS[1] == Approx(160));
      CHECK(vectorS[2] == Approx(80));
      CHECK(mi::abs(mi::dot(matrixU.col(0), rotation.basisX())) == Approx(1));
      CHECK(mi::abs(mi::dot(matrixU.col(1), rotation.basisY())) == Approx(1));
      CHECK(mi::abs(mi::dot(matrixU.col(2), rotation.basisZ())) == Approx(1));
    }
  }
}
