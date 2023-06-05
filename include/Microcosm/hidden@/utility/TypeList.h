/*-*- C++ -*-*/
#pragma once

#include "./common.h"
#include <cstddef>
#include <tuple>

namespace mi {

/// A type list.
template <typename... T> struct TypeList {
  static constexpr size_t size = sizeof...(T);
  template <size_t N> using type = typename std::tuple_element<N, std::tuple<T...>>::type;
};

/// This forms a type list by concatenating the given types. If either
/// type is already a type list, then it is flattened (not nested) in the
/// resulting type list.
template <typename T, typename U> struct TypeListJoin {
  using type = TypeList<T, U>;
};
template <typename T, typename... U> struct TypeListJoin<T, TypeList<U...>> {
  using type = TypeList<T, U...>;
};
template <typename... T, typename U> struct TypeListJoin<TypeList<T...>, U> {
  using type = TypeList<T..., U>;
};
template <typename... T, typename... U> struct TypeListJoin<TypeList<T...>, TypeList<U...>> {
  using type = TypeList<T..., U...>;
};

/// This associates an integral index with each type in a type list.
template <size_t N, typename T> struct IndexedType {
  static constexpr size_t index = N;
  using type = T;
};
template <size_t N, typename... T> struct IndexedTypeList;
template <size_t N, typename T, typename... U> struct IndexedTypeList<N, T, U...> {
  using type = typename TypeListJoin<IndexedType<N, T>, typename IndexedTypeList<N + 1, U...>::type>::type;
};
template <size_t N, typename T> struct IndexedTypeList<N, T> {
  using type = TypeList<IndexedType<N, T>>;
};
template <> struct IndexedTypeList<0> {
  using type = TypeList<>;
};
template <typename... T> using IndexedTypeList_t = typename IndexedTypeList<0, T...>::type;

/// This using the parameters of a callable to construct an indexed
/// type list, which is useful for compile-time argument processing. See the
/// option parser implementation in <pre/terminal>.
template <typename T> struct CallableIndexedTypeList;
template <typename T, typename... Args> struct CallableIndexedTypeList<T(Args...)> {
  using type = IndexedTypeList_t<Args...>;
};
template <typename T, typename... Args> struct CallableIndexedTypeList<T (*)(Args...)> {
  using type = IndexedTypeList_t<Args...>;
};
template <typename T, typename Class, typename... Args> struct CallableIndexedTypeList<T (Class::*)(Args...)> {
  using type = IndexedTypeList_t<Args...>;
};
template <typename T, typename Class, typename... Args> struct CallableIndexedTypeList<T (Class::*)(Args...) const> {
  using type = IndexedTypeList_t<Args...>;
};
template <concepts::functor T> struct CallableIndexedTypeList<T> : CallableIndexedTypeList<decltype(&T::operator())> {};
template <typename T> struct CallableIndexedTypeList<T &> : CallableIndexedTypeList<T> {};
template <typename T> struct CallableIndexedTypeList<T &&> : CallableIndexedTypeList<T> {};
template <typename T> using CallableIndexedTypeList_t = typename CallableIndexedTypeList<T>::type;

} // namespace mi
