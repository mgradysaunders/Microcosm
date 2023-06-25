#include "Microcosm/Quaternion"
#include "testing.h"

template <std::floating_point Float, typename Field> static mi::Quaternion<Field> bruteForceSlerpDeriv(Float mu, const mi::Quaternion<Field> &quaternionQ, const mi::Quaternion<Field> &quaternionR) {
  return (mi::slerp(mu + Float(0.5e-3), quaternionQ, quaternionR) - mi::slerp(mu - Float(0.5e-3), quaternionQ, quaternionR)) / Float(1e-3);
}

TEST_CASE("Quaternion") {
  SUBCASE("Angle/axis") {
    float angle = 1.2f;
    mi::Vector3f axis = mi::normalize(mi::Vector3f(1, 2, 3));
    mi::Quaternionf quat = mi::Quaternionf::rotate(angle, axis);
    CHECK(quat.rotationAngle() == doctest::Approx(angle));
    CHECK(mi::isNear<1e-5f>(quat.rotationAxis(), axis));
    CHECK(mi::isNear<1e-5f>(mi::Quaternionf::rotateX(90.0_degreesf).applyLinear(mi::Vector3f(0, 1, 0)), mi::Vector3f(0, 0, 1)));
    CHECK(mi::isNear<1e-5f>(mi::Quaternionf::rotateY(90.0_degreesf).applyLinear(mi::Vector3f(0, 0, 1)), mi::Vector3f(1, 0, 0)));
    CHECK(mi::isNear<1e-5f>(mi::Quaternionf::rotateZ(90.0_degreesf).applyLinear(mi::Vector3f(1, 0, 0)), mi::Vector3f(0, 1, 0)));
    CHECK(mi::isNear<1e-5f>(mi::Quaternionf(mi::Matrix3f(quat)), quat));
  }
  SUBCASE("From two vectors") {
    mi::Vector3f vectorU = mi::normalize(mi::Vector3f(+1, +2, +3));
    mi::Vector3f vectorV = mi::normalize(mi::Vector3f(-4, -1, +2));
    mi::Quaternionf quat = mi::Quaternionf::rotate(vectorU, vectorV);
    CHECK(mi::isNear<1e-5f>(quat.applyLinear(vectorU), vectorV));
  }
  SUBCASE("Inverse") {
    mi::Quaternionf quatQ = {1, 2, 3, 4};
    mi::Quaternionf quatR = mi::inverse(quatQ);
    CHECK(mi::isNear<1e-5f>(quatQ * quatR, mi::Quaternionf(1)));
    CHECK(mi::isNear<1e-5f>(quatR * quatQ, mi::Quaternionf(1)));
  }
  SUBCASE("Exponential") {
    mi::Quaternionf quatQ = {-0.2f, 0.1f, 0.5f, 0.3f};
    mi::Quaternionf quatR = {1, 0, 0, 0};
    mi::Quaternionf quatS = {};
    for (int k = 1; k < 100; k++) {
      quatS += quatR;
      quatR *= quatQ / k;
    }
    CHECK(mi::isNear<1e-5f>(mi::exp(quatQ), quatS));
    CHECK(mi::isNear<1e-5f>(mi::log(mi::exp(quatQ)), quatQ));
    CHECK(mi::isNear<1e-5f>(mi::pow(quatQ, 2.0f), quatQ * quatQ));
    CHECK(mi::isNear<1e-5f>(mi::pow(quatQ, 3.0f), quatQ * quatQ * quatQ));
  }
  SUBCASE("Slerp") {
    float mu = 0.723f;
    mi::Quaternionf quatQ = mi::Quaternionf::rotate(+0.45f, mi::Vector3f(+1, +4, -3));
    mi::Quaternionf quatR = mi::Quaternionf::rotate(-1.72f, mi::Vector3f(-4, -2, +2));
    mi::Quaternionf deriv = {};
    void(mi::slerp(mu, quatQ, quatR, &deriv));
    CHECK(mi::isNear<1e-3f>(deriv, bruteForceSlerpDeriv(mu, quatQ, quatR)));
    CHECK(mi::isNear<1e-5f>(quatQ, mi::slerp(0.0f, quatQ, quatR)));
    CHECK(mi::isNear<1e-5f>(quatR, mi::slerp(1.0f, quatQ, quatR)));
  }
  SUBCASE("Serialization") { CHECK(isMemcmpEqualAfterSerializeRoundTrip(mi::Quaternionf(1, 2, 3, 4))); }
}

TEST_CASE("DualQuaternion") {
  float angle = 1.2f;
  mi::Vector3f axis = mi::normalize(mi::Vector3f(1, 2, 3));
  mi::Vector3f offs = mi::Vector3f(3, 4, 5);
  mi::DualQuaternionf quat = mi::DualQuaternionf::translate(offs) * mi::DualQuaternionf::rotate(angle, axis);
  SUBCASE("Construction") {
    CHECK(quat.rotationAngle() == doctest::Approx(angle));
    CHECK(mi::isNear<1e-5f>(quat.rotationAxis(), axis));
    CHECK(mi::isNear<1e-5f>(quat.translation(), offs));
    CHECK(mi::isNear<1e-5f>(mi::DualQuaternionf(mi::Matrix4f(quat)).real(), quat.real()));
    CHECK(mi::isNear<1e-5f>(mi::DualQuaternionf(mi::Matrix4f(quat)).dual(), quat.dual()));
    CHECK(mi::isNear<1e-5f>(quat.applyAffine(mi::Vector3f(-1, 2, 7)), mi::dot(mi::Matrix4f(quat), mi::Vector4f(-1, 2, 7, 1))[mi::Slice<0, 3>()]));
  }
  SUBCASE("Inverse") {
    CHECK(mi::isNear<1e-4f>(quat * mi::inverse(quat), mi::DualQuaternionf::identity()));
    CHECK(mi::isNear<1e-4f>(mi::inverse(quat) * quat, mi::DualQuaternionf::identity()));
  }
  SUBCASE("Exponential") {
    CHECK(mi::isNear<1e-4f>(mi::exp(mi::log(quat)), quat));
    CHECK(mi::isNear<1e-4f>(mi::pow(quat, 2.0f), quat * quat));
    CHECK(mi::isNear<1e-4f>(mi::pow(quat, 3.0f), quat * quat * quat));
    CHECK(mi::isNear<1e-4f>(mi::exp(quat * 0), mi::DualQuaternionf::identity()));
  }
  SUBCASE("Slerp") {
    float mu = 0.723f;
    float angleR = -2.2f;
    mi::Vector3f axisR = mi::normalize(mi::Vector3f(7, -4, 2));
    mi::Vector3f offsR = mi::Vector3f(-1, -6, 2);
    mi::DualQuaternionf quatR = mi::DualQuaternionf::translate(offsR) * mi::DualQuaternionf::rotate(angleR, axisR);
    mi::DualQuaternionf quatQ = quat;
    mi::DualQuaternionf deriv;
    void(mi::slerp(mu, quatQ, quatR, &deriv));
    CHECK(mi::isNear<1e-3f>(deriv, bruteForceSlerpDeriv(mu, quatQ, quatR)));
    CHECK(mi::isNear<1e-5f>(quatQ, mi::slerp(0.0f, quatQ, quatR)));
    CHECK(mi::isNear<1e-5f>(quatR, mi::slerp(1.0f, quatQ, quatR)));
  }
  SUBCASE("Serialization") { CHECK(isMemcmpEqualAfterSerializeRoundTrip(quat)); }
}
