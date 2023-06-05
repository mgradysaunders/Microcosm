#include "Microcosm/Geometry/glTF"
#include "doctest.h"

static const char *avocado = R"(
{
  "accessors": [
    {"bufferView": 0, "componentType": 5126, "count": 406, "type": "VEC2"},
    {"bufferView": 1, "componentType": 5126, "count": 406, "type": "VEC3"},
    {"bufferView": 2, "componentType": 5126, "count": 406, "type": "VEC4"},
    {"bufferView": 3, "componentType": 5126, "count": 406, "type": "VEC3", "max": [0.02128091, 0.06284806, 0.0138090011], "min": [-0.02128091, -4.773855E-05, -0.013809]},
    {"bufferView": 4, "componentType": 5123, "count": 2046, "type": "SCALAR"}],
  "asset": {"generator": "glTF Tools for Unity", "version": "2.0"},
  "bufferViews": [
    {"buffer": 0, "byteLength": 3248},
    {"buffer": 0, "byteOffset": 3248, "byteLength": 4872},
    {"buffer": 0, "byteOffset": 8120, "byteLength": 6496},
    {"buffer": 0, "byteOffset": 14616, "byteLength": 4872},
    {"buffer": 0, "byteOffset": 19488, "byteLength": 4092}],
  "buffers": [
    {"uri": "Avocado.bin", "byteLength": 23580}],
  "images": [
    {"uri": "Avocado_baseColor.png"},
    {"uri": "Avocado_roughnessMetallic.png"},
    {"uri": "Avocado_normal.png"}],
  "meshes": [{
      "primitives": [{"attributes": {"TEXCOORD_0": 0, "NORMAL": 1, "TANGENT": 2, "POSITION": 3}, "indices": 4, "material": 0}],
      "name": "Avocado"}],
  "materials": [{
      "pbrMetallicRoughness": {
        "baseColorTexture": {"index": 0},
        "metallicRoughnessTexture": {"index": 1}
      },
      "normalTexture": {"index": 2},
      "name": "2256_Avocado_d"}],
  "nodes": [{
      "mesh": 0,
      "rotation": [0.0, 1.0, 0.0, 0.0],
      "name": "Avocado"}],
  "scene": 0,
  "scenes": [{"nodes": [0]}],
  "textures": [{"source": 0}, {"source": 1}, {"source": 2}]
}
)";

TEST_CASE("glTF") {
  mi::geometry::glTF::File file = mi::Json::parse(avocado);
  mi::Json backToJson = file;
  file = backToJson; // Round-trip
  CHECK(file.accessors.size() == 5);
  CHECK(file.accessors[0].bufferView == 0);
  CHECK(file.accessors[0].component == mi::geometry::glTF::Accessor::Component::Float);
  CHECK(file.accessors[0].count == 406);
  CHECK(file.accessors[0].type == "VEC2");
  CHECK(file.asset.version == "2.0");
  CHECK(file.asset.generator == "glTF Tools for Unity");
  CHECK(file.buffers.size() == 1);
  CHECK(file.buffers[0].uri == "Avocado.bin");
  CHECK(file.buffers[0].byteLength == 23580);
  CHECK(file.bufferViews.size() == 5);
  CHECK(file.bufferViews[0].buffer == 0);
  CHECK(file.bufferViews[0].byteOffset == 0);
  CHECK(file.bufferViews[0].byteLength == 3248);
  CHECK(file.bufferViews[1].buffer == 0);
  CHECK(file.bufferViews[1].byteOffset == 3248);
  CHECK(file.bufferViews[1].byteLength == 4872);
  CHECK(file.images.size() == 3);
  CHECK(file.images[0].uriOrMimeType == "Avocado_baseColor.png");
  CHECK(file.images[1].uriOrMimeType == "Avocado_roughnessMetallic.png");
  CHECK(file.images[2].uriOrMimeType == "Avocado_normal.png");
  CHECK(file.meshes.size() == 1);
  CHECK(file.meshes[0].name == "Avocado");
  CHECK(file.meshes[0].primitives.size() == 1);
  CHECK(file.meshes[0].primitives[0].indices == 4);
  CHECK(file.meshes[0].primitives[0].material == 0);
  CHECK(file.meshes[0].primitives[0].attributes.at("TEXCOORD_0") == 0);
  CHECK(file.meshes[0].primitives[0].attributes.at("NORMAL") == 1);
  CHECK(file.meshes[0].primitives[0].attributes.at("TANGENT") == 2);
  CHECK(file.meshes[0].primitives[0].attributes.at("POSITION") == 3);
  CHECK(file.materials.size() == 1);
  CHECK(file.materials[0].name == "2256_Avocado_d");
  CHECK(file.materials[0].pbr.baseColorTexture.index == 0);
  CHECK(file.materials[0].pbr.metallicRoughnessTexture.index == 1);
  CHECK(file.materials[0].normalTexture.index == 2);
}
