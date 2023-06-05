#include "Microcosm/Geometry/FilePLY"
#include "Microcosm/memory"
#include "Microcosm/utility"
#include <bit>

namespace mi::geometry {

#if 0
void FilePLY::read(const std::string &filename) {
  clear();
  std::ifstream ifs(filename, std::ios_base::in | std::ios_base::binary);
  if (!ifs.is_open()) throw Error(std::runtime_error("Can't open "s + show(filename)));
  read(ifs);
}

enum class PLYFormat { Invalid, LittleEndian, BigEndian, Ascii };

struct PLYProperty {
  StaticString<char, 256> name;
  DType dataPrim = DType::None;
  DType sizePrim = DType::None;
  float *offset = nullptr;
  size_t stride = 0;
};

struct PLYElement {
  StaticString<char, 256> name;
  size_t count = 0;
  std::vector<PLYProperty> props;
  [[nodiscard]] bool has(const char *propName) const noexcept {
    for (const auto &prop : props)
      if (prop.name == propName) return true;
    return false;
  }
};

struct PLYHeaderParser {
  PLYHeaderParser(const std::string &header) : scanner(header) {
    scanner.next_line(); // Skip "ply"
  }

  PLYFormat parseFormatLine() {
    scanner.expect("format");
    scanner.skip(" \t");
    PLYFormat format = scanner.accept("binary_little_endian") ? PLYFormat::LittleEndian
                       : scanner.accept("binary_big_endian")  ? PLYFormat::BigEndian
                       : scanner.accept("ascii")              ? PLYFormat::Ascii
                                                              : PLYFormat::Invalid;
    if (format == PLYFormat::Invalid)
      scanner.fail("Expected valid format (binary_little_endian, "
                   "binary_big_endian, or ascii)");
    scanner.skip(" \t").expect("1.0");
    scanner.skip(" \t").expect('\n');
    return {};
  }

  DType parseDatatype() {
    static const std::pair<std::pair<const char *, const char *>, DType> lookup[] = {
      {{"uint8", "uchar"}, DType::UInt8},     //
      {{"uint16", "ushort"}, DType::UInt16},  //
      {{"uint32", "uint"}, DType::UInt32},    //
      {{"int8", "char"}, DType::Int8},        //
      {{"int16", "short"}, DType::Int16},     //
      {{"int32", "int"}, DType::Int32},       //
      {{"float32", "float"}, DType::Float32}, //
      {{"float64", "double"}, DType::Float64}};
    for (auto &[names, type] : lookup)
      if (
        scanner.accept(names.first) || //
        scanner.accept(names.second)) {
        scanner.skip(" \t");
        return type;
      }
    scanner.fail("Expected valid datatype");
    return {};
  }

  std::vector<PLYElement> parseElements() {
    std::vector<PLYElement> elems;
    while (not scanner.eof()) {
      while (scanner.accept("comment") || scanner.accept("obj_info")) scanner.next_line();
      if (scanner.accept("element")) {
        scanner.skip(" \t");
        auto &elem = elems.emplace_back();
        elem.name = scanner.accept(char_class::word);
        scanner.skip(" \t");
        elem.count = string_to<size_t>(scanner.accept(char_class::digit));
        scanner.next_line();
        while (scanner.accept("comment") or scanner.accept("obj_info")) scanner.next_line();
        while (scanner.accept("property")) {
          scanner.skip(" \t");
          auto &prop = elem.props.emplace_back();
          if (scanner.accept("list")) {
            scanner.skip(" \t");
            prop.sizePrim = parseDatatype();
          }
          prop.dataPrim = parseDatatype();
          prop.name = scanner.accept(char_class::word);
          scanner.next_line();
        }
        continue;
      }
      if (scanner.accept("end_header")) break;
      scanner.next_line();
    }
    return elems;
  }

