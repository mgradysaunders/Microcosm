#include "Microcosm/miniz"
#include "Microcosm/utility"
#include "miniz.h"
#include <fstream>

#define DEFLATOR(ptr) static_cast<tdefl_compressor *>(ptr)
#define INFLATOR(ptr) static_cast<mz_stream *>(ptr)
#define ARCHIVE(ptr) static_cast<mz_zip_archive *>(ptr)

namespace mi::miniz {

Bytes deflate(const void *buffer, size_t size, int level) {
  Bytes result;
  auto putter = [](const void *buffer, int size, void *user) -> mz_bool {
    auto &result = *static_cast<Bytes *>(user);
    if (size > 0) {
      result.reserve(result.size() + size);
      result.insert(
        result.end(),                           //
        static_cast<const std::byte *>(buffer), //
        static_cast<const std::byte *>(buffer) + size);
    }
    return MZ_TRUE;
  };
  int flags = tdefl_create_comp_flags_from_zip_params(level, MZ_DEFAULT_WINDOW_BITS, MZ_DEFAULT_STRATEGY);
  if (!tdefl_compress_mem_to_output(buffer, size, putter, &result, flags))
    throw Error(std::runtime_error("Compression failed!"));
  return result;
}

Bytes inflate(const void *buffer, size_t size) {
  Bytes result;
  auto putter = [](const void *buffer, int size, void *user) -> mz_bool {
    auto &result = *static_cast<Bytes *>(user);
    if (size > 0) {
      result.reserve(result.size() + size);
      result.insert(
        result.end(),                           //
        static_cast<const std::byte *>(buffer), //
        static_cast<const std::byte *>(buffer) + size);
    }
    return MZ_TRUE;
  };
  if (!tinfl_decompress_mem_to_callback(buffer, &size, putter, &result, TINFL_FLAG_PARSE_ZLIB_HEADER))
    throw Error(std::runtime_error("Decompression failed!"));
  return result;
}

void StreamDeflator::open(const std::shared_ptr<std::ostream> &stream, int level) {
  close();
  if (stream == nullptr) return;
  ostream = stream;
  static auto putter = [](const void *buffer, int size, void *user) -> mz_bool {
    if (!static_cast<std::ostream *>(user)->write(static_cast<const char *>(buffer), size)) return MZ_FALSE;
    return MZ_TRUE;
  };
  deflator = new tdefl_compressor;
  int flags = tdefl_create_comp_flags_from_zip_params(level, MZ_DEFAULT_WINDOW_BITS, MZ_DEFAULT_STRATEGY);
  tdefl_init(DEFLATOR(deflator), putter, ostream.get(), flags);
}

void StreamDeflator::write(const void *ptr, size_t num) { //
  tdefl_compress_buffer(DEFLATOR(deflator), ptr, num, TDEFL_NO_FLUSH);
}

void StreamDeflator::close() {
  const char *error = nullptr;
  if (deflator) {
    if (tdefl_compress_buffer(DEFLATOR(deflator), nullptr, 0, TDEFL_FINISH) != TDEFL_STATUS_DONE) error = "Compression failed!";
    delete DEFLATOR(deflator);
    deflator = nullptr;
  }
  ostream.reset();
  if (error) throw Error(std::runtime_error(error));
}

void StreamInflator::open(const std::shared_ptr<std::istream> &stream) {
  close();
  if (stream == nullptr) return;
  istream = stream;
  inflator = new mz_stream;
  std::memset(inflator, 0, sizeof(mz_stream));
  mz_inflateInit(INFLATOR(inflator));
}

void StreamInflator::read(void *ptr, size_t num) {
  if (istream == nullptr) return;
  auto *infl = INFLATOR(inflator);
  infl->next_out = static_cast<unsigned char *>(ptr);
  infl->avail_out = num;
  while (infl->avail_out > 0 && *istream) {
    if (infl->avail_in == 0) {
      istream->read(reinterpret_cast<char *>(&buffer[0]), 1024);
      infl->next_in = reinterpret_cast<unsigned char *>(&buffer[0]);
      infl->avail_in = 1024;
      if (istream->eof()) infl->avail_in = istream->gcount();
    }
    if (mz_inflate(infl, MZ_NO_FLUSH) < 0) throw Error(std::runtime_error("Decompression failed!"));
  }
}

void StreamInflator::close() {
  if (inflator) {
    mz_inflateEnd(INFLATOR(inflator));
    delete INFLATOR(inflator);
    inflator = nullptr;
  }
  istream.reset();
}

ArchiveReader::ArchiveReader() : zip(new mz_zip_archive) { mz_zip_zero_struct(ARCHIVE(zip)); }

ArchiveReader::~ArchiveReader() {
  close();
  delete ARCHIVE(zip);
  zip = nullptr;
}

void ArchiveReader::open(const std::shared_ptr<std::istream> &stream) {
  close();
  if (!zip) new (this) ArchiveReader(); // In case we've been moved
  if (!stream) return;
  if (!stream->good()) throw Error(std::invalid_argument("Invalid stream"));
  auto streamOffset = stream->tellg();
  auto streamSize = remainingStreamsize(*stream);
  if (!stream->good()) throw Error(std::runtime_error("Can't determine stream offset or remaining size"));
  state.stream = stream;
  state.offset = streamOffset;
  ARCHIVE(zip)->m_pIO_opaque = &state;
  ARCHIVE(zip)->m_pRead = [](void *opaque, mz_uint64 pos, void *ptr, size_t count) -> size_t {
    auto &[stream, offset] = *static_cast<State *>(opaque);
    auto pos0 = offset;
    auto pos1 = stream->tellg();
    if (pos1 - pos0 != std::streamsize(pos)) stream->seekg(pos0 + pos, std::ios::beg);
    stream->read(static_cast<char *>(ptr), count);
    return stream->fail() ? 0 : count;
  };
  if (!mz_zip_reader_init(ARCHIVE(zip), streamSize, 0)) {
    mz_zip_error error = mz_zip_get_last_error(ARCHIVE(zip));
    close();
    throw Error(std::runtime_error("Can't initialize archive: "s + mz_zip_get_error_string(error)));
  }
}

void ArchiveReader::open(const std::string &filename) {
  auto stream = std::make_shared<std::ifstream>(filename, std::ios::in | std::ios::binary);
  if (!stream->is_open()) throw Error(std::runtime_error("Can't open "s + show(filename)));
  open(stream);
}

void ArchiveReader::close() {
  if (isOpen()) {
    mz_zip_reader_end(ARCHIVE(zip));
    mz_zip_zero_struct(ARCHIVE(zip));
    ARCHIVE(zip)->m_pIO_opaque = nullptr;
    state = {};
  }
}

bool ArchiveReader::isOpen() const noexcept { return zip && ARCHIVE(zip)->m_pIO_opaque; }

size_t ArchiveReader::archiveSize() const noexcept { return mz_zip_get_archive_size(ARCHIVE(zip)); }

size_t ArchiveReader::numFiles() const noexcept { return mz_zip_reader_get_num_files(ARCHIVE(zip)); }

bool ArchiveReader::isDirectory(size_t fileIndex) const noexcept {
  return mz_zip_reader_is_file_a_directory(ARCHIVE(zip), fileIndex);
}

bool ArchiveReader::isFileEncrypted(size_t fileIndex) const noexcept {
  return mz_zip_reader_is_file_encrypted(ARCHIVE(zip), fileIndex);
}

bool ArchiveReader::isFileSupported(size_t fileIndex) const noexcept {
  return mz_zip_reader_is_file_supported(ARCHIVE(zip), fileIndex);
}

std::string ArchiveReader::getFilename(size_t fileIndex) const {
  size_t size = mz_zip_reader_get_filename(ARCHIVE(zip), fileIndex, nullptr, 0);
  std::string filename;
  filename.resize(size);
  mz_zip_reader_get_filename(ARCHIVE(zip), fileIndex, filename.data(), filename.size());
  return filename;
}

std::optional<size_t> ArchiveReader::locate(const char *filename, bool ignoreCase, bool ignorePath) const {
  mz_uint flags = 0;
  if (!ignoreCase) flags |= MZ_ZIP_FLAG_CASE_SENSITIVE;
  if (ignorePath) flags |= MZ_ZIP_FLAG_IGNORE_PATH;
  int result = mz_zip_reader_locate_file(ARCHIVE(zip), filename, nullptr, flags);
  if (result < 0) return std::nullopt;
  return size_t(result);
}

Bytes ArchiveReader::extract(size_t fileIndex, bool decompress) const {
  Bytes result;
  auto putter = [](void *user, mz_uint64, const void *data, size_t size) -> size_t {
    auto &result = *static_cast<Bytes *>(user);
    result.insert(
      result.end(),                         //
      static_cast<const std::byte *>(data), //
      static_cast<const std::byte *>(data) + size);
    return size;
  };
  mz_uint flags = 0;
  if (!decompress) flags |= MZ_ZIP_FLAG_COMPRESSED_DATA;
  if (!mz_zip_reader_extract_to_callback(ARCHIVE(zip), fileIndex, putter, &result, flags))
    throw Error(std::runtime_error("Can't extract file: "s + mz_zip_get_error_string(mz_zip_get_last_error(ARCHIVE(zip)))));
  return result;
}

void ArchiveReader::extractTo(size_t fileIndex, bool decompress, std::ostream &stream) const {
  auto putter = [](void *user, mz_uint64, const void *data, size_t size) -> size_t {
    if (!static_cast<std::ostream *>(user)->write(static_cast<const char *>(data), size)) return 0;
    return size;
  };
  mz_uint flags = 0;
  if (!decompress) flags |= MZ_ZIP_FLAG_COMPRESSED_DATA;
  if (!mz_zip_reader_extract_to_callback(ARCHIVE(zip), fileIndex, putter, &stream, flags))
    throw Error(std::runtime_error("Can't extract file: "s + mz_zip_get_error_string(mz_zip_get_last_error(ARCHIVE(zip)))));
}

void ArchiveReader::extractTo(size_t fileIndex, bool decompress, const std::string &filename) const {
  auto stream = std::ofstream(filename, std::ios::out | std::ios::binary);
  if (!stream.is_open()) throw Error(std::runtime_error("Can't open "s + show(filename)));
  extractTo(fileIndex, decompress, stream);
}

} // namespace mi::miniz
