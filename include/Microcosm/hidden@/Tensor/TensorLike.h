#pragma once

#include "./TensorShape.h"

namespace mi {

namespace concepts {

template <typename T>
concept tensor = std::same_as<typename std::decay_t<T>::tensor_like_tag, std::true_type>;

template <typename T>
concept lvalue_tensor = std::same_as<typename std::decay_t<T>::lvalue_tensor_like_tag, std::true_type>;

template <typename T, size_t Rank>
concept tensor_with_rank = (tensor<T> && std::decay_t<T>::Rank == Rank);

template <typename T, size_t... Sizes>
concept tensor_with_shape = tensor<T> && std::same_as<typename std::decay_t<T>::shape_type, TensorShape<Sizes...>>;

template <typename T, typename U>
concept tensor_tensor_op2 = (tensor<T> && tensor<U>);

template <typename T, typename U>
concept tensor_number_op2 = (tensor<T> && number<U>);

template <typename T, typename U>
concept tensor_op2 = (tensor_tensor_op2<T, U> || tensor_number_op2<T, U> || tensor_number_op2<U, T>);

template <typename T>
concept tensor_object = std::same_as<typename std::decay_t<T>::tensor_object_tag, std::true_type>;

template <typename T>
concept tensor_lambda = std::same_as<typename std::decay_t<T>::tensor_lambda_tag, std::true_type>;

template <typename T>
concept tensor_vector = tensor_with_rank<T, 1>;

template <typename T>
concept tensor_matrix = tensor_with_rank<T, 2>;

} // namespace concepts

template <typename Value, size_t Rank> struct Tensor_initializer_list {
  using type = std::initializer_list<typename Tensor_initializer_list<Value, Rank - 1>::type>;
};

template <typename Value> struct Tensor_initializer_list<Value, 0> {
  using type = Value;
};

template <typename Shape> struct TensorLike_shape {
  Shape shape{};
};

// Necessary to prevent silly structure padding in static types.
template <typename Shape> requires(Shape::DynamicRank == 0) struct TensorLike_shape<Shape> {
  static constexpr Shape shape{};
};

template <typename Derived, typename Access, typename Shape, bool Resizable = false>
struct TensorLike : TensorLike_shape<Shape> {
private:
  [[nodiscard, strong_inline]] constexpr auto &derived() noexcept { return static_cast<Derived &>(*this); }

  [[nodiscard, strong_inline]] constexpr auto &derived() const noexcept { return static_cast<const Derived &>(*this); }

public:
  using TensorLike_shape<Shape>::shape;

  static constexpr size_t Rank = Shape::Rank;

  static constexpr size_t DynamicRank = Shape::DynamicRank;

  template <size_t K> static constexpr bool IsDynamic = Shape::template IsDynamic<K>;

  static constexpr bool IsAnyDynamic = DynamicRank != 0;

  using access_type = Access;

  using value_type = std::decay_t<Access>;

  using index_type = IndexVector<Rank>;

  using shape_type = Shape;

  using initializer_list = typename Tensor_initializer_list<value_type, Rank>::type;

  using tensor_like_tag = std::true_type;

  using lvalue_tensor_like_tag =
    std::bool_constant<std::is_reference_v<access_type> && !std::is_const_v<std::remove_reference_t<access_type>>>;

  static constexpr bool IsLValue = std::same_as<lvalue_tensor_like_tag, std::true_type>;

  constexpr TensorLike() noexcept = default;

  constexpr TensorLike(const Shape &sh) {
    if constexpr (DynamicRank != 0) shape = sh;
  }

  [[nodiscard, strong_inline]] constexpr auto empty() const noexcept { return shape.empty(); }

  [[nodiscard, strong_inline]] constexpr auto size(size_t k) const noexcept { return shape.size(k); }

  [[nodiscard, strong_inline]] constexpr auto size() const noexcept requires(Rank == 1) { return shape.size(); }

  [[nodiscard, strong_inline]] constexpr auto rows() const noexcept requires(Rank == 2) { return shape.rows(); }

  [[nodiscard, strong_inline]] constexpr auto cols() const noexcept requires(Rank == 2) { return shape.cols(); }

  [[nodiscard, strong_inline]] constexpr auto totalSize() const noexcept { return shape.totalSize(); }

  [[nodiscard, strong_inline]] constexpr decltype(auto) row(auto i) requires(Rank == 2) { return derived().access(i); }

