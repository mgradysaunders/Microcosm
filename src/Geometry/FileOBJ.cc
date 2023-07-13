#include "Microcosm/Geometry/FileOBJ"
#include <fstream>
#include <iostream>

namespace mi::geometry {

void FileOBJ::read(const std::string &filename) {
  auto stream = openIFStreamOrThrow(filename);
  read(stream);
}

void FileOBJ::read(std::istream &stream) {
  clear();
  positions.v.reserve(512), positions.f.reserve(512);
  texcoords.v.reserve(512), texcoords.f.reserve(512);
  normals.v.reserve(512), normals.f.reserve(512);
  int16_t material{-1};
  int16_t object{-1};
  int16_t group{-1};
  int16_t smoothGroup{-1};
  std::map<std::string, int16_t> materialNameToIdx;
  std::map<std::string, int16_t> objectNameToIdx;
  std::map<std::string, int16_t> groupNameToIdx;
  std::string lineBuffer;
  long lineNo{0};
  while (std::getline(stream, lineBuffer)) {
    std::string_view line{lineBuffer};
    line = trim(line);
    lineNo++;
    if (line.empty()) [[unlikely]]
      continue; // Skip empty lines
    if (line.starts_with("#")) [[unlikely]]
      continue; // Skip comments
    if (line.starts_with("v ")) {
      auto values = SplitString(line.substr(2), char_class::space, /*skipEmpty=*/true);
      positions.v.emplace_back(stringTo<float>(values.at(0)), stringTo<float>(values.at(1)), stringTo<float>(values.at(2)));
    } else if (line.starts_with("vt ")) {
      auto values = SplitString(line.substr(3), char_class::space, /*skipEmpty=*/true);
      texcoords.v.emplace_back(stringTo<float>(values.at(0)), stringTo<float>(values.at(1)));
    } else if (line.starts_with("vn ")) {
      auto values = SplitString(line.substr(3), char_class::space, /*skipEmpty=*/true);
      normals.v.emplace_back(stringTo<float>(values.at(0)), stringTo<float>(values.at(1)), stringTo<float>(values.at(2)));
    } else if (line.starts_with("f ")) {
      Face &face = faces.emplace_back();
      face.first = positions.f.size();
      face.metadata.material = material;
      face.metadata.object = object;
      face.metadata.group = group;
      face.metadata.smoothGroup = smoothGroup;
      auto tokens = SplitString(line.substr(2), char_class::space, /*skipEmpty=*/true);
      for (auto token : tokens) {
        auto values = SplitString(token, char_class::these("/"), /*skipEmpty=*/false);
        auto valueItr = values.begin();
        auto parseIdx = [&](const auto &buffer) -> uint32_t {
          if (valueItr != values.end()) {
            auto value = *valueItr++;
            if (!value.empty()) {
              int32_t i = stringTo<int32_t>(value);
              if (i < 0) {
                i += buffer.v.size();
                return i < 0 ? None : i;
              } else {
                return i - 1;
              }
            }
          }
          return None;
        };
        uint32_t positionIdx = parseIdx(positions);
        uint32_t texcoordIdx = parseIdx(texcoords);
        uint32_t normalIdx = parseIdx(normals);
        if (texcoordIdx != None && !texcoords) [[unlikely]]
          texcoords.f.resize(positions.f.size(), None);
        if (normalIdx != None && !normals) [[unlikely]]
          normals.f.resize(positions.f.size(), None);
        positions.f.emplace_back(positionIdx);
        texcoords.f.emplace_back(texcoordIdx);
        normals.f.emplace_back(normalIdx);
        face.count++;
      }
    } else if (line.starts_with("mtllib ")) {
      metadata.materialFiles.emplace_back(std::string(trim(line.substr(7))));
    } else if (line.starts_with("usemtl ")) {
      auto name = std::string(trim(line.substr(7)));
      auto [itr, inserted] = materialNameToIdx.emplace(name, int16_t(metadata.materialNames.size()));
      if (inserted) metadata.materialNames.emplace_back(std::move(name));
      material = itr->second;
    } else if (line.starts_with("o ")) { // Object
      auto name = std::string(trim(line.substr(2)));
      auto [itr, inserted] = objectNameToIdx.emplace(name, int16_t(metadata.objectNames.size()));
      if (inserted) metadata.objectNames.emplace_back(std::move(name));
      object = itr->second;
    } else if (line.starts_with("g ")) { // Group
      auto name = std::string(trim(line.substr(2)));
      auto [itr, inserted] = groupNameToIdx.emplace(name, int16_t(metadata.groupNames.size()));
      if (inserted) metadata.groupNames.emplace_back(std::move(name));
      group = itr->second;
    } else if (line.starts_with("s ")) { // Smooth group
      auto name = trim(line.substr(2));
      try {
        smoothGroup = name == "off" ? int16_t(None) : stringTo<int16_t>(name);
      } catch (...) {
        smoothGroup = int16_t(None); // TODO Warning
      }
    }
  }
  if (!texcoords) texcoords = {};
  if (!normals) normals = {};
}

void FileOBJ::write(const std::string &filename) const {
  auto stream = openOFStreamOrThrow(filename);
  write(stream);
}

void FileOBJ::write(std::ostream &stream) const {
  if (faces.empty() || !positions) return;
  for (const auto &filename : metadata.materialFiles) stream << "mtllib " << filename << '\n';
  stream << std::string(positions);
  stream << std::string(texcoords);
  stream << std::string(normals);
  struct MetadataWriter {
    void update(int16_t value) {
      if ((!nameLookup || !nameLookup->empty()) && activeValue != value) {
        if (value < 0) {
          stream << prefix << " off\n";
        } else if (nameLookup) {
          stream << prefix << ' ' << nameLookup->at(value) << '\n';
        } else {
          stream << prefix << ' ' << value << '\n';
        }
      }
      activeValue = value;
    }
    std::ostream &stream;
    const std::vector<std::string> *nameLookup{};
    std::string_view prefix{};
    std::optional<int16_t> activeValue{};
  };
  MetadataWriter usemtl{stream, &metadata.materialNames, "usemtl"};
  MetadataWriter o{stream, &metadata.objectNames, "o"};
  MetadataWriter g{stream, &metadata.groupNames, "g"};
  MetadataWriter s{stream, nullptr, "s"};
  for (const auto &face : faces) {
    usemtl.update(face.metadata.material);
    o.update(face.metadata.object);
    g.update(face.metadata.group);
    s.update(face.metadata.smoothGroup);
    stream << "f ";
    for (uint32_t i = face.first; i < face.first + face.count; i++) {
      uint32_t positionIdx = positions ? positions.f[i] : None;
      uint32_t texcoordIdx = texcoords ? texcoords.f[i] : None;
      uint32_t normalIdx = normals ? normals.f[i] : None;
      stream << positionIdx + 1;
      switch (((texcoordIdx != None) << 1) | (normalIdx != None)) {
      default:
      case 0b00: break;
      case 0b01: stream << '/' << '/' << normalIdx + 1; break;
      case 0b10: stream << '/' << texcoordIdx + 1; break;
      case 0b11: stream << '/' << texcoordIdx + 1 << '/' << normalIdx + 1; break;
      }
      stream << (i + 1 < face.first + face.count ? ' ' : '\n');
    }
  }
}

} // namespace mi::geometry
