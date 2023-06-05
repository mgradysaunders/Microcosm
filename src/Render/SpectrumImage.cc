#include "Microcosm/Render/SpectrumImage"

namespace mi::render {

void SpectrumImage::resize(int newNumBands, Vector2i newSize) noexcept {
  clear();
  mNumBands = max(0, newNumBands);
  mSizeX = max(0, newSize[0]);
  mSizeY = max(0, newSize[1]);
  mData = static_cast<std::byte *>(std::malloc(imageSizeInBytes()));
}

void SpectrumImage::clear() noexcept {
  mSizeX = mSizeY = mNumBands = 0;
  std::free(mData), mData = nullptr;
}

void SpectrumImage::add(Vector2i index, const Spectrum &values, double weight) {
  const char *error = //
    int(values.size()) != mNumBands ? "Inconsistent bands"
    : !allTrue(isfinite(values))    ? "Non-finite spectrum values"
    : !isfinite(weight)             ? "Non-finite spectrum weight"
    : !isIndexValid(index)          ? "Invalid index"
                                    : nullptr;
  if (error) [[unlikely]] {
    throw Error(std::logic_error("Call to SpectrumImage::add() failed! Reason: {}"_format(error)));
  }
  PixelReference pixelRef{pixelReference(index)};
  pixelRef.num += 1;
  pixelRef.weight += weight;
  if (weight != 0) [[likely]] {
    for (int i = 0; i < mNumBands; i++) {
      pixelRef.values[i] += weight * values[i];
    }
  }
}

Spectrum SpectrumImage::extract(Vector2i index, bool divideOutNum, bool divideOutWeight) {
  if (!isIndexValid(index)) [[unlikely]] {
    throw Error(std::logic_error("Call to SpectrumImage::extract() failed! Reason: Invalid index"));
  }
  PixelReference pixelRef{pixelReference(index)};
  Spectrum values{with_shape, size_t(mNumBands)};
  for (int i = 0; i < mNumBands; i++) {
    values[i] = pixelRef.values[i].load();
  }
  auto currentNum{pixelRef.num.load()};
  auto currentWeight{pixelRef.weight.load()};
  if (divideOutNum && currentNum != 0) values /= currentNum;
  if (divideOutWeight && currentWeight != 0) values /= currentWeight;
  return values;
}

} // namespace mi::render
