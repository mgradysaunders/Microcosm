#include "Microcosm/Geometry/Mesh"
#include "Microcosm/memory"
#include <set>

namespace mi::geometry {

Mesh::Mesh(const FileOBJ &file) {
  positions.v = file.positions.v, positions.f = file.positions.f;
  texcoords.v = file.texcoords.v, texcoords.f = file.texcoords.f, texcoords.fixInvalid();
  normals.v = file.normals.v, normals.f = file.normals.f, normals.fixInvalid();
  for (auto face : file.faces) {
    faces.push_back({face.first, face.count, {face.metadata.material, face.metadata.object}});
  }
}

#if 0
Mesh::Mesh(const FilePLY& file) {
}
#endif

Mesh::operator FileOBJ() const {
  FileOBJ file;
  file.positions.v = positions.v, file.positions.f = positions.f;
  file.texcoords.v = texcoords.v, file.texcoords.f = texcoords.f;
  file.normals.v = normals.v, file.normals.f = normals.f;
  for (const auto &face : faces) {
    file.faces.push_back({face.first, face.count, {face.metadata.material, face.metadata.object}});
  }
  return file;
}

#if 0
Mesh::operator FilePLY() const {
  FilePLY file;
  return file;
}
#endif

Mesh::Face Mesh::pushFace(uint32_t count) {
  Face face = faces.emplace_back(Face{indexCount, count});
  indexCount += count;
  if (positions.f.size() != indexCount) positions.f.resize(indexCount);
  if (texcoords.f.size() != indexCount) texcoords.f.resize(indexCount);
  if (normals.f.size() != indexCount) normals.f.resize(indexCount);
  return face;
}

void Mesh::reserveFace(Faces::iterator face, uint32_t count) {
  if (uint32_t capacity = capacityOfFace(face); capacity < count) {
    uint32_t insert_pos = face->first + capacity;
    uint32_t difference = count - capacity;
    positions.f.insert(positions.f.begin() + insert_pos, difference, 0);
    texcoords.f.insert(texcoords.f.begin() + insert_pos, difference, 0);
    normals.f.insert(normals.f.begin() + insert_pos, difference, 0);
    while (++face < faces.end()) face->first += difference;
    indexCount += difference;
  }
}

void Mesh::append(const Mesh &other) {
  for (auto [first, count, metadata] : other.faces) faces.push_back({first + indexCount, count, metadata});
  size_t positionOffset = positions.v.size();
  size_t texcoordOffset = texcoords.v.size();
  size_t normalOffset = normals.v.size();
  positions.v.insert(positions.v.end(), other.positions.v.begin(), other.positions.v.end());
  texcoords.v.insert(texcoords.v.end(), other.texcoords.v.begin(), other.texcoords.v.end());
  normals.v.insert(normals.v.end(), other.normals.v.begin(), other.normals.v.end());
  positions.f.reserve(indexCount + other.indexCount);
  texcoords.f.reserve(indexCount + other.indexCount);
  normals.f.reserve(indexCount + other.indexCount);
  for (auto i : other.positions.f) positions.f.push_back(i + positionOffset);
  for (auto i : other.texcoords.f) texcoords.f.push_back(i + texcoordOffset);
  for (auto i : other.normals.f) normals.f.push_back(i + normalOffset);
  indexCount += other.indexCount;
}

void Mesh::triangulate() {
  Mesh mesh;
  mesh.faces.reserve(faces.size());
  mesh.positions.v = std::move(positions.v);
  mesh.texcoords.v = std::move(texcoords.v);
  mesh.normals.v = std::move(normals.v);
  mesh.positions.f.reserve(3 * faces.size());
  mesh.texcoords.f.reserve(3 * faces.size());
  mesh.normals.f.reserve(3 * faces.size());
  for (const auto &face : faces) {
    if (face.count <= 2) continue;
    for (uint32_t local = 1; local + 1 < face.count; local++) {
      mesh.faces.push_back({mesh.indexCount, 3});
      auto pushTriangle = [&](const auto &from, auto &to) {
        if (!from.empty()) to.insert(to.end(), {from[face.first], from[face.first + local], from[face.first + local + 1]});
      };
      pushTriangle(positions.f, mesh.positions.f);
      pushTriangle(texcoords.f, mesh.texcoords.f);
      pushTriangle(normals.f, mesh.normals.f);
      mesh.indexCount += 3;
    }
  }
  *this = std::move(mesh);
}

void Mesh::consolidate() {
  Mesh mesh;
  mesh.faces.reserve(faces.size());
  mesh.positions.v = std::move(positions.v), mesh.positions.f.reserve(3 * faces.size());
  mesh.texcoords.v = std::move(texcoords.v), mesh.texcoords.f.reserve(3 * faces.size());
  mesh.normals.v = std::move(normals.v), mesh.normals.f.reserve(3 * faces.size());
  for (const auto &face : faces) {
    if (face.count <= 2) continue; // Remove bad faces.
    mesh.faces.push_back({mesh.indexCount, face.count});
    auto push = [&](const auto &from, auto &to) {
      to.insert(
        to.end(),                  //
        from.begin() + face.first, //
        from.begin() + face.first + face.count);
    };
    push(positions.f, mesh.positions.f);
    push(texcoords.f, mesh.texcoords.f);
    push(normals.f, mesh.normals.f);
    mesh.indexCount += face.count;
  }
  mesh.positions.discardUnused();
  mesh.texcoords.discardUnused();
  mesh.normals.discardUnused();
  *this = std::move(mesh);
}

void Mesh::validate() const {
  std::set<std::string> errors;
  auto errorIfNaN = [&](auto &prop) {
    int counter = 0;
    for (auto each : prop.v) counter += anyTrue(isnan(each));
    if (counter > 0) errors.insert("Found {} NaNs in {}.\n"_format(counter, prop.name()));
  };
  errorIfNaN(positions);
  errorIfNaN(texcoords);
  errorIfNaN(normals);
  auto errorIfInvalidIndexing = [&](auto &prop) {
    if (!prop.f.empty() && prop.f.size() != indexCount)
      errors.insert("Invalid indexing in {} ({} indexes, expected {} indexes)."_format(prop.name(), prop.f.size(), indexCount));
    int counter = 0;
    for (uint32_t i : prop.f) counter += i >= prop.v.size();
    if (counter > 0) {
      errors.insert("Invalid indexing in {} ({} indexes out-of-range)."_format(prop.name(), counter));
    }
  };
  errorIfInvalidIndexing(positions);
  errorIfInvalidIndexing(texcoords);
  errorIfInvalidIndexing(normals);
  std::vector<uint32_t> indexes;
  for (Face face : faces) {
    if (face.count < 3) errors.insert("Invalid topology (detected face with {} vert(s))."_format(face.count));
    auto errorIfDuplicates = [&](auto &prop) {
      if (!prop.f.empty()) {
        indexes.clear();
        for (size_t i = 0; i < face.count; i++) indexes.push_back(prop.f[face[i]]);
        if (std::unique(indexes.begin(), indexes.end()) != indexes.end()) {
          errors.insert("Invalid topology in {} (detected face with duplicate indexes)."_format(prop.name()));
        }
      }
    };
    errorIfDuplicates(positions);
    errorIfDuplicates(texcoords);
  }
  if (!errors.empty()) {
    std::string errorAccum = "Mesh validation failed! Detected {} error(s):\n"_format(errors.size());
    for (const auto &error : errors) errorAccum += "  - {}\n"_format(error);
    throw Error(std::runtime_error(errorAccum));
  }
}

Vector3f Mesh::vectorArea(Face face) const {
  Vector3f vectorAreaSum = {};
  Vector3f origin = positions(face, 0);
  for (uint32_t local = 1; local + 1 < face.count; local++)
    vectorAreaSum += cross(positions(face, local + 0) - origin, positions(face, local + 1) - origin);
  return 0.5f * vectorAreaSum;
}

float Mesh::volume() const {
  double volumeSum = 0;
  for (const Face &face : faces) volumeSum += dot(vectorArea(face), -positions(face, 0));
  volumeSum /= 3.0;
  // There is some conflict-of-convention here. Note that the
  // math definition of the normal vector points the opposite direction
  // than we usually expect in graphics (inside instead of outside). We
  // take the negative here so that CCW-orientation corresponds to positive
  // interior volume.
  return volumeSum * -1;
}

std::pair<Matrix3f, Vector3f> Mesh::MassData::principalInertia() const {
  DecompSVD<
    float, TensorShape<3, 3>,
    /*EnableU=*/true,
    /*EnableV=*/false>
    decomp{inertia};
  return {
    Matrix3f(decomp.matrixU()), //
    Vector3f(decomp.vectorS())};
}

void Mesh::MassData::rotate(const Matrix3f &amount) { inertia = dot(amount, inertia, transpose(amount)); }

// TODO Explain this.
Mesh::MassData Mesh::massData(float density) const {
  double densityOverSix{density / 6.0};
  double mass{0};
  Vector3d center;
  Matrix3d covariance;
  Matrix3d inertia;
  for (const Face &face : faces) {
    constexpr Matrix3d matrixA = {
      {2.0 / 20.0, 1.0 / 20.0, 1.0 / 20.0}, //
      {1.0 / 20.0, 2.0 / 20.0, 1.0 / 20.0}, //
      {1.0 / 20.0, 1.0 / 20.0, 2.0 / 20.0}};
    Matrix3d matrixX;
    auto vectorU = matrixX.col(0);
    auto vectorV = matrixX.col(1);
    auto vectorW = matrixX.col(2);
    vectorU.assign(positions(face, 0));
    for (uint32_t local = 1; local + 1 < face.count; local++) {
      vectorV.assign(positions(face, local + 0));
      vectorW.assign(positions(face, local + 1));
      double massX = densityOverSix * determinant(matrixX);
      mass += massX;
      center += massX * 0.25 * (vectorU + vectorV + vectorW);
      covariance += massX * dot(matrixX, matrixA, transpose(matrixX));
    }
  }
  center /= mass;
  inertia = trace(covariance) * Matrix3d::identity() - covariance;
  inertia -= mass * (lengthSquare(center) * Matrix3d::identity() - outer(center, center));
  return MassData{float(mass), Vector3f(center), Matrix3f(inertia)};
}

void Mesh::calculateNormals(bool perPosition) {
  if (perPosition) {
    normals.f = indexPerPosition();
    normals.v.clear();
    normals.v.resize(positions.v.size());
  } else {
    normals.f = indexPerFace();
    normals.v.clear();
    normals.v.resize(faces.size());
  }
  for (const Face &face : faces) {
    for (uint32_t local = 0; local < face.count; local++) {
      const auto &positionA = positions(face, local);
      const auto &positionB = positions(face, local + 1);
      const auto &positionC = positions(face, local + 2);
      normals(face, local + 1) += cross(positionC - positionB, positionA - positionB);
    }
  }
  normalizeNormals();
}

// TODO Explain this, and probably rewrite to be cleaner/more obvious.
template <typename Value, auto Name>
[[nodiscard]] static Mesh::Property<Value, Name> subdivideProperty(const auto &faces, const Mesh::Property<Value, Name> &prop) {
  if (prop.v.empty() || prop.f.empty()) return prop;
  struct VertSums {
    Value faceSum{};
    Value edgeSum{};
    Value edgeHoleSum{};
    uint32_t faceValence{0};
    uint32_t edgeValence{0};
    uint32_t edgeHoleValence{0};
  };
  struct Edge {
    uint32_t newVert{0};
    StaticStack<uint32_t, 2> faces{};
    Value center{};
  };
  std::vector<Value> faceCenters{reservedVectorSTL<Value>(faces.size())};
  std::vector<VertSums> vertSums{prop.v.size()};
  std::map<std::pair<uint32_t, uint32_t>, Edge> edges;
  for (const auto &face : faces) {
    auto &center = faceCenters.emplace_back();
    for (uint32_t i = 0; i < face.count; i++) {
      center += prop(face, i) / face.count;
      uint32_t v0 = prop.f[face[i + 0]];
      uint32_t v1 = prop.f[face[i + 1]];
      edges[std::minmax(v0, v1)].faces.push(&face - &faces[0]);
    }
    for (uint32_t i = 0; i < face.count; i++) {
      vertSums[prop.f[face[i]]].faceSum += center;
      vertSums[prop.f[face[i]]].faceValence++;
    }
  }
  for (auto &[key, edge] : edges) {
    edge.center = 0.5f * (prop.v[key.first] + prop.v[key.second]);
    vertSums[key.first].edgeSum += edge.center;
    vertSums[key.first].edgeValence++;
    vertSums[key.second].edgeSum += edge.center;
    vertSums[key.second].edgeValence++;
    if (!edge.faces.full()) {
      vertSums[key.first].edgeHoleSum += edge.center;
      vertSums[key.first].edgeHoleValence++;
      vertSums[key.second].edgeHoleSum += edge.center;
      vertSums[key.second].edgeHoleValence++;
    }
  }
  Mesh::Property<Value, Name> newProp;
  newProp.v = prop.v;
  for (uint32_t i = 0; i < prop.v.size(); i++) {
    auto &sums = vertSums[i];
    if (sums.edgeHoleValence == 0) {
      Value edgeAverage = sums.edgeSum / sums.edgeValence;
      Value faceAverage = sums.faceSum / sums.faceValence;
      float n = 1.0f / sums.faceValence;
      newProp.v[i] *= 1.0f - 3.0f * n;
      newProp.v[i] += 1.0f * n * faceAverage;
      newProp.v[i] += 2.0f * n * edgeAverage;
    } else {
      newProp.v[i] += sums.edgeHoleSum;
      newProp.v[i] /= sums.edgeHoleValence + 1;
    }
  }
  for (auto &[key, edge] : edges) {
    edge.newVert = newProp.v.size();
    if (edge.faces.full()) {
      const auto &faceCenter0 = faceCenters[edge.faces[0]];
      const auto &faceCenter1 = faceCenters[edge.faces[1]];
      newProp.v.push_back(0.5f * (edge.center + 0.5f * (faceCenter0 + faceCenter1)));
    } else
      newProp.v.push_back(edge.center);
  }
  newProp.v.insert(newProp.v.end(), faceCenters.begin(), faceCenters.end());
  for (const auto &face : faces) {
    uint32_t vc = prop.v.size() + edges.size() + (&face - &faces[0]);
    for (uint32_t i = 0; i < face.count; i++) {
      uint32_t v0 = prop.f[face[i]];
      uint32_t v1 = prop.f[face[i + 1]];
      uint32_t v2 = prop.f[face[i + 2]];
      newProp.f.insert(newProp.f.end(), {vc, edges[std::minmax(v0, v1)].newVert, v1, edges[std::minmax(v1, v2)].newVert});
    }
  }
  return newProp;
}

void Mesh::subdivide() {
  Property<Vector3f, "positions"_constant> newPositions;
  Property<Vector2f, "texcoords"_constant> newTexcoords;
  try {
    newPositions = subdivideProperty(faces, positions);
    newTexcoords = subdivideProperty(faces, texcoords);
  } catch (const std::exception &) {
    throw Error(std::runtime_error("Subdivision failed! The mesh data is non-manifold or otherwise corrupted."));
  }
  positions = std::move(newPositions);
  texcoords = std::move(newTexcoords);
  faces.resize(positions.f.size() / 4);
  for (size_t f = 0; f < faces.size(); f++) faces[f] = {4 * uint32_t(f), 4};
  normals.v.clear();
  normals.f.clear();
  calculateNormals();
  indexCount = positions.f.size();
}

void Mesh::displace(float amount, const std::function<float(Vector3f, Vector2f)> &func) {
  std::vector<std::pair<Vector3f, int>> offsets(positions.v.size());
  for (const Face &face : faces) {
    for (uint32_t local = 0; local < face.count; local++) {
      auto &offset = offsets[positions.f[face.first + local]];
      const auto &position = positions(face, local);
      const auto &texcoord = texcoords(face, local);
      offset.first += func(position, texcoord) * normals(face, local);
      offset.second++;
    }
  }
  for (auto &&[offset, position] : ranges::zip(offsets, positions.v)) position += amount * (offset.first / offset.second);
  calculateNormals();
}

void Mesh::transform(const DualQuaternionf &amount) {
  for (Vector3f &position : positions) position = amount.applyAffine(position);
  for (Vector3f &normal : normals) normal = amount.applyNormal(normal);
}

void Mesh::transform(const Quaternionf &amount) {
  for (Vector3f &position : positions) position = amount.applyAffine(position);
  for (Vector3f &normal : normals) normal = amount.applyNormal(normal);
}

void Mesh::transform(const Matrix3f &matrix) {
  Matrix3f invtee = inverse(transpose(matrix));
  for (Vector3f &position : positions) position = dot(matrix, position);
  for (Vector3f &normal : normals) normal = dot(invtee, normal);
}

void Mesh::translate(const Vector3f &amount) {
  for (Vector3f &position : positions) position += amount;
}

void Mesh::scale(const Vector3f &amount) {
  for (Vector3f &position : positions) position *= amount;
  for (Vector3f &normal : normals) normal /= amount;
}

void Mesh::scale(float amount) {
  for (Vector3f &position : positions) position *= amount;
}

Mesh Mesh::makeCube() {
  Mesh mesh;
  mesh.faces = {{0, 4}, {4, 4}, {8, 4}, {12, 4}, {16, 4}, {20, 4}};
  mesh.indexCount = 24;
  mesh.positions.v = {{+1.0f, +1.0f, -1.0f}, {+1.0f, -1.0f, -1.0f}, {+1.0f, +1.0f, +1.0f}, {+1.0f, -1.0f, +1.0f},
                      {-1.0f, +1.0f, -1.0f}, {-1.0f, -1.0f, -1.0f}, {-1.0f, +1.0f, +1.0f}, {-1.0f, -1.0f, +1.0f}};
  mesh.texcoords.v = {{0.625f, 0.500f}, {0.875f, 0.500f}, {0.875f, 0.750f}, {0.625f, 0.750f}, {0.375f, 0.750f},
                      {0.625f, 1.000f}, {0.375f, 1.000f}, {0.375f, 0.000f}, {0.625f, 0.000f}, {0.625f, 0.250f},
                      {0.375f, 0.250f}, {0.125f, 0.500f}, {0.375f, 0.500f}, {0.125f, 0.750f}};
  mesh.normals.v = {{0.0f, +1.0f, 0.0f}, {0.0f, 0.0f, +1.0f}, {-1.0f, 0.0f, 0.0f},
                    {0.0f, -1.0f, 0.0f}, {+1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}};
  mesh.positions.f = {0, 4, 6, 2, 3, 2, 6, 7, 7, 6, 4, 5, 5, 1, 3, 7, 1, 0, 2, 3, 5, 4, 0, 1};
  mesh.texcoords.f = {0, 1, 2, 3, 4, 3, 5, 6, 7, 8, 9, 10, 11, 12, 4, 13, 12, 0, 3, 4, 10, 9, 0, 12};
  mesh.normals.f = {0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5};
  return mesh;
}

Mesh Mesh::makeSurface(const Surface &surface) {
  Mesh mesh;
  if (!surface.func || surface.paramsU.size() < 2 || surface.paramsV.size() < 2) return mesh;
  uint32_t nU = surface.paramsU.size();
  uint32_t nV = surface.paramsV.size();
  uint32_t n = nU * nV;
  mesh.indexCount = (nU - 1) * (nV - 1) * 4;
  mesh.faces.reserve((nU - 1) * (nV - 1));
  for (uint32_t f = 0; f < (nU - 1) * (nV - 1); f++) mesh.faces.emplace_back(Face{4 * f, 4});
  mesh.positions.v.resize(n);
  mesh.texcoords.v.resize(n);
  for (uint32_t kU = 0; kU < nU; kU++)
    for (uint32_t kV = 0; kV < nV; kV++) {
      uint32_t k = kU * nV + kV;
      const float &paramU = surface.paramsU[kU];
      const float &paramV = surface.paramsV[kV];
      mesh.positions.v[k] = surface.func(paramU, paramV);
      mesh.texcoords.v[k] = Vector2f(paramU, paramV);
    }
  for (uint32_t kU = 0; kU + 1 < nU; kU++)
    for (uint32_t kV = 0; kV + 1 < nV; kV++) {
      uint32_t kU0 = (kU + 0) * nV, kV0 = (kV + 0);
      uint32_t kU1 = (kU + 1) * nV, kV1 = (kV + 1);
      mesh.positions.f.push_back(kU0 + kV0);
      mesh.positions.f.push_back(kU0 + kV1);
      mesh.positions.f.push_back(kU1 + kV1);
      mesh.positions.f.push_back(kU1 + kV0);
    }
  mesh.texcoords.f = mesh.positions.f;
  mesh.calculateNormals();
  return mesh;
}

Mesh Mesh::makePlane(uint32_t subdivsU, uint32_t subdivsV, const Vector3f &vectorU, const Vector3f &vectorV) {
  Surface surface;
  surface.func = [&](float paramU, float paramV) { return paramU * vectorU + paramV * vectorV; };
  surface.paramsU.set(0, 1, subdivsU);
  surface.paramsV.set(0, 1, subdivsV);
  Mesh mesh = makeSurface(surface);
  mesh.normals.v.resize(1);
  mesh.normals.v[0] = normalize(cross(vectorU, vectorV));
  std::fill(
    mesh.normals.f.begin(), //
    mesh.normals.f.end(), 0);
  return mesh;
}

#if 0
Mesh Mesh::makePlaneDisk(uint32_t countRad, uint32_t countPhi, const Vector3f &vectorX, const Vector3f &vectorY) {
  if (countRad < 1) countRad = 1;
  if (countPhi < 3) countPhi = 3;
  Mesh mesh;
  mesh.reserve(countRad * countPhi, countRad * countPhi);
  // Emit center.
  mesh.positions.v.emplace_back(Vector3f());
  mesh.texcoords.v.emplace_back(Vector2f());
  // Emit disk vertexes.
  for (float r : linspace(countRad, Exclusive(0.0f), 1.0f)) {
    for (float phi : linspace(countPhi, 0.0f, Exclusive(360.0_degreesf))) {
      float paramX = r * cos(phi);
      float paramY = r * sin(phi);
      mesh.positions.v.emplace_back(Vector3f(paramX * vectorX + paramY * vectorY));
      mesh.texcoords.v.emplace_back(Vector2f(paramX, paramY));
    }
  }
  auto indexLookup = [](uint32_t kRad, uint32_t kPhi) { return kRad * countPhi + (kPhi % countPhi) + 1; };
  // Emit inner triangle fan.
  for (uint32_t kPhi = 0; kPhi < countPhi; kPhi++) {
    mesh.positions.f.push_back(0);
    mesh.positions.f.push_back(indexLookup(0, kPhi));
    mesh.positions.f.push_back(indexLookup(0, kPhi + 1));
    mesh.pushFace(3);
  }
  // Emit outer quad strips.
  for (uint32_t kRad = 0; kRad + 1 < countRad; kRad++) {
    for (uint32_t kPhi = 0; kPhi < countPhi; kPhi++) {
      mesh.positions.f.push_back(indexLookup(kRad + 0, kPhi + 0));
      mesh.positions.f.push_back(indexLookup(kRad + 1, kPhi + 0));
      mesh.positions.f.push_back(indexLookup(kRad + 1, kPhi + 1));
      mesh.positions.f.push_back(indexLookup(kRad + 0, kPhi + 1));
      mesh.pushFace(4);
    }
  }
  mesh.texcoords.f = mesh.positions.f;
  mesh.normals.v.resize(1);
  mesh.normals.v[0] = normalize(cross(vectorX, vectorY));
  mesh.normals.f.resize(mesh.positions.f.size());
  std::fill(
    mesh.normals.f.begin(), //
    mesh.normals.f.end(), 0);
  return mesh;
}
#endif

Mesh Mesh::makeSphere(uint32_t subdivsU, uint32_t subdivsV, float radius) {
  Surface surface;
  surface.func = [=](float paramU, float paramV) {
    paramU *= 360.0_degreesf;
    paramV *= 180.0_degreesf;
    return Vector3f(
      radius * sin(paramV) * cos(paramU), //
      radius * sin(paramV) * sin(paramU), //
      radius * cos(paramV));
  };
  surface.paramsU.set(0, 1, subdivsU);
  surface.paramsV.set(0, 1, subdivsV);
  return makeSurface(surface);
}

Mesh Mesh::makeSphube(uint32_t levels, float radius) {
  auto mesh = makeCube();
  mesh.subdivide(levels);
  for (auto &position : mesh.positions.v) {
    position = radius * normalize(position);
  }
  mesh.calculateNormals();
  return mesh;
}

#if 0
Mesh Mesh::makeCylinder(uint32_t countU, uint32_t countV, float radius, float minZ, float maxZ) {
  Surface surface;
  surface.func = [=](float paramU, float paramV) {
    paramU *= 360.0_degreesf;
    return Vector3f(radius * cos(paramU), radius * sin(paramU), lerp(paramV, minZ, maxZ));
  };
  surface.paramsU.set(0, 1, countU);
  surface.paramsV.set(0, 1, countV);
  return makeSurface(surface);
}
#endif

} // namespace mi::geometry
