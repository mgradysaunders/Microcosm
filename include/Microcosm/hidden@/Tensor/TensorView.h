/*-*- C++ -*-*/
#pragma once

#include "./TensorLambda.h"

namespace mi {

template <typename Value, concepts::tensor_shape Shape>
struct TensorView : TensorLike<TensorView<Value, Shape>, Value &, Shape> {
public:
  using Super = TensorLike<TensorView<Value, Shape>, Value &, Shape>;

  using Super::DynamicRank;

  using Super::IsLValue;

  using Super::Rank;

  using Super::shape;

  using access_type = typename Super::access_type;

  using value_type = typename Super::value_type;

  using shape_type = typename Super::shape_type;

  using index_type = typename Super::index_type;

  using initializer_list = typename Super::initializer_list;

  using tensor_object_tag = std::true_type;

public:
  constexpr TensorView() = default;

  constexpr TensorView(Value *first, Shape shape, std::array<ssize_t, Rank> skips) : Super(shape), skips(skips), first(first) {}

  constexpr TensorView(Value *first, Shape shape) : Super(shape), first(first) {
    auto skips0 = shape.skips();
    std::copy(skips0.begin(), skips0.end(), skips.begin());
  }

public:
  [[nodiscard, strong_inline]] constexpr auto &access(index_type i) noexcept {
    auto value{first};
    for (size_t k = 0; k < Rank; k++) value += skips[k] * i[k];
    return *value;
  }

  [[nodiscard, strong_inline]] constexpr auto &access(index_type i) const noexcept {
    auto value{first};
    for (size_t k = 0; k < Rank; k++) value += skips[k] * i[k];
    return *value;
  }

  [[nodiscard, strong_inline]] constexpr decltype(auto) access(auto... i) {
    if constexpr ((sizeof...(i) == Rank) && (std::integral<std::decay_t<decltype(i)>> && ...))
      return access(index_type{i...});
    else
      return capture_in_tensor_lambda(*this)(i...);
  }

  [[nodiscard, strong_inline]] constexpr decltype(auto) access(auto... i) const {
    if constexpr ((sizeof...(i) == Rank) && (std::integral<std::decay_t<decltype(i)>> && ...))
      return access(index_type{i...});
    else
      return capture_in_tensor_lambda(*this)(i...);
  }

public:
  std::array<ssize_t, Rank> skips{};

  Value *first{nullptr};
};

template <typename Value, size_t Size = Dynamic> using VectorView = TensorView<Value, TensorShape<Size>>;

template <typename Value, size_t Rows = Dynamic, size_t Cols = Dynamic>
using MatrixView = TensorView<Value, TensorShape<Rows, Cols>>;

} // namespace mi