  [[nodiscard, strong_inline]] constexpr decltype(auto) col(auto i) requires(Rank == 2) { return derived().access(Slice(), i); }

  [[nodiscard, strong_inline]] constexpr decltype(auto) row(auto i) const requires(Rank == 2) { return derived().access(i); }

  [[nodiscard, strong_inline]] constexpr decltype(auto) col(auto i) const requires(Rank == 2) {
    return derived().access(Slice(), i);
  }

  /// The linear (square) block of a possibly rectangular matrix.
  [[nodiscard, strong_inline]] constexpr decltype(auto) linear() requires(Rank == 2) {
    if constexpr (DynamicRank == 0) {
      constexpr size_t Size0 = Shape::template Size<0>;
      constexpr size_t Size1 = Shape::template Size<1>;
      constexpr size_t MinSize = std::min(Size0, Size1);
      return derived().access(Slice<0, MinSize>(), Slice<0, MinSize>());
    } else {
      size_t minSize = std::min(rows(), cols());
      return derived().access(Slice(0, minSize), Slice(0, minSize));
    }
  }

  /// The linear (square) block of a possibly rectangular matrix.
  [[nodiscard, strong_inline]] constexpr decltype(auto) linear() const requires(Rank == 2) {
    if constexpr (DynamicRank == 0) {
      constexpr size_t Size0 = Shape::template Size<0>;
      constexpr size_t Size1 = Shape::template Size<1>;
      constexpr size_t MinSize = std::min(Size0, Size1);
      return derived().access(Slice<0, MinSize>(), Slice<0, MinSize>());
    } else {
      size_t minSize = std::min(rows(), cols());
      return derived().access(Slice(0, minSize), Slice(0, minSize));
    }
  }

  [[nodiscard, strong_inline]] constexpr decltype(auto) operator[](auto i) { return derived().access(i); }

  [[nodiscard, strong_inline]] constexpr decltype(auto) operator[](auto i) const { return derived().access(i); }

  [[nodiscard, strong_inline]] constexpr decltype(auto) operator()(auto i, auto... j) { return derived().access(i, j...); }

  [[nodiscard, strong_inline]] constexpr decltype(auto) operator()(auto i, auto... j) const {
    return derived().access(i, j...);
  }

  [[nodiscard, strong_inline]] constexpr auto &doIt() noexcept { return derived(); }

  [[nodiscard, strong_inline]] constexpr auto &doIt() const noexcept { return derived(); }

  template <size_t K> void resize(auto count) requires(Shape::template IsDynamic<K> && Resizable) {
    Shape oldShape = shape;
    Shape newShape = shape;
    newShape.template resize<K>(count);
    derived().onResize(oldShape, newShape);
    shape = newShape;
  }

  void resize(auto... args) requires(Resizable) {
    Shape oldShape = shape;
    Shape newShape = shape;
    newShape.resize(args...);
    if constexpr (DynamicRank != 0) {
      derived().onResize(oldShape, newShape);
      shape = newShape;
    }
  }

  void resizeLike(auto &&other) requires(Resizable) { resize(other.shape); }

  template <typename Other> requires(IsLValue && concepts::tensor<Other>)
  [[strong_inline]] constexpr void assign(Other &&other) {
    equalShapes(shape, other.shape).forEach([&](auto i) constexpr { derived().access(i) = other.access(i); });
  }

  template <typename Other> requires(IsLValue && std::convertible_to<Other, value_type>)
  [[strong_inline]] constexpr void assign(const Other &other) {
    shape.forEach([&](auto i) constexpr { derived().access(i) = value_type(other); });
  }

public:
  // TODO Move these into the top namespace
  [[nodiscard, strong_inline]] constexpr auto fold(auto &&pred) const requires(Rank == 1) {
    value_type value{};
    if (!empty()) {
      value = derived().access(0);
      for (size_t i = 1; i < size(); i++) value = pred(value, derived().access(i));
    }
    return value;
  }

  [[nodiscard, strong_inline]] constexpr auto sum() const requires(Rank == 1) { return fold(std::plus<>()); }

  [[nodiscard, strong_inline]] constexpr auto product() const requires(Rank == 1) { return fold(std::multiplies<>()); }

public:
  [[strong_inline]] constexpr void swapInPlace(size_t i, size_t j) requires(IsLValue && Rank == 1) {
    std::swap(derived().access(i), derived().access(j));
  }

