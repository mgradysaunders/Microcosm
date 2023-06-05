#pragma once

#include "./MallocArray.h"
#include "./TensorLambda.h"
#include "./TensorLike.h"

namespace mi {

constexpr struct with_shape_t {
} with_shape = {};

constexpr struct from_rows_t {
} from_rows;

constexpr struct from_cols_t {
} from_cols;

template <typename Value, concepts::tensor_shape Shape> struct Tensor_initializers {};

template <typename Value, concepts::tensor_shape Shape, size_t SmallSize>
struct Tensor : Tensor_initializers<Value, Shape>,
                TensorLike<Tensor<Value, Shape, SmallSize>, Value &, Shape, /*SupportsResize=*/true> {
public:
  using Super = TensorLike<Tensor<Value, Shape, SmallSize>, Value &, Shape, /*SupportsResize=*/true>;

  using Super::DynamicRank;

  using Super::Rank;

  using Super::shape;

  using value_type = typename Super::value_type;

  using shape_type = typename Super::shape_type;

  using index_type = typename Super::index_type;

  using initializer_list = typename Super::initializer_list;

  using values_type = std::conditional_t<DynamicRank != 0, MallocArray<Value, SmallSize>, std::array<Value, Shape::TotalSize>>;

  using tensor_object_tag = std::true_type;

  static constexpr size_t Alignment = []() constexpr noexcept {
    if constexpr (DynamicRank != 0) {
      return alignof(std::max_align_t);
    } else {
      size_t align = sizeof(Value[Shape::TotalSize]);
      if (align & (align - 1)) return alignof(Value);
      if (align > alignof(std::max_align_t)) align = alignof(std::max_align_t);
      return align;
    }
  }();

public:
  constexpr Tensor() noexcept = default;

  [[strong_inline]] constexpr Tensor(const Shape &shape) : Super(shape) {
    if constexpr (DynamicRank != 0) mValues.resize(shape.totalSize());
  }

  template <typename OtherValue, typename OtherShape, size_t OtherSmallSize> requires(DynamicRank != 0) //
  [[strong_inline]] constexpr Tensor(const Tensor<OtherValue, OtherShape, OtherSmallSize> &other) : Tensor(other.shape) {
    std::copy(other.mValues.begin(), other.mValues.end(), mValues.begin());
  }

  template <typename OtherValue, typename OtherShape, size_t OtherSmallSize> requires(DynamicRank == 0) //
  [[strong_inline]] constexpr Tensor(const Tensor<OtherValue, OtherShape, OtherSmallSize> &other) {
    min(shape, other.shape).forEach([&](auto i) constexpr { access(i) = other.access(i); });
  }

  template <typename Other> requires(concepts::tensor_lambda<Other>) //
  [[strong_inline]] constexpr Tensor(Other &&other) : Tensor(other.execute()) {}

  template <typename... Values> requires(
    (Rank == 1) && (std::convertible_to<Values, Value> && ...) && sizeof...(Values) > 0 &&
    (DynamicRank == 1 || sizeof...(Values) == Shape::TotalSize))
  [[strong_inline]] constexpr Tensor(Values... values) : mValues{Value(values)...} {
    if constexpr (DynamicRank == 1) shape.mValues[0] = sizeof...(Values);
  }

  [[strong_inline]] constexpr Tensor(initializer_list ilist) requires(Rank == 2) {
    if constexpr (DynamicRank != 0) this->resize(ilist.size(), ilist.begin()->size());
    capture_in_tensor_lambda(*this) = ilist;
  }

  template <typename Other> requires(std::convertible_to<Other, Value> && DynamicRank == 0)
  [[strong_inline]] explicit constexpr Tensor(const Other &other) {
    std::fill(mValues.begin(), mValues.end(), other);
  }

  /// __Static tensor only!__ Support randomization.
  [[strong_inline]] constexpr Tensor(std::uniform_random_bit_generator auto &gen) requires(DynamicRank == 0) {
    for (auto &value : mValues) value = randomize<Value>(gen);
  }

  template <typename... Args>
  [[strong_inline]] explicit constexpr Tensor(with_shape_t, Args &&...args) : Tensor(Shape(std::forward<Args>(args)...)) {}

  /// __Static matrix only!__ Support construction from row vectors.
  template <typename... Args> requires(Rank == 2 && DynamicRank == 0 && sizeof...(Args) == Shape::template Size<0>)
  [[strong_inline]] explicit constexpr Tensor(from_rows_t, Args &&...args) {
    using RowVector = Tensor<Value, TensorShape<Shape::template Size<1>>>;
    const RowVector argsAsRows[] = {RowVector(auto_forward(args))...};
    for (size_t i = 0; i < Shape::template Size<0>; i++) this->row(i).assign(argsAsRows[i]);
  }

  /// __Static matrix only!__ Support construction from column vectors.
  template <typename... Args> requires(Rank == 2 && DynamicRank == 0 && sizeof...(Args) == Shape::template Size<1>)
  [[strong_inline]] explicit constexpr Tensor(from_cols_t, Args &&...args) {
    using ColVector = Tensor<Value, TensorShape<Shape::template Size<0>>>;
    const ColVector argsAsCols[] = {ColVector(auto_forward(args))...};
    for (size_t i = 0; i < Shape::template Size<1>; i++) this->col(i).assign(argsAsCols[i]);
  }

  /// __Static vector only!__ Support construction from STL array.
  template <typename Other> requires(Rank == 1 && DynamicRank == 0)
  explicit Tensor(const std::array<Other, Shape::TotalSize> &array) {
    std::copy(array.begin(), array.end(), mValues.begin());
  }

  /// __Dynamic only!__ Support in-place construction from pointer and shape.
  ///
  /// \note
  /// The purpose of this is entirely to avoid unnecessary copying if the user already has a pointer to
  /// the relevant data that is known to be in the correct linear layout and allocated via `std::malloc`.
  [[strong_inline]] explicit Tensor(std::in_place_t, Value *ptr, const Shape &shape) requires(DynamicRank != 0)
    : Super(shape), mValues(std::in_place, ptr, shape.totalSize()) {}

public:
  [[nodiscard, strong_inline]] constexpr auto begin() noexcept requires(Rank == 1) { return mValues.begin(); }

  [[nodiscard, strong_inline]] constexpr auto begin() const noexcept requires(Rank == 1) { return mValues.begin(); }

  [[nodiscard, strong_inline]] constexpr auto end() noexcept requires(Rank == 1) { return mValues.end(); }

  [[nodiscard, strong_inline]] constexpr auto end() const noexcept requires(Rank == 1) { return mValues.end(); }

  [[nodiscard, strong_inline, gnu::assume_aligned(Alignment)]] constexpr auto *data() noexcept { return mValues.data(); }

  [[nodiscard, strong_inline, gnu::assume_aligned(Alignment)]] constexpr auto *data() const noexcept { return mValues.data(); }

public:
  constexpr Tensor &operator=(const Value &value) {
    std::fill(mValues.begin(), mValues.end(), value);
    return *this;
  }

  [[nodiscard, strong_inline]] constexpr auto &access(index_type i) noexcept { return data()[shape.linearize(i)]; }

  [[nodiscard, strong_inline]] constexpr auto &access(index_type i) const noexcept { return data()[shape.linearize(i)]; }

  [[nodiscard, strong_inline]] constexpr decltype(auto) access(auto... i) requires(sizeof...(i) > 0) {
    if constexpr ((sizeof...(i) == Rank) && (std::integral<std::decay_t<decltype(i)>> && ...))
      return access(index_type{i...});
    else
      return capture_in_tensor_lambda(*this)(i...);
  }

  [[nodiscard, strong_inline]] constexpr decltype(auto) access(auto... i) const requires(sizeof...(i) > 0) {
    if constexpr ((sizeof...(i) == Rank) && (std::integral<std::decay_t<decltype(i)>> && ...))
      return access(index_type{i...});
    else
      return capture_in_tensor_lambda(*this)(i...);
  }

  template <size_t Index> [[nodiscard]] constexpr auto &get() noexcept { return access(Index); }

  template <size_t Index> [[nodiscard]] constexpr auto &get() const noexcept { return access(Index); }

  template <typename Other> [[nodiscard]] constexpr auto cast() noexcept {
    return capture_in_tensor_lambda(*this).template cast<Other>();
  }

  template <typename Other> [[nodiscard]] constexpr auto cast() const noexcept {
    return capture_in_tensor_lambda(*this).template cast<Other>();
  }

  constexpr void sortInPlace() noexcept requires(Rank == 1) { std::sort(mValues.begin(), mValues.end()); }

  constexpr void sortInPlace(auto &&pred) noexcept requires(Rank == 1) {
    std::sort(mValues.begin(), mValues.end(), std::forward<decltype(pred)>(pred));
  }

  /// Join with the given value. (Useful to append an affine weight or alpha channel.)
  [[nodiscard]] constexpr auto append(const Value &value) const noexcept requires(Rank == 1) {
    auto resultShape = shape.plus(TensorShape<1>());
    auto result = Tensor<Value, std::decay_t<decltype(resultShape)>, SmallSize>{resultShape};
    std::copy(mValues.begin(), mValues.end(), result.mValues.begin());
    result.mValues.back() = value;
    return result;
  }

public:
  void onResize(const Shape &oldShape, const Shape &newShape) {
    if constexpr (DynamicRank != 0) {
      values_type oldValues = std::move(mValues);
      values_type &newValues = mValues;
      newValues.resize(newShape.totalSize());
      std::fill(newValues.begin(), newValues.end(), Value());
      min(oldShape, newShape).forEach([&](auto i) { newValues[newShape.linearize(i)] = oldValues[oldShape.linearize(i)]; });
    }
  }

public:
  alignas(Alignment) values_type mValues = {};

  void onSerialize(auto &serializer) {
    if constexpr (DynamicRank > 0) serializer <=> shape;
    serializer <=> mValues;
  }
};

template <concepts::arithmetic_or_complex... Values>
Tensor(const Values &...) -> Tensor<std::common_type_t<Values...>, TensorShape<sizeof...(Values)>, 0>;

template <typename Value, typename Shape, size_t SmallSize>
struct to_float<Tensor<Value, Shape, SmallSize>> : to_float<Value> {};

template <typename Value, typename Shape, size_t SmallSize>
struct to_field<Tensor<Value, Shape, SmallSize>> : to_field<Value> {};

} // namespace mi

