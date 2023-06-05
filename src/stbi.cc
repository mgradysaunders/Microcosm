#include "Microcosm/stbi"

#include "miniz.h"

// Miniz compression override for STBIW.
static unsigned char *stbiw_miniz_compress(unsigned char *src, int srcLen, int *outLen, int quality) {
  mz_ulong resultLen = mz_compressBound(srcLen);
  unsigned char *result = static_cast<unsigned char *>(std::malloc(resultLen));
  if (mz_compress2(result, &resultLen, src, srcLen, quality) != MZ_OK) {
    std::free(result);
    return nullptr;
  }
  *outLen = resultLen;
  return result;
}

// Generate implementation for STB image.
#define STBI_ASSERT(x) ((void)0)
#define STBI_MALLOC std::malloc
#define STBI_REALLOC std::realloc
#define STBI_FREE std::free
#define STBI_NO_STDIO 1
#define STB_IMAGE_STATIC 1
#define STB_IMAGE_IMPLEMENTATION 1
#include "stb_image.h"

// Generate implementation for STB image write.
#define STBIW_ASSERT(x) ((void)0)
#define STBIW_MALLOC std::malloc
#define STBIW_REALLOC std::realloc
#define STBIW_MEMMOVE std::memmove
#define STBIW_FREE std::free
#define STBIW_ZLIB_COMPRESS stbiw_miniz_compress
#define STB_IMAGE_WRITE_NO_STDIO 1
#define STB_IMAGE_WRITE_STATIC 1
#define STB_IMAGE_WRITE_IMPLEMENTATION 1
#include "stb_image_write.h"

namespace stbi_istream {

static int read(void *s, char *data, int size) { return reinterpret_cast<std::istream *>(s)->read(data, size).gcount(); }

static void skip(void *s, int size) { reinterpret_cast<std::istream *>(s)->seekg(size, std::ios::cur); }

static int eof(void *s) { return reinterpret_cast<std::istream *>(s)->eof(); }

static const stbi_io_callbacks callbacks = {&read, &skip, &eof};

} // namespace stbi_istream

namespace stbi_ostream {

static void write(void *s, void *data, int size) { static_cast<std::ostream *>(s)->write(static_cast<char *>(data), size); }

} // namespace stbi_ostream

namespace mi::stbi {

template <typename Value> Image<Value> load(std::istream &stream, int forceNumChannels) {
  Value *buffer{nullptr};
  int sizeX{0};
  int sizeY{0};
  int channels{0};
  if constexpr (std::same_as<Value, uint8_t>) {
    buffer = reinterpret_cast<uint8_t *>(
      stbi_load_from_callbacks(&stbi_istream::callbacks, &stream, &sizeX, &sizeY, &channels, forceNumChannels));
  } else if constexpr (std::same_as<Value, uint16_t>) {
    buffer = reinterpret_cast<uint16_t *>(
      stbi_load_16_from_callbacks(&stbi_istream::callbacks, &stream, &sizeX, &sizeY, &channels, forceNumChannels));
  } else if constexpr (std::same_as<Value, float>) {
    buffer = reinterpret_cast<float *>(
      stbi_loadf_from_callbacks(&stbi_istream::callbacks, &stream, &sizeX, &sizeY, &channels, forceNumChannels));
  }
  if (!buffer) throw Error(std::runtime_error("STBI failure: {}"_format(stbi_failure_reason())));
  return Image<Value>(std::in_place, buffer, {sizeY, sizeX, forceNumChannels > 0 ? forceNumChannels : channels});
}

template <typename Value> Image<Value> load(const std::string &filename, int forceNumChannels) {
  auto stream = openIFStreamOrThrow(filename);
  try {
    return load<Value>(stream, forceNumChannels);
  } catch (const std::exception &error) {
    throw Error(std::runtime_error("Can't load {}: {}"_format(show(filename), error.what())));
  }
}

template Image<uint8_t> load<uint8_t>(std::istream &, int);

template Image<uint8_t> load<uint8_t>(const std::string &, int);

template Image<uint16_t> load<uint16_t>(std::istream &, int);

template Image<uint16_t> load<uint16_t>(const std::string &, int);

template Image<float> load<float>(std::istream &, int);

template Image<float> load<float>(const std::string &, int);

void saveU8(FileType fileType, std::ostream &stream, const ImageU8 &image) {
  if (fileType == FileType::Default) fileType = FileType::PNG;
  int result{0};
  int sizeY{image.size(0)};
  int sizeX{image.size(1)};
  int numChannels{image.size(2)};
  switch (fileType) {
  case FileType::BMP:
    result = stbi_write_bmp_to_func(stbi_ostream::write, &stream, sizeX, sizeY, numChannels, image.data());
    break;
  case FileType::JPG:
    result = stbi_write_jpg_to_func(stbi_ostream::write, &stream, sizeX, sizeY, numChannels, image.data(), 90);
    break;
  default:
  case FileType::PNG:
    result = stbi_write_png_to_func(stbi_ostream::write, &stream, sizeX, sizeY, numChannels, image.data(), sizeX * numChannels);
    break;
  case FileType::TGA:
    result = stbi_write_tga_to_func(stbi_ostream::write, &stream, sizeX, sizeY, numChannels, image.data());
    break;
  }
  if (!result) {
    throw Error(std::runtime_error("STBI write failure!"));
  }
}

void saveU8(FileType fileType, const std::string &filename, const ImageU8 &image) {
  if (fileType == FileType::Default) {
    if (filename.ends_with(".bmp")) fileType = FileType::BMP;
    if (filename.ends_with(".jpg")) fileType = FileType::JPG;
    if (filename.ends_with(".png")) fileType = FileType::PNG;
    if (filename.ends_with(".tga")) fileType = FileType::TGA;
  }
  auto stream = openOFStreamOrThrow(filename);
  try {
    saveU8(fileType, stream, image);
  } catch (const std::exception &error) {
    throw Error(std::runtime_error("Can't save {}: {}"_format(show(filename), error.what())));
  }
}

} // namespace mi::stbi
