#pragma once

#include "./IndexVector.h"
#include "./Slice.h"

namespace mi {

template <size_t... Sizes> struct TensorShape;

template <auto Array, typename> struct array_to_TensorShape;

template <auto Array, size_t... Seq> struct array_to_TensorShape<Array, std::integer_sequence<size_t, Seq...>> {
  using type = TensorShape<Array[Seq]...>;
};

template <auto Array>
using array_to_TensorShape_t = typename array_to_TensorShape<Array, std::make_index_sequence<Array.size()>>::type;

template <size_t... Sizes> struct TensorShape_values {};

template <size_t... Sizes> requires((Sizes == Dynamic) || ...)
struct TensorShape_values<Sizes...> : ArrayLike<TensorShape_values<Sizes...>> {
  MI_ARRAY_LIKE_CONSTEXPR_DATA(&mValues[0]);

  MI_ARRAY_LIKE_STATIC_CONSTEXPR_SIZE(sizeof...(Sizes));

  IndexVector<sizeof...(Sizes)> mValues{(Sizes == Dynamic ? 0 : Sizes)...};
};

template <size_t... Sizes> struct TensorShape final : TensorShape_values<Sizes...> {
public:
  constexpr TensorShape() noexcept = default;

  constexpr TensorShape(auto... args) { resize(args...); }

  template <size_t... Other> constexpr TensorShape(const TensorShape<Other...> &other) { resize(other); }

public:
  static constexpr size_t TotalSize = (Sizes * ...);

  static constexpr size_t Rank = sizeof...(Sizes);

  template <size_t K> requires(K < Rank) static constexpr size_t Size = IndexVector{Sizes...}[K];

  static constexpr size_t SizeIfSame = ([]() constexpr {
    IndexVector sizes{Sizes...};
    if constexpr (Rank == 1)
      return sizes[0];
    else if constexpr (Rank == 2)
      return sizes[0] == Dynamic ? sizes[1] : sizes[0];
    else
      return Dynamic;
  })();

  static constexpr size_t DynamicRank = ((Sizes == Dynamic ? 1 : 0) + ...);

  template <size_t K> static constexpr bool IsDynamic = (Size<K> == Dynamic);

  [[nodiscard]] static constexpr bool isDynamic(size_t k) noexcept { return IndexVector{Sizes...}[k] == Dynamic; }

public:
  /// \name Sizing
  ///
  /// \{

  [[nodiscard]] constexpr auto empty() const noexcept {
    if constexpr (DynamicRank == 0)
      return false;
    else
      return totalSize() == 0;
  }

  [[nodiscard]] constexpr auto size(size_t k) const noexcept {
    if constexpr (DynamicRank == 0)
      return IndexVector{Sizes...}[k];
    else
      return this->mValues[k];
  }

  [[nodiscard]] constexpr auto size() const noexcept requires(Rank == 1) { return size(0); }

  [[nodiscard]] constexpr auto rows() const noexcept requires(Rank == 2) { return size(0); }

  [[nodiscard]] constexpr auto cols() const noexcept requires(Rank == 2) { return size(1); }

  [[nodiscard]] constexpr auto totalSize() const noexcept {
    if constexpr (DynamicRank == 0)
      return TotalSize;
    else {
      size_t prod = 1;
      for (size_t each : this->mValues) prod *= each;
      return prod;
    }
  }

  [[nodiscard]] constexpr auto sizes() const noexcept requires(DynamicRank > 0) { return this->mValues; }

  [[nodiscard]] constexpr auto skips() const noexcept requires(DynamicRank > 0) {
    IndexVector<Rank> result{};
    result.back() = 1;
    for (size_t k = Rank - 1; k > 0; --k) {
      result[k - 1] = result[k];
      result[k - 1] *= this->mValues[k];
    }
    return result;
  }

  [[nodiscard]] static constexpr auto sizes() noexcept requires(DynamicRank == 0) { return IndexVector{Sizes...}; }

  [[nodiscard]] static constexpr auto skips() noexcept requires(DynamicRank == 0) {
    IndexVector<Rank> result{};
    result.back() = 1;
    for (size_t k = Rank - 1; k > 0; --k) {
      result[k - 1] = result[k];
      result[k - 1] *= IndexVector{Sizes...}[k];
    }
    return result;
  }

  [[nodiscard, strong_inline]] constexpr auto linearize(IndexVector<Rank> i) const noexcept {
    size_t offset = i[0];
    for (size_t k = 1; k < Rank; k++) offset = size(k) * offset + i[k];
    return offset;
  }

  /// \}
public:
  /// \name Resizing
  ///
  /// If dynamic dimensions are present, this structure implements the
  /// `resize()` method. The main implementation accepts a new size for
  /// every dimension. A runtime error is thrown if any given dimension
  /// is incompatible with dimensions fixed at compile-time.
  ///
  /// If there are both dynamic and compile-time dimensions, there
  /// is an overload of `resize()` which accepts a new size for each dynamic
  /// dimension only. An index-template overload of `resize()` is always
  /// available to resize dimensions individually. Note that this overload
  /// will produce a compile-time error if the template dimension is not
  /// dynamic.
  ///
  /// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
  /// Tensor<float, TensorShape<3, Dynamic>> tensor;
  /// tensor.resize(3, 4);  // (1) Ok
  /// tensor.resize(4, 4);  // (2) Runtime error!
  /// tensor.resize<0>(3);  // (3) Compile-time error!
  /// tensor.resize<1>(4);  // (4) Same behavior as (1)
  /// tensor.resize(4);     // (5) Same behavior as (1)
  /// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  ///
  /// \{

  template <size_t K> constexpr void resize(size_t count) requires IsDynamic<K> { this->mValues[K] = count; }

  template <std::integral... Args>
  constexpr void resize(Args... counts)
    requires((sizeof...(Args) == Rank) || (sizeof...(Args) == DynamicRank && DynamicRank != 0)) {
    resize(IndexVector{counts...});
  }

  constexpr void resize(IndexVector<Rank> counts) {
    if constexpr (Rank != DynamicRank)
      for (size_t k = 0; k < Rank; k++)
        if (!isDynamic(k) && size(k) != counts[k])
          throw std::invalid_argument("Resize incompatible with compile-time dimensions!");
    if constexpr (DynamicRank != 0) this->mValues = counts;
  }

  constexpr void resize(IndexVector<DynamicRank> counts) //
    requires(DynamicRank != Rank) {
    constexpr auto which = ([]() consteval {
      auto which = IndexVector<DynamicRank>{};
      auto whichItr = which.begin();
      for (size_t k = 0; k < Rank; k++)
        if (isDynamic(k)) *whichItr++ = k;
      return which;
    })();
    for (size_t k = 0; k < DynamicRank; k++) this->mValues[which[k]] = counts[k];
  }

  template <size_t... Other> constexpr void resize(const TensorShape<Other...> &other) {
    static_assert(sizeof...(Sizes) == sizeof...(Other));
    if constexpr (DynamicRank == 0 && TensorShape<Other...>::DynamicRank == 0)
      static_assert(sizes() == TensorShape<Other...>::sizes());
    else
      resize(other.sizes());
  }

  /// \}

public:
  /// \name Helpers
  /// \{

  template <size_t... K> [[nodiscard, strong_inline]] constexpr auto take() const noexcept {
    using Result = TensorShape<Size<K>...>;
    if constexpr (Result::DynamicRank == 0)
      return Result{};
    else {
      Result result;
      result.mValues = {this->mValues[K]...};
      return result;
    }
  }

  template <size_t... K> [[nodiscard, strong_inline]] constexpr auto drop() const noexcept {
    static_assert(sizeof...(K) < Rank);
    static_assert([]() constexpr {
      IndexVector dimsToDrop{K...};
      return std::unique(dimsToDrop.begin(), dimsToDrop.end()) == dimsToDrop.end();
    }());
    constexpr auto keep = ([]() constexpr {
      IndexVector drop{K...};
      IndexVector<Rank - sizeof...(K)> keep;
      IndexVector<Rank - sizeof...(K)> keepSizes;
      for (size_t k = 0, s = 0; k < Rank; k++)
        if (std::find(drop.begin(), drop.end(), k) == drop.end()) {
          keep[s] = k, keepSizes[s] = IndexVector{Sizes...}[k];
          s++;
        }
      return std::make_pair(keep, keepSizes);
    })();
    using Result = array_to_TensorShape_t<keep.second>;
    Result result{};
    if constexpr (Result::DynamicRank != 0) {
      for (size_t k = 0; k < Result::Rank; k++) result.mValues[k] = this->mValues[keep.first[k]];
    }
    return result;
  }

  template <size_t... Other>
  [[nodiscard, strong_inline]] constexpr auto append(const TensorShape<Other...> &other) const noexcept {
    using Result = TensorShape<Sizes..., Other...>;
    if constexpr (DynamicRank == 0 && TensorShape<Other...>::DynamicRank == 0) {
      return Result();
    } else {
      Result result;
      const auto sizesA{this->sizes()};
      const auto sizesB{other.sizes()};
      std::copy(sizesA.begin(), sizesA.end(), &result.mValues[0]);
      std::copy(sizesB.begin(), sizesB.end(), &result.mValues[0] + Rank);
      return result;
    }
  }

  template <size_t... Other> requires(sizeof...(Sizes) == sizeof...(Other))
  [[nodiscard, strong_inline]] constexpr auto plus(const TensorShape<Other...> &other) const noexcept {
    constexpr auto resultSize = [](size_t sizeA, size_t sizeB) constexpr noexcept {
      if (sizeA == Dynamic) return Dynamic;
      if (sizeB == Dynamic) return Dynamic;
      return sizeA + sizeB;
    };
    using Result = TensorShape<resultSize(Sizes, Other)...>;
    if constexpr (DynamicRank == 0 && TensorShape<Other...>::DynamicRank == 0) {
      return Result();
    } else {
      Result result;
      const auto sizesA{this->sizes()};
      const auto sizesB{other.sizes()};
      for (size_t i = 0; i < sizeof...(Sizes); i++) result.mValues[i] = sizesA[i] + sizesB[i];
      return result;
    }
  }

  template <size_t K> [[nodiscard, strong_inline]] constexpr auto bind(concepts::slice auto slice) const {
    static_assert(K < Rank);
    constexpr auto newSize{std::decay_t<decltype(slice)>::template Size<Size<K>>};
    constexpr auto newSizes{([]() constexpr {
      IndexVector newSizes{Sizes...};
      newSizes[K] = newSize;
      return newSizes;
    })()};
    using Result = array_to_TensorShape_t<newSizes>;
    Result result{};
    if constexpr (Result::DynamicRank != 0) {
      result.mValues = sizes();
      result.mValues[K] = newSize != Dynamic ? newSize : slice.extent(size(K));
    }
    return result;
  }

  /// \}

public:
  template <typename Func> [[strong_inline]] constexpr void forEach(Func &&func) const {
    using Return = std::invoke_result_t<Func, IndexVector<Rank>>;
    constexpr bool ReturnVoid = std::same_as<std::decay_t<Return>, void>;
    constexpr bool ReturnBool = std::same_as<std::decay_t<Return>, bool>;
    static_assert(
      ReturnVoid || ReturnBool, //
      "The supplied function must return void or bool!\n"
      "  - Return void to process each index unconditionally.\n"
      "  - Return bool to process each index until the function\n"
      "    returns false.");
    if constexpr (Rank == 1) {
      for (size_t i = 0; i < size(); i++) {
        if constexpr (ReturnVoid)
          std::invoke(std::forward<Func>(func), IndexVector{i});
        else if (!std::invoke(std::forward<Func>(func), IndexVector{i}))
          return;
      }
    } else if constexpr (Rank == 2) {
      for (size_t i = 0; i < size(0); i++)
        for (size_t j = 0; j < size(1); j++) {
          if constexpr (ReturnVoid)
            std::invoke(std::forward<Func>(func), IndexVector{i, j});
          else if (!std::invoke(std::forward<Func>(func), IndexVector{i, j}))
            return;
        }
    } else {
      IndexVector<Rank> limit = sizes();
      IndexVector<Rank> index;
      for (size_t i = 0; i < totalSize(); i++, index.incrementInPlace(limit)) {
        if constexpr (ReturnVoid)
          std::invoke(std::forward<Func>(func), index);
        else if (!std::invoke(std::forward<Func>(func), index))
          return;
      }
    }
  }
};

template <size_t... SizesA, size_t... SizesB>
[[nodiscard, strong_inline]] constexpr auto min(const TensorShape<SizesA...> &shapeA, const TensorShape<SizesB...> &shapeB) {
  static_assert(sizeof...(SizesA) == sizeof...(SizesB));
  constexpr auto Rank = sizeof...(SizesA);
  constexpr auto choose = [](size_t Size0, size_t Size1) constexpr {
    if (Size0 == Dynamic || Size1 == Dynamic)
      return Dynamic;
    else
      return std::min(Size0, Size1);
  };
  TensorShape<choose(SizesA, SizesB)...> shape;
  if constexpr (TensorShape<SizesA...>::DynamicRank != 0 || TensorShape<SizesB...>::DynamicRank != 0) {
    for (size_t k = 0; k < Rank; k++) shape.mValues[k] = std::min(shapeA.size(k), shapeB.size(k));
  }
  return shape;
}

template <size_t... SizesA, size_t... SizesB>
[[nodiscard, strong_inline]] constexpr auto max(const TensorShape<SizesA...> &shapeA, const TensorShape<SizesB...> &shapeB) {
  static_assert(sizeof...(SizesA) == sizeof...(SizesB));
  constexpr auto Rank = sizeof...(SizesA);
  constexpr auto choose = [](size_t Size0, size_t Size1) constexpr {
    if (Size0 == Dynamic || Size1 == Dynamic)
      return Dynamic;
    else
      return std::max(Size0, Size1);
  };
  TensorShape<choose(SizesA, SizesB)...> shape;
  if constexpr (TensorShape<SizesA...>::DynamicRank != 0 || TensorShape<SizesB...>::DynamicRank != 0) {
    for (size_t k = 0; k < Rank; k++) shape.mValues[k] = std::max(shapeA.size(k), shapeB.size(k));
  }
  return shape;
}

/// Assert equal dimensions, either at compile-time or runtime as
/// appropriate.
///
/// \returns
/// A new dimensions object that propagates compile-time
/// assumptions from both arguments.
///
/// \note
/// The implementation of this function is probably the most concentrated
/// dose of modern C++ I have ever written. It is a testament to the mind
/// boggling power of the additions since C++17.
///
template <size_t... SizesA, size_t... SizesB>
[[strong_inline]] constexpr auto equalShapes(const TensorShape<SizesA...> &shapeA, const TensorShape<SizesB...> &shapeB) {
  constexpr size_t Rank{sizeof...(SizesA)};
  static_assert(sizeof...(SizesA) == sizeof...(SizesB));
  static_assert([](const auto &sizesA, const auto &sizesB) constexpr {
    for (size_t k = 0; k < Rank; k++) {
      if (
        sizesA[k] != Dynamic && //
        sizesB[k] != Dynamic && //
        sizesA[k] != sizesB[k]) {
        return false;
      }
    }
    return true;
  }(IndexVector{SizesA...}, IndexVector{SizesB...}));

  if constexpr (
    TensorShape<SizesA...>::DynamicRank != 0 || //
    TensorShape<SizesB...>::DynamicRank != 0) {
    for (size_t k = 0; k < Rank; k++) {
      if (shapeA.size(k) != shapeB.size(k)) {
        throw Error(std::runtime_error("Shapes not equal!"));
      }
    }
  }

  constexpr auto Sizes = [](const auto &sizesA, const auto &sizesB) constexpr {
    auto sizes = IndexVector<Rank>{};
    for (size_t k = 0; k < Rank; k++) {
      if (sizesA[k] != Dynamic) {
        sizes[k] = sizesA[k];
      } else {
        sizes[k] = sizesB[k];
      }
    }
    return sizes;
  }(IndexVector{SizesA...}, IndexVector{SizesB...});

  using Result = array_to_TensorShape_t<Sizes>;
  Result result{};
  if constexpr (Result::DynamicRank != 0) result.mValues = shapeA.mValues;
  return result;
}

namespace concepts {

template <typename T> //
struct is_tensor_shape : std::false_type {};

template <size_t... Sizes> //
struct is_tensor_shape<TensorShape<Sizes...>> : std::true_type {};

template <typename T>
concept tensor_shape = is_tensor_shape<std::decay_t<T>>::value;

} // namespace concepts

} // namespace mi