namespace std {

template <typename ValueA, typename ValueB, typename ShapeA, typename ShapeB, size_t SmallSizeA, size_t SmallSizeB>
struct common_type<mi::Tensor<ValueA, ShapeA, SmallSizeA>, mi::Tensor<ValueB, ShapeB, SmallSizeB>> {
  using type = mi::Tensor<
    std::common_type_t<ValueA, ValueB>, std::decay_t<decltype(mi::equalShapes(ShapeA(), ShapeB()))>,
    mi::min(SmallSizeA, SmallSizeB)>;
};

template <typename Value, typename Shape, size_t SmallSize, mi::concepts::number Other>
struct common_type<mi::Tensor<Value, Shape, SmallSize>, Other> {
  using type = mi::Tensor<std::common_type_t<Value, Other>, Shape, SmallSize>;
};

template <typename Value, typename Shape, size_t SmallSize, mi::concepts::number Other>
struct common_type<Other, mi::Tensor<Value, Shape, SmallSize>> {
  using type = mi::Tensor<std::common_type_t<Value, Other>, Shape, SmallSize>;
};

} // namespace std

namespace mi {

inline namespace tensor_aliases {

template <typename Value, size_t Size, size_t SmallSize = 0> using Vector = Tensor<Value, TensorShape<Size>, SmallSize>;

using Vectorf = Vector<float, Dynamic>;

using Vectord = Vector<double, Dynamic>;

using Vectorcf = Vector<std::complex<float>, Dynamic>;

using Vectorcd = Vector<std::complex<double>, Dynamic>;

#define MI_VECTOR_ALIAS_DECLARATION(Name, M)               \
  template <typename Value> using Name = Vector<Value, M>; \
  using Name##i = Name<int>;                               \
  using Name##f = Name<float>;                             \
  using Name##d = Name<double>;                            \
  using Name##cf = Name<std::complex<float>>;              \
  using Name##cd = Name<std::complex<float>>;              \
  using Name##b = Name<uint8_t>

MI_VECTOR_ALIAS_DECLARATION(Vector1, 1);

MI_VECTOR_ALIAS_DECLARATION(Vector2, 2);

MI_VECTOR_ALIAS_DECLARATION(Vector3, 3);

MI_VECTOR_ALIAS_DECLARATION(Vector4, 4);

#undef MI_VECTOR_ALIAS_DECLARATION

template <typename Value, size_t Size0, size_t Size1, size_t SmallSize = 0>
using Matrix = Tensor<Value, TensorShape<Size0, Size1>, SmallSize>;

using Matrixf = Matrix<float, Dynamic, Dynamic>;

using Matrixd = Matrix<double, Dynamic, Dynamic>;

using Matrixcf = Matrix<std::complex<float>, Dynamic, Dynamic>;

using Matrixcd = Matrix<std::complex<double>, Dynamic, Dynamic>;

#define MI_MATRIX_ALIAS_DECLARATION(Name, M, N)               \
  template <typename Value> using Name = Matrix<Value, M, N>; \
  using Name##f = Name<float>;                                \
  using Name##d = Name<double>;                               \
  using Name##cf = Name<std::complex<float>>;                 \
  using Name##cd = Name<std::complex<double>>

MI_MATRIX_ALIAS_DECLARATION(Matrix2, 2, 2);

MI_MATRIX_ALIAS_DECLARATION(Matrix2x3, 2, 3);

MI_MATRIX_ALIAS_DECLARATION(Matrix2x4, 2, 4);

MI_MATRIX_ALIAS_DECLARATION(Matrix3, 3, 3);

MI_MATRIX_ALIAS_DECLARATION(Matrix3x2, 3, 2);

MI_MATRIX_ALIAS_DECLARATION(Matrix3x4, 3, 4);

MI_MATRIX_ALIAS_DECLARATION(Matrix4, 4, 4);

MI_MATRIX_ALIAS_DECLARATION(Matrix4x3, 4, 2);

MI_MATRIX_ALIAS_DECLARATION(Matrix4x2, 4, 3);

#undef MI_MATRIX_ALIAS_DECLARATION

} // namespace tensor_aliases

} // namespace mi

namespace std {

template <typename Value, size_t Size, size_t SmallSize> requires(Size != mi::Dynamic)
struct tuple_size<mi::Tensor<Value, mi::TensorShape<Size>, SmallSize>> : public integral_constant<size_t, Size> {};

template <size_t Index, typename Value, size_t Size, size_t SmallSize>
struct tuple_element<Index, mi::Tensor<Value, mi::TensorShape<Size>, SmallSize>> {
  using type = Value;
};

} // namespace std
