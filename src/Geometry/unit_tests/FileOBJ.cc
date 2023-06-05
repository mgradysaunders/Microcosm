#include "Microcosm/Geometry/FileOBJ"
#include "doctest.h"
#include <sstream>

static const char *source = R"(
# Blender v2.82 (sub 7) OBJ File: ''
# www.blender.org
mtllib Cube.mtl
o Cube
v 1 1 -1
v 1 -1 -1
v 1 1 1
v 1 -1 1
v -1 1 -1
v -1 -1 -1
v -1 1 1
v -1 -1 1
vn 0 1 0
vn 0 0 1
vn -1 0 0
vn 0 -1 0
vn 1 0 0
vn 0 0 -1
vt 0.625 0.500
vt 0.875 0.500
vt 0.875 0.750
vt 0.625 0.750
vt 0.375 0.750
vt 0.625 1.000
vt 0.375 1.000
vt 0.375 0.000
vt 0.625 0.000
vt 0.625 0.250
vt 0.375 0.250
vt 0.125 0.500
vt 0.375 0.500
vt 0.125 0.750
usemtl Material
s off
f 1/1/1 5/2/1 7/3/1 3/4/1
f 4/5/2 3/4/2 7/6/2 8/7/2
f 8/8/3 7/9/3 5/10/3 6/11/3
f 6/12/4 2/13/4 4/5/4 8/14/4
f 2/13/5 1/1/5 3/4/5 4/5/5
f 6/11/6 5/10/6 1/1/6 2/13/6
)";

TEST_CASE("FileOBJ") {
  std::stringstream stream;
  std::stringstream stream2;
  stream << source;
  mi::geometry::FileOBJ cube;
  CHECK_NOTHROW(cube.read(stream));
  CHECK_NOTHROW(cube.write(stream2));
  CHECK_NOTHROW(cube.read(stream2));
  CHECK(cube.positions.v.size() == 8);
  CHECK(cube.texcoords.v.size() == 14);
  CHECK(cube.normals.v.size() == 6);
  CHECK(cube.faces.size() == 6);
  CHECK(cube.metadata.materialFiles.size() == 1);
  CHECK(cube.metadata.materialFiles[0] == "Cube.mtl");
  CHECK(cube.metadata.materialNames.size() == 1);
  CHECK(cube.metadata.materialNames[0] == "Material");
  CHECK(cube.metadata.objectNames.size() == 1);
  CHECK(cube.metadata.objectNames[0] == "Cube");
}
