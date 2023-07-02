#include "Microcosm/Json"

namespace mi {

namespace {

[[nodiscard]] static std::string encodeString(std::string_view source) {
  std::string target;
  target.reserve(source.size());
  target += '"';
  for (size_t i = 0; i < source.size(); i++) {
    char c = source[i];
    switch (c) {
    case '\\': target += "\\\\"; break;
    case '\b': target += "\\b"; break;
    case '\f': target += "\\f"; break;
    case '\n': target += "\\n"; break;
    case '\r': target += "\\r"; break;
    case '\t': target += "\\t"; break;
    case '"': target += "\\\""; break;
    default: {
      if (uint8_t(c) <= 0x1f) {
        target += "\\x";
        target += "0123456789ABCDEF"[(c >> 4) & 0xF];
        target += "0123456789ABCDEF"[c & 0xF];
      } else if (uint8_t(c) == 0xe2 && uint8_t(source[i + 1]) == 0x80 && uint8_t(source[i + 2]) == 0xa8) {
        target += "\\u2028";
        i += 2;
      } else if (uint8_t(c) == 0xe2 && uint8_t(source[i + 1]) == 0x80 && uint8_t(source[i + 2]) == 0xa9) {
        target += "\\u2029";
        i += 2;
      } else
        target += c;
      break;
    }
    }
  }
  target += '"';
  return target;
}

[[nodiscard]] static std::string decodeString(std::string_view source) {
  std::string target;
  if (source.empty()) return target;
  if (!(source.front() == '"' && source.back() == '"' && source.size() >= 2)) throw Error(std::runtime_error("Expected string inside '\"'!"));
  target.reserve(source.size());
  size_t i = 1, n = source.size() - 1;
  while (i < n) {
    auto next = [&] {
      if (i >= n) throw Error(std::runtime_error("Unexpected end of string!"));
      return source[i++];
    };
    if (char c = next(); c != '\\') {
      target += c;
      continue;
    }
    switch (next()) {
    case '\\': target += '\\'; break;
    case 'b': target += '\b'; break;
    case 'f': target += '\f'; break;
    case 'n': target += '\n'; break;
    case 'r': target += '\r'; break;
    case 't': target += '\t'; break;
    case '"': target += '"'; break;
    case 'x': {
      char c0 = next();
      char c1 = next();
      if (!char_class::xdigit(c0) || !char_class::xdigit(c1)) throw Error(std::runtime_error("Expected two hexidecimal characters after '\\x'!"));
      target += char((hexToInt(c0) << 4) | hexToInt(c1));
      break;
    }
    case 'u': {
      auto nextUTF16 = [&] {
        char c0 = next();
        char c1 = next();
        char c2 = next();
        char c3 = next();
        if (!char_class::xdigit(c0) || !char_class::xdigit(c1) || !char_class::xdigit(c2) || !char_class::xdigit(c3)) throw Error(std::runtime_error("Expected four hexidecimal characters after '\\u'!"));
        return (uint16_t(hexToInt(c0)) << 12) | (uint16_t(hexToInt(c1)) << 8) | //
               (uint16_t(hexToInt(c2)) << 4) | uint16_t(hexToInt(c3));
      };
      uint32_t codepoint = nextUTF16();
      if (0xD800 <= codepoint && codepoint <= 0xDBFF) {
        char c0 = next();
        char c1 = next();
        if (c0 != '\\' || c1 != 'u') throw std::runtime_error("Expected pair of UTF-16 surrogates!");
        codepoint = (((codepoint - 0xD800) << 10) | (nextUTF16() - 0xDC00)) + 0x10000;
      }
      target += UTF8Encoding(codepoint);
      break;
    }
    default: throw Error(std::runtime_error("Invalid escape!")); break;
    }
  }
  return target;
}

struct Parser {
public:
  Parser(std::string_view source) : mScanner(source) {}

  [[nodiscard]] Json parse() {
    mScanner.ignore(char_class::space);
    if (mScanner.isEOF()) mScanner.fail("Unexpected EOF!");
    auto demandString = [&]() -> std::string {
      try {
        return decodeString(mScanner.ignore(char_class::space).quote('"', '"', '\\'));
      } catch (const std::exception &error) {
        mScanner.fail(error.what());
      }
      return {};
    };
    switch (mScanner.peek()) {
    case '"': return demandString();
    case 'n': mScanner.demand("null"); return nullptr;
    case 't': mScanner.demand("true"); return true;
    case 'f': mScanner.demand("false"); return false;
    case '[': {
      Json::array_t array;
      mScanner.ignore(1);
      mScanner.ignore(char_class::space);
      if (!mScanner.accept(']')) {
        while (true) {
          array.push_back(parse());
          mScanner.ignore(char_class::space);
          if (mScanner.accept(']')) break;
          mScanner.demand(',');
        }
      }
      return Json(std::in_place, std::move(array));
    }
    case '{': {
      Json::table_t table;
      mScanner.ignore(1);
      mScanner.ignore(char_class::space);
      if (!mScanner.accept('}')) {
        while (true) {
          std::string key = demandString();
          mScanner.ignore(char_class::space);
          mScanner.demand(':');
          table[key] = parse();
          mScanner.ignore(char_class::space);
          if (mScanner.accept('}')) break;
          mScanner.demand(',');
        }
      }
      return Json(std::in_place, std::move(table));
    }
    default:
      try {
        if (mScanner.peek() == '-' || char_class::digit(mScanner.peek())) return stringTo<double>(mScanner.accept(char_class::digit || char_class::these("+-.eE")));
      } catch (const std::exception &error) {
        mScanner.fail(error.what());
      }
      break;
    }
    mScanner.fail("Unrecognized token");
    return nullptr;
  }

private:
  Scanner<std::string_view> mScanner;
};

} // namespace

Json Json::parse(std::string_view source) { return Parser(source).parse(); }

void Json::renderTo(std::string &result, size_t depth) const {
  auto indent = [&](size_t n) {
    while (n--) result += "  ";
  };
  switch (kind()) {
  case Kind::None: result += "null"; break;
  case Kind::Bool: result += mi::toString(asBool()); break;
  case Kind::Number: result += mi::toString(asNumber()); break;
  case Kind::String: result += encodeString(asString()); break;
  case Kind::Array: {
    result += '[';
    const auto &array = asArray();
    size_t count = array.size();
    size_t index = 0;
    if (count > 1) result += '\n';
    for (const auto &each : array) {
      if (count > 1) indent(depth + 1);
      each.renderTo(result, depth + 1);
      if (++index != count) result += ',';
      if (count > 1) result += '\n';
    }
    if (count > 1) indent(depth);
    result += ']';
    break;
  }
  case Kind::Table: {
    result += '{';
    const auto &table = asTable();
    size_t count = table.size();
    size_t index = 0;
    if (count > 1) result += '\n';
    for (const auto &each : table) {
      if (count > 1) indent(depth + 1);
      result += encodeString(each.first);
      result += ": ";
      each.second.renderTo(result, depth + 1);
      if (++index != count) result += ',';
      if (count > 1) result += '\n';
    }
    if (count > 1) indent(depth);
    result += '}';
    break;
  }
  default: break;
  }
}

} // namespace mi
