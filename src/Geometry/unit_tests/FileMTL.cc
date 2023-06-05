#include "Microcosm/Geometry/FileMTL"
#include "doctest.h"
#include <sstream>

static const char *source = R"(
newmtl Material1
  Ka 0.3 0.4 0.5
  Kd 0.2 0.6 0.7
  Ks 0.6 0.5 0.4
  Ke 0.0 0.0 0.2
  map_Ka AmbientTexture.png
  map_Kd DiffuseTexture.png
  map_Ks SpecularTexture.png
  map_Ke EmissiveTexture.png
  map_bump BumpTexture.png
  map_normal NormalTexture.png
  Ns 0.25
  Ni 1.5
  d 1.0
  illum 2

)";
TEST_CASE("FileMTL") {
  mi::geometry::FileMTL file;
  std::stringstream stream;
  std::stringstream stream2;
  stream << source;
  CHECK_NOTHROW(file.read(stream));
  CHECK_NOTHROW(file.write(stream2));
  CHECK_NOTHROW(file.read(stream2));
  CHECK(file.materials.contains("Material1"));
  const auto &material = file.materials["Material1"];
  CHECK(material.ambient);
  CHECK(material.diffuse);
  CHECK(material.specular);
  CHECK(material.emissive);
  CHECK(mi::isNear<1e-5f>(*material.ambient, mi::Vector3f(0.3f, 0.4f, 0.5f)));
  CHECK(mi::isNear<1e-5f>(*material.diffuse, mi::Vector3f(0.2f, 0.6f, 0.7f)));
  CHECK(mi::isNear<1e-5f>(*material.specular, mi::Vector3f(0.6f, 0.5f, 0.4f)));
  CHECK(mi::isNear<1e-5f>(*material.emissive, mi::Vector3f(0.0f, 0.0f, 0.2f)));
  CHECK(material.ambientTexture);
  CHECK(material.diffuseTexture);
  CHECK(material.specularTexture);
  CHECK(material.emissiveTexture);
  CHECK(material.bumpTexture);
  CHECK(material.normalTexture);
  CHECK(*material.ambientTexture == "AmbientTexture.png");
  CHECK(*material.diffuseTexture == "DiffuseTexture.png");
  CHECK(*material.specularTexture == "SpecularTexture.png");
  CHECK(*material.emissiveTexture == "EmissiveTexture.png");
  CHECK(*material.bumpTexture == "BumpTexture.png");
  CHECK(*material.normalTexture == "NormalTexture.png");
  CHECK(material.illuminationModel);
  CHECK(material.specularHighlight);
  CHECK(material.refractiveIndex);
  CHECK(material.opacity);
  CHECK(*material.illuminationModel == 2);
  CHECK(*material.specularHighlight == 0.25f);
  CHECK(*material.refractiveIndex == 1.5f);
  CHECK(*material.opacity == 1);
}