  [[strong_inline]] constexpr void swapRowsInPlace(size_t i, size_t j) requires(IsLValue && Rank == 2) {
    if (i != j) [[likely]]
      for (size_t k = 0; k < this->cols(); k++) std::swap(derived().access(i, k), derived().access(j, k));
  }

  [[strong_inline]] constexpr void swapColsInPlace(size_t i, size_t j) requires(IsLValue && Rank == 2) {
    if (i != j) [[likely]]
      for (size_t k = 0; k < this->rows(); k++) std::swap(derived().access(k, i), derived().access(k, j));
  }

  constexpr void transposeInPlace() requires(IsLValue && Rank <= 2) {
    if constexpr (Rank == 2) {
      equalShapes(shape.template take<0>(), shape.template take<1>()); // Must be square!
      for (size_t i = 0; i < this->rows(); i++) {
        for (size_t j = i + 1; j < this->cols(); j++) {
          std::swap(derived().access(i, j), derived().access(j, i));
        }
      }
    }
  }
};

template <typename Expr> requires(concepts::tensor<Expr>)
[[nodiscard, strong_inline]] constexpr bool anyTrue(Expr &&expr) noexcept {
  bool result = false;
  expr.shape.forEach([&](auto i) constexpr { return !(result = expr(i)); });
  return result;
}

template <typename Expr> requires(concepts::tensor<Expr>)
[[nodiscard, strong_inline]] constexpr bool allTrue(Expr &&expr) noexcept {
  bool result = true;
  expr.shape.forEach([&](auto i) constexpr { return (result = expr(i)); });
  return result;
}

template <typename Expr> requires(concepts::tensor<Expr>)
[[nodiscard, strong_inline]] constexpr auto argmin(Expr &&expr) noexcept {
  typename std::decay_t<Expr>::index_type minIndex{};
  typename std::decay_t<Expr>::value_type minValue{};
  if (!expr.empty()) {
    minValue = expr(minIndex);
    expr.shape.forEach([&](auto i) constexpr {
      if (auto value = expr(i); minValue > value) minValue = value, minIndex = i;
    });
  }
  if constexpr (std::decay_t<Expr>::Rank == 1)
    return minIndex[0]; // Just return the integer.
  else
    return minIndex;
}

template <typename Expr> requires(concepts::tensor<Expr>)
[[nodiscard, strong_inline]] constexpr auto argmax(Expr &&expr) noexcept {
  typename std::decay_t<Expr>::index_type maxIndex{};
  typename std::decay_t<Expr>::value_type maxValue{};
  if (!expr.empty()) {
    maxValue = expr(maxIndex);
    expr.shape.forEach([&](auto i) constexpr {
      if (auto value = expr(i); maxValue < value) maxValue = value, maxIndex = i;
    });
  }
  if constexpr (std::decay_t<Expr>::Rank == 1)
    return maxIndex[0]; // Just return the integer.
  else
    return maxIndex;
}

template <typename, concepts::tensor_shape, size_t SmallSize = 0> struct Tensor;

namespace detail {

template <concepts::tensor Expr> struct tensor_rank {
  static constexpr size_t value = Expr::Rank;
};

template <concepts::tensor Expr, size_t Index> requires(Index < Expr::Rank) struct tensor_size {
  using shape_type = typename Expr::shape_type;
  static constexpr size_t value = shape_type::template Size<Index>;
};

} // namespace detail

template <typename Expr> constexpr size_t tensor_rank_v = detail::tensor_rank<std::decay_t<Expr>>::value;

template <typename Expr, size_t Index> constexpr size_t tensor_size_v = detail::tensor_size<std::decay_t<Expr>, Index>::value;

template <concepts::tensor_object Expr> struct DoesntAlias {
  constexpr DoesntAlias(Expr &expr) noexcept : expr(expr) {}

  [[strong_inline]] constexpr void operator+=(auto &&other) { expr.assign(expr + auto_forward(other)); }

  [[strong_inline]] constexpr void operator-=(auto &&other) { expr.assign(expr - auto_forward(other)); }

  [[strong_inline]] constexpr void operator*=(auto &&other) { expr.assign(expr * auto_forward(other)); }

  [[strong_inline]] constexpr void operator/=(auto &&other) { expr.assign(expr / auto_forward(other)); }

  Expr &expr;
};

template <concepts::tensor_object Expr> DoesntAlias(Expr &) -> DoesntAlias<Expr>;

} // namespace mi
