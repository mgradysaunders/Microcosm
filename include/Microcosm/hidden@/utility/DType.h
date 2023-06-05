/*-*- C++ -*-*/
#pragma once

#include "./Half.h"
#include "./common.h"
#include <algorithm>

namespace mi {

enum class DType : unsigned {
  None = 0,
  UInt8 = 0b1100'0001U,
  UInt16 = 0b1100'0010U,
  UInt32 = 0b1100'0100U,
  UInt64 = 0b1100'1000U,
  Int8 = 0b0100'0001U,
  Int16 = 0b0100'0010U,
  Int32 = 0b0100'0100U,
  Int64 = 0b0100'1000U,
  Float16 = 0b0000'0010U,
  Float32 = 0b0000'0100U,
  Float64 = 0b0000'1000U
};

[[nodiscard]] constexpr size_t sizeOf(DType type) noexcept { return unsigned(type) & 0b0001'1111U; }

[[nodiscard]] constexpr bool isUnsigned(DType type) noexcept { return (unsigned(type) & 0b1000'0000U) != 0; }

[[nodiscard]] constexpr bool isIntegral(DType type) noexcept { return (unsigned(type) & 0b0100'0000U) != 0; }

[[nodiscard]] constexpr bool isFloating(DType type) noexcept { return (unsigned(type) & 0b0100'0000U) == 0; }

[[nodiscard]] constexpr DType toFloat(DType type) noexcept {
  if (type == DType::Float16) return DType::Float32;
  if (isFloating(type)) return type;
  return sizeOf(type) < sizeof(float) ? DType::Float32 : DType::Float64;
}

template <typename Type>
constexpr DType dtype_of = std::same_as<Type, std::byte>  ? DType::UInt8
                           : std::same_as<Type, uint8_t>  ? DType::UInt8
                           : std::same_as<Type, uint16_t> ? DType::UInt16
                           : std::same_as<Type, uint32_t> ? DType::UInt32
                           : std::same_as<Type, uint64_t> ? DType::UInt64
                           : std::same_as<Type, int8_t>   ? DType::Int8
                           : std::same_as<Type, int16_t>  ? DType::Int16
                           : std::same_as<Type, int32_t>  ? DType::Int32
                           : std::same_as<Type, int64_t>  ? DType::Int64
                           : std::same_as<Type, Half>     ? DType::Float16
                           : std::same_as<Type, float>    ? DType::Float32
                           : std::same_as<Type, double>   ? DType::Float64
                                                          : DType::None;

template <DType> struct dtype_type : using_type<void> {};
template <> struct dtype_type<DType::Int8> : using_type<int8_t> {};
template <> struct dtype_type<DType::Int16> : using_type<int16_t> {};
template <> struct dtype_type<DType::Int32> : using_type<int32_t> {};
template <> struct dtype_type<DType::Int64> : using_type<int64_t> {};
template <> struct dtype_type<DType::UInt8> : using_type<uint8_t> {};
template <> struct dtype_type<DType::UInt16> : using_type<uint16_t> {};
template <> struct dtype_type<DType::UInt32> : using_type<uint32_t> {};
template <> struct dtype_type<DType::UInt64> : using_type<uint64_t> {};
template <> struct dtype_type<DType::Float16> : using_type<Half> {};
template <> struct dtype_type<DType::Float32> : using_type<float> {};
template <> struct dtype_type<DType::Float64> : using_type<double> {};

template <DType Type> using dtype_type_t = typename dtype_type<Type>::type;

template <DType From, DType To> constexpr void dispatchCast(size_t size, const void *from, void *to) noexcept {
  std::copy(
    static_cast<const dtype_type_t<From> *>(from), static_cast<const dtype_type_t<From> *>(from) + size,
    static_cast<dtype_type_t<To> *>(to));
}

template <DType To> constexpr void dispatchCast(size_t size, const DType From, const void *from, void *to) noexcept {
  switch (From) {
#define DispatchCastCase(Enum) \
  case Enum: dispatchCast<Enum, To>(size, from, to); break
    DispatchCastCase(DType::UInt8);
    DispatchCastCase(DType::UInt16);
    DispatchCastCase(DType::UInt32);
    DispatchCastCase(DType::UInt64);
    DispatchCastCase(DType::Int8);
    DispatchCastCase(DType::Int16);
    DispatchCastCase(DType::Int32);
    DispatchCastCase(DType::Int64);
    DispatchCastCase(DType::Float16);
    DispatchCastCase(DType::Float32);
    DispatchCastCase(DType::Float64);
#undef DispatchCastCase
  default: break;
  }
}

constexpr void dispatchCast(size_t size, const DType From, const DType To, const void *from, void *to) noexcept {
  switch (To) {
#define DispatchCastCase(Enum) \
  case Enum: dispatchCast<Enum>(size, From, from, to); break
    DispatchCastCase(DType::UInt8);
    DispatchCastCase(DType::UInt16);
    DispatchCastCase(DType::UInt32);
    DispatchCastCase(DType::UInt64);
    DispatchCastCase(DType::Int8);
    DispatchCastCase(DType::Int16);
    DispatchCastCase(DType::Int32);
    DispatchCastCase(DType::Int64);
    DispatchCastCase(DType::Float16);
    DispatchCastCase(DType::Float32);
    DispatchCastCase(DType::Float64);
#undef DispatchCastCase
  default: break;
  }
}

} // namespace mi
