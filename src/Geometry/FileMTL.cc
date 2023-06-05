#include "Microcosm/Geometry/FileMTL"

namespace mi::geometry {

FileMTL::Material::operator std::string() const {
  std::string buffer;
  buffer.reserve(1024);
  if (ambient) buffer += "Ka {:7f}\n"_format(join(*ambient, " "));
  if (diffuse) buffer += "Kd {:7f}\n"_format(join(*diffuse, " "));
  if (specular) buffer += "Ks {:7f}\n"_format(join(*specular, " "));
  if (emissive) buffer += "Ke {:7f}\n"_format(join(*emissive, " "));
  if (ambientTexture) buffer += "map_Ka {}\n"_format(*ambientTexture);
  if (diffuseTexture) buffer += "map_Kd {}\n"_format(*diffuseTexture);
  if (specularTexture) buffer += "map_Ks {}\n"_format(*specularTexture);
  if (emissiveTexture) buffer += "map_Ke {}\n"_format(*emissiveTexture);
  if (bumpTexture) buffer += "map_bump {}\n"_format(*bumpTexture);
  if (normalTexture) buffer += "map_normal {}\n"_format(*normalTexture);
  if (opacityTexture) buffer += "map_d {}\n"_format(*opacityTexture);
  if (illuminationModel) buffer += "illum {}\n"_format(*illuminationModel);
  if (specularHighlight) buffer += "Ns {:7f}\n"_format(*specularHighlight);
  if (refractiveIndex) buffer += "Ni {:7f}\n"_format(*refractiveIndex);
  if (opacity) buffer += "d {:7f}\n"_format(*opacity);
  return buffer;
}

void FileMTL::read(const std::string &filename) {
  auto stream = openIFStreamOrThrow(filename);
  read(stream);
}

void FileMTL::read(std::istream &stream) {
  materials.clear();
  Material *material = &materials[""];
  std::string lineBuffer;
  long lineNo{0};
  while (std::getline(stream, lineBuffer)) {
    std::string_view line{lineBuffer};
    line = trim(line);
    lineNo++;
    auto parseOneOrThree = [](std::string_view strv) {
      auto values = SplitString(strv, char_class::space, /*skipEmpty=*/true);
      if (values.size() == 1) {
        return Vector3f(stringTo<float>(values.at(0)));
      } else {
        return Vector3f(stringTo<float>(values.at(0)), stringTo<float>(values.at(1)), stringTo<float>(values.at(2)));
      }
    };
    if (line.empty()) [[unlikely]]
      continue;
    if (line.starts_with("#")) [[unlikely]]
      continue;
    if (line.starts_with("newmtl ")) {
      material = &materials[std::string(trim(line.substr(7)))];
    } else if (line.starts_with("Ka ")) {
      material->ambient = parseOneOrThree(line.substr(3));
    } else if (line.starts_with("Kd ")) {
      material->diffuse = parseOneOrThree(line.substr(3));
    } else if (line.starts_with("Ks ")) {
      material->specular = parseOneOrThree(line.substr(3));
    } else if (line.starts_with("Ke ")) {
      material->emissive = parseOneOrThree(line.substr(3));
    } else if (line.starts_with("map_Ka ")) {
      material->ambientTexture = std::string(trim(line.substr(7)));
    } else if (line.starts_with("map_Kd ")) {
      material->diffuseTexture = std::string(trim(line.substr(7)));
    } else if (line.starts_with("map_Ks ")) {
      material->specularTexture = std::string(trim(line.substr(7)));
    } else if (line.starts_with("map_Ke ")) {
      material->emissiveTexture = std::string(trim(line.substr(7)));
    } else if (line.starts_with("map_bump ")) {
      material->bumpTexture = std::string(trim(line.substr(9)));
    } else if (line.starts_with("map_normal ")) {
      material->normalTexture = std::string(trim(line.substr(11)));
    } else if (line.starts_with("map_d ")) {
      material->opacityTexture = std::string(trim(line.substr(6)));
    } else if (line.starts_with("illum ")) {
      material->illuminationModel = stringTo<int>(trim(line.substr(6)));
    } else if (line.starts_with("Ns ")) {
      material->specularHighlight = stringTo<float>(trim(line.substr(3)));
    } else if (line.starts_with("Ni ")) {
      material->refractiveIndex = stringTo<float>(trim(line.substr(3)));
    } else if (line.starts_with("d ")) {
      material->opacity = stringTo<float>(trim(line.substr(2)));
    }
  }
}

void FileMTL::write(const std::string &filename) const {
  auto stream = openOFStreamOrThrow(filename);
  write(stream);
}

void FileMTL::write(std::ostream &stream) const { stream << std::string(*this); }

FileMTL::operator std::string() const {
  std::string buffer;
  buffer.reserve(1024);
  for (const auto &[name, material] : materials)
    if (!name.empty()) buffer += "newmtl {}\n{}\n"_format(name, std::string(material));
  return buffer;
}

} // namespace mi::geometry
