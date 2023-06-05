#include "Microcosm/Tensor"

#include "doctest.h"
#include <iostream>
using doctest::Approx;
using doctest::getContextOptions;

// Verify no unecessary structure padding.
static_assert(sizeof(mi::Vector2f) == 8);
static_assert(sizeof(mi::Vector3f) == 12);
static_assert(sizeof(mi::Vector4f) == 16);

TEST_CASE("Tensor") {
  SUBCASE("Type transformations") {
    static_assert(std::is_same_v<mi::Vector3d, std::common_type_t<mi::Vector3i, double>>);
    static_assert(std::is_same_v<mi::Vector3d, std::common_type_t<mi::Vector3f, mi::Vector3d>>);
    static_assert(std::is_same_v<double, mi::to_float_t<mi::Vector3d>>);
  }
  SUBCASE("Tensor shape") {
    mi::TensorShape<2, 3> shapeA;
    mi::TensorShape<4, 5> shapeB;
    static_assert(std::same_as<std::decay_t<decltype(shapeA.append(shapeB))>, mi::TensorShape<2, 3, 4, 5>>);
    static_assert(std::same_as<std::decay_t<decltype(shapeA.plus(shapeB))>, mi::TensorShape<6, 8>>);

    mi::TensorShape<mi::Dynamic, 3> shapeC;
    CHECK(shapeC.sizes()[0] == 0);
    CHECK(shapeC.sizes()[1] == 3);
  }
  SUBCASE("Vector usage") {
    mi::Vector<int, 4> vectorU = {0, 1, 2, 3};
    CHECK(vectorU[0] == 0);
    CHECK(vectorU[1] == 1);
    CHECK(vectorU[2] == 2);
    CHECK(vectorU[3] == 3);

    vectorU(mi::Slice<2, mi::ToEnd>()) = {4, 5};
    CHECK(vectorU[2] == 4);
    CHECK(vectorU[3] == 5);

    CHECK(mi::allTrue(mi::Vector2i::unitX() == mi::Vector2i(1, 0)));
    CHECK(mi::allTrue(mi::Vector3f::unitY() == mi::Vector3f(0, 1, 0)));
    CHECK(mi::allTrue(mi::Vector3f::unitZ() == mi::Vector3f(0, 0, 1)));
    CHECK(mi::allTrue(mi::Vector2f(mi::Vector3i(2, 3, 4)) == mi::Vector2f(2, 3)));
    CHECK(mi::allTrue(mi::Vector2i(1, 2).append(3) == mi::Vector3i(1, 2, 3)));
  }
  SUBCASE("Matrix usage") {
    mi::Matrix<int, 2, 3> matrixA = {{1, 2, 3}, {4, 5, 6}};
    CHECK(matrixA.rows() == 2);
    CHECK(matrixA.cols() == 3);
    CHECK(matrixA(0, 0) == 1);
    CHECK(matrixA(0, 1) == 2);
    CHECK(matrixA(0, 2) == 3);
    CHECK(matrixA(1, 0) == 4);
    CHECK(matrixA(1, 1) == 5);
    CHECK(matrixA(1, 2) == 6);

    // Edit
    matrixA.col(0) = {7, 8};
    matrixA.row(1)(mi::Slice<>{1, 3}) = {11, 12};
    CHECK(matrixA(0, 0) == 7);
    CHECK(matrixA(0, 1) == 2);
    CHECK(matrixA(0, 2) == 3);
    CHECK(matrixA(1, 0) == 8);
    CHECK(matrixA(1, 1) == 11);
    CHECK(matrixA(1, 2) == 12);

    // Look at diagonal and transpose
    CHECK(mi::diag(matrixA).size() == 2);
    CHECK(mi::diag(matrixA)[0] == 7);
    CHECK(mi::diag(matrixA)[1] == 11);
    CHECK(&matrixA(1, 1) == &mi::diag(matrixA)[1]);
    CHECK(&matrixA(1, 2) == &mi::transpose(matrixA)(2, 1));

    SUBCASE("From rows/cols") {
      mi::Matrix<int, 2, 3> fromRows{mi::from_rows, matrixA.row(0), matrixA.row(1)};
      mi::Matrix<int, 2, 3> fromCols{mi::from_cols, matrixA.col(0), matrixA.col(1), matrixA.col(2)};
      CHECK(mi::allTrue(matrixA == fromRows));
      CHECK(mi::allTrue(matrixA == fromCols));
    }
  }

  SUBCASE("Dot product, vector with vector") {
    mi::Vector<int, 3> vectorU = {1, 2, 3};
    mi::Vector<int, mi::Dynamic> vectorV = {3, 5, 7};
    CHECK(mi::dot(vectorU, vectorV * 2 + 1) == 1 * (3 * 2 + 1) + 2 * (5 * 2 + 1) + 3 * (7 * 2 + 1));
    CHECK(mi::dot(vectorU, vectorV) == mi::trace(mi::outer(vectorU, vectorV)));
  }

  SUBCASE("Geometric") {
    mi::Vector3f vectorU = {+1, +2, +3};
    mi::Vector3f vectorV = {+2, +0, +0};
    mi::Vector3f vectorW = {+0, +7, +0};
    mi::Vector3f vectorX = {-5, +7, -1};
    CHECK(mi::length(mi::normalize(vectorU)) == Approx(1));
    CHECK(mi::length(mi::clampLength(vectorU, 0.0f, 0.5f)) == Approx(0.5f));
    CHECK(mi::length(mi::clampLength(vectorU, 7.5f, 8.5f)) == Approx(7.5f));
    CHECK(mi::allTrue(mi::clampLength(mi::Vector3f(), 1, 2) == mi::Vector3f(1, 0, 0)));
    CHECK(mi::angleBetween(vectorV, vectorW) == Approx(90.0_degreesf));
    CHECK(
      mi::angleBetween(vectorU, vectorX) == Approx(mi::angleBetweenUnitLength(mi::normalize(vectorU), mi::normalize(vectorX))));
    CHECK(mi::allTrue(mi::hodge(mi::Vector2f(1, 0)) == mi::Vector2f(0, 1)));
    CHECK(mi::allTrue(mi::cross(vectorU, vectorX) == mi::dot(mi::hodge(vectorU), vectorX)));
    CHECK(mi::cross(mi::Vector2f(1, 0), mi::Vector2f(0, 1)) == 1);
    // clang-format off
    CHECK(mi::allTrue(
           mi::hodge(mi::cross(vectorU, vectorX)) ==
           mi::dot(mi::hodge(vectorU), mi::hodge(vectorX)) - 
           mi::dot(mi::hodge(vectorX), mi::hodge(vectorU)))); // Lie-bracket identity
    // clang-format on
    CHECK(mi::isNear<1e-5>(mi::unitCircleLinspace(5, 0.0, 90.0_degrees)[0], mi::Vector2d(1, 0)));
    CHECK(mi::isNear<1e-5>(mi::unitCircleLinspace(5, 0.0, 90.0_degrees)[4], mi::Vector2d(0, 1)));
    CHECK(mi::isNear<1e-5>(mi::unitCircleLinspace(5, 0.0, mi::Exclusive(90.0_degrees))[0], mi::Vector2d(1, 0)));
    CHECK(mi::isNear<1e-5>(mi::unitCircleLinspace(5, mi::Exclusive(0.0), 90.0_degrees)[4], mi::Vector2d(0, 1)));
  }

  SUBCASE("Decomp Cholesky") {
    SUBCASE("Zero 3x3") {
      mi::DecompChol decomp{mi::Matrix3f{}};
      CHECK(mi::isNearIdentity<0.0f>(decomp.matrixP()));
      CHECK(mi::isNearZero<0.0f>(decomp.matrixL()));
    }
    SUBCASE("Identity 3x3") {
      mi::DecompChol decomp{mi::identity<float>(mi::TensorShape<3, 3>())};
      CHECK(mi::isNearIdentity<0.0f>(decomp.matrixP()));
      CHECK(mi::isNearIdentity<0.0f>(decomp.matrixL()));
    }
    SUBCASE("Non-trivial 4x4") {
      mi::Matrix4f matrixX = {{14, +8, +9, -3}, {+8, 12, +3, +2}, {+9, +3, +9, -3}, {-3, +2, -3, 10}};
      mi::DecompChol decomp{matrixX};
      CHECK(mi::isNear<1e-5f>(
        matrixX, mi::dot(decomp.matrixP(), decomp.matrixL(), mi::adjoint(decomp.matrixL()), mi::adjoint(decomp.matrixP()))));
      CHECK(mi::isNearIdentity<1e-5f>(mi::dot(matrixX, decomp.inverse())));
      CHECK(decomp.determinant() == Approx(2025));
    }
  }

  SUBCASE("Decomp LU") {
    SUBCASE("Non-trivial 4x4") {
      mi::Matrix4f matrixX = {{+1, +0, -3, -5}, {+7, +2, -1, -1}, {-4, -3, +0, +0}, {+8, +5, +2, +1}};
      mi::DecompLU decomp{matrixX};
      mi::Matrix4f matrixP = decomp.matrixP();
      mi::Matrix4f matrixL = decomp.matrixL();
      mi::Matrix4f matrixU = decomp.matrixU();
      mi::Vector4f vectorB = mi::Vector4f(-2, -1, +3, +4);
      CHECK(mi::isNear<1e-5f>(matrixX, mi::dot(matrixP, matrixL, matrixU)));
      CHECK(mi::isNear<2e-5f>(vectorB, mi::dot(matrixX, decomp.solve(vectorB))));
      CHECK(mi::isNearIdentity<1e-5f>(mi::dot(matrixX, decomp.inverse())));
      CHECK(decomp.determinant() == Approx(-96));
    }
  }

  SUBCASE("Decomp QR") {
    SUBCASE("Zero 7x4") {
      mi::DecompQR decomp{mi::Matrix<float, 7, 4>{}};
      CHECK(mi::isNearIdentity<0.0f>(decomp.matrixQ()));
      CHECK(mi::isNearZero<0.0f>(decomp.matrixR()));
    }
    SUBCASE("Identity 3x5") {
      mi::DecompQR decomp{mi::identity<float>(mi::TensorShape<3, 5>())};
      CHECK(mi::isNearIdentity<0.0f>(decomp.matrixQ()));
      CHECK(mi::isNearIdentity<0.0f>(decomp.matrixR()));
    }
    SUBCASE("Non-trivial 3x7") {
      mi::Matrix<float, 3, 7> matrixX = {
        {+3, -7, -4, -2, +7, -3, +5}, {-3, +1, -1, -4, -4, +1, +6}, {-5, -4, -6, -6, -7, -3, +3}};
      mi::DecompQR decomp{matrixX};
      mi::Matrix3f matrixQ = decomp.matrixQ();
      mi::Matrix<float, 3, 7> matrixR = decomp.matrixR();
      CHECK(mi::isNear<1e-5f>(matrixX, mi::dot(matrixQ, matrixR)));
      CHECK(mi::isNearUnitary<1e-5f>(matrixQ));
    }
  }

  SUBCASE("Decomp SVD") {
    SUBCASE("Non-trivial 2x2") {
      mi::Matrix<float, 2, 2> matrixX = {{0.93406f, 0.09446f}, {0.94537f, 0.42963f}};
      mi::DecompSVD decomp{matrixX};
      CHECK(decomp.singularValue(0) == doctest::Approx(1.38155603));
      CHECK(decomp.singularValue(1) == doctest::Approx(0.22583097));
      CHECK(decomp.conditionNumber() == doctest::Approx(6.1176553));
      CHECK(mi::isNearUnitary<1e-5f>(decomp.matrixU()));
      CHECK(mi::isNearUnitary<1e-5f>(decomp.matrixV()));
      CHECK(mi::isNear<1e-5f>(matrixX, mi::dot(decomp.matrixU(), decomp.matrixS(), decomp.matrixV())));
      CHECK(mi::isNearIdentity<1e-5f>(mi::dot(matrixX, decomp.pseudoInverse())));
    }
    SUBCASE("Non-trivial 3x3") {
      mi::Matrix<float, 3, 3> matrixX = {
        {+0.10160f, +0.41630f, -0.41819f}, //
        {+0.02166f, +0.78589f, +0.79259f}, //
        {-0.74883f, -0.58551f, -0.89707f}};
      mi::DecompSVD decomp{matrixX};
      CHECK(decomp.singularValue(0) == doctest::Approx(1.6394387));
      CHECK(decomp.singularValue(1) == doctest::Approx(0.6073009));
      CHECK(decomp.singularValue(2) == doctest::Approx(0.5064815));
      CHECK(decomp.conditionNumber() == doctest::Approx(3.2369169));
      CHECK(mi::isNearUnitary<1e-5f>(decomp.matrixU()));
      CHECK(mi::isNearUnitary<1e-5f>(decomp.matrixV()));
      CHECK(mi::isNear<1e-5f>(matrixX, mi::dot(decomp.matrixU(), decomp.matrixS(), decomp.matrixV())));
      CHECK(mi::isNearIdentity<1e-5f>(mi::dot(matrixX, decomp.pseudoInverse())));
    }
    SUBCASE("Non-trivial 4x6") {
      mi::Matrix<float, 4, 6> matrixX = {
        {+1, -7, +3, -2, +5, +8}, //
        {+3, +4, +7, +3, +3, +4}, //
        {-9, -3, +1, +2, -6, +3}, //
        {+7, -6, +1, +7, -6, +5}};
      mi::DecompSVD decomp{matrixX};
      CHECK(decomp.singularValue(0) == doctest::Approx(15.54617011));
      CHECK(decomp.singularValue(1) == doctest::Approx(12.82219201));
      CHECK(decomp.singularValue(2) == doctest::Approx(10.87871816));
      CHECK(decomp.singularValue(3) == doctest::Approx(8.45940175));
      CHECK(decomp.conditionNumber() == doctest::Approx(1.837737));
      CHECK(mi::isNearUnitary<1e-5f>(decomp.matrixU()));
      CHECK(mi::isNearUnitary<1e-5f>(decomp.matrixV()));
      CHECK(mi::isNear<1e-5f>(matrixX, mi::dot(decomp.matrixU(), decomp.matrixS(), decomp.matrixV())));
      CHECK(mi::isNearIdentity<1e-5f>(mi::dot(matrixX, decomp.pseudoInverse())));
      CHECK(decomp.conditionNumber() == Approx(1.83774));
    }
    SUBCASE("Non-trivial 4x3 with complex numbers") {
      using namespace std::complex_literals;
      mi::Matrix<std::complex<double>, 4, 3> matrixX = {
        {-0.12801 - 0.73084i, -0.94815 + 0.02716i, +0.09932 - 0.63112i},
        {-0.12936 + 0.57067i, -0.15926 + 0.70795i, -0.33933 - 0.01153i},
        {-0.59070 + 0.69312i, +0.23854 - 0.84071i, -0.40069 + 0.01049i},
        {-0.46635 - 0.86943i, +0.24227 - 0.14376i, +0.05828 - 0.80694i}};
      mi::DecompSVD decomp{matrixX};
      CHECK(decomp.singularValue(0) == doctest::Approx(1.98247757));
      CHECK(decomp.singularValue(1) == doctest::Approx(1.50832428));
      CHECK(decomp.singularValue(2) == doctest::Approx(0.31365814));
      CHECK(decomp.conditionNumber() == doctest::Approx(6.320504087130698));
    }
    SUBCASE("More tests for nullspace and pseudo-inverse") {
      // Note the 1e-5 tolerances here reflect the fact that this matrix (generated
      // randomly with NumPy) is hardcoded in with only 5 decimal places of precision.
      mi::Matrix<double, 5, 5> matrixX = {
        {0.53411, 0.72032, 0.00011, 0.13613, 0.14676},
        {0.30390, 0.18626, 0.34556, 0.12681, 0.53882},
        {0.44275, 0.68522, 0.20445, 0.30621, 0.02739},
        {0.29879, 0.41730, 0.55869, 0.43598, 0.19810},
        {0.95827, 0.96826, 0.31342, 0.19855, 0.87639}};
      mi::DecompSVD decomp{matrixX};
      CHECK(decomp.rank<1e-5>() == 3);
      CHECK(decomp.singularValue(0) == doctest::Approx(2.23951900e+00));
      CHECK(decomp.singularValue(1) == doctest::Approx(6.38865911e-01));
      CHECK(decomp.singularValue(2) == doctest::Approx(5.84585604e-01));
      CHECK(decomp.singularValue(3) == doctest::Approx(0));
      CHECK(decomp.singularValue(4) == doctest::Approx(0));
      auto nullSpaceU = decomp.nullMatrixU<1e-5>();
      auto nullSpaceV = decomp.nullMatrixV<1e-5>();
      CHECK(nullSpaceU.rows() == 5);
      CHECK(nullSpaceU.cols() == 2);
      CHECK(nullSpaceV.rows() == 2);
      CHECK(nullSpaceV.cols() == 5);
      mi::Vector<double, 5> nullVectorU = 2 * nullSpaceU.col(0) + 3 * nullSpaceU.col(1);
      mi::Vector<double, 5> nullVectorV = 2 * nullSpaceV.row(0) + 3 * nullSpaceV.row(1);
      CHECK(mi::isNear<5e-5>(mi::dot(nullVectorU, matrixX), mi::Vector<double, 5>(0)));
      CHECK(mi::isNear<5e-5>(mi::dot(matrixX, nullVectorV), mi::Vector<double, 5>(0)));
      mi::Vector<double, 5> vectorX = +0.55 * matrixX.col(0) + -0.25 * matrixX.col(1) + -0.97 * matrixX.col(2) +
                                      +0.11 * matrixX.col(3) + -0.44 * matrixX.col(4);
      CHECK(mi::isNear<1e-10>(decomp.solve<1e-5>(vectorX), mi::dot(decomp.pseudoInverse<1e-5>(), vectorX)));
      CHECK(mi::isNear<1e-10>(decomp.solve<1e-5>(mi::identity<double>(mi::TensorShape<5, 5>())), decomp.pseudoInverse<1e-5>()));
    }
  }

  SUBCASE("Color") {
    SUBCASE("Encode/decode sRGB") {
      mi::Vector4f color = {0.8f, 0.1f, 0.4f, 0.5f};
      mi::Vector4f afterEncode = mi::encodeSRGB(color);
      mi::Vector4f afterDecode = mi::decodeSRGB(afterEncode);
      CHECK(mi::isNear<1e-5f>(color, afterDecode));
      CHECK(color[3] == afterEncode[3]); // Don't mess with alpha
      CHECK(color[3] == afterDecode[3]);
    }
    SUBCASE("Convert RGB/XYZ") {
      mi::Vector3f color = {0.7f, 0.4f, 0.3f};
      mi::Vector3f convert = mi::convertXYZToRGB(color);
      mi::Vector3f recover = mi::convertRGBToXYZ(convert);
      CHECK(mi::isNear<1e-5f>(color, convert) == false);
      CHECK(mi::isNear<1e-5f>(color, recover));
      const mi::Vector2f standardCr = {0.64f, 0.33f};
      const mi::Vector2f standardCg = {0.30f, 0.60f};
      const mi::Vector2f standardCb = {0.15f, 0.06f};
      const mi::Vector3f standardW = {0.95047f, 1.0f, 1.08883f};
      const mi::Matrix3f standardM = {
        {0.412456f, 0.357576f, 0.180438f}, //
        {0.212673f, 0.715152f, 0.072175f}, //
        {0.019334f, 0.119192f, 0.950304f}};
      CHECK(mi::isNear<1e-5f>(mi::convertRGBToXYZ(standardCr, standardCg, standardCb, standardW), standardM));
    }
    SUBCASE("Convert XYZ/xyY") {
      mi::Vector3f color = {0.1f, 0.8f, 0.4f};
      mi::Vector3f convert = mi::convertXYZToXYY(color);
      mi::Vector3f recover = mi::convertXYYToXYZ(convert);
      CHECK(mi::isNear<1e-5f>(color, convert) == false);
      CHECK(mi::isNear<1e-5f>(color, recover));
      mi::Vector3f zero = {};
      CHECK(mi::allTrue(zero == mi::convertXYZToXYY(zero)));
      CHECK(mi::allTrue(zero == mi::convertXYYToXYZ(zero)));
    }
    SUBCASE("Convert XYZ/LMS") {
      mi::Vector3f color = {0.2f, 0.8f, 0.9f};
      mi::Vector3f convert = mi::convertXYZToLMS(color);
      mi::Vector3f recover = mi::convertLMSToXYZ(convert);
      CHECK(mi::isNear<1e-5f>(color, convert) == false);
      CHECK(mi::isNear<1e-5f>(color, recover));
    }
    SUBCASE("Convert XYZ/LAB") {
      mi::Vector3f color = {0.2e-4f, 0.8f, 0.9f};
      mi::Vector3f convert = mi::convertXYZToLAB(color);
      mi::Vector3f recover = mi::convertLABToXYZ(convert);
      CHECK(mi::isNear<1e-5f>(color, convert) == false);
      CHECK(mi::isNear<1e-5f>(color, recover));
    }
    SUBCASE("Convert RGB/LAB") {
      mi::Vector3f color = {0.6f, 0.8e-2f, 0.3f};
      mi::Vector3f convert = mi::convertRGBToLAB(color);
      mi::Vector3f recover = mi::convertLABToRGB(convert);
      CHECK(mi::isNear<1e-5f>(color, convert) == false);
      CHECK(mi::isNear<1e-5f>(color, recover));
      mi::Vector3f convert2 = mi::convertLABToLCH(convert);
      mi::Vector3f recover2 = mi::convertLCHToLAB(convert2);
      CHECK(mi::isNear<1e-5f>(convert, convert2) == false);
      CHECK(mi::isNear<1e-5f>(convert, recover2));
    }
  }

  SUBCASE("Combinations") {
    CHECK(mi::combination<5, 3>(0) == mi::IndexVector{0, 1, 2});
    CHECK(mi::combination<5, 3>(1) == mi::IndexVector{0, 1, 3});
    CHECK(mi::combination<5, 3>(2) == mi::IndexVector{0, 1, 4});
    CHECK(mi::combination<5, 3>(3) == mi::IndexVector{0, 2, 3});
    CHECK(mi::combination<5, 3>(4) == mi::IndexVector{0, 2, 4});
    CHECK(mi::combination<5, 3>(5) == mi::IndexVector{0, 3, 4});
    CHECK(mi::combination<5, 3>(6) == mi::IndexVector{1, 2, 3});
    CHECK(mi::combination<5, 3>(7) == mi::IndexVector{1, 2, 4});
    CHECK(mi::combination<5, 3>(8) == mi::IndexVector{1, 3, 4});
    CHECK(mi::combination<5, 3>(9) == mi::IndexVector{2, 3, 4});
  }
}
