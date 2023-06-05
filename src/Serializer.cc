#include "Microcosm/Serializer"

namespace mi {

SerializableNewTable &serializableNew() {
  static SerializableNewTable serializableNewTable;
  return serializableNewTable;
}

void Serializer::readOrWrite(void *ptr, size_t num, size_t sz) {
  if (!num) return;
  if (!ptr || sz == 0 || sz > 16) throw Error(std::invalid_argument("Invalid pointer or size"));

  if (reading()) {
    onRead(ptr, num * sz);
    if (std::endian::native == std::endian::big && sz >= 2) {
      for (size_t pos = 0; pos < num; pos++) {
        std::reverse(
          static_cast<std::byte *>(ptr) + (pos + 0) * sz, //
          static_cast<std::byte *>(ptr) + (pos + 1) * sz);
      }
    }
  } else {
    if (std::endian::native == std::endian::big && sz >= 2) {
      std::byte tmp_bytes[16];
      for (size_t pos = 0; pos < num; pos++) {
        std::copy(
          static_cast<std::byte *>(ptr) + (pos + 0) * sz, //
          static_cast<std::byte *>(ptr) + (pos + 1) * sz, &tmp_bytes[0]);
        std::reverse(&tmp_bytes[0], &tmp_bytes[0] + sz);
        onWrite(&tmp_bytes[0], sz);
      }
    } else {
      onWrite(ptr, num * sz);
    }
  }
}

void Serializer::readOrWrite(RefPtr<Serializable> &value) {
  constexpr auto Next = uint32_t(-1);
  constexpr auto Null = uint32_t(-2);
  if (reading()) {
    uint32_t index = 0;
    readOrWrite(&index, 1, 4);
    if (index == Null)
      value = nullptr;
    else if (index != Next) // Already read?
      value = mObjectsRead.at(index);
    else {
      char subclass[64] = {};
      readOrWrite(&subclass[0], 64, 1);
      value = serializableNew().at(StaticString<char, 64>(&subclass[0]))();
      mObjectsRead.push_back(value);
      value->serialize(*this);
    }
  } else {
    if (value.get() == nullptr) {
      uint32_t index = Null;
      readOrWrite(&index, 1, 4);
      return;
    }
    // Attempt to add to objects-written table.
    auto [itr, inserted] = mObjectsWritten.insert(std::make_pair(value, mObjectsWritten.size()));
    // Already written?
    if (!inserted) {
      // Write index.
      uint32_t index = itr->second;
      readOrWrite(&index, 1, 4);
      return;
    }
    // Else, properly serialize.
    uint32_t index = Next;
    StaticString<char, 64> subclass = value->serialSubclass();
    readOrWrite(&index, 1, 4);
    readOrWrite(&subclass[0], 64, 1);
    value->serialize(*this);
  }
}

} // namespace mi