  Scanner<char> scanner;
};

void FilePLY::read(std::istream &stream) {
  clear();
  std::string magic;
  std::string header;
  if (!std::getline(stream, magic) || magic != "ply") throw Error(std::runtime_error("Bad header!"));
  header += magic;
  header += '\n';
  std::string line;
  while (std::getline(stream, line)) {
    header += line;
    header += '\n';
    if (line == "end_header") break;
  }
  if (stream.eof()) throw Error(std::runtime_error("Bad header!"));
  PLYHeaderParser parser(header);
  auto format = parser.parseFormatLine();
  bool flipEndian = (format == PLYFormat::LittleEndian && std::endian::native == std::endian::big) ||
                    (format == PLYFormat::BigEndian && std::endian::native == std::endian::little);
  auto readDouble = [&](DType prim) {
    double value = 0;
    if (format == PLYFormat::Ascii)
      stream >> value;
    else {
      char buff[8] = {};
      stream.read(&buff[0], sizeOf(prim));
      if (flipEndian) std::reverse(buff, buff + sizeOf(prim));
      dispatchCast<DType::Float64>(1, prim, &buff[0], &value);
    }
    if (stream.fail()) throw std::runtime_error("Read error!");
    return value;
  };
  auto readUInt32 = [&](DType prim) {
    uint32_t value = 0;
    if (format == PLYFormat::Ascii)
      stream >> value;
    else {
      char buff[8] = {};
      stream.read(&buff[0], sizeOf(prim));
      if (flipEndian) std::reverse(buff, buff + sizeOf(prim));
      dispatchCast<DType::UInt32>(1, prim, &buff[0], &value);
    }
    if (stream.fail()) throw std::runtime_error("Read error!");
    return value;
  };
  auto elems = parser.parseElements();
  for (auto &elem : elems) {
    if (elem.count == 0) continue;
    if (elem.name == "vertex") {
      positions.resize(elem.count);
      if (elem.has("s") or elem.has("t")) texcoords.resize(elem.count);
      if (elem.has("nx") or elem.has("ny") or elem.has("nz")) normals.resize(elem.count);
      if (elem.has("r") or elem.has("g") or elem.has("b")) colors.resize(elem.count);
      for (auto &prop : elem.props) {
        prop.stride = 3;
        if (
          prop.name == "s" or //
          prop.name == "t")
          prop.stride = 2;
        if (prop.name == "x") prop.offset = &positions[0][0];
        if (prop.name == "y") prop.offset = &positions[0][1];
        if (prop.name == "z") prop.offset = &positions[0][2];
        if (prop.name == "s") prop.offset = &texcoords[0][0];
        if (prop.name == "t") prop.offset = &texcoords[0][1];
        if (prop.name == "nx") prop.offset = &normals[0][0];
        if (prop.name == "ny") prop.offset = &normals[0][1];
        if (prop.name == "nz") prop.offset = &normals[0][2];
        if (prop.name == "r") prop.offset = &colors[0][0];
        if (prop.name == "g") prop.offset = &colors[0][1];
        if (prop.name == "b") prop.offset = &colors[0][2];
      }
    }
    bool isFace = elem.name == "face";
    for (size_t index = 0; index < elem.count; index++)
      for (auto &prop : elem.props) {
        bool isVertexIndices = prop.name == "vertex_indices" or prop.name == "vertexIndex";
        if (prop.sizePrim == DType::None) {
          double data = readDouble(prop.dataPrim);
          if (prop.offset != nullptr) prop.offset[prop.stride * index] = data;
        } else {
          uint32_t size = readUInt32(prop.sizePrim);
          if (size > 255) throw std::runtime_error("Face size too large!");
          if (isFace && isVertexIndices) faceSizes.push_back(size);
          while (size-- != 0) {
            uint32_t data = readUInt32(prop.dataPrim);
            if (isFace and isVertexIndices) faceIndexes.push_back(data);
          }
        }
      }
  }
}

void FilePLY::write(const std::string &filename) const {
  std::ofstream ofs(filename, std::ios_base::out | std::ios_base::binary);
  if (!ofs.is_open()) throw Error(std::runtime_error("Can't open "s + show(filename)));
  write(ofs);
}
#endif

void FilePLY::write(std::ostream &stream) const {
  const char *format = "binary_little_endian";
  if constexpr (std::endian::native == std::endian::big) format = "binary_big_endian";
  stream << "ply\n";
  stream << "format " << format << " 1.0\n";
  stream << "element vertex " << positions.size() << "\n";
  stream << "property float x\n"
            "property float y\n"
            "property float z\n";
  if (!texcoords.empty())
    stream << "property float s\n"
              "property float t\n";
  if (!normals.empty())
    stream << "property float nx\n"
              "property float ny\n"
              "property float nz\n";
  if (!colors.empty())
    stream << "property float r\n"
              "property float g\n"
              "property float b\n";
  stream << "element face " << faceSizes.size() << '\n';
  stream << "property list uint8 uint32 vertex_indices\n";
  stream << "end_header\n";
  auto write1 = [&](const auto &value) { stream.write(reinterpret_cast<const char *>(&value), sizeof(value)); };
  for (size_t index = 0; index < positions.size(); index++) {
    write1(positions[index][0]);
    write1(positions[index][1]);
    write1(positions[index][2]);
    if (!texcoords.empty()) {
      write1(texcoords[index][0]);
      write1(texcoords[index][1]);
    }
    if (!normals.empty()) {
      write1(normals[index][0]);
      write1(normals[index][1]);
      write1(normals[index][2]);
    }
    if (!colors.empty()) {
      write1(colors[index][0]);
      write1(colors[index][1]);
      write1(colors[index][2]);
    }
  }
  auto faceIndex = faceIndexes.begin();
  for (uint8_t faceSize : faceSizes) {
    write1(faceSize);
    while (faceSize-- != 0) write1(uint32_t(*faceIndex++));
  }
}

void FilePLY::triangulate() {
  uint32_t offset = 0;
  std::vector<uint8_t> newFaceSizes;
  std::vector<uint32_t> newFaceIndexes;
  newFaceSizes.reserve(faceSizes.size());
  newFaceIndexes.reserve(faceIndexes.size());
  for (uint8_t faceSize : faceSizes) {
    for (uint8_t local = 1; local + 1 < faceSize; local++) {
      newFaceIndexes.push_back(faceIndexes[offset]);
      newFaceIndexes.push_back(faceIndexes[offset + local]);
      newFaceIndexes.push_back(faceIndexes[offset + local + 1]);
      newFaceSizes.push_back(3);
    }
    offset += faceSize;
  }
  std::swap(faceSizes, newFaceSizes);
  std::swap(faceIndexes, newFaceIndexes);
}

void FilePLY::normalizeNormals() {
  for (auto &v : normals) v = normalize(v);
}

} // namespace mi::geometry
